// .3sm → 3MX 캐시. 3SM 구조(광령리 유구배치.3sm 로 확인):
//  SMMasterHeader.GCS        좌표계 WKT(COMPD_CS …)
//  SMNodeHeader              NodeId·ParentNodeId(-1 = 뿌리)·TexID·Extent(double 6: 최소 xyz, 최대 xyz)·GeometryResolution(m)
//  SMPoint.PointData         zlib, double xyz × n (실좌표)        IndexData   zlib, int32 × 3 × 삼각형(1부터)
//  SMUVs.UVData              zlib, double uv × k (v=0 이 영상 아래)  UVIndexData zlib, int32(1부터) — 꼭짓점 번호와 따로
//  SMTexture(NodeId = TexID) 16 바이트 머리(가로·세로·채널·0) + JPG
#include "sm3convert.hpp"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include "asec/tmx.hpp"

using namespace asec;

namespace {
constexpr int kCacheVersion = 1;

struct SmNode {
    qint64 id = 0, parent = -1, texId = -1;
    double ext[6] = {0, 0, 0, 0, 0, 0};
    bool hasExt = false;
    double geomRes = 0;
    QByteArray pts, idx, uv, uvIdx, tex;
    qint64 nPts = 0, nIdx = 0, nUv = 0, nUvIdx = 0;
    std::vector<qint64> children;
};

/// zlib(78 xx) 이면 qUncompress(앞에 4 바이트 길이를 붙여야 함), 아니면 그대로
QByteArray unz(const QByteArray& b, qint64 rawSize) {
    if (b.size() >= 2 && uchar(b[0]) == 0x78) {
        QByteArray w(4, '\0');
        const quint32 n = quint32(std::max<qint64>(rawSize, 0));
        w[0] = char(n >> 24); w[1] = char(n >> 16); w[2] = char(n >> 8); w[3] = char(n);
        w += b;
        return qUncompress(w);
    }
    return b;
}

template <class T>
std::vector<T> asVec(const QByteArray& b) {
    std::vector<T> v(size_t(b.size()) / sizeof(T));
    if (!v.empty()) std::memcpy(v.data(), b.constData(), v.size() * sizeof(T));
    return v;
}

QString nodeFile(qint64 id) { return QStringLiteral("n%1.3mxb").arg(id); }

/// 한 노드 → Mesh(실좌표 − origin, UV 번호가 따로라 (점, UV) 짝마다 꼭짓점 하나로 풂)
MeshPtr buildMesh(const SmNode& n, const Vec3& o) {
    const auto P = asVec<double>(unz(n.pts, n.nPts));
    const auto I = asVec<int32_t>(unz(n.idx, n.nIdx));
    if (P.size() < 9 || I.size() < 3) return nullptr;
    const size_t np = P.size() / 3;
    auto m = std::make_shared<Mesh>();
    std::vector<double> U;
    std::vector<int32_t> UI;
    if (!n.uv.isEmpty() && !n.uvIdx.isEmpty()) {
        U = asVec<double>(unz(n.uv, n.nUv));
        UI = asVec<int32_t>(unz(n.uvIdx, n.nUvIdx));
        if (UI.size() != I.size()) { U.clear(); UI.clear(); }
    }
    const size_t nuv = U.size() / 2;
    const bool textured = !UI.empty() && n.tex.size() > 20 && uchar(n.tex[16]) == 0xFF && uchar(n.tex[17]) == 0xD8;
    if (textured) {
        std::unordered_map<uint64_t, uint32_t> remap;
        remap.reserve(I.size());
        m->idx.reserve(I.size());
        for (size_t k = 0; k + 2 < I.size(); k += 3) {
            uint32_t tri[3];
            bool ok = true;
            for (int c = 0; c < 3 && ok; ++c) {
                const int64_t pi = int64_t(I[k + c]) - 1, ui = int64_t(UI[k + c]) - 1;
                if (pi < 0 || size_t(pi) >= np || ui < 0 || size_t(ui) >= nuv) { ok = false; break; }
                const uint64_t key = (uint64_t(pi) << 32) | uint64_t(ui);
                auto it = remap.find(key);
                if (it == remap.end()) {
                    const uint32_t nv = uint32_t(m->pos.size() / 3);
                    m->pos.push_back(float(P[3 * pi] - o.x)); m->pos.push_back(float(P[3 * pi + 1] - o.y)); m->pos.push_back(float(P[3 * pi + 2] - o.z));
                    m->uv.push_back(float(U[2 * ui])); m->uv.push_back(float(U[2 * ui + 1]));
                    it = remap.emplace(key, nv).first;
                }
                tri[c] = it->second;
            }
            if (ok) { m->idx.push_back(tri[0]); m->idx.push_back(tri[1]); m->idx.push_back(tri[2]); }
        }
        auto t = std::make_shared<Texture>();
        t->format = "jpg";
        t->encoded.assign(reinterpret_cast<const uint8_t*>(n.tex.constData()) + 16, reinterpret_cast<const uint8_t*>(n.tex.constData()) + n.tex.size());
        m->texture = t;
    } else {
        m->pos.resize(np * 3);
        for (size_t i = 0; i < np; ++i) {
            m->pos[3 * i] = float(P[3 * i] - o.x); m->pos[3 * i + 1] = float(P[3 * i + 1] - o.y); m->pos[3 * i + 2] = float(P[3 * i + 2] - o.z);
        }
        for (size_t k = 0; k + 2 < I.size(); k += 3) {
            const int64_t a = int64_t(I[k]) - 1, b = int64_t(I[k + 1]) - 1, c = int64_t(I[k + 2]) - 1;
            if (a < 0 || b < 0 || c < 0 || size_t(a) >= np || size_t(b) >= np || size_t(c) >= np) continue;
            m->idx.push_back(uint32_t(a)); m->idx.push_back(uint32_t(b)); m->idx.push_back(uint32_t(c));
        }
    }
    if (m->idx.empty()) return nullptr;
    m->computeBBox();
    return m;
}

TmxWriteNode writeNode(const SmNode& n, const Vec3& o) {
    TmxWriteNode w;
    w.id = std::to_string(n.id);
    if (MeshPtr m = buildMesh(n, o)) w.meshes.push_back(m);
    for (qint64 c : n.children) w.childFiles.push_back(nodeFile(c).toStdString());
    // 3MX 규칙: 노드 대각선(m) / 요청 해상도(m/px) ≤ maxScreenDiameter 이면 이 노드로 충분 → 대각선 / 이 노드 해상도
    const double dx = n.ext[3] - n.ext[0], dy = n.ext[4] - n.ext[1], dz = n.ext[5] - n.ext[2];
    const double diag = std::sqrt(dx * dx + dy * dy + dz * dz);
    w.maxScreenDiameter = (n.hasExt && n.geomRes > 0) ? diag / n.geomRes : 1e9;
    return w;
}
}  // namespace

