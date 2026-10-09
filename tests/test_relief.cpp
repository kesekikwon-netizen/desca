#include "catch_amalgamated.hpp"
#include "asec/raster.hpp"

using namespace asec;
using Catch::Approx;

static HeightGrid flatGrid(double x0, double y0, double w, double h, double step, double z) {
    HeightGrid g;
    g.x0 = x0; g.y0 = y0; g.step = step;
    g.nx = int(std::lround(w / step)) + 1;
    g.ny = int(std::lround(h / step)) + 1;
    g.z.assign((size_t)g.nx * g.ny, z);
    return g;
}

TEST_CASE("relief: 평평한 면 = 0, 등고선 없음", "[relief]") {
    HeightGrid g = flatGrid(0, 0, 4, 4, 0.05, 45.5);
    HeightGrid r = localRelief(g, 0.5);
    REQUIRE(r.nx == g.nx);
    REQUIRE(r.ny == g.ny);
    for (double v : r.z) CHECK(v == Approx(0).margin(1e-9));
    CHECK(contoursAbove(r, 0.03).empty());
}

TEST_CASE("relief: 솟은 둔덕 하나 = 원 윤곽 하나", "[relief]") {
    // 매끈한 둔덕(합성 모델의 돌과 같은 모양). 깎아지른 원기둥은 창 경계에서
    // 안쪽 고리가 하나 더 생기므로(창이 바깥을 보기 시작하는 자리) 여기서 쓰지 않는다.
    HeightGrid g = flatGrid(0, 0, 4, 4, 0.05, 0.0);
    for (int j = 0; j < g.ny; ++j)
        for (int i = 0; i < g.nx; ++i) {
            double x = g.x0 + i * g.step, y = g.y0 + j * g.step;
            double d = std::hypot(x - 2.0, y - 2.0);
            if (d <= 0.5) g.z[(size_t)j * g.nx + i] = 0.2 * (1 - (d / 0.5) * (d / 0.5));
        }
    HeightGrid r = localRelief(g, 0.5);
    // 가운데는 주변보다 높아 기복이 기준(3 cm)보다 큼
    CHECK(r.z[(size_t)(g.ny / 2) * g.nx + g.nx / 2] > 0.03);
    auto C = contoursAbove(r, 0.03);
    REQUIRE(C.size() == 1);
    // 닫힌 고리: 처음 = 끝
    CHECK((C[0].front() - C[0].back()).len() < 1e-6);
    // 반지름 ≈ 원기둥 반지름(격자·평균창 오차 안에서)
    double cx = 0, cy = 0;
    for (auto& p : C[0]) { cx += p.x; cy += p.y; }
    cx /= C[0].size(); cy /= C[0].size();
    CHECK(cx == Approx(2.0).margin(0.05));
    CHECK(cy == Approx(2.0).margin(0.05));
    double rr = 0;
    for (auto& p : C[0]) rr += std::hypot(p.x - cx, p.y - cy);
    rr /= C[0].size();
    CHECK(rr == Approx(0.5).margin(0.15));
}

TEST_CASE("relief: 경사면은 기복 0에 가까움", "[relief]") {
    HeightGrid g = flatGrid(0, 0, 4, 4, 0.05, 0.0);
    for (int j = 0; j < g.ny; ++j)
        for (int i = 0; i < g.nx; ++i)
            g.z[(size_t)j * g.nx + i] = 0.04 * (g.x0 + i * g.step) + 0.02 * (g.y0 + j * g.step);
    HeightGrid r = localRelief(g, 0.5);
    double mx = 0;
    for (double v : r.z) mx = std::max(mx, std::fabs(v));
    CHECK(mx < 0.03);  // 경사 4.5% × 창 0.5 m = 2.2 cm 이내
    CHECK(contoursAbove(r, 0.03).empty());
}

TEST_CASE("relief: 빈 격자·반지름 0", "[relief]") {
    CHECK(localRelief(HeightGrid{}, 0.5).z.empty());
    HeightGrid g = flatGrid(0, 0, 1, 1, 0.1, 1.0);
    CHECK(localRelief(g, 0.0).z.empty());
    CHECK(contoursAbove(HeightGrid{}, 0.03).empty());
}
