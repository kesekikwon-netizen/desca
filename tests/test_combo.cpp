// 평·단면도 한 장 조판(2026-10-16): 레벨선은 땅속에만 · 단면 표시 높이는 잘린 선 기준 · 단면 배치(펼친 평면에 맞춤)
#include <cmath>
#include <sstream>
#include "catch_amalgamated.hpp"
#include "asec/engine.hpp"
#include "asec/sheet.hpp"

using namespace asec;
using Catch::Approx;

namespace {
// 지면 z=1 → 수혈(s 2–6, 바닥 z=0.2) → 지면 z=1 (s 0–8)
std::vector<Polyline> pitProfile() {
    return {{{0, 1.0}, {2, 1.0}, {2.5, 0.2}, {5.5, 0.2}, {6, 1.0}, {8, 1.0}}};
}
}  // namespace

TEST_CASE("레벨선 땅속만: 수혈 속 · 지표 위에는 없고 수혈 바닥 아래는 끝까지", "[combo]") {
    const auto pr = pitProfile();
    // z = 0.6: 벽 교차점 s = 2.25 · 5.75 → [0, 2.25] [5.75, 8]
    auto v = levelLineSpans(pr, 0.6, 0, 8);
    REQUIRE(v.size() == 2);
    CHECK(v[0].s0 == Approx(0)); CHECK(v[0].s1 == Approx(2.25));
    CHECK(v[1].s0 == Approx(5.75)); CHECK(v[1].s1 == Approx(8));
    // 수혈 바닥보다 깊음 → 전체 한 줄
    v = levelLineSpans(pr, 0.1, 0, 8);
    REQUIRE(v.size() == 1);
    CHECK(v[0].s0 == Approx(0)); CHECK(v[0].s1 == Approx(8));
    // 지표 위 → 없음
    CHECK(levelLineSpans(pr, 1.05, 0, 8).empty());
    // 지표와 같은 높이 → 지표가 있는 곳만(점이 z 위면 땅으로 셈)
    v = levelLineSpans(pr, 1.0, 0, 8);
    REQUIRE(v.size() == 2);
    CHECK(v[0].s1 == Approx(2)); CHECK(v[1].s0 == Approx(6));
    // [s0, s1] 로 자름
    v = levelLineSpans(pr, 0.6, 1, 7);
    REQUIRE(v.size() == 2);
    CHECK(v[0].s0 == Approx(1)); CHECK(v[1].s1 == Approx(7));
}

TEST_CASE("레벨선 땅속만: 닫힌 고리(돌)는 무시 · 잘린 선 빈 구간에는 긋지 않음", "[combo]") {
    std::vector<Polyline> pr = {{{0, 1}, {3, 1}}, {{5, 1}, {8, 1}}};
    pr.push_back({{4, 2}, {4.5, 2}, {4.5, 2.5}, {4, 2.5}, {4, 2}});   // 공중의 닫힌 고리
    auto v = levelLineSpans(pr, 0.5, 0, 8);
    REQUIRE(v.size() == 2);
    CHECK(v[0].s1 == Approx(3)); CHECK(v[1].s0 == Approx(5));
    CHECK(levelLineSpans(pr, 2.2, 0, 8).empty());
}

TEST_CASE("DXF 레벨선 땅속만 선택: 수혈 가운데를 지나는 레벨선 없음, 끄면 옛 동작", "[combo][dxf]") {
    SectionResult r;
    r.line = SectionLine{{0, 0}, {8, 0}, 0, 0.5};
    r.srs.origin = Vec3(0, 0, 30);
    r.profile = pitProfile();
    r.zMin = 0.1; r.zMax = 1.1;
    auto spansAt = [&](bool below) {
        DxfExportOptions o; o.levelsBelowGround = below;
        std::string txt, err;
        REQUIRE(exportSectionDxf(r, o, {}, &err, &txt));
        std::istringstream in(txt);
        std::string c, val, layer;
        int crossing = 0, lines = 0;
        bool inLine = false; double x0 = 0, x1 = 0;
        while (std::getline(in, c) && std::getline(in, val)) {
            const int code = std::stoi(c);
            if (code == 0) { inLine = val == "LINE"; layer.clear(); }
            if (!inLine) continue;
            if (code == 8) layer = val;
            if (code == 10) x0 = std::stod(val);
            if (code == 11) {
                x1 = std::stod(val);
                if (layer.rfind("LEVEL_", 0) == 0 && layer != "LEVEL_TEXT") {
                    ++lines;
                    if (std::min(x0, x1) < 4.0 && std::max(x0, x1) > 4.0) ++crossing;
                }
            }
        }
        return std::make_pair(lines, crossing);
    };
    const auto off = spansAt(false), on = spansAt(true);
    CHECK(off.first == 11);          // 30.1 – 31.1 m, 10 cm 마다 한 줄
    CHECK(off.second == 11);
    CHECK(on.second == 2);           // 수혈 바닥(30.2)과 같거나 깊은 30.1 · 30.2 m 만 가운데를 지남
    CHECK(on.first == 2 + 2 * 8);    // 30.3 – 31.0 은 수혈 양쪽 두 조각, 31.1(지표 위)은 없음
}