bool convert3smTo3mx(const QString& file3sm, QString& out3mx, QString* err, const std::function<void(double)>& progress) {
    auto fail = [&](const QString& m) { if (err) *err = m; return false; };
    const QFileInfo fi(file3sm);
    if (!fi.exists()) return fail(QStringLiteral("파일이 없습니다"));
    const QByteArray key = (fi.absoluteFilePath() + QStringLiteral("|%1|%2|v%3").arg(fi.size()).arg(fi.lastModified().toMSecsSinceEpoch()).arg(kCacheVersion)).toUtf8();
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(key, QCryptographicHash::Sha1).toHex().left(16));
    QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (base.isEmpty()) base = QDir::tempPath() + QStringLiteral("/Kerf");
    const QString dir = base + QStringLiteral("/3sm/") + hash;
    out3mx = dir + QStringLiteral("/") + fi.completeBaseName() + QStringLiteral(".3mx");
    if (QFile::exists(dir + QStringLiteral("/ok.txt")) && QFile::exists(out3mx)) { if (progress) progress(0.7); return true; }   // 전에 바꾼 것
    QDir(dir).removeRecursively();   // 끝나지 못한 옛 변환(이 앱 캐시 폴더 안)
    if (!QDir().mkpath(dir)) return fail(QStringLiteral("캐시 폴더를 만들 수 없습니다: %1").arg(dir));

    // ---- SQLite 읽기(읽기 전용)
    std::map<qint64, SmNode> nodes;
    std::string gcs;
    const QString conn = QStringLiteral("kerf3sm_%1").arg(hash);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(fi.absoluteFilePath());
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (!db.open()) { const QString e = db.lastError().text(); db = QSqlDatabase(); QSqlDatabase::removeDatabase(conn); return fail(QStringLiteral("3SM(SQLite)을 열 수 없습니다: ") + e); }
        bool ok = true;
        QString qerr;
        auto run = [&](const QString& sql, const std::function<void(QSqlQuery&)>& row) {
            if (!ok) return;
            QSqlQuery q(db);
            q.setForwardOnly(true);
            if (!q.exec(sql)) { ok = false; qerr = q.lastError().text(); return; }
            while (q.next()) row(q);
        };
        run(QStringLiteral("SELECT GCS FROM SMMasterHeader LIMIT 1"), [&](QSqlQuery& q) { gcs = q.value(0).toString().toStdString(); });
        run(QStringLiteral("SELECT NodeId, ParentNodeId, TexID, Extent, GeometryResolution FROM SMNodeHeader"), [&](QSqlQuery& q) {
            SmNode n;
            n.id = q.value(0).toLongLong(); n.parent = q.value(1).isNull() ? -1 : q.value(1).toLongLong();
            n.texId = q.value(2).isNull() ? n.id : q.value(2).toLongLong();
            const QByteArray e = q.value(3).toByteArray();
            if (e.size() >= 48) { std::memcpy(n.ext, e.constData(), 48); n.hasExt = true; }
            n.geomRes = q.value(4).toDouble();
            nodes[n.id] = std::move(n);
        });
        if (progress) progress(0.05);
        run(QStringLiteral("SELECT NodeId, PointData, IndexData, SizePts, SizeIndices FROM SMPoint"), [&](QSqlQuery& q) {
            auto it = nodes.find(q.value(0).toLongLong()); if (it == nodes.end()) return;
            it->second.pts = q.value(1).toByteArray(); it->second.idx = q.value(2).toByteArray();
            it->second.nPts = q.value(3).toLongLong(); it->second.nIdx = q.value(4).toLongLong();
        });
        if (progress) progress(0.12);
        run(QStringLiteral("SELECT NodeId, UVData, SizeUVs, UVIndexData, SizeUVIndex FROM SMUVs"), [&](QSqlQuery& q) {
            auto it = nodes.find(q.value(0).toLongLong()); if (it == nodes.end()) return;
            it->second.uv = q.value(1).toByteArray(); it->second.nUv = q.value(2).toLongLong();
            it->second.uvIdx = q.value(3).toByteArray(); it->second.nUvIdx = q.value(4).toLongLong();
        });
        std::map<qint64, QByteArray> tex;
        run(QStringLiteral("SELECT NodeId, TexData FROM SMTexture"), [&](QSqlQuery& q) { tex[q.value(0).toLongLong()] = q.value(1).toByteArray(); });
        for (auto& [id, n] : nodes) if (auto t = tex.find(n.texId); t != tex.end()) n.tex = t->second;
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(conn);
        if (!ok) return fail(QStringLiteral("3SM 표를 읽지 못했습니다(ScalableMesh 형식이 아닐 수 있음): ") + qerr);
    }
    if (progress) progress(0.2);
    if (nodes.empty()) return fail(QStringLiteral("3SM 에 메시 노드가 없습니다"));

    // ---- 트리·원점
    std::vector<qint64> roots;
    for (auto& [id, n] : nodes) {
        if (n.parent >= 0 && nodes.count(n.parent)) nodes[n.parent].children.push_back(id);
        else roots.push_back(id);
    }
    for (auto& [id, n] : nodes) std::sort(n.children.begin(), n.children.end());
    Box3 all;
    for (qint64 r : roots) if (nodes[r].hasExt) { all.add(Vec3(nodes[r].ext[0], nodes[r].ext[1], nodes[r].ext[2])); all.add(Vec3(nodes[r].ext[3], nodes[r].ext[4], nodes[r].ext[5])); }
    Vec3 o;   // 원점: 범위 가운데를 1 m 로 반올림(xy), 높이는 0(파일 높이 그대로)
    if (all.valid()) { o.x = std::round(all.center().x); o.y = std::round(all.center().y); }

    // ---- 노드마다 .3mxb (여러 스레드). 뿌리들은 root.3mxb 한 파일
    std::vector<qint64> jobs;
    for (auto& [id, n] : nodes) if (n.parent >= 0 && nodes.count(n.parent)) jobs.push_back(id);
    std::atomic<size_t> next{0}, done{0};
    std::atomic<bool> bad{false};
    std::mutex emu;
    std::string firstErr;
    const unsigned nth = std::max(2u, std::min(8u, std::thread::hardware_concurrency()));
    std::vector<std::thread> pool;
    for (unsigned t = 0; t < nth; ++t)
        pool.emplace_back([&] {
            for (size_t k; !bad && (k = next++) < jobs.size();) {
                const SmNode& n = nodes.at(jobs[k]);
                std::string e;
                if (!writeTmxTile(std::filesystem::path((dir + QStringLiteral("/") + nodeFile(n.id)).toStdWString()), {writeNode(n, o)}, &e)) {
                    std::lock_guard<std::mutex> g(emu);
                    if (firstErr.empty()) firstErr = e;
                    bad = true;
                }
                ++done;
            }
        });
    while (done < jobs.size() && !bad) {
        if (progress) progress(0.2 + 0.45 * double(done) / std::max<size_t>(1, jobs.size()));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    for (auto& th : pool) th.join();
    if (bad) return fail(QStringLiteral("3MX 캐시를 쓰지 못했습니다: ") + QString::fromStdString(firstErr));
    std::vector<TmxWriteNode> rootNodes;
    for (qint64 r : roots) rootNodes.push_back(writeNode(nodes[r], o));
    std::string e;
    if (!writeTmxTile(std::filesystem::path((dir + QStringLiteral("/root.3mxb")).toStdWString()), rootNodes, &e))
        return fail(QStringLiteral("3MX 캐시를 쓰지 못했습니다: ") + QString::fromStdString(e));
    SrsInfo srs;
    srs.srs = gcs;   // 파일 원문 WKT 그대로
    srs.origin = o; srs.hasOrigin = true;
    if (!writeTmxScene(std::filesystem::path(out3mx.toStdWString()), fi.completeBaseName().toStdString(), srs, "root.3mxb", &e))
        return fail(QStringLiteral("3MX 캐시를 쓰지 못했습니다: ") + QString::fromStdString(e));
    QFile ok(dir + QStringLiteral("/ok.txt"));
    if (ok.open(QIODevice::WriteOnly)) ok.write(QStringLiteral("Kerf 3sm cache v%1\n%2\n").arg(kCacheVersion).arg(fi.absoluteFilePath()).toUtf8());
    if (progress) progress(0.7);
    return true;
}
