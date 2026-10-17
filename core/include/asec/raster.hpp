// 단면 입면 영상: 단면 평면에 수직으로 본 정사(orthographic) 텍스처 영상, 두께 띠로 잘라서 렌더(CPU, Z 버퍼).
// 같은 (s, z) 좌표계를 쓰므로 단면선·레벨선과 정확히 겹친다.
#pragma once
#include <atomic>
#include <functional>
#include "asec/section.hpp"

namespace asec {

struct ElevationImage {
    RgbaImage img;         // 행 0 = 위(z1), 열 0 = s0. 빈 곳 alpha = 0
    double s0 = 0, z1 = 0; // 왼쪽 위 모서리의 (s, z)
    double res = 0.01;     // m / 픽셀
    double s1() const { return s0 + img.w * res; }
    double z0() const { return z1 - img.h * res; }
    size_t trianglesDrawn = 0;
};

struct RasterRequest {
    double s0 = 0, s1 = 1, z0 = 0, z1 = 1;  // 영상 범위(m)
    double res = 0.005;                      // m/px
    double dNear = 0, dFar = 0.5;            // 두께 띠(+d = 보는 방향)
    size_t maxPixels = size_t(60) << 20;     // 안전 한도
    double depthFade = 0;                    // 깊이 음영 세기(0 = 끔, 0.6 = 가장 먼 면을 흰색 쪽으로 60%)
    double fadeRef = 2.0;                    // 음영 기준 깊이(m): max(dFar, fadeRef) 에서 최대 — 얇은 띠에서는 약하게
    int threads = 0;                         // 0 = 자동(최대 8), 1 = 단일 스레드
};

/// meshes 를 단면 좌표계로 정사 투영. 텍스처(rgba)가 없으면 법선 음영 회색.
bool renderElevation(const std::vector<MeshPtr>& meshes, const SectionFrame& f, const RasterRequest& rq, ElevationImage& out,
                     const std::atomic<bool>* cancel = nullptr);

/// 평면 정사영상(위에서 수직으로, 가장 높은 면). x0,y1 = 왼쪽 위 모서리(로컬), res m/px, W×H
bool renderPlan(const std::vector<MeshPtr>& meshes, double x0, double y1, double res, int W, int H, RgbaImage& out, const std::atomic<bool>* cancel = nullptr);
/// 평면 정사영상을 돌린 틀로 바로 그림(평·단면도 한 장, 2026-10-16): s = (p − A)·u, t = (p − A)·n 일 때
/// 픽셀 (i, j) 가운데 = (s0 + (i + .5)·res, t1 − (j + .5)·res). 다 그린 영상을 다시 돌려 표본화하지 않으므로 흐려지지 않는다.
/// u = (ux, uy) 단위 벡터, n = (nx, ny) 그에 수직인 단위 벡터(보통 왼쪽 법선)
bool renderPlanFrame(const std::vector<MeshPtr>& meshes, double ax, double ay, double ux, double uy, double nx, double ny, double s0, double t1,
                     double res, int W, int H, RgbaImage& out, const std::atomic<bool>* cancel = nullptr);
/// renderPlanFrame 을 위에서부터 stripRows 행씩 가로 띠로 나눠 그림 — 깊이 버퍼가 띠 크기라 큰 평면도
/// 메모리가 덜 든다(평·단면도 저장, 2026-10-17). 띠마다 sink(띠 첫 행, 띠 영상)를 부르고, sink 가
/// false 를 돌려주면 멈춘다. 픽셀은 한 번에 그린 것과 같다
bool renderPlanFrameStrips(const std::vector<MeshPtr>& meshes, double ax, double ay, double ux, double uy, double nx, double ny,
                           double s0, double t1, double res, int W, int H, int stripRows,
                           const std::function<bool(int, const RgbaImage&)>& sink, const std::atomic<bool>* cancel = nullptr);

/// 높이 격자(로컬 XY, 균일 간격 노드). z[j*nx+i] = (x0+i*step, y0+j*step) 높이(m).
struct HeightGrid {
    double x0 = 0, y0 = 0, step = 1;
    int nx = 0, ny = 0;
    std::vector<double> z;
    bool valid() const { return nx > 0 && ny > 0 && step > 0 && z.size() == (size_t)nx * ny; }
    double at(int i, int j) const { return z[(size_t)j * (size_t)nx + (size_t)i]; }
};
/// 국소 기복(A3): 각 셀 높이 − 반경 radius(m) 안 셀들의 평균 높이. 평평·평면 경사는 0 근처.
/// 격자가 비었거나 radius<=0 이면 빈 격자. 가장자리는 들어있는 셀만으로 평균낸다.
HeightGrid localRelief(const HeightGrid& g, double radius);
/// 기준 높이의 등고선(마칭 스퀘어, 닫힌 고리는 처음=끝). 구멍·경계에 닿는 선은 열린 채로 둔다.
std::vector<Polyline> contoursAbove(const HeightGrid& g, double level);

/// 메시들의 z 범위(띠 안 삼각형 기준)
bool bandZRange(const std::vector<MeshPtr>& meshes, const SectionFrame& f, double dNear, double dFar, double sMin, double sMax, double& zmin, double& zmax);

}  // namespace asec