namespace {
// 평평한 땅(z=0, 10×6 m) + 단면선 뒤 2.8 m 의 기둥(높이 1.5 m)
std::shared_ptr<StaticSource> postScene() {
    auto s = std::make_shared<StaticSource>();
    auto g = std::make_shared<Mesh>();
    const int nx = 50, ny = 30;
    for (int j = 0; j <= ny; ++j) for (int i = 0; i <= nx; ++i) g->pos.insert(g->pos.end(), {float(i * 0.2), float(j * 0.2), 0.f});
    for (int j = 0; j < ny; ++j) for (int i = 0; i < nx; ++i) { uint32_t a = uint32_t(j * (nx + 1) + i), b = a + 1, c = a + uint32_t(nx + 1), d = c + 1; g->idx.insert(g->idx.end(), {a, b, d, a, d, c}); }
    g->computeBBox();
    auto post = std::make_shared<Mesh>();
    const double x0 = 4.9, y0 = 3.7, x1 = 5.1, y1 = 3.9, z0 = -0.05, z1 = 1.5;
    const double P[8][3] = {{x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}, {x0, y1, z0}, {x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}};
    for (auto& p : P) post->pos.insert(post->pos.end(), {float(p[0]), float(p[1]), float(p[2])});
    const uint32_t F[12][3] = {{0, 1, 5}, {0, 5, 4}, {1, 2, 6}, {1, 6, 5}, {2, 3, 7}, {2, 7, 6}, {3, 0, 4}, {3, 4, 7}, {4, 5, 6}, {4, 6, 7}, {0, 2, 1}, {0, 3, 2}};
    for (auto& f : F) post->idx.insert(post->idx.end(), {f[0], f[1], f[2]});
    post->computeBBox();
    s->meshes = {g, post};
    s->bounds = g->bbox; s->bounds.add(post->bbox);
    return s;
}
}  // namespace

TEST_CASE("단면 표시 높이: 잘린 선 기준이면 뒤 배경 기둥 높이를 따라가지 않음, 끄면 옛 동작", "[combo]") {
    auto s = postScene();
    SectionRequest rq; rq.line.a = {1, 1}; rq.line.b = {9, 1}; rq.line.front = 0; rq.line.back = 3.0; rq.imageRes = 0.02;
    SectionOutput oldO, newO; std::string err;
    REQUIRE(computeSection(*s, rq, oldO, &err));
    rq.zRangeFromProfile = true;
    REQUIRE(computeSection(*s, rq, newO, &err));
    CHECK(oldO.result.zMax >= 1.5);
    CHECK(newO.result.zMax == Approx(0.3));   // 지면 0 + 여유 0.25 → 10 cm 올림
    CHECK(newO.result.zMin == Approx(-0.3));
    CHECK(newO.result.zMinCut == Approx(0).margin(1e-6));
    CHECK(newO.result.zMaxCut == Approx(0).margin(1e-6));
    // 잘린 선은 같다
    REQUIRE(oldO.result.profile.size() == newO.result.profile.size());
    for (size_t i = 0; i < oldO.result.profile.size(); ++i) CHECK(oldO.result.profile[i] .size() == newO.result.profile[i].size());
}

