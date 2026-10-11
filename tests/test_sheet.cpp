#include "catch_amalgamated.hpp"
#include "asec/orbit.hpp"
#include "asec/sheet.hpp"
#include <cmath>
#include <limits>

using namespace asec;
using Catch::Approx;

TEST_CASE("sheet: paper sizes and layout", "[sheet]") {
    double w, h;
    paperSizeMm(Paper::A4, true, w, h); CHECK(w == 297); CHECK(h == 210);
    paperSizeMm(Paper::A3, false, w, h); CHECK(w == 297); CHECK(h == 420);
    SheetSpec s; s.denom = 40;
    SheetLayout L = layoutSheet(s, 5.0, 2.0);   // 5 m × 2 m at 1:40 = 125 × 50 mm
    CHECK(L.contentW == Approx(125)); CHECK(L.contentH == Approx(50));
    CHECK(L.fits);
    CHECK(L.plotX + L.plotW <= L.frameX + L.frameW + 1e-9);
    CHECK(L.plotY + L.plotH <= L.titleY + 1e-9);
    CHECK(L.fitLenM == Approx(L.plotW * 40 / 1000));
}

TEST_CASE("sheet: overflow advice (real Jeju size 38 m)", "[sheet]") {
    SheetSpec s; s.denom = 40;
    SheetAdvice a = adviseSheet(s, 38.0, 5.0);
    REQUIRE(a.overflow);
    CHECK(a.fitLenM < 38.0);
    CHECK(a.smallerDenom == 200);          // 1:100 = 380 mm > A4 plot, 1:200 = 190 mm fits
    CHECK_FALSE(a.a3Fits);                 // 1:40 → 950 mm
    CHECK(a.splitCols == 4);               // 950 / ~233 mm
    CHECK(a.splitRows == 1);
    CHECK(a.splitSheets == 4);
    SheetSpec s3 = s; s3.paper = Paper::A3;
    SheetAdvice b = adviseSheet(s3, 38.0, 5.0);
    CHECK(b.splitSheets == 3);
    CHECK(b.smallerDenom == 200);       // A3 그림 칸 ~368 mm < 1:100 의 380 mm
    // 들어가면 추천 없음
    SheetSpec t = s; t.denom = 200;
    CHECK_FALSE(adviseSheet(t, 38.0, 6.0).overflow);
}

TEST_CASE("sheet: fit denominator is the tightest integer scale", "[sheet]") {
    SheetSpec s;
    double d = fitDenominator(s, 38.0, 6.0);
    SheetSpec t = s; t.denom = d;
    CHECK(layoutSheet(t, 38.0, 6.0).fits);
    t.denom = d - 1;
    CHECK_FALSE(layoutSheet(t, 38.0, 6.0).fits);
}

TEST_CASE("orbit: middle drag up lowers pitch past the right-button floor", "[orbit]") {
    CHECK(orbitPitch(45, -10, false) == Approx(48));
    CHECK(orbitPitch(45, -10, true) == Approx(42));
    CHECK(orbitPitch(5, -40, true) < 0);
    CHECK(orbitPitch(20, 1000, false) == Approx(8));
    CHECK(orbitPitch(0, -1000, true) == Approx(-89));
    CHECK(orbitPitch(80, -100, false) == Approx(90));
}

TEST_CASE("sheet: plan scale steps of 10 and outside coordinate ticks", "[sheet]") {
    CHECK(snapScaleDenom10(10) == 10);
    CHECK(snapScaleDenom10(14) == 10);
    CHECK(snapScaleDenom10(15) == 20);
    CHECK(snapScaleDenom10(100000) == 100000);
    CHECK(snapScaleDenom10(0) == 10);
    CHECK(snapScaleDenom10(-30) == 10);
    OutsideTicks t = outsideTicks(0, 100, 6);
    CHECK(t.step == 20);
    CHECK(t.count == 6);
    CHECK(t.at(0) == 0);
    CHECK(t.at(5) == 100);
    OutsideTicks u = outsideTicks(148000.2, 148038.7, 6);
    CHECK(u.step == 10);
    CHECK(u.first == Approx(148010));
    CHECK(u.count == 3);
    CHECK(u.at(2) <= 148038.7 + 1e-6);
}

