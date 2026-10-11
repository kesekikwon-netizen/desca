// 3SM(ScalableMesh, SQLite) 판독: 합성 3SM(tests/sm3_synth.hpp) 으로 색인·기하·가상 타일·단면 일치·읽기 전용을 고정한다.
#include "catch_amalgamated.hpp"
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <thread>
#include "asec/engine.hpp"
#include "asec/lod.hpp"
#include "asec/sm3.hpp"
#include "asec/stream.hpp"
#include "common.hpp"
#include "sm3_synth.hpp"
#include "synth.hpp"

using namespace asec;
using Catch::Approx;

namespace {
const Vec3 kOrigin{200000.0, 450000.0, 40.0};   // 합성 지면의 실좌표 원점(synth 와 같음)
constexpr double kSize = 8, kLeaf = 0.05;

fs::path writeGround(const fs::path& dir, sm3synth::File* keep = nullptr) {
    auto f = sm3synth::groundFile(kOrigin, kSize, kLeaf);
    fs::path out = dir / "ground.3sm";
    std::string err;
    REQUIRE(sm3synth::write(out, f, &err));
    if (keep) *keep = f;
    return out;
}
Box3 extBox(const double* e) { Box3 b; b.add(Vec3(e[0], e[1], e[2])); b.add(Vec3(e[3], e[4], e[5])); return b; }
bool sameBox(const Box3& a, const Box3& b, double tol = 1e-6) {
    return std::fabs(a.mn.x - b.mn.x) < tol && std::fabs(a.mn.y - b.mn.y) < tol && std::fabs(a.mn.z - b.mn.z) < tol &&
           std::fabs(a.mx.x - b.mx.x) < tol && std::fabs(a.mx.y - b.mx.y) < tol && std::fabs(a.mx.z - b.mx.z) < tol;
}
std::vector<fs::path> sideFiles(const fs::path& dir) {
    std::vector<fs::path> v;
    for (auto& e : fs::directory_iterator(dir)) {
        auto n = e.path().filename().u8string();
        if (n.find("-journal") != std::string::npos || n.find("-wal") != std::string::npos || n.find("-shm") != std::string::npos) v.push_back(e.path());
    }
    return v;
}
}  // namespace