TEST_CASE("한 장 조판: 첫 단면이 가로(위), 위로 가는 단면은 왼쪽 · 아래로 가는 단면은 오른쪽, 끝이 평면 표시와 맞음", "[combo][sheet]") {
    SheetSpec sp; sp.denom = 30;
    // 첫 단면 A(0,0)→A′(4,0) 동쪽. 둘째 B(2,-2)→B′(2,2) 북쪽(위로). 셋째 C(3,2)→C′(3,-2) 남쪽(아래로). 넷째 D(4,1)→D′(0,1) 서쪽
    std::vector<ComboSection> secs = {{0, 0, 4, 0, 1.0}, {2, -2, 2, 2, 0.8}, {3, 2, 3, -2, 0.8}, {4, 1, 0, 1, 0.6}};
    ComboLayout c = layoutCombo(sp, secs, 1.0);
    REQUIRE(c.sections.size() == 4);
    CHECK(c.sections[0].slot == ComboSlot::Top);
    CHECK(c.sections[1].slot == ComboSlot::Left);  CHECK(c.sections[1].rotDeg == -90);
    CHECK(c.sections[2].slot == ComboSlot::Right); CHECK(c.sections[2].rotDeg == 90);
    CHECK(c.sections[3].slot == ComboSlot::Bottom);
    CHECK(c.northDeg() == Approx(0));
    const double k = 1000.0 / 30;
    // 평면 칸 = 단면선 범위 + 1 m
    CHECK(c.planW == Approx(6 * k));
    CHECK(c.planH == Approx(6 * k));
    double px, py;
    // A–A′(위): 단면 x = 0 / L 이 평면의 A / A′ 가로 자리, 단면 칸 아래 + 여백이 평면 위
    const ComboPlaced& a = c.sections[0];
    a.toPaper(0, 0, px, py); CHECK(px == Approx(c.xMm(0)));
    a.toPaper(a.plotW, 0, px, py); CHECK(px == Approx(c.xMm(4)));
    a.toPaper(0, a.plotH + sp.gutterBottomMm, px, py); CHECK(py < c.planY);
    // B–B′(왼쪽, 반시계 90°): B 아래 · B′ 위가 평면의 B · B′ 세로 자리, 땅속(+y)이 평면 쪽(오른쪽)
    const ComboPlaced& b = c.sections[1];
    b.toPaper(0, 0, px, py); CHECK(py == Approx(c.yMm(-2)));
    b.toPaper(b.plotW, 0, px, py); CHECK(py == Approx(c.yMm(2)));
    double pxDeep, pyDeep; b.toPaper(0, b.plotH, pxDeep, pyDeep);
    CHECK(pxDeep > px);
    b.toPaper(0, b.plotH + sp.gutterBottomMm, px, py); CHECK(px < c.planX);
    // C–C′(오른쪽, 시계 90°): C 위 · C′ 아래, 땅속이 평면 쪽(왼쪽)
    const ComboPlaced& cc = c.sections[2];
    cc.toPaper(0, 0, px, py); CHECK(py == Approx(c.yMm(2)));
    cc.toPaper(cc.plotW, 0, px, py); CHECK(py == Approx(c.yMm(-2)));
    cc.toPaper(0, cc.plotH, pxDeep, pyDeep); CHECK(pxDeep < px);
    cc.toPaper(0, cc.plotH + sp.gutterBottomMm, px, py); CHECK(px > c.planX + c.planW);
    // 아래: 평면 아래에 보통 방향
    const ComboPlaced& d = c.sections[3];
    d.toPaper(0, -sp.gutterTopMm, px, py); CHECK(py > c.planY + c.planH);
    // 1:30 A4 가로(그림 칸 267 × 181 mm)에 안 들어감 → 맞춤 축척은 그보다 큼
    const double fit = fitComboDenomStep10(sp, secs, 1.0);
    CHECK(fit > 30);
    sp.denom = fit;
    CHECK(layoutCombo(sp, secs, 1.0).fits);
    sp.denom = fit - 10;
    CHECK_FALSE(layoutCombo(sp, secs, 1.0).fits);
}

TEST_CASE("한 장 조판: 첫 단면이 북쪽으로 가면 평면을 돌리고 진북은 오른쪽(90°)", "[combo][sheet]") {
    SheetSpec sp; sp.denom = 50;
    ComboLayout c = layoutCombo(sp, {{10, 10, 10, 15, 1.0}}, 1.0);
    REQUIRE(c.sections.size() == 1);
    CHECK(c.sections[0].slot == ComboSlot::Top);
    CHECK(c.northDeg() == Approx(90));
    // 돌린 평면 영상 범위는 단면선 둘레 1 m 를 덮는다
    PlanStripArea ar = comboPlanArea(c);
    CHECK(ar.x0 == Approx(9)); CHECK(ar.x1 == Approx(11));
    CHECK(ar.y0 == Approx(9)); CHECK(ar.y1 == Approx(16));
    CHECK(layoutCombo(sp, {}, 1.0).sections.empty());
}
