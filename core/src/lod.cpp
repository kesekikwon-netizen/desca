#include "asec/lod.hpp"
#include <chrono>

#include <algorithm>
#include <functional>
#include <queue>
#include <thread>

namespace asec {

static size_t tileBytes(const TmxTile& t) {
    size_t b = t.bytes.size();
    for (auto& n : t.nodes) for (auto& m : n.meshes) b += m->pos.size() * 4 + m->uv.size() * 4 + m->idx.size() * 4;
    for (auto& tx : t.textures) b += tx.second->rgba.px.size();
    return b;
}

namespace {
/// RGBA 를 f×f 상자 평균으로 줄인다(f = 2ⁿ)
void downsampleRgba(RgbaImage& im, int f) {
    if (f <= 1 || im.empty()) return;
    const int w = std::max(1, im.w / f), h = std::max(1, im.h / f);
    std::vector<uint8_t> out(size_t(w) * h * 4);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            unsigned acc[4] = {0, 0, 0, 0}; unsigned n = 0;
            for (int dy = 0; dy < f; ++dy) {
                const int sy = y * f + dy; if (sy >= im.h) break;
                const uint8_t* row = &im.px[(size_t(sy) * im.w) * 4];
                for (int dx = 0; dx < f; ++dx) {
                    const int sx = x * f + dx; if (sx >= im.w) break;
                    acc[0] += row[sx * 4]; acc[1] += row[sx * 4 + 1]; acc[2] += row[sx * 4 + 2]; acc[3] += row[sx * 4 + 3]; ++n;
                }
            }
            uint8_t* o = &out[(size_t(y) * w + x) * 4];
            for (int k = 0; k < 4; ++k) o[k] = uint8_t(n ? (acc[k] + n / 2) / n : 0);
        }
    im.w = w; im.h = h; im.px.swap(out);
}
/// 노드 크기와 영상 크기로 텍셀 길이(m) 추정
double texelOf(const Box3& bb, const RgbaImage& im) {
    if (!bb.valid() || im.empty()) return 0;
    const Vec3 d = bb.mx - bb.mn;
    const double side = std::max({d.x, d.y, d.z});
    return side > 0 ? side / std::max(im.w, im.h) : 0;
}
/// 요청 텍셀(want)에 맞는 2ⁿ 축소 배수(원본 텍셀 texel): 줄인 텍셀 texel×f ≤ want 인 가장 큰 f(최대 16) — 영상 1 px 에 텍셀 1개 이상
int downsampleFactor(double texel, double want) {
    if (!(texel > 0) || !(want > 0)) return 1;
    int f = 1;
    while (f < 16 && texel * (f * 2) <= want) f *= 2;
    return f;
}
}  // namespace

namespace { std::atomic<size_t> g_texDecodes{0}, g_evictScans{0}, g_evicted{0}; std::atomic<double> g_texMs{0.0}; }
TileCacheCounters tileCacheCounters() { TileCacheCounters c; c.texDecodes = g_texDecodes.load(); c.texMs = g_texMs.load(); c.evictScans = g_evictScans.load(); c.evicted = g_evicted.load(); return c; }

void TileCache::setBytesLocked(Entry& e, size_t b) {
    total_ -= e.bytes; e.bytes = b; total_ += b;
}

std::shared_ptr<TmxTile> TileCache::get(const fs::path& p, std::string* err) {
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = tiles_.find(p);
        if (it != tiles_.end()) { it->second.lastUse = ++clock_; return it->second.tile; }
    }
    auto t = std::make_shared<TmxTile>();
    if (!readTmxTile(p, *t, err)) return nullptr;  // 파일 읽기는 잠금 밖
    std::lock_guard<std::mutex> lk(mu_);
    auto it = tiles_.find(p);
    if (it != tiles_.end()) return it->second.tile;
    Entry& e = tiles_[p];
    e.tile = t; e.lastUse = ++clock_;
    setBytesLocked(e, tileBytes(*t));   // 3MX = 원본 바이트. 3SM 가상 타일 = 원시 BLOB 바이트
    evictLocked();
    return t;
}