TEST_CASE("sheet: azimuth text, facing, names, recent list", "[sheet]") {
    CHECK(formatAzimuth(90.4) == "N 90.4\xC2\xB0 E");
    CHECK(formatAzimuth(270) == "N 90.0\xC2\xB0 W");
    CHECK(formatAzimuth(-10) == "N 10.0\xC2\xB0 W");
    CHECK(facingKo(90) == "북쪽을 봄");      // 서→동 선은 북쪽(왼쪽 법선)을 봄
    CHECK(facingKo(270) == "남쪽을 봄");
    CHECK(facingKo(0) == "서쪽을 봄");
    CHECK(facingKo(135) == "북동쪽을 봄");
    CHECK(sectionLetterName(0) == "A\xE2\x80\x93" "A\xE2\x80\xB2");
    CHECK(sectionLetterName(1) == "B\xE2\x80\x93" "B\xE2\x80\xB2");
    CHECK(sectionLetterName(26) == "A1\xE2\x80\x93" "A1\xE2\x80\xB2");
    auto r = pushRecent({"C:/a.3mx", "D:/b.3mx"}, "c:\\A.3MX", 8);
    REQUIRE(r.size() == 2);
    CHECK(r[0] == "c:\\A.3MX"); CHECK(r[1] == "D:/b.3mx");
    std::vector<std::string> many;
    for (int i = 0; i < 12; ++i) many.push_back("f" + std::to_string(i));
    CHECK(pushRecent(many, "new", 8).size() == 8);
    CHECK(pushRecent(many, "new", 8)[0] == "new");
}

TEST_CASE("sheet: section list json round trip", "[sheet]") {
    std::vector<SavedSection> v(2);
    v[0].name = sectionLetterName(0); v[0].ax = 148076; v[0].ay = 98129.5; v[0].bx = 148114; v[0].by = 98129.5; v[0].back = 5; v[0].note = "1호 수혈 장축";
    v[1].name = sectionLetterName(1); v[1].ax = 148090; v[1].ay = 98120; v[1].bx = 148090; v[1].by = 98140; v[1].front = 0.2; v[1].back = 0.5;
    std::string t = sectionsToJson(v, "Production_2.3mx", "EPSG:5186", 1);
    std::vector<SavedSection> w; int cur = -1; std::string err;
    REQUIRE(sectionsFromJson(t, w, &cur, &err));
    REQUIRE(w.size() == 2);
    CHECK(cur == 1);
    CHECK(w[0].name == v[0].name); CHECK(w[0].note == v[0].note);
    CHECK(w[0].ax == 148076); CHECK(w[0].back == 5);
    CHECK(w[1].by == 98140); CHECK(w[1].front == Approx(0.2));
    CHECK_FALSE(sectionsFromJson("{\"x\":1}", w, &cur, &err));
    CHECK_FALSE(sectionsFromJson("not json", w, &cur, &err));
}

TEST_CASE("sheet: plan cover scale, outside step, section window", "[sheet]") {
    SheetSpec s;
    CHECK(fitDenomStep10(s, 26.9, 16.1) == 110);
    CHECK(fitDenomStep10(s, 38.0, 6.0) == 160);
    CHECK(outsideStepMeters(0.11) == 5);
    CHECK(outsideStepMeters(0.055) == 2);
    s.denom = 110;
    PlanPlace a = placePlan(s, 148074.0, 148100.9, 98103.4, 98119.5, 1, 0, 0);
    CHECK(a.imageFillsPlot);
    CHECK(a.rangeInsideImage);
    CHECK(a.tickStep == 5);
    CHECK(a.plotW == Approx(245));
    CHECK(a.plotH == Approx(146));
    PlanPlace z = placePlan(s, 148074.0, 148100.9, 98103.4, 98119.5, 2.07, 0, 0);
    CHECK(z.imageFillsPlot);
    CHECK(z.rangeInsideImage);
    CHECK(z.tickStep == 2);
    CHECK(z.mPerMm < a.mPerMm);
    PlanPlace m = placePlan(s, 148074.0, 148100.9, 98103.4, 98119.5, 1, 12, 0);
    CHECK(std::abs((m.visX0 - a.visX0) - (-12 * a.mPerMm)) < 1e-6);
    CHECK(m.plotX == a.plotX);
    CHECK(m.plotY == a.plotY);
    SectionPaperWindow w0 = sectionPaperWindow(3, 2, 60, a.plotW, a.plotH, 40, 1, 0, 0);
    SectionPaperWindow w1 = sectionPaperWindow(3, 2, 60, a.plotW, a.plotH, 40, 2, 0, 0);
    CHECK(w0.s0 == Approx(-(a.plotW * 0.04 - 3) / 2));
    CHECK(w1.visLen == Approx(w0.visLen / 2));
    CHECK(w1.mPerMm == Approx(w0.mPerMm / 2));
    SectionPaperWindow sh = sectionPaperWindow(3, 2, 60, a.plotW, a.plotH, 40, 1, 12, 0);
    CHECK(sh.s0 == Approx(w0.s0 - 12 * w0.mPerMm));
}

