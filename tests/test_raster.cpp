#include "catch_amalgamated.hpp"
#include "asec/raster.hpp"
#include "synth.hpp"

using namespace asec;
using Catch::Approx;

static MeshPtr wall(double x, double y0, double y1, double z0, double z1, uint8_t r, uint8_t g, uint8_t b) {
    // 수직 벽(평면 x=const), 단색 텍스처
    auto m = std::make_shared<Mesh>();
    m->pos = {float(x), float(y0), float(z0), float(x), float(y1), float(z0), float(x), float(y1), float(z1), float(x), float(y0), float(z1)};
    m->uv = {0, 0, 1, 0, 1, 1, 0, 1};
    m->idx = {0, 1, 2, 0, 2, 3};
    m->texture = std::make_shared<Texture>();
    m->texture->rgba.w = 2; m->texture->rgba.h = 2; m->texture->rgba.px.assign(16, 255);
    for (int i = 0; i < 4; ++i) { m->texture->rgba.px[i * 4] = r; m->texture->rgba.px[i * 4 + 1] = g; m->texture->rgba.px[i * 4 + 2] = b; }
    m->computeBBox();
    return m;
}

TEST_CASE("입면 영상: 가까운 면이 앞, 두께 띠 밖은 제외, 좌표 정렬") {
    // 단면선 y=0, x 0→4. 보는 방향 +y(n). 벽 A: y=0.2 빨강(x=1..3), 벽 B: y=0.4 파랑(x=0..4), 벽 C: y=2.0 초록(띠 밖)
    auto A = std::make_shared<Mesh>(); // x=const 벽이 아니라 y=const 면이 필요 → 직접 구성
    auto face = [](double y, double x0, double x1, double z0, double z1, uint8_t r, uint8_t g, uint8_t b) {
        auto m = std::make_shared<Mesh>();
        m->pos = {float(x0), float(y), float(z0), float(x1), float(y), float(z0), float(x1), float(y), float(z1), float(x0), float(y), float(z1)};
        m->uv = {0, 0, 1, 0, 1, 1, 0, 1};
        m->idx = {0, 1, 2, 0, 2, 3};
        m->texture = std::make_shared<Texture>();
        m->texture->rgba.w = 1; m->texture->rgba.h = 1; m->texture->rgba.px = {r, g, b, 255};
        m->computeBBox();
        return m;
    };
    std::vector<MeshPtr> ms{face(0.4, 0, 4, 0, 2, 0, 0, 255), face(0.2, 1, 3, 0, 1, 255, 0, 0), face(2.0, 0, 4, 0, 3, 0, 255, 0)};
    SectionFrame f(SectionLine{{0, 0}, {4, 0}});
    RasterRequest rq; rq.s0 = 0; rq.s1 = 4; rq.z0 = 0; rq.z1 = 3; rq.res = 0.01; rq.dNear = 0; rq.dFar = 1.0;
    ElevationImage im;
    REQUIRE(renderElevation(ms, f, rq, im));
    REQUIRE(im.img.w == 400); REQUIRE(im.img.h == 300);
    auto px = [&](double s, double z) { int c = int((s - im.s0) / im.res), r = int((im.z1 - z) / im.res); return &im.img.px[(size_t(r) * im.img.w + c) * 4]; };
    CHECK(int(px(2.0, 0.5)[0]) == 255);  // 빨강이 앞
    CHECK(int(px(0.5, 0.5)[2]) == 255);  // 빨강 밖은 파랑
    CHECK(int(px(2.0, 1.5)[2]) == 255);
    CHECK(int(px(2.0, 2.5)[3]) == 0);    // 초록(띠 밖) 안 보임 → 투명
    // 경계 정렬: 빨강 시작 s=1.0 → 열 100
    CHECK(int(im.img.px[(size_t(250) * 400 + 99) * 4 + 2]) == 255);
    CHECK(int(im.img.px[(size_t(250) * 400 + 100) * 4 + 0]) == 255);
    (void)wall;
}

TEST_CASE("입면 영상 위쪽 경계 = 단면선 높이(정렬 확인)") {
    auto m = synth::gridMesh(0, -1, 10, 1, 0.05);
    // 텍스처 없는 메시 → 회색 음영
    SectionLine ln{{1, 0}, {9, 0}, 0.0, 0.01};
    SectionFrame f(ln);
    RasterRequest rq; rq.s0 = 0; rq.s1 = f.L; rq.z0 = 4; rq.z1 = 6; rq.res = 0.005; rq.dNear = 0; rq.dFar = 0.01;
    ElevationImage im;
    REQUIRE(renderElevation({m}, f, rq, im));
    int bad = 0;
    for (int c = 5; c < im.img.w - 5; c += 7) {
        double s = im.s0 + (c + 0.5) * im.res;
        Vec2 xy = f.planXY(s);
        double zt = synth::height(xy.x, xy.y);
        int top = -1;
        for (int r = 0; r < im.img.h; ++r) if (im.img.px[(size_t(r) * im.img.w + c) * 4 + 3]) { top = r; break; }
        double ztop = im.z1 - top * im.res;
        if (std::fabs(ztop - zt) > 0.02) ++bad;  // 두께 1 cm 안의 경사 + 1 px
    }
    CHECK(bad == 0);
}