bool TileCache::decode(const std::shared_ptr<TmxTile>& t, size_t node, std::string* err) {
    if (!t || node >= t->nodes.size()) return false;
    std::vector<TexturePtr> todo;
    const double want = wantTexelM.load();
    Box3 nodeBox;
    {
        std::lock_guard<std::mutex> lk(*t->mu);  // 이 타일만 잠금
        TmxNode& N = t->nodes[node];
        if (!N.ready || want > 0) {
            if (!decodeNode(*t, node, err)) return false;
            nodeBox = N.bb;
            if (textureDecoder)
                for (auto& m : N.meshes) {
                    auto& tx = m->texture;
                    if (!tx || tx->encoded.empty() || std::find(todo.begin(), todo.end(), tx) != todo.end()) continue;
                    const bool missing = tx->rgba.empty();
                    // 전에 더 거칠게 줄여 둔 것이면(요청이 더 촘촘) 다시 디코드
                    const bool tooCoarse = !missing && want > 0 && tx->rgbaDownsample > 1 && tx->rgbaTexelM > want * 1.001;
                    if (missing || tooCoarse) todo.push_back(tx);
                }
        }
        if (todo.empty()) { N.ready = true; }
    }
    if (!todo.empty()) {
        // JPG 디코드(가장 비싼 부분)는 잠금 없이 사본에 → 잠금 안에서 옮김. 요청 해상도보다 훨씬 촘촘한 사진은 2ⁿ 로 줄여 둔다(메모리·예산)
        std::vector<RgbaImage> imgs(todo.size());
        std::vector<double> texel(todo.size(), 0.0);
        std::vector<int> factor(todo.size(), 1);
        for (size_t k = 0; k < todo.size(); ++k) {
            Texture tmp;
            tmp.format = todo[k]->format; tmp.filePath = todo[k]->filePath; tmp.encoded = todo[k]->encoded;
            const auto td0 = std::chrono::steady_clock::now();
            textureDecoder(tmp);
            g_texDecodes++; { double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - td0).count(); double cur = g_texMs.load(); while (!g_texMs.compare_exchange_weak(cur, cur + ms)) {} }
            texel[k] = texelOf(nodeBox, tmp.rgba);
            factor[k] = downsampleFactor(texel[k], want);
            downsampleRgba(tmp.rgba, factor[k]);
            imgs[k] = std::move(tmp.rgba);
        }
        std::lock_guard<std::mutex> lk(*t->mu);
        for (size_t k = 0; k < todo.size(); ++k) {
            auto& tx = todo[k];
            const bool replaceCoarser = !tx->rgba.empty() && tx->rgbaDownsample > factor[k];
            if ((tx->rgba.empty() || replaceCoarser) && !imgs[k].empty()) { tx->rgba = std::move(imgs[k]); tx->rgbaTexelM = texel[k] * factor[k]; tx->rgbaDownsample = factor[k]; }
        }
        t->nodes[node].ready = true;
    }
    size_t b;
    {
        std::lock_guard<std::mutex> lk(*t->mu);
        b = tileBytes(*t);
    }
    std::lock_guard<std::mutex> lk(mu_);
    auto it = tiles_.find(t->path);
    if (it != tiles_.end() && it->second.tile == t) { setBytesLocked(it->second, b); evictLocked(); }
    return true;
}

bool TileCache::decodeGeometry(const std::shared_ptr<TmxTile>& t, size_t node, std::string* err) {
    if (!t || node >= t->nodes.size()) return false;
    {
        std::lock_guard<std::mutex> lk(*t->mu);
        if (!decodeNode(*t, node, err)) return false;   // 이미 했으면 바로 돌아옴(decoded)
    }
    size_t b;
    { std::lock_guard<std::mutex> lk(*t->mu); b = tileBytes(*t); }
    std::lock_guard<std::mutex> lk(mu_);
    auto it = tiles_.find(t->path);
    if (it != tiles_.end() && it->second.tile == t) { setBytesLocked(it->second, b); evictLocked(); }
    return true;
}

size_t TileCache::loadedBytes() const {
    std::lock_guard<std::mutex> lk(mu_);
    return total_;
}

void TileCache::clear() {
    std::lock_guard<std::mutex> lk(mu_);
    tiles_.clear(); total_ = 0;
}