// ---- 단계 14: 도면 파일 이름 ----
TEST_CASE("sheet: 도면 파일 이름 규칙", "[sheet]") {
    CHECK(sheetFileName("Production_2", "A–A′", 20) == "Production_2_A-A′_1-20.svg");
    CHECK(sheetFileName("Production_2", "평면도", 110) == "Production_2_평면도_1-110.svg");
    CHECK(sheetFileName("제주", "B–B′", 20.0) == "제주_B-B′_1-20.svg");
}

TEST_CASE("sheet: 파일 이름 금지 글자·앞뒤 공백점", "[sheet]") {
    CHECK(sheetFileName("a/b:c*d?e\"f<g>h|i\\j", "도면", 20) == "a_b_c_d_e_f_g_h_i_j_도면_1-20.svg");
    CHECK(sheetFileName("  모델. ", " 도면.", 20) == "모델_도면_1-20.svg");
    CHECK(sheetFileName("...", "...", 20) == "__1-20.svg");
}

TEST_CASE("sheet: 같은 이름이면 _2 _3", "[sheet]") {
    CHECK(uniqueSheetFileName("a_1-20.svg", {}) == "a_1-20.svg");
    CHECK(uniqueSheetFileName("a_1-20.svg", {"a_1-20.svg"}) == "a_1-20_2.svg");
    CHECK(uniqueSheetFileName("a_1-20.svg", {"a_1-20.svg", "a_1-20_2.svg"}) == "a_1-20_3.svg");
    CHECK(uniqueSheetFileName("a_1-20.svg", {"b.svg"}) == "a_1-20.svg");
}

// ---- 단계 12: 위에서 본 범위 ----
TEST_CASE("sheet: 위에서 본 범위", "[sheet]") {
    TopDownRange r = topDownRange(100, 200, 0.05, 200, 100);
    CHECK(r.x0 == Approx(95));
    CHECK(r.x1 == Approx(105));
    CHECK(r.yBot == Approx(197.5));
    CHECK(r.yTop == Approx(202.5));
}

TEST_CASE("sheet: 위에서 본 범위 = 이동 없는 평면 배치의 보이는 범위", "[sheet]") {
    SheetSpec s;
    PlanPlace p = placePlan(s, 148074.0, 148100.9, 98103.4, 98119.5, 1, 0, 0);
    TopDownRange r = topDownRange(p.cX, p.cY, p.mPerMm, p.plotW, p.plotH);
    CHECK(r.x0 == Approx(p.visX0));
    CHECK(r.x1 == Approx(p.visX1));
    CHECK(r.yBot == Approx(p.visYBot));
    CHECK(r.yTop == Approx(p.visYTop));
}

TEST_CASE("sheet: 위에서 본 범위 0 이하면 한 점", "[sheet]") {
    TopDownRange r = topDownRange(100, 200, 0, 200, 100);
    CHECK(r.x0 == Approx(100));
    CHECK(r.x1 == Approx(100));
    TopDownRange n = topDownRange(100, 200, -1, 200, 100);
    CHECK(n.x0 == Approx(100));
    CHECK(n.x1 == Approx(100));
}

