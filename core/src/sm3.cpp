// 3SM(Bentley ScalableMesh, SQLite) 읽기 전용 판독 — 구조는 research/3sm-format.md(실제 파일 3개로 확인).
//  SMMasterHeader(GCS, RootNodeId) · SMNodeHeader(색인) 만 열 때 읽고, 노드 기하(SMPoint)·UV(SMUVs)·텍스처(SMTexture)는 요청 때 PK 조회.
//  SQLite 멀티스레드 모드(SQLITE_THREADSAFE=2, SQLITE_OPEN_NOMUTEX): 연결 하나는 한 번에 한 스레드만 → 연결 풀로 빌려 쓴다.
//  URI immutable=1: 잠금·저널·WAL 검사 없음 → 읽기 전용 매체에서도 열리고 부속 파일을 만들지 않는다.
#include "asec/sm3.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <zlib.h>
#include "sqlite3.h"

namespace asec {

namespace {
void setErr(std::string* err, const std::string& m) { if (err) *err = m; }

// ---- 가상 타일 등록부(열린 3SM: 경로 → SmFile). readTmxTile 이 `<x.3sm>/n<id>` 를 만나면 여기서 찾는다
std::mutex& regMu() { static std::mutex m; return m; }
std::map<fs::path, std::vector<std::weak_ptr<SmFile>>>& reg() { static std::map<fs::path, std::vector<std::weak_ptr<SmFile>>> r; return r; }

fs::path canon(const fs::path& p) {
    std::error_code ec;
    fs::path a = fs::absolute(p, ec);
    if (ec) a = p;
    return a.lexically_normal();
}

/// 경로 → SQLite URI("file:///D:/a%20b/x.3sm?immutable=1"). UTF-8 바이트를 RFC 3986 대로 퍼센트 인코딩(영숫자·-._~/: 는 그대로)
std::string toUri(const fs::path& p) {
    std::string u8 = canon(p).u8string();
    for (auto& c : u8) if (c == '\\') c = '/';
    std::string out = "file://";
    if (u8.empty() || u8[0] != '/') out += '/';   // "D:/..." → "file:///D:/..."
    static const char* hex = "0123456789ABCDEF";
    for (unsigned char c : u8) {
        const bool keep = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' || c == '~' || c == '/' || c == ':';
        if (keep) out += char(c);
        else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
    }
    out += "?immutable=1";
    return out;
}

struct Stmt {   // 준비된 문장 RAII
    sqlite3_stmt* s = nullptr;
    Stmt() = default;
    Stmt(const Stmt&) = delete;
    Stmt& operator=(const Stmt&) = delete;
    ~Stmt() { finalize(); }
    void finalize() { if (s) { sqlite3_finalize(s); s = nullptr; } }
    bool prepare(sqlite3* db, const char* sql) { if (s) { sqlite3_finalize(s); s = nullptr; } return sqlite3_prepare_v2(db, sql, -1, &s, nullptr) == SQLITE_OK; }
    void reset() { if (s) { sqlite3_reset(s); sqlite3_clear_bindings(s); } }
    std::vector<uint8_t> blob(int col) const {
        const void* p = sqlite3_column_blob(s, col);
        const int n = sqlite3_column_bytes(s, col);
        if (!p || n <= 0) return {};
        return std::vector<uint8_t>(static_cast<const uint8_t*>(p), static_cast<const uint8_t*>(p) + n);
    }
    bool isNull(int col) const { return sqlite3_column_type(s, col) == SQLITE_NULL; }
};

/// zlib(머리 0x78) 이면 풀고(크기 = size 힌트, 모르면 늘려 가며), 아니면 원시 바이트 그대로
constexpr int64_t kMaxBlobBytes = int64_t(256) << 20;   // 노드 하나의 풀린 BLOB 상한(실제 파일 최대 ~0.6 MB). 깨진 Size* 값으로 거대 할당을 막는다

bool inflateBlob(const std::vector<uint8_t>& b, int64_t sizeHint, std::vector<uint8_t>& out, std::string* err) {
    if (b.empty()) { out.clear(); return true; }
    if (sizeHint > kMaxBlobBytes) { setErr(err, "3SM BLOB 크기 값이 비정상입니다(" + std::to_string(sizeHint) + " B)"); return false; }
    // zlib 머리: 첫 바이트 0x78(deflate, 32K 창) 이고 두 바이트가 31 의 배수(FCHECK). 아니면 원시(비압축)
    const bool looksZlib = b.size() >= 2 && b[0] == 0x78 && ((unsigned(b[0]) << 8) | b[1]) % 31 == 0;
    if (!looksZlib) { out = b; return true; }
    size_t cap = sizeHint > 0 ? size_t(sizeHint) : std::max<size_t>(b.size() * 4, 1 << 16);
    for (int attempt = 0; attempt < 6; ++attempt) {
        out.assign(cap, 0);
        uLongf n = uLongf(cap);
        const int rc = ::uncompress(out.data(), &n, b.data(), uLong(b.size()));
        if (rc == Z_OK) { out.resize(n); return true; }
        if (rc == Z_DATA_ERROR && int64_t(b.size()) == sizeHint) { out = b; return true; }   // 우연히 zlib 머리처럼 보인 원시 배열
        if (rc != Z_BUF_ERROR) { setErr(err, "3SM BLOB 압축을 풀 수 없습니다(zlib " + std::to_string(rc) + ")"); return false; }
        cap *= 4;
        if (int64_t(cap) > kMaxBlobBytes) break;
    }
    setErr(err, "3SM BLOB 압축을 풀 수 없습니다(크기 초과)");
    return false;
}

template <class T>
std::vector<T> asVec(const std::vector<uint8_t>& b) {
    std::vector<T> v(b.size() / sizeof(T));
    if (!v.empty()) std::memcpy(v.data(), b.data(), v.size() * sizeof(T));
    return v;
}

/// TexData = 16 B 머리(int32 w,h,c,0) + JPEG. 머리 뒤가 JPEG 가 아니면 FF D8 FF 를 찾는다
bool jpegOf(const std::vector<uint8_t>& tex, std::vector<uint8_t>& out) {
    if (tex.size() > 18 && tex[16] == 0xFF && tex[17] == 0xD8) { out.assign(tex.begin() + 16, tex.end()); return true; }
    for (size_t i = 0; i + 2 < tex.size(); ++i)
        if (tex[i] == 0xFF && tex[i + 1] == 0xD8 && tex[i + 2] == 0xFF) { out.assign(tex.begin() + ptrdiff_t(i), tex.end()); return true; }
    return false;
}
}  // namespace

// ---------------------------------------------------------------- 연결
struct SmFile::Conn {
    sqlite3* db = nullptr;
    Stmt point, uv, tex;
    ~Conn() { point.finalize(); uv.finalize(); tex.finalize(); if (db) sqlite3_close(db); }
};

bool SmFile::openConnection(sqlite3** db, std::string* err) const {
    *db = nullptr;
    const int rc = sqlite3_open_v2(uri_.c_str(), db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI | SQLITE_OPEN_NOMUTEX, nullptr);
    if (rc != SQLITE_OK) {
        setErr(err, std::string("3SM(SQLite)을 열 수 없습니다: ") + (*db ? sqlite3_errmsg(*db) : sqlite3_errstr(rc)));
        if (*db) sqlite3_close(*db);
        *db = nullptr;
        return false;
    }
    return true;
}

std::shared_ptr<SmFile::Conn> SmFile::acquire(std::string* err) {
    {
        std::lock_guard<std::mutex> lk(poolMu_);
        if (!pool_.empty()) { auto c = pool_.back(); pool_.pop_back(); return c; }
    }
    auto c = std::make_shared<Conn>();
    if (!openConnection(&c->db, err)) return nullptr;
    if (!c->point.prepare(c->db, "SELECT PointData, IndexData, SizePts, SizeIndices FROM SMPoint WHERE NodeId=?") ||
        !c->uv.prepare(c->db, "SELECT UVData, SizeUVs, UVIndexData, SizeUVIndex FROM SMUVs WHERE NodeId=?") ||
        !c->tex.prepare(c->db, "SELECT TexData FROM SMTexture WHERE NodeId=?")) {
        setErr(err, std::string("3SM 표를 읽지 못했습니다(ScalableMesh 형식이 아닐 수 있음): ") + sqlite3_errmsg(c->db));
        return nullptr;
    }
    return c;
}

void SmFile::release(std::shared_ptr<Conn> c) {
    if (!c) return;
    std::lock_guard<std::mutex> lk(poolMu_);
    if (pool_.size() < 8) pool_.push_back(std::move(c));   // 넘치는 연결은 그냥 닫힌다
}

// ---------------------------------------------------------------- 열기(색인만)
std::shared_ptr<SmFile> SmFile::open(const fs::path& p, std::string* err) {
    std::error_code ec;
    if (!fs::exists(p, ec)) { setErr(err, "3SM 파일이 없습니다: " + p.u8string()); return nullptr; }
    if (!isSqliteFile(p)) { setErr(err, "3SM(ScalableMesh, SQLite) 파일이 아닙니다: " + p.u8string()); return nullptr; }
    std::shared_ptr<SmFile> f(new SmFile());
    f->path_ = canon(p);
    f->uri_ = toUri(p);
    sqlite3* db = nullptr;
    if (!f->openConnection(&db, err)) return nullptr;
    struct Closer { sqlite3* d; ~Closer() { if (d) sqlite3_close(d); } } closer{db};

    Stmt st;
    if (!st.prepare(db, "SELECT GCS, RootNodeId FROM SMMasterHeader LIMIT 1") || sqlite3_step(st.s) != SQLITE_ROW) {
        setErr(err, "3SM(ScalableMesh) 형식이 아닙니다(SMMasterHeader 없음): " + p.filename().u8string());
        return nullptr;
    }
    if (const unsigned char* g = sqlite3_column_text(st.s, 0)) f->gcs_ = reinterpret_cast<const char*>(g);
    f->rootId_ = st.isNull(1) ? -1 : sqlite3_column_int64(st.s, 1);

    if (!st.prepare(db, "SELECT NodeId, ParentNodeId, Resolution, Extent, NodeCount, TexID, SubNode, GeometryResolution FROM SMNodeHeader")) {
        setErr(err, "3SM(ScalableMesh) 형식이 아닙니다(SMNodeHeader 없음): " + p.filename().u8string());
        return nullptr;
    }
    int rc;
    while ((rc = sqlite3_step(st.s)) == SQLITE_ROW) {
        SmNodeInfo n;
        n.id = sqlite3_column_int64(st.s, 0);
        n.parent = st.isNull(1) ? -1 : sqlite3_column_int64(st.s, 1);
        n.level = sqlite3_column_int(st.s, 2);
        if (sqlite3_column_bytes(st.s, 3) >= 48) {
            double e[6];
            std::memcpy(e, sqlite3_column_blob(st.s, 3), 48);
            bool fin = true;
            for (double v : e) fin = fin && std::isfinite(v);
            if (fin && e[0] <= e[3] && e[1] <= e[4] && e[2] <= e[5]) { n.extentWorld.add(Vec3(e[0], e[1], e[2])); n.extentWorld.add(Vec3(e[3], e[4], e[5])); }
        }
        n.hasMesh = sqlite3_column_int64(st.s, 4) > 0;
        n.texId = st.isNull(5) ? -1 : sqlite3_column_int64(st.s, 5);
        const int sb = sqlite3_column_bytes(st.s, 6);
        if (sb >= 4) {
            const auto* raw = static_cast<const uint8_t*>(sqlite3_column_blob(st.s, 6));
            for (int k = 0; k + 4 <= sb; k += 4) { int32_t c; std::memcpy(&c, raw + k, 4); if (c >= 0) n.children.push_back(c); }
        }
        n.geomRes = sqlite3_column_double(st.s, 7);
        f->nodes_[n.id] = std::move(n);
    }
    if (rc != SQLITE_DONE) { setErr(err, std::string("3SM 노드 색인을 읽지 못했습니다: ") + sqlite3_errmsg(db)); return nullptr; }
    if (f->nodes_.empty()) { setErr(err, "3SM 에 메시 노드가 없습니다: " + p.filename().u8string()); return nullptr; }
    // 자식 목록 정리: 없는 노드 제거, SubNode 가 비었으면 ParentNodeId 로 보완
    for (auto& [id, n] : f->nodes_) {
        auto& ch = n.children;
        ch.erase(std::remove_if(ch.begin(), ch.end(), [&](int64_t c) { return c == id || !f->nodes_.count(c); }), ch.end());
    }
    for (auto& [id, n] : f->nodes_)
        if (n.parent >= 0 && n.parent != id)
            if (auto it = f->nodes_.find(n.parent); it != f->nodes_.end())
                if (std::find(it->second.children.begin(), it->second.children.end(), id) == it->second.children.end()) { it->second.children.push_back(id); f->backfills_++; }
    for (auto& [id, n] : f->nodes_) std::sort(n.children.begin(), n.children.end());
    if (f->rootId_ < 0 || !f->nodes_.count(f->rootId_)) {
        f->rootId_ = -1;
        for (auto& [id, n] : f->nodes_) if (n.parent < 0) { f->rootId_ = id; break; }
        if (f->rootId_ < 0) f->rootId_ = f->nodes_.begin()->first;
    }
    // 뿌리부터 너비 우선으로 돌며 이미 방문한 노드로 가는 간선(순환·두 부모)은 끊는다 → 트리 보장(collect·LodStreamer 무한 재귀 방지)
    {
        std::vector<int64_t> q{f->rootId_};
        std::map<int64_t, bool> seen; seen[f->rootId_] = true;
        for (size_t qi = 0; qi < q.size(); ++qi) {
            auto& ch = f->nodes_.at(q[qi]).children;
            std::vector<int64_t> keep;
            for (int64_t c : ch) {
                if (seen.count(c)) { f->cycleEdges_++; continue; }
                seen[c] = true; keep.push_back(c); q.push_back(c);
            }
            ch.swap(keep);
        }
    }
    const SmNodeInfo& root = f->nodes_.at(f->rootId_);
    Box3 ext = root.extentWorld;
    if (!ext.valid()) for (auto& [id, n] : f->nodes_) ext.add(n.extentWorld);   // 뿌리 상자가 없으면 전체 노드 상자
    if (!ext.valid()) { setErr(err, "3SM 노드 범위(Extent)가 없습니다: " + p.filename().u8string()); return nullptr; }
    f->origin_ = Vec3(std::round(ext.center().x), std::round(ext.center().y), 0.0);   // z 는 0: 높이 값 그대로
    f->boundsLocal_ = Box3();
    f->boundsLocal_.add(ext.mn - f->origin_);
    f->boundsLocal_.add(ext.mx - f->origin_);
    {
        std::lock_guard<std::mutex> lk(regMu());
        reg()[f->path_].push_back(f);
    }
    return f;
}

SmFile::~SmFile() {
    std::lock_guard<std::mutex> lk(regMu());
    auto it = reg().find(path_);
    if (it == reg().end()) return;
    auto& v = it->second;
    v.erase(std::remove_if(v.begin(), v.end(), [](const std::weak_ptr<SmFile>& w) { return w.expired(); }), v.end());
    if (v.empty()) reg().erase(it);
}

fs::path SmFile::virtualTilePath(int64_t id) const { return path_ / ("n" + std::to_string(id)); }
size_t SmFile::geometryReads() const { return reads_.load(); }

// ---------------------------------------------------------------- 노드 → 가상 타일
bool SmFile::readNodeTile(int64_t id, TmxTile& out, std::string* err) {
    try {
        return readNodeTileImpl(id, out, err);
    } catch (const std::bad_alloc&) {
        out = TmxTile();
        setErr(err, "3SM 노드 n" + std::to_string(id) + " 를 읽을 메모리가 부족합니다(BLOB 크기 비정상)");
        return false;
    }
}

namespace {
/// 가상 타일에 담긴 원시 BLOB 위치(out.bytes 안). 압축 풀기·변환은 lazyDecode 에서
struct SmRaw {
    size_t ptsOff = 0, ptsLen = 0, idxOff = 0, idxLen = 0, uvOff = 0, uvLen = 0, uvIdxOff = 0, uvIdxLen = 0, texOff = 0, texLen = 0;
    int64_t sizePts = 0, sizeIdx = 0, sizeUv = 0, sizeUvIdx = 0;
    int64_t nodeId = 0, texId = -1;
    Vec3 origin;
};
std::vector<uint8_t> slice(const std::vector<uint8_t>& b, size_t off, size_t len) {
    if (!len || off + len > b.size()) return {};
    return std::vector<uint8_t>(b.begin() + ptrdiff_t(off), b.begin() + ptrdiff_t(off + len));
}

/// 원시 BLOB → Mesh(+텍스처). 타일 잠금 아래에서 불린다(TileCache::decode / decodeGeometry / LodStreamer 작업 스레드)
bool decodeSmNode(const SmRaw& r, TmxTile& t, size_t ni, std::string* err) {
    TmxNode& N = t.nodes[ni];
    const std::string tag = "3SM 노드 n" + std::to_string(r.nodeId);
    const std::vector<uint8_t> ptsRaw = slice(t.bytes, r.ptsOff, r.ptsLen), idxRaw = slice(t.bytes, r.idxOff, r.idxLen);
    const std::vector<uint8_t> uvRaw = slice(t.bytes, r.uvOff, r.uvLen), uvIdxRaw = slice(t.bytes, r.uvIdxOff, r.uvIdxLen), texRaw = slice(t.bytes, r.texOff, r.texLen);
    if (ptsRaw.empty() || idxRaw.empty()) return true;   // 메시 없음
    std::vector<uint8_t> pb, ib, ub, uib;
    if (!inflateBlob(ptsRaw, r.sizePts, pb, err) || !inflateBlob(idxRaw, r.sizeIdx, ib, err)) return false;
    const auto P = asVec<double>(pb);
    const auto I = asVec<int32_t>(ib);
    const size_t np = P.size() / 3;
    if (np == 0 || I.size() < 3 || I.size() % 3 != 0 || pb.size() % 24 != 0 || ib.size() % 4 != 0) { setErr(err, tag + " 의 기하 크기가 맞지 않습니다"); return false; }
    std::vector<double> U; std::vector<int32_t> UI;
    if (!uvRaw.empty()) {
        if (!inflateBlob(uvRaw, r.sizeUv, ub, err)) return false;
        U = asVec<double>(ub);
        if (!uvIdxRaw.empty()) { if (!inflateBlob(uvIdxRaw, r.sizeUvIdx, uib, err)) return false; UI = asVec<int32_t>(uib); }
        if (U.size() % 2 != 0 || (UI.empty() && U.size() / 2 != np) || (!UI.empty() && UI.size() != I.size())) { U.clear(); UI.clear(); }   // 맞지 않는 UV 는 버림(텍스처 없이)
    }
    const bool sameIdx = UI.empty() || std::equal(UI.begin(), UI.end(), I.begin());
    const Vec3& o = r.origin;
    auto m = std::make_shared<Mesh>();
    if (U.empty() || (sameIdx && U.size() / 2 == np)) {
        m->pos.resize(np * 3);
        for (size_t i = 0; i < np; ++i) { m->pos[3 * i] = float(P[3 * i] - o.x); m->pos[3 * i + 1] = float(P[3 * i + 1] - o.y); m->pos[3 * i + 2] = float(P[3 * i + 2] - o.z); }
        if (!U.empty()) { m->uv.resize(np * 2); for (size_t i = 0; i < np * 2; ++i) m->uv[i] = float(U[i]); }
        m->idx.reserve(I.size());
        for (int32_t k : I) {
            if (k < 1 || size_t(k) > np) { setErr(err, tag + " 의 삼각형 인덱스가 범위를 벗어났습니다(" + std::to_string(k) + ")"); return false; }
            m->idx.push_back(uint32_t(k - 1));
        }
    } else {
        // UV 인덱스가 따로 → (점, UV) 짝마다 꼭짓점 하나
        const size_t nuv = U.size() / 2;
        std::unordered_map<uint64_t, uint32_t> remap;
        remap.reserve(I.size());
        m->idx.reserve(I.size());
        for (size_t k = 0; k < I.size(); ++k) {
            const int64_t pi = int64_t(I[k]) - 1, ui = int64_t(UI[k]) - 1;
            if (pi < 0 || size_t(pi) >= np || ui < 0 || size_t(ui) >= nuv) { setErr(err, tag + " 의 인덱스가 범위를 벗어났습니다"); return false; }
            const uint64_t key = (uint64_t(pi) << 32) | uint64_t(ui);
            auto f = remap.find(key);
            if (f == remap.end()) {
                const uint32_t nv = uint32_t(m->pos.size() / 3);
                m->pos.push_back(float(P[3 * pi] - o.x)); m->pos.push_back(float(P[3 * pi + 1] - o.y)); m->pos.push_back(float(P[3 * pi + 2] - o.z));
                m->uv.push_back(float(U[2 * ui])); m->uv.push_back(float(U[2 * ui + 1]));
                f = remap.emplace(key, nv).first;
            }
            m->idx.push_back(f->second);
        }
    }
    std::vector<uint8_t> jpg;
    if (!m->uv.empty() && jpegOf(texRaw, jpg)) {
        auto tx = std::make_shared<Texture>();
        tx->format = "jpg";
        tx->encoded = std::move(jpg);
        m->texture = tx;
        t.textures.emplace_back("tex" + std::to_string(r.texId), tx);   // TileCache 가 RGBA 크기를 예산에 넣는 목록
    }
    m->computeBBox();
    if (!N.bb.valid()) N.bb = m->bbox;
    N.meshes.push_back(m);
    return true;
}
}  // namespace

bool SmFile::readNodeTileImpl(int64_t id, TmxTile& out, std::string* err) {
    auto it = nodes_.find(id);
    if (it == nodes_.end()) { setErr(err, "3SM 노드가 없습니다: n" + std::to_string(id)); return false; }
    const SmNodeInfo& n = it->second;
    reads_++;
    out = TmxTile();
    out.path = virtualTilePath(id);
    TmxNode N;
    N.id = std::to_string(id);
    if (n.extentWorld.valid()) { N.bb.add(n.extentWorld.mn - origin_); N.bb.add(n.extentWorld.mx - origin_); }
    // 3MX 규칙: 화면 지름(px) > maxScreenDiameter 이면 자식으로. 노드 해상도 geomRes(m) 가 화면 1 px 보다 거칠어지는 지점 = 대각선/geomRes
    N.maxScreenDiameter = (!n.children.empty() && n.geomRes > 0 && N.bb.valid()) ? N.bb.diag() / n.geomRes : 0.0;
    for (int64_t c : n.children) N.children.push_back(virtualTilePath(c));
    N.decoded = true;   // 메시 없는 노드는 더 할 일 없음
    N.ready = false;
    if (n.hasMesh) {
        auto conn = acquire(err);
        if (!conn) return false;
        struct Ret { SmFile* f; std::shared_ptr<Conn>& c; ~Ret() { f->release(std::move(c)); } } ret{this, conn};
        // ---- 바이트만 읽는다(압축 풀기·변환·텍스처는 lazyDecode 에서 — 수집 순회가 직렬이라 여기서는 가볍게)
        SmRaw r; r.nodeId = id; r.texId = n.texId; r.origin = origin_;
        auto append = [&](const std::vector<uint8_t>& b, size_t& off, size_t& len) { off = out.bytes.size(); len = b.size(); out.bytes.insert(out.bytes.end(), b.begin(), b.end()); };
        conn->point.reset();
        sqlite3_bind_int64(conn->point.s, 1, id);
        int rc = sqlite3_step(conn->point.s);
        if (rc == SQLITE_ROW) {
            append(conn->point.blob(0), r.ptsOff, r.ptsLen); append(conn->point.blob(1), r.idxOff, r.idxLen);
            r.sizePts = sqlite3_column_int64(conn->point.s, 2); r.sizeIdx = sqlite3_column_int64(conn->point.s, 3);
        } else if (rc != SQLITE_DONE) { setErr(err, std::string("3SM 기하를 읽지 못했습니다: ") + sqlite3_errmsg(conn->db)); return false; }
        conn->point.reset();
        conn->uv.reset();
        sqlite3_bind_int64(conn->uv.s, 1, id);
        rc = sqlite3_step(conn->uv.s);
        if (rc == SQLITE_ROW) {
            append(conn->uv.blob(0), r.uvOff, r.uvLen); r.sizeUv = sqlite3_column_int64(conn->uv.s, 1);
            append(conn->uv.blob(2), r.uvIdxOff, r.uvIdxLen); r.sizeUvIdx = sqlite3_column_int64(conn->uv.s, 3);
        } else if (rc != SQLITE_DONE) { setErr(err, std::string("3SM UV 를 읽지 못했습니다: ") + sqlite3_errmsg(conn->db)); return false; }
        conn->uv.reset();
        if (n.texId >= 0) {
            conn->tex.reset();
            sqlite3_bind_int64(conn->tex.s, 1, n.texId);
            rc = sqlite3_step(conn->tex.s);
            if (rc == SQLITE_ROW) append(conn->tex.blob(0), r.texOff, r.texLen);
            else if (rc != SQLITE_DONE) { setErr(err, std::string("3SM 텍스처를 읽지 못했습니다: ") + sqlite3_errmsg(conn->db)); return false; }
            conn->tex.reset();
        }
        // 깨진 크기 값은 읽는 단계에서 바로 거른다(거대 할당 방지)
        for (int64_t v : {r.sizePts, r.sizeIdx, r.sizeUv, r.sizeUvIdx})
            if (v > kMaxBlobBytes) { setErr(err, "3SM 노드 n" + std::to_string(id) + " 의 BLOB 크기 값이 비정상입니다(" + std::to_string(v) + " B)"); return false; }
        if (r.ptsLen && r.idxLen) {
            N.decoded = false;
            N.resourceIds.push_back("geometry0");   // LodStreamer 는 resourceIds 로 "그릴 기하가 있는 노드" 를 판단한다
            out.lazyDecode = [r](TmxTile& tt, size_t ni, std::string* e) { return decodeSmNode(r, tt, ni, e); };
        }
    }
    out.nodes.push_back(std::move(N));
    return true;
}

// ---------------------------------------------------------------- 보조
bool isSqliteFile(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return false;
    char head[16] = {};
    f.read(head, 16);
    return f.gcount() == 16 && std::memcmp(head, "SQLite format 3\0", 16) == 0;
}

bool parseSmVirtualPath(const fs::path& p, fs::path* file3sm, int64_t* id) {
    const fs::path parent = p.parent_path();
    if (parent.empty()) return false;
    std::string pe = parent.extension().u8string();
    for (auto& c : pe) c = char(std::tolower(static_cast<unsigned char>(c)));
    if (pe != ".3sm") return false;
    const std::string name = p.filename().u8string();
    if (name.size() < 2 || name[0] != 'n') return false;
    int64_t v = 0;
    for (size_t i = 1; i < name.size(); ++i) {
        if (name[i] < '0' || name[i] > '9') return false;
        v = v * 10 + (name[i] - '0');
    }
    if (file3sm) *file3sm = parent;
    if (id) *id = v;
    return true;
}

bool readSmVirtualTile(const fs::path& p, TmxTile& out, std::string* err) {
    fs::path f3sm; int64_t id = 0;
    if (!parseSmVirtualPath(p, &f3sm, &id)) { setErr(err, "3SM 가상 타일 경로가 아닙니다: " + p.u8string()); return false; }
    std::shared_ptr<SmFile> f;
    {
        std::lock_guard<std::mutex> lk(regMu());
        auto it = reg().find(canon(f3sm));
        if (it != reg().end()) for (auto& w : it->second) if ((f = w.lock())) break;   // 같은(불변) 파일이면 어느 것이든 같다
    }
    if (!f) { setErr(err, "3SM 파일이 닫혀 있습니다: " + f3sm.filename().u8string()); return false; }
    return f->readNodeTile(id, out, err);
}

}  // namespace asec
