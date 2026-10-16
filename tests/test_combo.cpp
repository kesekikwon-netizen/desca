// 평·단면도 한 장 조판(2026-10-16): 레벨선은 땅속에만 · 단면 표시 높이는 잘린 선 기준 · 단면 배치(펼친 평면에 맞춤)
#include <cmath>
#include <sstream>
#include "catch_amalgamated.hpp"
#include "asec/engine.hpp"
#include "asec/export.hpp"
#include "asec/raster.hpp"
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

TEST_CASE("레벨선 땅속만: 처마처럼 뒤로 기운 벽 아래 빈 곳에는 긋지 않음", "[combo]") {
    // 낮은 지면 z=0(s 0–3) → 벽이 뒤로 기울어 위 끝이 s=2 (처마) → 높은 단 z=1(s 2–6)
    // 벽 아래(s 2–2.5, z 0.5)는 처마 밑 빈 곳 — 옛 방식(덮개 합)은 여기에도 그었다(사용자 2026-10-16 「빨간선을 넘어간다」)
    const std::vector<Polyline> pr = {{{0, 0}, {3, 0}, {2, 1}, {6, 1}}};
    auto v = levelLineSpans(pr, 0.5, 0, 6);
    REQUIRE(v.size() == 1);
    CHECK(v[0].s0 == Approx(2.5)); CHECK(v[0].s1 == Approx(6));
    // 낮은 지면보다 깊으면 끝까지 한 줄
    v = levelLineSpans(pr, -0.5, 0, 6);
    REQUIRE(v.size() == 1);
    CHECK(v[0].s0 == Approx(0)); CHECK(v[0].s1 == Approx(6));
    // 높은 단 위는 없음
    CHECK(levelLineSpans(pr, 1.2, 0, 6).empty());
}