// 디자인 v5 결정 L — 레벨선 10 cm 0.2 pt · 50 cm 0.5 pt (일러스트레이터 기준). 1 pt = 25.4/72 mm
TEST_CASE("sheet: line weights in pt convert to mm and clamp", "[sheet][pt]") {
    CHECK(ptToMm(1.0) == Approx(0.35278).margin(1e-4));
    CHECK(ptToMm(0.2) == Approx(0.07056).margin(1e-4));
    CHECK(ptToMm(0.5) == Approx(0.17639).margin(1e-4));
    CHECK(clampLineWeightPt(0.0) == Approx(0.05));
    CHECK(clampLineWeightPt(10.0) == Approx(3.0));
    CHECK(clampLineWeightPt(-1.0) == Approx(0.05));
    CHECK(clampLineWeightPt(0.35) == Approx(0.35));
    CHECK(clampLineWeightPt(std::numeric_limits<double>::quiet_NaN()) == Approx(0.2));
    const LineWeights d = defaultLineWeights();
    CHECK(d.minorPt == Approx(0.2));
    CHECK(d.majorPt == Approx(0.5));
}

// 디자인 v5 결정 L — DXF 레이어 굵기(그룹 370, 1/100 mm)는 pt 를 DXF 허용값으로 반올림. 0.2 pt = 0.071 mm → 9(0.09; 5 보다 가까움), 0.5 pt = 0.176 mm → 18
TEST_CASE("sheet: pt to DXF lineweight snaps to allowed values", "[sheet][pt][dxf]") {
    CHECK(dxfLineWeight(0.2) == 9);
    CHECK(dxfLineWeight(0.5) == 18);
    CHECK(dxfLineWeight(1.0) == 35);     // 0.353 mm
    CHECK(dxfLineWeight(2.0) == 70);     // 0.706 mm
    CHECK(dxfLineWeight(3.0) == 106);    // 1.058 mm
    CHECK(dxfLineWeight(0.05) == 0);     // 0.018 mm → 가장 가까운 허용값 0
    CHECK(dxfLineWeight(99.0) == 106);   // 3.0 pt 로 잘림
}

// ---- 단면도 조판의 위치도(평면 띠) — 사용자 결정 2026-10-11: 단면선이 가로가 되게 돌린 평면, 앞·뒤 각 3 m(뒤 깊이가 크면 그 값)
TEST_CASE("sheet: plan strip half width = max(3 m, front, back)", "[sheet]") {
    CHECK(planStripHalfWidth(0, 3) == 3);
    CHECK(planStripHalfWidth(0, 5) == 5);
    CHECK(planStripHalfWidth(4, 1) == 4);
    CHECK(planStripHalfWidth(0.5, 0.5) == 3);
}

TEST_CASE("sheet: plan strip area is the bounding box of the rotated band", "[sheet]") {
    PlanStripArea a = planStripArea(0, 0, 10, 0, 3, 1);   // 동서 선
    CHECK(a.x0 == Approx(-1)); CHECK(a.x1 == Approx(11)); CHECK(a.y0 == Approx(-3)); CHECK(a.y1 == Approx(3));
    PlanStripArea d = planStripArea(0, 0, 10, 10, 3, 1);  // 대각선: u=(1,1)/√2, n=(−1,1)/√2
    const double r2 = std::sqrt(2.0), L = 10 * r2;
    CHECK(d.x0 == Approx((-1 - 3) / r2)); CHECK(d.x1 == Approx((L + 1 + 3) / r2));
    CHECK(d.y0 == Approx((-1 - 3) / r2)); CHECK(d.y1 == Approx((L + 1 + 3) / r2));
    PlanStripArea z = planStripArea(2, 2, 2, 2, 3, 0);   // 길이 0 → u=(1,0)
    CHECK(z.x0 == Approx(2)); CHECK(z.x1 == Approx(2)); CHECK(z.y0 == Approx(-1)); CHECK(z.y1 == Approx(5));
}

