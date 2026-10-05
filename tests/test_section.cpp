#include "catch_amalgamated.hpp"
#include "asec/section.hpp"
#include "synth.hpp"

using namespace asec;
using Catch::Approx;

static MeshPtr tri(std::vector<float> p) {
    auto m = std::make_shared<Mesh>();
    m->pos = p;
    for (uint32_t i = 0; i < p.size() / 3; ++i) m->idx.push_back(i);
    m->computeBBox();
    return m;
}

TEST_CASE("평면-삼각형 교차: 알려진 값") {
    // 단면선 x축(A=(0,0)→B=(10,0)), 평면 y=0. 삼각형이 y=-1..1 에 걸침
    auto m = tri({2, -1, 10, 4, 1, 12, 2, 1, 14});
    SectionFrame f(SectionLine{{0, 0}, {10, 0}});
    std::vector<CutSeg> s;
    REQUIRE(cutMesh(*m, f, 0, 0, 10, s) == 1);
    double a0 = std::min(s[0].s0, s[0].s1), a1 = std::max(s[0].s0, s[0].s1);
    CHECK(a0 == Approx(2.0));   // (2,-1,10)-(2,1,14) 중점 → s=2, z=12
    CHECK(a1 == Approx(3.0));   // (2,-1,10)-(4,1,12) 중점 → s=3, z=11
    CHECK((s[0].s0 < s[0].s1 ? s[0].z0 : s[0].z1) == Approx(12.0));
    CHECK((s[0].s0 < s[0].s1 ? s[0].z1 : s[0].z0) == Approx(11.0));
}

TEST_CASE("꼭짓점이 평면 위: 퇴화·겹선 없음") {
    // 두 삼각형이 평면 위 꼭짓점(5,0)을 공유
    auto m = tri({5, 0, 1, 4, -1, 1, 6, -1, 1,   5, 0, 1, 6, 1, 1, 4, 1, 1});
    SectionFrame f(SectionLine{{0, 0}, {10, 0}});
    std::vector<CutSeg> s;
    cutMesh(*m, f, 0, 0, 10, s);
    for (auto& g : s) CHECK(std::hypot(g.s1 - g.s0, g.z1 - g.z0) >= 0.0);
    auto L = stitchSegments(s, 1e-6);
    CHECK(countDuplicateVertices(L, 1e-9) == 0);
}

TEST_CASE("공유 모서리 교점은 비트 단위로 같다") {
    auto m = synth::gridMesh(0, 0, 2, 2, 0.1);
    SectionFrame f(SectionLine{{0.013, 0.37}, {1.97, 1.61}});
    std::vector<CutSeg> s;
    cutMesh(*m, f, 0, 0, f.L, s);
    REQUIRE(s.size() > 20);
    auto L = stitchSegments(s, 1e-12);  // 공차 거의 0 이어도 한 줄
    CHECK(L.size() == 1);
}

TEST_CASE("평탄 격자 단면 → 직선 2점") {
    auto m = std::make_shared<Mesh>(*synth::gridMesh(0, 0, 4, 4, 0.2));
    for (size_t i = 2; i < m->pos.size(); i += 3) m->pos[i] = 7.0f;
    SectionFrame f(SectionLine{{0.5, 0.5}, {3.5, 3.1}});
    std::vector<CutSeg> s;
    cutMesh(*m, f, 0, 0, f.L, s);
    auto L = buildProfile(s, CleanupParams{});
    REQUIRE(L.size() == 1);
    CHECK(L[0].size() == 2);
    CHECK(L[0].front().x == Approx(0.0).margin(1e-9));
    CHECK(L[0].back().x == Approx(f.L).margin(1e-6));
}

TEST_CASE("단면 점은 메시 위(격자 보간 대비 < 0.01 mm), 해석해 대비 < 3 cm") {
    auto m = synth::gridMesh(2, 2, 14, 10, 0.04);
    SectionLine ln{{3, 6}, {13, 6.3}};
    SectionFrame f(ln);
    std::vector<CutSeg> s;
    cutMesh(*m, f, 0, 0, f.L, s);
    auto L = buildProfile(s, CleanupParams{});
    REQUIRE(L.size() == 1);
    double maxErr = 0, maxAna = 0;
    for (auto& p : L[0]) {
        Vec2 xy = f.planXY(p.x);
        maxErr = std::max(maxErr, std::fabs(p.y - synth::gridZ(xy.x, xy.y, 0.04)));
        maxAna = std::max(maxAna, std::fabs(p.y - synth::height(xy.x, xy.y)));
    }
    CHECK(maxErr < 1e-5);
    CHECK(maxAna < 0.03);
}

TEST_CASE("레벨선: cm 정수 산술, 등급, 표기") {
    auto L = levelLines(44.93, 45.61, 10);
    REQUIRE(L.size() == 7);
    CHECK(formatElevation(L.front().z) == "45.0");
    CHECK(formatElevation(L.back().z) == "45.6");
    CHECK(L.front().cls == LevelClass::Master);
    CHECK(L[5].cls == LevelClass::Major);  // 45.50
    CHECK(L[3].cls == LevelClass::Minor);  // 45.30
    for (size_t i = 0; i < L.size(); ++i) CHECK(L[i].cm == 4500 + long(i) * 10);
    CHECK(formatElevation(45.3) == "45.3");
    CHECK(formatElevation(78.5) == "78.5");
    CHECK(formatElevation(79.0) == "79.0");
    CHECK(formatElevation(45.3, 2) == "45.30");
    CHECK(formatElevation(-0.05, 2) == "-0.05");
    CHECK(formatElevation(-0.04) == "0.0");
    CHECK(formatElevation(1234.5678, 2) == "1234.57");
    // 경계값 포함
    CHECK(levelLines(45.0, 45.2).size() == 3);
    // 긴 범위 누적 오차 없음
    auto B = levelLines(0, 1000, 10);
    CHECK(formatElevation(B[7777].z) == "777.7");
}

TEST_CASE("라벨 간격: 겹치지 않게") {
    CHECK(labelStepCm(400, 14) == 50);   // 넉넉해도 라벨은 50 cm 마다(10 cm 라벨 없음)
    CHECK(labelStepCm(100, 14) == 50);   // 10cm=10px → 50cm=50px
    CHECK(labelStepCm(20, 14) == 100);
    CHECK(labelStepCm(2, 14) == 1000);
    CHECK(niceStep(10, 10) == Approx(1.0));
    CHECK(niceStep(23, 10) == Approx(2.0));
    CHECK(niceStep(0.7, 10) == Approx(0.05));
}

TEST_CASE("두께 띠 vs 상자(SAT)") {
    SectionLine l{{0, 0}, {10, 10}, 0.0, 0.5};
    auto q = sectionBand(l);
    Box3 in; in.add(Vec3(4, 4, 0)); in.add(Vec3(5, 5, 1));
    Box3 off; off.add(Vec3(8, 0, 0)); off.add(Vec3(9, 1, 1));
    Box3 corner; corner.add(Vec3(-2, 9, 0)); corner.add(Vec3(1, 12, 1));  // 축 상자로는 겹치지만 띠와는 안 겹침
    CHECK(bandIntersectsBox(q, in));
    CHECK_FALSE(bandIntersectsBox(q, off));
    CHECK_FALSE(bandIntersectsBox(q, corner));
}