void TileCache::evictLocked() {
    // 예산 안이면 아무것도 훑지 않는다(수집 중에는 get/decode 마다 불리므로 O(n) 훑기가 큰 비용이었다).
    // 예산을 넘었는데 모든 타일이 사용 중(수집이 붙잡음)이면 훑어도 뺄 것이 없다 → 타일 수만큼 건너뛰어 상각 O(1)
    if (total_ <= budget_) { evictSkip_ = 0; return; }
    if (evictSkip_ > 0) { --evictSkip_; return; }
    // 한 번 훑어 사용 중이 아닌 타일을 오래된 순으로 모아 예산 안에 들 때까지 뺀다(훑기 한 번에 여러 개 — 수집 중 반복 호출에도 O(n) 한 번)
    g_evictScans++;
    std::vector<std::pair<uint64_t, std::map<fs::path, Entry>::iterator>> cand;
    for (auto it = tiles_.begin(); it != tiles_.end(); ++it)
        if (it->second.tile.use_count() <= 1) cand.emplace_back(it->second.lastUse, it);
    if (cand.empty()) { evictSkip_ = tiles_.size(); return; }
    std::sort(cand.begin(), cand.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (auto& c : cand) {
        if (total_ <= budget_ || tiles_.size() <= 1) break;
        total_ -= c.second->second.bytes;
        tiles_.erase(c.second);
        g_evicted++;
    }
    if (total_ > budget_) evictSkip_ = tiles_.size();   // 남은 건 전부 사용 중 → 당분간 훑지 않는다
}

namespace {
struct Job { std::shared_ptr<TmxTile> tile; size_t node; };

/// 모은 노드를 여러 스레드로 디코드(타일 단위 잠금이라 서로 다른 타일·노드가 동시에 진행).
bool decodeJobs(TileCache& c, const std::vector<Job>& jobs, std::string* err, const std::atomic<bool>* cancel, bool textures = true, size_t maxThreads = 8) {
    if (jobs.empty()) return true;
    unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    size_t nth = std::min<size_t>({jobs.size(), size_t(hw), std::max<size_t>(1, maxThreads)});
    std::atomic<size_t> next{0};
    std::atomic<bool> failed{false};
    std::mutex emu; std::string firstErr;
    auto run = [&] {
        for (;;) {
            if (failed.load() || (cancel && cancel->load())) return;
            size_t k = next++;
            if (k >= jobs.size()) return;
            std::string e;
            if (!(textures ? c.decode(jobs[k].tile, jobs[k].node, &e) : c.decodeGeometry(jobs[k].tile, jobs[k].node, &e))) {
                std::lock_guard<std::mutex> lk(emu);
                if (!failed.exchange(true)) firstErr = e;
            }
        }
    };
    if (nth <= 1) run();
    else {
        std::vector<std::thread> th;
        for (size_t i = 1; i < nth; ++i) th.emplace_back(run);
        run();
        for (auto& t : th) t.join();
    }
    if (failed) { if (err) *err = firstErr; return false; }
    if (cancel && cancel->load()) return false;
    return true;
}
}  // namespace

static bool collectRec(TileCache& c, const fs::path& p, const BandQuad& band, std::vector<Job>& out, LeafStats& st, std::string* err, int depth,
                       const std::atomic<bool>* cancel) {
    if (cancel && cancel->load()) return false;
    auto t = c.get(p, err);
    if (!t) return false;
    st.tilesVisited++;
    st.maxDepth = std::max(st.maxDepth, depth);
    for (size_t i = 0; i < t->nodes.size(); ++i) {
        const TmxNode& n = t->nodes[i];
        if (n.bb.valid() && !bandIntersectsBox(band, n.bb)) continue;
        bool useSelf = n.isLeaf();
        if (!n.isLeaf()) {
            for (auto& ch : n.children) {
                std::string e2;
                if (!collectRec(c, ch, band, out, st, &e2, depth + 1, cancel)) {
                    if (cancel && cancel->load()) return false;
                    useSelf = true;  // 자식 파일 없음 → 이 노드가 가장 고해상도
                    st.fallbackNodes++;
                    if (err) *err = e2;
                }
            }
        }
        if (useSelf) { out.push_back({t, i}); st.leafNodes++; }
    }
    return true;
}

bool collectLeafMeshes(TileCache& c, const fs::path& root, const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err,
                       const std::atomic<bool>* cancel, bool textures, size_t maxThreads) {
    LeafStats s;
    std::vector<Job> jobs;
    const auto t0 = std::chrono::steady_clock::now();
    bool ok = collectRec(c, root, band, jobs, s, err, 0, cancel);
    const auto t1 = std::chrono::steady_clock::now();
    if (ok) ok = decodeJobs(c, jobs, err, cancel, textures, maxThreads);
    const auto t2 = std::chrono::steady_clock::now();
    s.msRead = std::chrono::duration<double, std::milli>(t1 - t0).count();
    s.msDecode = std::chrono::duration<double, std::milli>(t2 - t1).count();
    if (ok)
        for (auto& j : jobs)
            for (auto& m : j.tile->nodes[j.node].meshes) {
                if (m->bbox.valid() && !bandIntersectsBox(band, m->bbox)) continue;
                out.push_back(m); s.meshes++; s.triangles += m->triangleCount();
                if (m->texture) s.texBytes += m->texture->rgba.px.size();
            }
    if (st) *st = s;
    return ok;
}

static bool boxOverlapXY(const Box3& a, const Box3& b) {
    return a.valid() && b.valid() && !(a.mx.x < b.mn.x || a.mn.x > b.mx.x || a.mx.y < b.mn.y || a.mn.y > b.mx.y);
}

using OverlapFn = std::function<bool(const Box3&)>;

static bool resRec(TileCache& c, const fs::path& p, const OverlapFn& overlap, double res, std::vector<Job>& out, LeafStats& st, std::string* err, int depth,
                   const std::atomic<bool>* cancel) {
    if (cancel && cancel->load()) return false;
    auto t = c.get(p, err);
    if (!t) return false;
    st.tilesVisited++;
    st.maxDepth = std::max(st.maxDepth, depth);
    for (size_t i = 0; i < t->nodes.size(); ++i) {
        const TmxNode& n = t->nodes[i];
        if (n.bb.valid() && !overlap(n.bb)) continue;
        bool enough = n.isLeaf() || (n.maxScreenDiameter > 0 && n.bb.diag() / res <= n.maxScreenDiameter);
        bool useSelf = enough;
        if (!enough) {
            for (auto& ch : n.children) {
                std::string e2;
                if (!resRec(c, ch, overlap, res, out, st, &e2, depth + 1, cancel)) {
                    if (cancel && cancel->load()) return false;
                    useSelf = true; st.fallbackNodes++;
                }
            }
        }
        if (useSelf) { out.push_back({t, i}); st.leafNodes++; }
    }
    return true;
}

static bool collectRes(TileCache& c, const fs::path& root, const OverlapFn& overlap, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err,
                       const std::atomic<bool>* cancel) {
    LeafStats s;
    std::vector<Job> jobs;
    bool ok = resRec(c, root, overlap, res, jobs, s, err, 0, cancel);
    if (ok) ok = decodeJobs(c, jobs, err, cancel);
    if (ok)
        for (auto& j : jobs)
            for (auto& m : j.tile->nodes[j.node].meshes) {
                if (m->bbox.valid() && !overlap(m->bbox)) continue;
                out.push_back(m); s.meshes++; s.triangles += m->triangleCount();
            }
    if (st) *st = s;
    return ok;
}

bool collectMeshesForResolution(TileCache& c, const fs::path& root, const Box3& area, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err,
                                const std::atomic<bool>* cancel) {
    return collectRes(c, root, [&](const Box3& b) { return boxOverlapXY(b, area); }, res, out, st, err, cancel);
}

bool collectBandMeshesForResolution(TileCache& c, const fs::path& root, const BandQuad& band, double res, std::vector<MeshPtr>& out, LeafStats* st,
                                    std::string* err, const std::atomic<bool>* cancel) {
    if (res <= 0) return collectLeafMeshes(c, root, band, out, st, err, cancel);
    return collectRes(c, root, [&](const Box3& b) { return bandIntersectsBox(band, b); }, res, out, st, err, cancel);
}

namespace {
struct DNode {
    std::shared_ptr<TmxTile> tile; size_t idx; double diag; size_t tris;
};
}

bool selectDisplayMeshes(TileCache& c, const fs::path& root, size_t budget, std::vector<MeshPtr>& out, Box3* bbox, std::string* err,
                         const std::function<void(double)>& progress) {
    auto rt = c.get(root, err);
    if (!rt) return false;
    auto cmp = [](const DNode& a, const DNode& b) { return a.diag < b.diag; };
    std::priority_queue<DNode, std::vector<DNode>, decltype(cmp)> open(cmp);  // 세분 후보(큰 것 먼저)
    std::vector<DNode> final;
    size_t total = 0;
    auto triCount = [&](const std::shared_ptr<TmxTile>& t, size_t i) {
        size_t n = 0; for (auto& m : t->nodes[i].meshes) n += m->triangleCount(); return n;
    };
    for (size_t i = 0; i < rt->nodes.size(); ++i) {
        if (!c.decode(rt, i, err)) return false;
        DNode d{rt, i, rt->nodes[i].bb.diag(), triCount(rt, i)};
        total += d.tris;
        open.push(d);
    }
    int steps = 0;
    while (!open.empty()) {
        DNode d = open.top(); open.pop();
        const TmxNode& n = d.tile->nodes[d.idx];
        if (n.isLeaf()) { final.push_back(d); continue; }
        std::vector<DNode> kids; size_t kt = 0; bool ok = true;
        for (auto& ch : n.children) {
            std::string e2;
            auto t = c.get(ch, &e2);
            if (!t) { ok = false; break; }
            for (size_t i = 0; i < t->nodes.size(); ++i) {
                if (!c.decode(t, i, &e2)) { ok = false; break; }
                DNode k{t, i, t->nodes[i].bb.diag(), triCount(t, i)};
                kt += k.tris; kids.push_back(k);
            }
            if (!ok) break;
        }
        if (!ok || total - d.tris + kt > budget) { final.push_back(d); continue; }
        total = total - d.tris + kt;
        for (auto& k : kids) open.push(k);
        if (progress && (++steps % 4) == 0) progress(std::min(1.0, double(total) / double(budget)));
    }
    Box3 bb;
    for (auto& d : final)
        for (auto& m : d.tile->nodes[d.idx].meshes) { out.push_back(m); bb.add(m->bbox); }
    if (bbox) *bbox = bb;
    return true;
}

}  // namespace asec