TEST_CASE("sheet: plan strip pixel→paper transform puts A at (s0,t0), A′ to the right, the back side up", "[sheet]") {
    // 선 A(5,5)→B(15,5): u=(1,0), n=(0,1). 영상 왼쪽 위 (0, 20), 0.5 m/px. 종이: s=0 → x 100 mm, t=0 → y 50 mm, 1:40 → 25 mm/m
    PlanStripXf T = planStripTransform(5, 5, 15, 5, 0, 20, 0.5, 100, 50, 25);
    auto X = [&](double i, double j) { return T.m11 * i + T.m21 * j + T.dx; };
    auto Y = [&](double i, double j) { return T.m12 * i + T.m22 * j + T.dy; };
    CHECK(X(10, 30) == Approx(100)); CHECK(Y(10, 30) == Approx(50));      // A = 픽셀 (10, 30)
    CHECK(X(30, 30) == Approx(350)); CHECK(Y(30, 30) == Approx(50));      // B: 10 m 오른쪽
    CHECK(X(10, 26) == Approx(100)); CHECK(Y(10, 26) == Approx(0));       // A + n·2 m(뒤) → 위로 50 mm
    // 남북 선 A(5,5)→B(5,15): u=(0,1), n=(−1,0) → 서쪽이 '뒤'(위)
    PlanStripXf S = planStripTransform(5, 5, 5, 15, 0, 20, 0.5, 100, 50, 25);
    auto Xs = [&](double i, double j) { return S.m11 * i + S.m21 * j + S.dx; };
    auto Ys = [&](double i, double j) { return S.m12 * i + S.m22 * j + S.dy; };
    CHECK(Xs(10, 30) == Approx(100)); CHECK(Ys(10, 30) == Approx(50));
    CHECK(Xs(10, 10) == Approx(350)); CHECK(Ys(10, 10) == Approx(50));     // B = (5,15) = 픽셀 (10, 10)
    CHECK(Xs(6, 30) == Approx(100)); CHECK(Ys(6, 30) == Approx(0));        // (3,5) = A + n·2 → 위
}

// 인쇄(사용자 요청 2026-10-12): 프린터 용지가 도면 용지와 같은가 · 프린터가 못 찍는 가장자리가 테두리 바깥 여백 안에 드는가
TEST_CASE("sheet: print paper check (size, orientation, printer margins)", "[sheet][print]") {
    SheetSpec s; s.paper = Paper::A3; s.landscape = true; s.denom = 20;
    const SheetLayout L = layoutSheet(s, 8.0, 2.0);
    REQUIRE(L.paperW == Approx(420)); REQUIRE(L.paperH == Approx(297));
    const double fm = std::min({L.frameX, L.frameY, L.paperW - L.frameX - L.frameW, L.paperH - L.frameY - L.frameH});
    REQUIRE(fm > 1.0);

    PrintPaperCheck c = checkPrintPaper(L, 420, 297, 4.2, 4.2, 4.2, 4.2);
    CHECK(c.sizeOk); CHECK(c.marginOk);
    CHECK(c.frameMarginMm == Approx(fm));
    CHECK(c.maxPrinterMarginMm == Approx(4.2));

    CHECK(checkPrintPaper(L, 420.6, 296.5, 0, 0, 0, 0).sizeOk);        // 1 mm 안은 같은 용지(드라이버 반올림)
    CHECK_FALSE(checkPrintPaper(L, 297, 420, 0, 0, 0, 0).sizeOk);      // 방향이 다름
    CHECK_FALSE(checkPrintPaper(L, 297, 210, 0, 0, 0, 0).sizeOk);      // A4
    CHECK_FALSE(checkPrintPaper(L, 422, 297, 0, 0, 0, 0).sizeOk);

    CHECK(checkPrintPaper(L, 420, 297, fm - 0.1, 0, 0, 0).marginOk);
    PrintPaperCheck cut = checkPrintPaper(L, 420, 297, 0, 0, 0, fm + 0.5);   // 아래쪽만 넓어도 테두리가 잘린다
    CHECK_FALSE(cut.marginOk);
    CHECK(cut.maxPrinterMarginMm == Approx(fm + 0.5));
}