TEST_CASE("레벨선 땅속만: 끝점은 늘 잘린 선 위(선을 넘지 않음)", "[combo]") {
    // 들쭉날쭉 앞뒤로 오가는 벽(실제 메시처럼) — 모든 구간 끝은 s 0 · 끝이거나 잘린 선과 만나는 점
    std::vector<Polyline> pr = {{{0, 1.0}, {1.0, 1.0}, {1.3, 0.8}, {0.9, 0.6}, {1.4, 0.4}, {1.1, 0.2}, {1.6, 0.0},
                                 {4.0, 0.0}, {4.2, 0.5}, {3.9, 0.7}, {4.4, 1.0}, {6.0, 1.0}}};
    auto onProfile = [&](double s, double z) {
        for (const auto& pl : pr)
            for (size_t i = 1; i < pl.size(); ++i) {
                const Vec2 a = pl[i - 1], b = pl[i];
                if ((a.y >= z) == (b.y >= z)) continue;
                const double sx = a.x + (b.x - a.x) * (z - a.y) / (b.y - a.y);
                if (std::fabs(sx - s) < 1e-9) return true;
            }
        return false;
    };
    for (double z = -0.05; z < 1.0; z += 0.1) {
        for (const SGap& g : levelLineSpans(pr, z, 0, 6)) {
            CHECK((g.s0 <= 1e-9 || onProfile(g.s0, z)));
            CHECK((g.s1 >= 6 - 1e-9 || onProfile(g.s1, z)));
            // 구간 가운데는 땅속: 위로 올라가며 잘린 선을 홀수 번 만난다
            const double m = 0.5 * (g.s0 + g.s1);
            int above = 0;
            for (size_t i = 1; i < pr[0].size(); ++i) {
                const Vec2 a = pr[0][i - 1], b = pr[0][i];
                if ((a.x <= m) == (b.x <= m)) continue;
                if (a.y + (b.y - a.y) * (m - a.x) / (b.x - a.x) >= z) ++above;
            }
            CHECK(above % 2 == 1);
        }
    }
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
                if (layer == "0.2" || layer == "0.5") {   // 해발선 레이어(사용자 2026-10-16 「0.2 0.5 0.8로 레이어」)
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
    CHECK(newO.result.zMax == Approx(0).margin(1e-9));   // 지면 0 = 10 cm 선 위 그대로
    CHECK(newO.result.zMin == Approx(-0.3));             // 0 선에서 10 cm 레벨선 세 칸 더 아래(사용자 2026-10-16 「3칸만 더」)
    CHECK(newO.result.zMinCut == Approx(0).margin(1e-6));
    CHECK(newO.result.zMaxCut == Approx(0).margin(1e-6));
    // 잘린 선은 같다
    // 절대 표고 기준 10 cm: 원점 z 가 0.05 면 잘린 선 절대 0.05 → 위 0.1, 아래 0.0 − 0.3
    s->srs.origin = Vec3(0, 0, 0.05);
    SectionOutput offO; REQUIRE(computeSection(*s, rq, offO, &err));
    CHECK(offO.result.zMax + 0.05 == Approx(0.1));
    CHECK(offO.result.zMin + 0.05 == Approx(-0.3));
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

TEST_CASE("한 장 조판(용지 없음): 내용 외접 상자 + 여백으로 종이를 잡고 배치 관계는 평행 이동만", "[combo][sheet]") {
    SheetSpec sp; sp.denom = 30;
    std::vector<ComboSection> secs = {{0, 0, 4, 0, 1.0}, {2, -2, 2, 2, 0.8}};
    ComboLayout c = layoutCombo(sp, secs, 1.0);
    const double ox = c.sections[1].originX - c.planX, oy = c.sections[0].originY - c.planY;
    freeComboPage(c, 10.0, 16.0);
    CHECK(c.fits);
    CHECK(c.sections[1].originX - c.planX == Approx(ox));
    CHECK(c.sections[0].originY - c.planY == Approx(oy));
    // 모든 칸이 [10, 종이 − 10] 안, 왼쪽 · 위 가장자리가 정확히 10 mm
    double x0 = 1e9, y0 = 1e9, x1 = -1e9, y1 = -1e9;
    auto add = [&](double x, double y) { x0 = std::min(x0, x); y0 = std::min(y0, y); x1 = std::max(x1, x); y1 = std::max(y1, y); };
    add(c.planX, c.planY); add(c.planX + c.planW, c.planY + c.planH);
    for (auto& p : c.sections)
        for (double x : {p.areaX, p.areaX + p.areaW}) for (double y : {p.areaY, p.areaY + p.areaH}) { double px, py; p.toPaper(x, y, px, py); add(px, py); }
    const ComboNorth n = comboNorthSpot(c);   // 진북도 내용(평면 칸 바깥에 놓이므로 종이가 그만큼 넓어진다)
    add(n.x0, n.y0); add(n.x1, n.y1);
    CHECK(x0 == Approx(10)); CHECK(y0 == Approx(10));
    CHECK(c.L.paperW == Approx(x1 + 10));
    CHECK(c.L.paperH == Approx(y1 + 10 + 16));
    CHECK(c.L.titleY > y1);
    // 평면은 단면 둘레 1 m · 1:30 → 6 m = 200 mm 폭: 용지 크기와 무관
    CHECK(c.planW == Approx(6000.0 / 30));
}

TEST_CASE("평·단면도 용지(사용자 2026-10-16 「용지방향이안돼」 → 「둘 다」): 단추 차례 · 정한 용지 가운데 · 넘치면 맞는 축척", "[combo][sheet]") {
    // 단추 차례: 내용 크기(기본) → A4 가로 → A4 세로 → A3 가로 → A3 세로 → 내용 크기
    CHECK(nextComboPaper(ComboPaper::Content) == ComboPaper::A4Landscape);
    CHECK(nextComboPaper(ComboPaper::A4Landscape) == ComboPaper::A4Portrait);
    CHECK(nextComboPaper(ComboPaper::A4Portrait) == ComboPaper::A3Landscape);
    CHECK(nextComboPaper(ComboPaper::A3Landscape) == ComboPaper::A3Portrait);
    CHECK(nextComboPaper(ComboPaper::A3Portrait) == ComboPaper::Content);
    double w = 0, h = 0;
    CHECK_FALSE(comboPaperSize(ComboPaper::Content, w, h));
    REQUIRE(comboPaperSize(ComboPaper::A4Landscape, w, h)); CHECK(w == Approx(297)); CHECK(h == Approx(210));
    REQUIRE(comboPaperSize(ComboPaper::A4Portrait, w, h)); CHECK(w == Approx(210)); CHECK(h == Approx(297));
    REQUIRE(comboPaperSize(ComboPaper::A3Landscape, w, h)); CHECK(w == Approx(420)); CHECK(h == Approx(297));
    REQUIRE(comboPaperSize(ComboPaper::A3Portrait, w, h)); CHECK(w == Approx(297)); CHECK(h == Approx(420));

    SheetSpec sp; sp.denom = 50;
    std::vector<ComboSection> secs = {{0, 0, 4, 0, 1.0}, {2, -2, 2, 2, 0.8}};
    ComboLayout fr = layoutCombo(sp, secs, 1.0);
    freeComboPage(fr, 10.0, 16.0);
    auto bbox = [](const ComboLayout& c, double& x0, double& y0, double& x1, double& y1) {
        x0 = c.planX; y0 = c.planY; x1 = c.planX + c.planW; y1 = c.planY + c.planH;
        for (auto& p : c.sections)
            for (double x : {p.areaX, p.areaX + p.areaW}) for (double y : {p.areaY, p.areaY + p.areaH}) {
                double px, py; p.toPaper(x, y, px, py);
                x0 = std::min(x0, px); y0 = std::min(y0, py); x1 = std::max(x1, px); y1 = std::max(y1, py);
            }
        const ComboNorth n = comboNorthSpot(c);
        x0 = std::min(x0, n.x0); y0 = std::min(y0, n.y0); x1 = std::max(x1, n.x1); y1 = std::max(y1, n.y1);
    };
    for (auto [pw, ph] : {std::pair<double, double>{297, 210}, {210, 297}}) {
        ComboLayout c = layoutCombo(sp, secs, 1.0);
        REQUIRE(placeComboOnPaper(c, pw, ph, 10.0, 16.0));
        CHECK(c.fits); CHECK(c.L.fits);
        CHECK(c.L.paperW == Approx(pw)); CHECK(c.L.paperH == Approx(ph));
        // 배치 관계는 내용 크기 종이와 같다(평행 이동만) · 축척 그대로
        CHECK(c.sections[1].originX - c.planX == Approx(fr.sections[1].originX - fr.planX));
        CHECK(c.sections[0].originY - c.planY == Approx(fr.sections[0].originY - fr.planY));
        CHECK(c.planW == Approx(fr.planW));
        CHECK(c.L.contentW == Approx(fr.L.contentW)); CHECK(c.L.contentH == Approx(fr.L.contentH));
        // 내용 외접 상자는 여백(10 mm)과 아래 띠(16 mm) 안쪽의 가운데
        double x0, y0, x1, y1; bbox(c, x0, y0, x1, y1);
        CHECK(x0 >= 10 - 1e-6); CHECK(y0 >= 10 - 1e-6);
        CHECK(x1 <= pw - 10 + 1e-6); CHECK(y1 <= ph - 10 - 16 + 1e-6);
        CHECK(x0 - 10 == Approx(pw - 10 - x1));
        CHECK(y0 - 10 == Approx(ph - 10 - 16 - y1));
        CHECK(c.L.titleY > y1); CHECK(c.L.titleY < ph);
    }
    // 넘침: 1:20 은 A4 세로에 안 들어감(가운데에 놓되 fits = false) → 들어가는 가장 큰 축척(10 단위)을 알려 준다
    sp.denom = 20;
    ComboLayout big = layoutCombo(sp, secs, 1.0);
    CHECK_FALSE(placeComboOnPaper(big, 210, 297, 10.0, 16.0));
    CHECK_FALSE(big.fits); CHECK_FALSE(big.L.fits);
    CHECK(big.L.paperW == Approx(210));
    const double fit = fitComboDenomOnPaper(sp, secs, 1.0, 210, 297, 10.0, 16.0);
    CHECK(fit > 20);
    sp.denom = fit;
    ComboLayout yes = layoutCombo(sp, secs, 1.0);
    CHECK(placeComboOnPaper(yes, 210, 297, 10.0, 16.0));
    sp.denom = fit - 10;
    ComboLayout no = layoutCombo(sp, secs, 1.0);
    CHECK_FALSE(placeComboOnPaper(no, 210, 297, 10.0, 16.0));
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

namespace {
struct MmBox { double x0, y0, x1, y1; };
bool apart(const MmBox& a, const MmBox& b, double gap) {
    return a.x1 + gap <= b.x0 || b.x1 + gap <= a.x0 || a.y1 + gap <= b.y0 || b.y1 + gap <= a.y0;
}
MmBox sectionBox(const ComboPlaced& p) {
    MmBox b{1e9, 1e9, -1e9, -1e9};
    for (double x : {p.areaX, p.areaX + p.areaW})
        for (double y : {p.areaY, p.areaY + p.areaH}) {
            double px, py; p.toPaper(x, y, px, py);
            b.x0 = std::min(b.x0, px); b.y0 = std::min(b.y0, py); b.x1 = std::max(b.x1, px); b.y1 = std::max(b.y1, py);
        }
    return b;
}
/// 진북이 평면 칸 · 단면 칸 · 평면 단면선 글자(A · A′, 선 끝에서 kComboPlanLabelMm 바깥)와 떨어져 있고, 원과 「N」 을 다 품는다
void checkNorthOutside(const ComboLayout& c) {
    const ComboNorth n = comboNorthSpot(c);
    const MmBox nb{n.x0, n.y0, n.x1, n.y1};
    CHECK(n.r == Approx(kComboNorthRMm));
    CHECK(n.x0 <= n.cx - n.r + 1e-9); CHECK(n.x1 >= n.cx + n.r - 1e-9);
    CHECK(n.y0 <= n.cy - n.r + 1e-9); CHECK(n.y1 >= n.cy + n.r - 1e-9);
    const double a = c.northDeg() * 3.14159265358979323846 / 180.0;
    const double tx = n.cx + std::sin(a) * (n.r + kComboNorthLabelMm), ty = n.cy - std::cos(a) * (n.r + kComboNorthLabelMm);
    CHECK(tx >= n.x0); CHECK(tx <= n.x1); CHECK(ty >= n.y0); CHECK(ty <= n.y1);
    CHECK(apart(nb, {c.planX, c.planY, c.planX + c.planW, c.planY + c.planH}, 1.0));
    for (const ComboPlaced& p : c.sections) {
        CHECK(apart(nb, sectionBox(p), 1.0));
        const double ax = c.xMm(p.sA), ay = c.yMm(p.tA), bx = c.xMm(p.sB), by = c.yMm(p.tB);
        const double dl = std::hypot(bx - ax, by - ay);
        if (dl < 1e-9) continue;
        const double ux = (bx - ax) / dl, uy = (by - ay) / dl;
        for (auto [lx, ly] : {std::pair<double, double>{ax - ux * kComboPlanLabelMm, ay - uy * kComboPlanLabelMm},
                              {bx + ux * kComboPlanLabelMm, by + uy * kComboPlanLabelMm}})
            CHECK(apart(nb, {lx - 2.2, ly - 1.5, lx + 2.2, ly + 1.5}, 0.5));
    }
}
} // namespace

TEST_CASE("평·단면도 진북(사용자 2026-10-16 「나침반이 안으로?」): 평면 사진 위가 아니라 칸 바깥 · 단면 · 글자와 안 겹침 · 종이 안", "[combo][sheet][north]") {
    SheetSpec sp; sp.denom = 30;
    SheetSpec app = sp;   // 앱 평·단면도의 글자 여백(comboSpec)
    app.gutterLeftMm = 14; app.gutterRightMm = 3; app.gutterTopMm = 6; app.gutterBottomMm = 2;
    const std::vector<std::vector<ComboSection>> cases = {
        {{0, 0, 4, 0, 1.0}, {2, -2, 2, 2, 0.8}},                                          // 위 + 왼쪽(사용자 화면)
        {{0, 0, 4, 0, 1.0}, {2, -2, 2, 2, 0.8}, {3, 2, 3, -2, 0.8}, {4, 1, 0, 1, 0.6}},   // 네 쪽 모두
        {{0, 0, 3, 3, 1.0}, {0, 3, 3, 0, 0.8}},                                           // 기준이 북동 → 진북이 비스듬
        {{10, 10, 10, 15, 1.0}},                                                          // 기준이 북쪽 → 진북 오른쪽(90°)
    };
    for (const SheetSpec& s : {sp, app})
        for (double margin : {1.0, 0.0})   // 0 = 단면선 끝이 평면 칸 테두리(사용자 화면처럼 A′ 글자가 칸 바로 밖)
            for (const auto& secs : cases) {
                ComboLayout c = layoutCombo(s, secs, margin, 8.0, 4.0);
                REQUIRE_FALSE(c.sections.empty());
                checkNorthOutside(c);
                // 내용 크기 종이 · 정한 용지 모두 진북이 여백 안(종이를 그림 + 진북에 맞춤)
                freeComboPage(c, 10.0, 16.0);
                checkNorthOutside(c);
                ComboNorth n = comboNorthSpot(c);
                CHECK(n.x0 >= 10 - 1e-6); CHECK(n.y0 >= 10 - 1e-6);
                CHECK(n.x1 <= c.L.paperW - 10 + 1e-6); CHECK(n.y1 <= c.L.paperH - 10 - 16 + 1e-6);
                ComboLayout big = layoutCombo(s, secs, margin, 8.0, 4.0);
                if (placeComboOnPaper(big, 420, 297, 10.0, 16.0)) {
                    n = comboNorthSpot(big);
                    CHECK(n.x0 >= 10 - 1e-6); CHECK(n.x1 <= 420 - 10 + 1e-6);
                    CHECK(n.y0 >= 10 - 1e-6); CHECK(n.y1 <= 297 - 10 - 16 + 1e-6);
                }
            }
    // 끄는 동안 붙든 틀(120° 돌린 평면)도 같은 규칙
    const ComboLayout fr = comboFrameForRotation(120.0);
    ComboLayout c = layoutCombo(app, cases[0], 0.0, 8.0, 4.0, &fr);
    CHECK(c.northDeg() == Approx(120.0));
    checkNorthOutside(c);
    // 사용자 화면(위 + 왼쪽): 평면 오른쪽 위 바깥
    c = layoutCombo(app, cases[0], 0.0, 8.0, 4.0);
    const ComboNorth n = comboNorthSpot(c);
    CHECK(n.x0 >= c.planX + c.planW);
    CHECK(n.cy < c.planY + c.planH * 0.5);
}

namespace {
// 바닥 판 하나(z = 0): x 0–4, y 0–2 — 위에서 보면 회색
MeshPtr floorSlab() {
    auto m = std::make_shared<Mesh>();
    m->pos = {0, 0, 0, 4, 0, 0, 4, 2, 0, 0, 2, 0};
    m->idx = {0, 1, 2, 0, 2, 3};
    m->computeBBox();
    return m;
}
bool covered(const RgbaImage& im, int i, int j) { return im.px[(size_t(j) * im.w + i) * 4 + 3] > 0; }
}  // namespace

TEST_CASE("돌린 평면을 바로 그림: 돌리지 않으면 renderPlan 과 같은 픽셀", "[combo][plan]") {
    const std::vector<MeshPtr> ms = {floorSlab()};
    RgbaImage a, b;
    // renderPlan: 왼쪽 위 (−1, 3), 0.1 m/px, 60 × 40
    REQUIRE(renderPlan(ms, -1, 3, 0.1, 60, 40, a));
    // 같은 틀: A = (0, 0), u = (1, 0), n = (0, 1), s0 = −1, t1 = 3
    REQUIRE(renderPlanFrame(ms, 0, 0, 1, 0, 0, 1, -1, 3, 0.1, 60, 40, b));
    REQUIRE(a.w == b.w); REQUIRE(a.h == b.h);
    int diff = 0;
    for (int j = 0; j < a.h; ++j)
        for (int i = 0; i < a.w; ++i) diff += covered(a, i, j) != covered(b, i, j);
    CHECK(diff == 0);
}

TEST_CASE("돌린 평면을 바로 그림: A→A′ 가 북쪽이면 판이 종이 가로로 눕는다", "[combo][plan]") {
    const std::vector<MeshPtr> ms = {floorSlab()};
    // u = (0, 1)(북), n = 왼쪽 법선 (−1, 0)(서). s = y, t = −x → 판은 s 0–2, t −4–0
    RgbaImage im;
    const double res = 0.1;
    REQUIRE(renderPlanFrame(ms, 0, 0, 0, 1, -1, 0, -1, 1, res, 40, 70, im));
    // 픽셀 (i, j) 가운데: s = −1 + (i + .5)·res, t = 1 − (j + .5)·res
    auto at = [&](double s, double t) { return covered(im, int((s + 1) / res), int((1 - t) / res)); };
    CHECK(at(1.0, -2.0));    // 판 안
    CHECK(at(0.1, -3.9));
    CHECK_FALSE(at(2.5, -2.0));   // s > 2 밖
    CHECK_FALSE(at(1.0, 0.5));    // t > 0 밖(동쪽 = 오른쪽 법선 쪽)
    CHECK_FALSE(at(1.0, -4.5));   // t < −4 밖
}

TEST_CASE("표고 글자 50 cm 마다(사용자 2026-10-16 「해발선이 50cm마다 표시」): 땅속 구간이 있는 선만, A 끝에 닿으면 왼쪽 밖", "[combo]") {
    // 둔덕: 지면 0.3 → 둔덕 1.2(s 3–5) → 0.3. 원점 높이 20 → 표고 범위 19.7–21.3 의 50 cm 선 = 20.0 · 20.5 · 21.0
    std::vector<Polyline> pr = {{{0, 0.3}, {2, 0.3}, {3, 1.2}, {5, 1.2}, {6, 0.3}, {8, 0.3}}};
    auto v = levelLabelSpots(pr, -0.3, 1.3, 20.0, 8.0);
    REQUIRE(v.size() == 3);
    CHECK(v[0].cm == 2000); CHECK(v[0].atStart); CHECK(v[0].s0 == Approx(0));
    CHECK(v[1].cm == 2050); CHECK_FALSE(v[1].atStart); CHECK(v[1].s0 == Approx(2 + 0.2 / 0.9));
    CHECK(v[2].cm == 2100); CHECK_FALSE(v[2].atStart); CHECK(v[2].s0 == Approx(2 + 0.7 / 0.9));
    CHECK(v[1].zAbs == Approx(20.5));
    // 지면 위(땅속 구간 없음) 선은 뺀다
    v = levelLabelSpots({{{0, 0.3}, {8, 0.3}}}, -0.3, 1.3, 20.0, 8.0);
    REQUIRE(v.size() == 1);
    CHECK(v[0].cm == 2000);
    // 간격을 넓히면(1 m) 그 배수만
    v = levelLabelSpots({{{0, 1.3}, {8, 1.3}}}, -0.3, 1.3, 20.0, 8.0, 100);
    REQUIRE(v.size() == 2);
    CHECK(v[0].cm == 2000); CHECK(v[1].cm == 2100);
    // 열린 잘린 선이 없으면(레벨선을 통째로 긋는 경우) 모두 A 끝 왼쪽 밖
    v = levelLabelSpots({{{2, 0}, {3, 0}, {3, 1}, {2, 1}, {2, 0}}}, -0.3, 1.3, 20.0, 8.0);
    REQUIRE(v.size() == 3);
    for (const auto& x : v) CHECK(x.atStart);
    CHECK(levelLabelSpots(pr, -0.3, 1.3, 20.0, 0.0).empty());
}

TEST_CASE("표고 숫자 배치: 50 cm 가 원칙 — 줄이기 · 번갈아 · 그래도 안 되면만 솎음", "[combo][levels]") {
    auto same = [](LevelLabelFit f, int cm, bool alt, double sc) { return f.labelCm == cm && f.alternate == alt && std::abs(f.scale - sc) < 1e-9; };
    CHECK(same(fitLevelLabels(10, 5), 50, false, 1.0));     // 넉넉함
    CHECK(same(fitLevelLabels(4.5, 5), 50, false, 0.9));    // 조금 모자람 → 글자만 조금 줄여 양쪽 그대로
    CHECK(same(fitLevelLabels(3, 5), 50, true, 1.0));       // 더 모자람 → 왼쪽 · 오른쪽 번갈아(같은 쪽 간격 1 m)
    CHECK(same(fitLevelLabels(2, 5), 50, true, 0.8));       // 번갈아도 모자람 → 더 줄임
    CHECK(same(fitLevelLabels(1, 5), 100, true, 0.8));      // 50 cm 로는 물리적으로 안 들어감 → 1 m
    CHECK(same(fitLevelLabels(2, 2.4, false), 50, false, 2 / 2.4));     // 번갈아 쓸 곳이 없으면 줄이기만
    CHECK(same(fitLevelLabels(1, 2.4, false), 100, false, 2 / 2.4));
    CHECK(same(fitLevelLabels(0, 5), 50, false, 1.0));      // 나쁜 입력은 기본값
}

TEST_CASE("표고 숫자: 1:20 ~ 1:200 은 화면 · 도면 · 평·단면도 · DXF 모두 50 cm 마다 · 도면 글자는 7 pt 그대로", "[combo][levels]") {
    const double em7 = 7.0 * 25.4 / 72.0;   // 7 pt 의 em(mm) — 사용자 2026-10-16 「글자크기는 7pt 고정」
    for (int d = 20; d <= 200; d += 5) {
        INFO("1:" << d);
        // 평·단면도 · 단면 도면(종이 mm): 숫자 한 줄 = 7 pt 의 em, 번갈아 쓸 곳 없음 → 줄이지도 번갈지도 솎지도 않는다
        const LevelLabelFit fc = fitLevelLabels(0.5 * 1000.0 / d, em7, false);
        CHECK(fc.labelCm == 50);
        CHECK(fc.scale == 1.0);
        CHECK_FALSE(fc.alternate);
        // 단면 DXF(도면 m): 글자 높이 th = 7 pt em × 축척, 한 줄 = th × 1.8(core exportSectionDxf — 좁으면 좌우 번갈아)
        const double th = DxfExportOptions{}.textMm / 1000.0 * d;
        CHECK(th == Approx(em7 / 1000.0 * d));
        CHECK(fitLevelLabels(0.5, th * 1.8).labelCm == 50);
        // 단면 화면 · 단면 도면(96 dpi 기준 px): 글꼴 11.5 px 의 줄 높이를 크게 잡아도(맑은 고딕 1.33 배 × 여유 1.35 ≈ 20.7 px)
        const double ppm = 96.0 / 25.4 * 1000.0 / d;
        CHECK(fitLevelLabels(0.5 * ppm, 11.5 * 1.33 * 1.35).labelCm == 50);
    }
    // 1:200 보다 작은 축척에서야 7 pt 숫자가 50 cm 간격(2.5 mm)에 빠듯해져 글자를 줄이기 시작한다(경계 확인)
    CHECK(fitLevelLabels(0.5 * 1000.0 / 210, em7, false).scale < 1.0);
}

TEST_CASE("한 장 조판: 끄는 동안 틀을 붙들면 평면이 돌지 않고 종이 ↔ 로컬 변환이 그대로", "[combo][sheet]") {
    SheetSpec sp; sp.denom = 30;
    const std::vector<ComboSection> secs0 = {{0, 0, 4, 0, 1.0}, {2, -2, 2, 2, 0.8}};
    const ComboLayout c0 = layoutCombo(sp, secs0, 1.0);
    // 기준 단면의 A′ 를 (4, 1) 로 끌었다
    const std::vector<ComboSection> secs1 = {{0, 0, 4, 1, 1.0}, {2, -2, 2, 2, 0.8}};
    const ComboLayout c1 = layoutCombo(sp, secs1, 1.0, 6.0, 3.0, &c0);
    CHECK(c1.ux == Approx(c0.ux)); CHECK(c1.uy == Approx(c0.uy));
    CHECK(c1.ax == Approx(c0.ax)); CHECK(c1.ay == Approx(c0.ay));
    REQUIRE(c1.sections.size() == 2);
    CHECK(c1.sections[0].slot == ComboSlot::Top);
    CHECK(c1.sections[0].sB == Approx(4)); CHECK(c1.sections[0].tB == Approx(1));
    // 붙들지 않으면 새 기준 방향으로 돈다
    const ComboLayout c2 = layoutCombo(sp, secs1, 1.0);
    CHECK(std::fabs(c2.uy) > 0.2);
    // 종이 mm ↔ 돌린 좌표 ↔ 로컬 XY 왕복
    double s, t, x, y;
    c1.toST(3.0, 0.5, s, t);
    const double mx = c1.xMm(s), my = c1.yMm(t);
    double s2, t2;
    c1.paperToST(mx, my, s2, t2);
    c1.toXY(s2, t2, x, y);
    CHECK(x == Approx(3.0)); CHECK(y == Approx(0.5));
}

TEST_CASE("평면 방향: 각 정리 · 방향 틀 · 두 점 맞춤", "[combo][rotation]") {
    CHECK(normDeg180(0) == 0);
    CHECK(normDeg180(190) == Approx(-170));
    CHECK(normDeg180(-180) == Approx(180));
    CHECK(normDeg180(540) == Approx(180));
    CHECK(normDeg180(-30.5) == Approx(-30.5));
    // 방향 틀: 진북의 종이 각(northDeg) = 방향 각, u · n 은 오른손 직교
    for (double r : {0.0, 30.0, 90.0, -45.0, 179.0}) {
        const ComboLayout f = comboFrameForRotation(r, 5, 6);
        CHECK(f.northDeg() == Approx(normDeg180(r)).margin(1e-9));
        CHECK(f.ux * f.nx + f.uy * f.ny == Approx(0).margin(1e-12));
        CHECK(f.ux * f.ny - f.uy * f.nx == Approx(1));
        CHECK(f.ax == 5); CHECK(f.ay == 6);
    }
    const ComboLayout r90 = comboFrameForRotation(90);   // 진북이 오른쪽 → 종이 오른쪽 = 북
    CHECK(r90.ux == Approx(0).margin(1e-12)); CHECK(r90.uy == Approx(1));
    // 두 점 맞춤: p → q 가 종이 오른쪽(가로) / 위(세로)
    for (bool vert : {false, true}) {
        const double r = rotationToAlign(1, 1, 4, 5, vert);
        const ComboLayout f = comboFrameForRotation(r);
        double s, t; f.toST(4 - 1, 5 - 1, s, t);   // p → q 벡터의 돌린 좌표
        if (!vert) { CHECK(s == Approx(5)); CHECK(t == Approx(0).margin(1e-9)); }
        else { CHECK(s == Approx(0).margin(1e-9)); CHECK(t == Approx(5)); }
    }
    CHECK(rotationToAlign(2, 2, 2, 2, false) == 0);   // 같은 점은 그대로
    // 방향 틀로 놓으면 단면이 가로 · 세로 칸에 바로 놓이고 첫 단면 기준 자동 회전을 쓰지 않는다
    SheetSpec sp; sp.denom = 50;
    const std::vector<ComboSection> secs = {{0, 0, 0, 4, 1.0}};   // 북쪽으로 4 m(자동이면 이 선이 가로)
    const ComboLayout north = comboFrameForRotation(0);
    const ComboLayout c = layoutCombo(sp, secs, 1.0, 6.0, 3.0, &north);
    REQUIRE(c.sections.size() == 1);
    CHECK(c.northDeg() == Approx(0).margin(1e-9));
    CHECK(c.sections[0].slot == ComboSlot::Left);   // 북쪽으로 그은 선은 서쪽(왼쪽 법선)을 본다 → 평면 왼쪽 세로 칸
    CHECK(layoutCombo(sp, secs, 1.0).northDeg() != Approx(0).margin(1e-6));
}

TEST_CASE("다이얼 한 칸(사용자 2026-10-16 「방향키로도 0.1도씩」): 휠 · ←/→ 0.1°, Shift 1°", "[combo][rotation]") {
    CHECK(rotationStep(0, +1) == 0.1);                       // 꼬리 없이 정확히(33.700000000000003 아님)
    CHECK(rotationStep(0.1, +1) == 0.2);
    CHECK(rotationStep(0.2, -1) == 0.1);
    CHECK(rotationStep(33.6, +1) == 33.7);
    CHECK(rotationStep(33.63, +1) == 33.7);                  // 끌어서 격자 밖이면 그 방향의 가장 가까운 칸
    CHECK(rotationStep(33.63, -1) == 33.6);
    CHECK(rotationStep(-0.1, +1) == 0.0);
    CHECK(rotationStep(-33.63, -1) == -33.7);
    CHECK(rotationStep(179.95, +1) == Approx(180).margin(1e-9));      // 180 까지, 넘으면 반대쪽으로 감기
    CHECK(rotationStep(180, +1) == Approx(-179.9).margin(1e-9));
    CHECK(rotationStep(-179.9, -1) == Approx(180).margin(1e-9));
    CHECK(rotationStep(33.63, +1, 1.0) == 34.0);             // Shift = 1°
    CHECK(rotationStep(-0.1, +1, 1.0) == 0.0);
    CHECK(rotationStep(12.5, 0) == 12.5);                    // 방향 없음 · 나쁜 간격은 그대로
    CHECK(rotationStep(12.5, +1, 0.0) == 12.5);
    double d = 0;                                            // 열 번 = 1°(쌓이지 않음)
    for (int i = 0; i < 10; ++i) d = rotationStep(d, +1);
    CHECK(d == 1.0);
}

TEST_CASE("단면선 방향 스냅: 화면 가로 · 세로와 다른 단면에 직각 · 평행, 3° 안에서만", "[combo][rotation]") {
    // 기준 0 = 화면 축(방향 각 20°), 기준 1 = 다른 단면(방향 각 63°)
    const std::vector<double> refs = {20.0, 63.0};
    AngleSnap a = snapAngle(21.5, refs);   // 화면 가로에 1.5°
    CHECK(a.snapped); CHECK(a.source == 0); CHECK(!a.perpendicular); CHECK(a.angleDeg == Approx(20));
    a = snapAngle(151.0, refs);            // 63 + 90 = 153 에 2° → 다른 단면에 직각
    CHECK(a.snapped); CHECK(a.source == 1); CHECK(a.perpendicular); CHECK(a.angleDeg == Approx(153));
    a = snapAngle(-26.0, refs);            // 63 − 90 = −27 에 1° (20 − 45 = 20−90 = −70 은 멀다)
    CHECK(a.snapped); CHECK(a.source == 1); CHECK(a.perpendicular); CHECK(a.angleDeg == Approx(-27));
    a = snapAngle(40.0, refs);             // 어느 것에도 3° 밖
    CHECK(!a.snapped); CHECK(a.source == -1); CHECK(a.angleDeg == Approx(40));
    a = snapAngle(64.0, refs, 0.5);        // 허용 각을 줄이면 안 붙음
    CHECK(!a.snapped);
    a = snapAngle(-178.5, {0.0});          // 180 을 넘는 쪽도
    CHECK(a.snapped); CHECK(std::fabs(normDeg180(a.angleDeg - 180.0)) < 1e-9);
}
