#include "catch_amalgamated.hpp"
#include "asec/section.hpp"

using namespace asec;
using Catch::Approx;

static Polyline rect(double x0, double y0, double x1, double y1) {
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}

TEST_CASE("hatch: 정사각형 0도", "[hatch]") {
    auto H = hatchPolygon({rect(0, 0, 2, 1)}, 0.0, 0.25);
    REQUIRE(H.size() == 3);  // y = .25, .5, .75
    for (auto& h : H) {
        CHECK(std::min(h.s0, h.s1) == Approx(0.0).margin(1e-9));
        CHECK(std::max(h.s0, h.s1) == Approx(2.0).margin(1e-9));
    }
    CHECK(H[0].z0 == Approx(0.25).margin(1e-9));
    CHECK(H[2].z0 == Approx(0.75).margin(1e-9));
}

TEST_CASE("hatch: 정사각형 90도", "[hatch]") {
    auto H = hatchPolygon({rect(0, 0, 2, 1)}, 90.0, 0.25);
    REQUIRE(H.size() == 7);  // x = .25 .. 1.75
    for (auto& h : H) {
        CHECK(std::min(h.z0, h.z1) == Approx(0.0).margin(1e-9));
        CHECK(std::max(h.z0, h.z1) == Approx(1.0).margin(1e-9));
    }
}

TEST_CASE("hatch: 정사각형 45도", "[hatch]") {
    auto H = hatchPolygon({rect(0, 0, 2, 1)}, 45.0, 0.5);
    REQUIRE(H.size() == 4);
    for (auto& h : H) {
        // 선분이 다각형 안에 있음: 끝점 모두 경계 안
        for (double s : {h.s0, h.s1}) CHECK(s >= -1e-9);
        for (double s : {h.s0, h.s1}) CHECK(s <= 2.0 + 1e-9);
    }
}

TEST_CASE("hatch: 오목 다각형", "[hatch]") {
    Polyline l = {{0, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}};
    auto H = hatchPolygon({l}, 0.0, 0.4);  // y = .4, .8 (너비 2) + 1.2, 1.6 (너비 1)
    REQUIRE(H.size() == 4);
    for (auto& h : H) {
        double y = (h.z0 + h.z1) / 2, w = std::max(h.s0, h.s1) - std::min(h.s0, h.s1);
        if (y > 1.0) CHECK(w == Approx(1.0).margin(1e-6));
        else CHECK(w == Approx(2.0).margin(1e-6));
    }
}

TEST_CASE("hatch: 구멍", "[hatch]") {
    auto H = hatchPolygon({rect(0, 0, 4, 4), rect(1, 1, 3, 2.5)}, 0.0, 1.0);
    REQUIRE(H.size() == 5);  // y=1: 2개, y=2: 2개, y=3: 1개
    int full = 0, split = 0;
    for (auto& h : H) {
        double w = std::max(h.s0, h.s1) - std::min(h.s0, h.s1);
        if (w > 3.5) ++full;
        else ++split;
    }
    CHECK(full == 1);
    CHECK(split == 4);
}

TEST_CASE("hatch: 아주 얇은 영역", "[hatch]") {
    auto H = hatchPolygon({rect(0, 0, 10, 0.001)}, 90.0, 1.0);
    REQUIRE(H.size() == 9);  // x = 1 .. 9
    for (auto& h : H) CHECK(std::fabs(h.z1 - h.z0) == Approx(0.001).margin(1e-9));
}

TEST_CASE("hatch: 빈 입력·간격 0", "[hatch]") {
    CHECK(hatchPolygon({}, 45.0, 0.1).empty());
    CHECK(hatchPolygon({rect(0, 0, 1, 1)}, 45.0, 0.0).empty());
    CHECK(hatchPolygon({rect(0, 0, 1, 1)}, 45.0, -1.0).empty());
}

TEST_CASE("closeCutRegion: 직선 위 + 아래 점들", "[hatch]") {
    std::vector<Polyline> prof{{{0, 5}, {10, 5}}};
    Polyline lower{{2, 5}, {5, 3}, {8, 5}};
    auto r = closeCutRegion(prof, 2.0, 8.0, lower);
    REQUIRE(r.ok);
    REQUIRE(r.poly.size() == 4);  // 윗경계 2점 + 아래 2점(양 끝 겹침은 하나로)
    CHECK(r.poly.front().x == Approx(2.0).margin(1e-9));
    CHECK(r.poly.front().y == Approx(5.0).margin(1e-9));
    CHECK(r.poly.back().x == Approx(2.0).margin(1e-9));  // 아래 첫점으로 닫힘
    CHECK(r.poly.back().y == Approx(5.0).margin(1e-9));
}

TEST_CASE("closeCutRegion: 윗경계가 끊기면 실패", "[hatch]") {
    std::vector<Polyline> prof{{{0, 5}, {4, 5}}, {{6, 5}, {10, 5}}};  // 4–6 빈 구간
    Polyline lower{{2, 5}, {5, 3}, {8, 5}};
    auto r = closeCutRegion(prof, 2.0, 8.0, lower);
    CHECK_FALSE(r.ok);
    CHECK(r.poly.empty());
}

TEST_CASE("closeCutRegion: s0 > s1 바꿔도 같음", "[hatch]") {
    std::vector<Polyline> prof{{{0, 5}, {10, 5}}};
    Polyline lower{{2, 5}, {5, 3}, {8, 5}};
    auto a = closeCutRegion(prof, 2.0, 8.0, lower);
    auto b = closeCutRegion(prof, 8.0, 2.0, lower);
    REQUIRE(a.ok);
    REQUIRE(b.ok);
    REQUIRE(a.poly.size() == b.poly.size());
    for (size_t i = 0; i < a.poly.size(); ++i) {
        CHECK(a.poly[i].x == Approx(b.poly[i].x).margin(1e-12));
        CHECK(a.poly[i].y == Approx(b.poly[i].y).margin(1e-12));
    }
}