TEST_CASE("3SM 열기: 노드 색인·뿌리·원점·범위를 읽고 기하는 읽지 않는다(한글·공백 경로)") {
    auto dir = tmpDir("sm3kor") / fs::u8path("한글 폴더 3sm");   // 한글·공백 경로(SQLite URI 퍼센트 인코딩)
    fs::create_directories(dir);
    sm3synth::File f;
    fs::path file = writeGround(dir, &f);
    CHECK(isSqliteFile(file));
    std::string err;
    auto sm = SmFile::open(file, &err);
    REQUIRE(sm);
    CHECK(sm->nodes().size() == 4);
    CHECK(sm->rootId() == 2);
    CHECK(sm->nodes().at(2).children == std::vector<int64_t>{3});
    CHECK(sm->nodes().at(3).children == std::vector<int64_t>{4, 5});
    CHECK(sm->nodes().at(4).children.empty());
    CHECK(sm->nodes().at(3).parent == 2);
    CHECK(sm->nodes().at(3).level == 1);
    CHECK_FALSE(sm->nodes().at(2).hasMesh);
    CHECK(sm->nodes().at(3).hasMesh);
    CHECK(sm->nodes().at(3).geomRes == Approx(kLeaf * 4));
    // 원점 = 뿌리 Extent 중심 xy 를 1 m 로 반올림, z = 0 (높이 값은 그대로)
    const Box3 rootExt = extBox(f.nodes[0].ext);
    CHECK(sm->origin().x == Approx(std::round(rootExt.center().x)));
    CHECK(sm->origin().y == Approx(std::round(rootExt.center().y)));
    CHECK(sm->origin().z == 0);
    Box3 expect; expect.add(rootExt.mn - sm->origin()); expect.add(rootExt.mx - sm->origin());
    CHECK(sameBox(sm->boundsLocal(), expect));
    CHECK(sm->gcs() == f.gcs);
    CHECK(sm->geometryReads() == 0);   // 열 때는 색인만
    CHECK(sm->virtualTilePath(3) == file / "n3");

    // 가상 경로 판별
    fs::path f3; int64_t id = 0;
    CHECK(parseSmVirtualPath(file / "n12", &f3, &id)); CHECK(id == 12); CHECK(f3 == file);
    CHECK_FALSE(parseSmVirtualPath(file / "x12", &f3, &id));
    CHECK_FALSE(parseSmVirtualPath(dir / "a.3mxb", &f3, &id));
    CHECK_FALSE(parseSmVirtualPath(file, &f3, &id));

    // 3SM 이 아닌 파일 → 오류(한국어, "3SM" 포함)
    { std::ofstream t(dir / "text.3sm"); t << "hello"; }
    CHECK_FALSE(isSqliteFile(dir / "text.3sm"));
    err.clear();
    CHECK_FALSE(SmFile::open(dir / "text.3sm", &err));
    CHECK(err.find("3SM") != std::string::npos);
    // 표 없는 SQLite → 오류
    {
        sqlite3* db = nullptr;
        REQUIRE(sqlite3_open_v2((dir / "empty.3sm").u8string().c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK);
        sqlite3_exec(db, "CREATE TABLE t (a)", nullptr, nullptr, nullptr);
        sqlite3_close(db);
    }
    err.clear();
    CHECK_FALSE(SmFile::open(dir / "empty.3sm", &err));
    CHECK(err.find("3SM") != std::string::npos);
    CHECK_FALSE(SmFile::open(dir / "none.3sm", &err));
}

TEST_CASE("3SM 노드 → 가상 타일: 좌표(실좌표−원점)·1 기반 보정·UV·JPEG 텍스처·자식·maxScreenDiameter") {
    auto dir = tmpDir("sm3node");
    sm3synth::File f;
    fs::path file = writeGround(dir, &f);
    std::string err;
    auto sm = SmFile::open(file, &err);
    REQUIRE(sm);
    const Vec3 o = sm->origin();
    const sm3synth::Node& n4 = f.nodes[2];
    TmxTile t;
    REQUIRE(sm->readNodeTile(4, t, &err));
    CHECK(t.path == sm->virtualTilePath(4));
    REQUIRE(t.nodes.size() == 1);
    const TmxNode& N = t.nodes[0];
    CHECK(N.id == "4");
    CHECK(N.children.empty());
    CHECK(N.decoded);
    CHECK_FALSE(N.ready);
    Box3 eb = extBox(n4.ext); Box3 ebl; ebl.add(eb.mn - o); ebl.add(eb.mx - o);
    CHECK(sameBox(N.bb, ebl, 1e-6));
    REQUIRE(N.meshes.size() == 1);
    const Mesh& m = *N.meshes[0];
    REQUIRE(m.vertexCount() == n4.xyz.size() / 3);
    for (size_t i = 0; i < m.vertexCount(); ++i) {
        CHECK(m.pos[3 * i] == Approx(float(n4.xyz[3 * i] - o.x)).margin(1e-6));
        CHECK(m.pos[3 * i + 1] == Approx(float(n4.xyz[3 * i + 1] - o.y)).margin(1e-6));
        CHECK(m.pos[3 * i + 2] == Approx(float(n4.xyz[3 * i + 2] - o.z)).margin(1e-6));
    }
    REQUIRE(m.idx.size() == n4.idx1.size());
    for (size_t k = 0; k < m.idx.size(); ++k) CHECK(m.idx[k] == uint32_t(n4.idx1[k] - 1));
    REQUIRE(m.uv.size() == n4.uv.size());
    for (size_t k = 0; k < m.uv.size(); ++k) CHECK(m.uv[k] == Approx(n4.uv[k]).margin(1e-6));
    CHECK(sameBox(m.bbox, ebl, 1e-5));
    REQUIRE(m.texture);
    CHECK(m.texture->format == "jpg");
    REQUIRE(m.texture->encoded.size() > 2);
    CHECK(m.texture->encoded[0] == 0xFF); CHECK(m.texture->encoded[1] == 0xD8);
    decodeTexStb(*m.texture);
    CHECK(m.texture->rgba.w == 32); CHECK(m.texture->rgba.h == 32);

    // 중간 노드 3: 자식 가상 경로, maxScreenDiameter = 대각선 / GeometryResolution
    TmxTile t3;
    REQUIRE(sm->readNodeTile(3, t3, &err));
    REQUIRE(t3.nodes.size() == 1);
    CHECK(t3.nodes[0].children == std::vector<fs::path>{sm->virtualTilePath(4), sm->virtualTilePath(5)});
    CHECK(t3.nodes[0].maxScreenDiameter == Approx(extBox(f.nodes[1].ext).diag() / f.nodes[1].geomRes));
    CHECK(t3.nodes[0].meshes.size() == 1);
    CHECK(sm->geometryReads() == 2);

    CHECK_FALSE(N.resourceIds.empty());   // 평면 스트리머(LodStreamer)는 resourceIds 가 비면 "그릴 것 없음" 으로 본다

    // 뿌리 2: 메시 없음, 자식만, 상자는 Extent
    TmxTile t2;
    REQUIRE(sm->readNodeTile(2, t2, &err));
    REQUIRE(t2.nodes.size() == 1);
    CHECK(t2.nodes[0].meshes.empty());
    CHECK(t2.nodes[0].resourceIds.empty());
    CHECK(t2.nodes[0].children == std::vector<fs::path>{sm->virtualTilePath(3)});
    CHECK(t2.nodes[0].bb.valid());
    CHECK(sameBox(t2.nodes[0].bb, sm->boundsLocal()));
    CHECK(t2.nodes[0].decoded);

    // 없는 노드
    TmxTile tx;
    CHECK_FALSE(sm->readNodeTile(99, tx, &err));
}

TEST_CASE("3SM: UV 인덱스가 점 인덱스와 다르면 (점, UV) 짝마다 꼭짓점을 나눈다") {
    auto dir = tmpDir("sm3uvsplit");
    sm3synth::File f;
    f.rootId = 2;
    sm3synth::Node n; n.id = 2; n.parent = -1; n.level = 0; n.geomRes = 0.1; n.texId = 1;
    n.xyz = {200000, 450000, 40, 200001, 450000, 40, 200001, 450001, 40.5, 200000, 450001, 40.5};
    n.idx1 = {1, 2, 3, 1, 3, 4};
    n.uv = {0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1};
    n.uvIdx1 = {1, 2, 3, 4, 5, 6};
    n.ext[0] = 200000; n.ext[1] = 450000; n.ext[2] = 40; n.ext[3] = 200001; n.ext[4] = 450001; n.ext[5] = 40.5;
    f.nodes = {n};
    f.jpegByTexId[1] = sm3synth::makeJpeg(); f.texSize[1] = {32, 32};
    fs::path file = dir / "split.3sm";
    std::string err;
    REQUIRE(sm3synth::write(file, f, &err));
    auto sm = SmFile::open(file, &err);
    REQUIRE(sm);
    TmxTile t;
    REQUIRE(sm->readNodeTile(2, t, &err));
    REQUIRE(t.nodes.size() == 1);
    REQUIRE(t.nodes[0].meshes.size() == 1);
    const Mesh& m = *t.nodes[0].meshes[0];
    CHECK(m.vertexCount() == 6);
    CHECK(m.idx.size() == 6);
    CHECK(m.uv.size() == 12);
    // 둘째 삼각형의 첫 꼭짓점 = 점 1 + UV 4 → 위치는 점 1, uv (0,0)
    const uint32_t v = m.idx[3];
    CHECK(m.pos[3 * v] == Approx(float(200000 - sm->origin().x)));
    CHECK(m.uv[2 * v] == Approx(0)); CHECK(m.uv[2 * v + 1] == Approx(0));
}

TEST_CASE("3SM: 텍스처 없음(TexID -1)·SMUVs 행 없음 → 텍스처 없는 메시, 깨진 BLOB·범위 밖 인덱스 → 그 노드만 실패") {
    auto dir = tmpDir("sm3bad");
    auto f = sm3synth::groundFile(kOrigin, kSize, kLeaf);
    f.nodes[2].texId = -1;                // 노드 4: 텍스처 없음
    f.nodes[3].writeUvs = false;          // 노드 5: UV 행 없음
    f.nodes[1].compress = false;          // 노드 3: 비압축 BLOB 도 읽힌다
    // 깨진 노드 6(zlib 머리만 있고 내용 없음), 7(인덱스 0), 8(인덱스 > 점 수)
    sm3synth::Node bad = sm3synth::nodeFromMesh(6, 3, 2, synth::gridMesh(0, 0, 1, 1, 0.5), kOrigin, kLeaf, -1);
    bad.rawPointBlob = {0x78, 0x9c, 0x00, 0x01, 0x02};
    sm3synth::Node zero = sm3synth::nodeFromMesh(7, 3, 2, synth::gridMesh(0, 0, 1, 1, 0.5), kOrigin, kLeaf, -1);
    zero.idx1[0] = 0;
    sm3synth::Node big = sm3synth::nodeFromMesh(8, 3, 2, synth::gridMesh(0, 0, 1, 1, 0.5), kOrigin, kLeaf, -1);
    big.idx1[1] = int32_t(big.xyz.size() / 3) + 1;
    f.nodes.push_back(bad); f.nodes.push_back(zero); f.nodes.push_back(big);
    f.nodes[1].children = {4, 5, 6, 7, 8};
    fs::path file = dir / "bad.3sm";
    std::string err;
    REQUIRE(sm3synth::write(file, f, &err));
    auto sm = SmFile::open(file, &err);
    REQUIRE(sm);
    CHECK(sm->nodes().size() == 7);
    TmxTile t4, t5, t3, t6, t7, t8;
    REQUIRE(sm->readNodeTile(4, t4, &err));
    REQUIRE(t4.nodes[0].meshes.size() == 1);
    CHECK_FALSE(t4.nodes[0].meshes[0]->texture);
    CHECK_FALSE(t4.nodes[0].meshes[0]->uv.empty());
    REQUIRE(sm->readNodeTile(5, t5, &err));
    REQUIRE(t5.nodes[0].meshes.size() == 1);
    CHECK(t5.nodes[0].meshes[0]->uv.empty());
    CHECK_FALSE(t5.nodes[0].meshes[0]->texture);
    REQUIRE(sm->readNodeTile(3, t3, &err));
    CHECK(t3.nodes[0].meshes.size() == 1);
    err.clear(); CHECK_FALSE(sm->readNodeTile(6, t6, &err)); CHECK_FALSE(err.empty());
    err.clear(); CHECK_FALSE(sm->readNodeTile(7, t7, &err)); CHECK_FALSE(err.empty());
    err.clear(); CHECK_FALSE(sm->readNodeTile(8, t8, &err)); CHECK_FALSE(err.empty());
    // 깨진 노드가 있어도 다른 노드는 계속 읽힌다
    TmxTile again;
    CHECK(sm->readNodeTile(4, again, &err));
}

TEST_CASE("3SM: 깨진 Size 값(거대)·순환 SubNode 는 그 노드 실패·간선 제거로 끝나고 프로그램은 계속 간다") {
    auto dir = tmpDir("sm3robust");
    auto f = sm3synth::groundFile(kOrigin, kSize, kLeaf);
    f.nodes[2].sizePtsOverride = int64_t(1) << 40;   // 노드 4: SizePts 가 1 TB
    f.nodes[1].children = {4, 5, 2, 4};              // 노드 3: 뿌리로 되돌아가는 간선 + 중복
    fs::path file = dir / "robust.3sm";
    std::string err;
    REQUIRE(sm3synth::write(file, f, &err));
    auto sm = SmFile::open(file, &err);
    REQUIRE(sm);
    CHECK(sm->nodes().at(3).children == std::vector<int64_t>{4, 5});
    CHECK(sm->cycleEdgesRemoved() == 2);
    TmxTile t4, t5;
    err.clear();
    CHECK_FALSE(sm->readNodeTile(4, t4, &err));
    CHECK(err.find("3SM") != std::string::npos);
    CHECK(sm->readNodeTile(5, t5, &err));
    SmSource src;
    REQUIRE(src.openSm(file, &err));
    CHECK_FALSE(src.scene.warnings.empty());
}

TEST_CASE("3SM 연결 풀: 여러 스레드가 동시에 노드를 읽어도 결과가 같다") {
    auto dir = tmpDir("sm3parallel");
    fs::path file = writeGround(dir);
    std::string err;
    auto sm = SmFile::open(file, &err);
    REQUIRE(sm);
    std::atomic<int> ok{0}, bad{0};
    std::vector<std::thread> th;
    for (int t = 0; t < 8; ++t)
        th.emplace_back([&, t] {
            for (int k = 0; k < 40; ++k) {
                TmxTile tile; std::string e;
                const int64_t id = 3 + (t + k) % 3;
                if (sm->readNodeTile(id, tile, &e) && tile.nodes.size() == 1 && tile.nodes[0].meshes.size() == 1) ++ok; else ++bad;
            }
        });
    for (auto& x : th) x.join();
    CHECK(ok == 320);
    CHECK(bad == 0);
    CHECK(sm->geometryReads() == 320);
}

TEST_CASE("3SM 가상 경로를 readTmxTile 이 읽고, 파일을 닫으면 오류로 돌려준다") {
    auto dir = tmpDir("sm3virtual");
    fs::path file = writeGround(dir);
    std::string err;
    auto sm = SmFile::open(file, &err);
    REQUIRE(sm);
    const fs::path vp = sm->virtualTilePath(3);
    TmxTile t;
    REQUIRE(readTmxTile(vp, t, &err));
    REQUIRE(t.nodes.size() == 1);
    CHECK(t.nodes[0].id == "3");
    CHECK(decodeNode(t, 0, &err));   // 이미 디코드됨 → 그대로 true
    CHECK(t.nodes[0].meshes.size() == 1);
    // TileCache 경로: 텍스처 디코드까지
    TileCache cache;
    cache.textureDecoder = decodeTexStb;
    auto ct = cache.get(vp, &err);
    REQUIRE(ct);
    REQUIRE(cache.decode(ct, 0, &err));
    CHECK(ct->nodes[0].ready);
    REQUIRE(ct->nodes[0].meshes[0]->texture);
    CHECK(ct->nodes[0].meshes[0]->texture->rgba.w == 32);
    CHECK(cache.loadedBytes() > 0);
    // 예산 회계: 가상 타일은 bytes 가 비어도 메시(float pos·uv·idx)와 풀린 텍스처 RGBA 가 잡혀야 한다
    {
        const Mesh& m = *ct->nodes[0].meshes[0];
        const size_t meshBytes = m.pos.size() * 4 + m.uv.size() * 4 + m.idx.size() * 4;
        CHECK(cache.loadedBytes() >= meshBytes + size_t(32) * 32 * 4);
        TileCache fresh;
        auto g = fresh.get(vp, &err);
        REQUIRE(g);
        CHECK(fresh.loadedBytes() >= meshBytes);   // get() 직후에도 0 이 아니다
    }
    // 같은 파일을 두 번 열어도(같은 불변 파일) 가상 타일은 계속 읽힌다
    {
        auto sm2 = SmFile::open(file, &err);
        REQUIRE(sm2);
        sm2.reset();
        TmxTile t3;
        CHECK(readTmxTile(vp, t3, &err));
    }
    sm.reset();   // 모델 닫음
    TmxTile t2;
    err.clear();
    CHECK_FALSE(readTmxTile(vp, t2, &err));
    CHECK(err.find("3SM") != std::string::npos);
}

TEST_CASE("3SM SmSource 단면 == 같은 형상 StaticSource 단면, LOD 깊이·표시 메시 선택") {
    auto dir = tmpDir("sm3section");
    fs::path file = writeGround(dir);
    std::string err;
    SmSource src;
    src.cache->textureDecoder = decodeTexStb;
    REQUIRE(src.openSm(file, &err));
    CHECK(src.scene.layers.size() == 1);
    CHECK(src.scene.roots().size() == 1);
    CHECK(src.srs.hasOrigin);
    CHECK(src.srs.origin.z == 0);
    CHECK(src.srs.epsg() == 5187);   // sm3synth 기본 GCS(동부원점 2010)
    CHECK(src.bounds.valid());
    CHECK(src.bounds.diag() < 20);

    StaticSource st;
    st.meshes = {synth::gridMesh(0, 0, kSize / 2, kSize, kLeaf), synth::gridMesh(kSize / 2, 0, kSize, kSize, kLeaf)};
    for (auto& m : st.meshes) st.bounds.add(m->bbox);
    st.srs.origin = kOrigin; st.srs.hasOrigin = true;

    // 실좌표 선(두 잎 노드를 모두 지남, 격자선 위에 놓이지 않게 y 는 0.05 의 배수를 피함) → 각 공급원의 로컬로
    const double ax = 200000.5, ay = 450004.313, bx = 200007.5, by = 450004.313;
    SectionRequest rA, rB;
    rA.line = SectionLine{{ax - src.srs.origin.x, ay - src.srs.origin.y}, {bx - src.srs.origin.x, by - src.srs.origin.y}, 0.0, 0.3};
    rB.line = SectionLine{{ax - kOrigin.x, ay - kOrigin.y}, {bx - kOrigin.x, by - kOrigin.y}, 0.0, 0.3};
    rA.wantImage = rB.wantImage = false;
    SectionOutput oA, oB;
    REQUIRE(computeSection(src, rA, oA, &err));
    REQUIRE(computeSection(st, rB, oB, &err));
    CHECK(oA.stats.fallbackNodes == 0);
    CHECK(oA.stats.maxDepth == 2);
    CHECK(oA.stats.leafNodes == 2);
    REQUIRE(oA.result.profile.size() == oB.result.profile.size());
    REQUIRE_FALSE(oA.result.profile.empty());
    for (size_t p = 0; p < oA.result.profile.size(); ++p) {
        REQUIRE(oA.result.profile[p].size() == oB.result.profile[p].size());
        for (size_t k = 0; k < oA.result.profile[p].size(); ++k) {
            CHECK(oA.result.profile[p][k].x == Approx(oB.result.profile[p][k].x).margin(2e-4));
            CHECK(oA.result.profile[p][k].y + src.srs.origin.z == Approx(oB.result.profile[p][k].y + kOrigin.z).margin(2e-4));
        }
    }
    // 화면 표시용 선택(평면 보기 예산 경로)
    std::vector<MeshPtr> disp; Box3 bb;
    REQUIRE(selectDisplayMeshes(*src.cache, src.scene.roots()[0], 1 << 20, disp, &bb, &err));
    CHECK_FALSE(disp.empty());
    CHECK(bb.valid());
    CHECK(src.cache->loadedBytes() > 0);
    // 지연 읽기: 노드 4개뿐이라 전부 읽혔을 수 있지만, 열기 직후에는 0 이었다(위 시험). 여기서는 읽기 수가 노드 수의 작은 배수 안
    CHECK(src.file->geometryReads() <= 12);
}

TEST_CASE("3SM 평면 스트리밍(LodStreamer): 가상 뿌리에서 내려가 잎까지 그린다") {
    auto dir = tmpDir("sm3stream");
    fs::path file = writeGround(dir);
    std::string err;
    SmSource src;
    REQUIRE(src.openSm(file, &err));
    LodStreamer::Config cfg; cfg.threads = 0;
    std::atomic<int> prepared{0};
    LodStreamer s(cfg, [&prepared](const TmxNode& n, size_t* bytes) -> LodStreamer::Payload {
        size_t b = 0;
        for (auto& m : n.meshes) b += m->pos.size() * 4 + m->idx.size() * 4;
        *bytes = b; ++prepared;
        return std::make_shared<int>(1);
    });
    s.setRoots(src.scene.roots());
    LodStreamer::View v;
    v.visible = [](const Box3&) { return true; };
    const double ppm = 200;   // 8 m 모델이 1600 px → 잎(5 cm)까지 세분해야 하는 배율
    v.screenDiameter = [ppm](const Box3& b) { return b.diag() * ppm; };
    LodStreamer::Frame f;
    for (int i = 0; i < 200; ++i) {
        f = s.update(v);
        for (auto k : f.upload) s.take(k);
        if (f.idle()) break;
        s.processOne();
    }
    CHECK(f.idle());
    CHECK(s.stats().tileFailures == 0);
    CHECK(s.stats().tilesLoaded == 4);
    CHECK(prepared == 3);   // 메시 있는 노드 3·4·5
    f = s.update(v);
    CHECK(f.draw.size() == 2);   // 잎 4·5 만 그린다
    CHECK(f.maxDepthDrawn == 2);
    for (auto k : f.draw) CHECK(s.nodeDepth(k) == 2);
}

TEST_CASE("3SM 읽기 전용: 열고 자른 뒤 파일 바이트 동일, 부속 파일 없음, 읽기 전용 속성 파일도 열린다") {
    auto dir = tmpDir("sm3readonly");
    fs::path file = writeGround(dir);
    std::vector<uint8_t> before, after;
    REQUIRE(readFileBytes(file, before));
    {
        std::string err;
        SmSource src;
        REQUIRE(src.openSm(file, &err));
        SectionRequest rq;
        rq.line = SectionLine{{-3, 0.3}, {3, 0.3}, 0.0, 0.3};
        rq.wantImage = false;
        SectionOutput so;
        REQUIRE(computeSection(src, rq, so, &err));
    }
    REQUIRE(readFileBytes(file, after));
    CHECK(before == after);
    CHECK(sideFiles(dir).empty());
    // 읽기 전용 속성
    std::error_code ec;
    fs::permissions(file, fs::perms::owner_write | fs::perms::group_write | fs::perms::others_write, fs::perm_options::remove, ec);
    {
        std::string err;
        auto sm = SmFile::open(file, &err);
        REQUIRE(sm);
        TmxTile t;
        CHECK(sm->readNodeTile(4, t, &err));
    }
    fs::permissions(file, fs::perms::owner_write, fs::perm_options::add, ec);
    CHECK(sideFiles(dir).empty());
}

TEST_CASE("실제 3SM(선택): 열기·노드 수·범위·좌표계·단면", "[real]") {
    const char* env = std::getenv("ASEC_REAL_3SM");
    if (!env || !*env) SKIP("ASEC_REAL_3SM 미설정");
    fs::path file(env);   // 환경 변수는 OS 문자 집합(Windows: ACP) — u8path 가 아님
    if (!fs::exists(file)) SKIP("ASEC_REAL_3SM 파일 없음");
    std::string err;
    SmSource src;
    src.cache->textureDecoder = decodeTexStb;
    REQUIRE(src.openSm(file, &err));
    CHECK(src.file->nodes().size() > 100);
    CHECK(src.bounds.valid());
    CHECK(src.bounds.diag() < 5000);
    SrsReport rep = analyzeSrs(src.srs, src.bounds);
    CHECK(rep.desc.horizontalEpsg != 0);
    CHECK_FALSE(rep.precisionWarning);
    INFO(rep.labelKo);
    const Vec3 c = src.bounds.center();
    const double hx = (src.bounds.mx.x - src.bounds.mn.x) * 0.45;
    SectionRequest rq;
    rq.line = SectionLine{{c.x - hx, c.y}, {c.x + hx, c.y}, 0.0, 1.0};
    rq.wantImage = false;
    SectionOutput so;
    REQUIRE(computeSection(src, rq, so, &err));
    CHECK_FALSE(so.result.profile.empty());
    CHECK(so.stats.fallbackNodes == 0);
    CHECK(src.file->geometryReads() < src.file->nodes().size());   // 전체 로드 아님
}
