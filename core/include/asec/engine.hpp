// 단면 계산 파이프라인(앱·CLI·향후 MDL 공용): 최고 해상도 타일 수집 → 자르기 → 이어붙이기·정리 → 입면 영상
#pragma once
#include <chrono>
#include "asec/export.hpp"
#include "asec/lod.hpp"
#include "asec/raster.hpp"
#include "asec/sm3.hpp"

namespace asec {

/// 메시 공급원: 3MX(LOD 트리) 또는 고정 메시(OBJ)
class MeshSource {
public:
    virtual ~MeshSource() = default;
    virtual bool leafMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) = 0;
    /// 해상도 res(m)에 충분한 LOD 의 메시(미리보기). 기본 = 잎. res<=0 이면 잎
    virtual bool bandMeshes(const BandQuad& band, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) {
        (void)res;
        return leafMeshes(band, out, st, err, cancel);
    }
    /// 단면선(잘린 면)용 최고 해상도 잎 메시 — 텍스처 불필요. 기본 = leafMeshes
    virtual bool cutMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) {
        return leafMeshes(band, out, st, err, cancel);
    }
    SrsInfo srs;
    Box3 bounds;  // 로컬
    /// 이번 수집의 영상 해상도(m/px) 힌트 — computeSection 이 넣고, TmxSource 가 TileCache 의 텍스처 축소에 쓴다. 0 = 힌트 없음(원본 그대로)
    double textureResHint = 0;
};

class TmxSource : public MeshSource {
public:
    TmxScene scene;
    std::shared_ptr<TileCache> cache = std::make_shared<TileCache>();
    bool open(const fs::path& p, std::string* err);
    bool leafMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) override;
    bool bandMeshes(const BandQuad& band, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) override;
    /// 잎 지오메트리만(텍스처 디코드 없음, 디코드 스레드 2개 — 끄는 동안 화면·평면 스트리밍과 CPU 를 덜 다툼)
    bool cutMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) override;
    /// XY 상자 + 해상도(정사영상용, 모든 레이어)
    bool areaMeshes(const Box3& areaXY, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel = nullptr);
};

/// 3SM(Bentley ScalableMesh, SQLite): 노드마다 가상 타일(`<x.3sm>/n<id>`) — 읽기·캐시·스트리밍은 TmxSource 와 같은 코드
class SmSource : public TmxSource {
public:
    std::shared_ptr<SmFile> file;
    /// 읽기 전용으로 열고 노드 색인만 읽는다(기하는 타일 요청 때). scene(SRS = 파일 WKT 원문, origin, 가상 뿌리)·bounds 를 채운다
    bool openSm(const fs::path& p, std::string* err);
};

class StaticSource : public MeshSource {
public:
    std::vector<MeshPtr> meshes;
    bool leafMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) override;
};

struct SectionRequest {
    SectionLine line;
    CleanupParams cleanup;
    bool wantImage = true;
    double imageRes = 0.004;   // m/px
    size_t maxImagePixels = size_t(24) << 20;
    double zMargin = 0.25;     // 표시 범위 위아래 여유
    /// 표시 범위(result.zMin · zMax)를 잘린 선 높이만으로(사용자 2026-10-16 「표고 여백을 너무 두지 마라」): 위 = 잘린 선 최고점
    /// 바로 위(같거나 높은) 10 cm 레벨선, 아래 = 최저점 바로 아래 10 cm 레벨선에서 zBelowM 더. 10 cm 는 절대 표고 기준.
    /// false = 옛 동작: 두께 띠(뒤 깊이) 안 메시 전체 높이 ± zMargin — 뒤 10 m 배경이 있으면 단면 0.7 m 에 화면 3–5 m.
    /// 입면 영상도 이 범위만 그린다(잘린 선보다 높은 뒤쪽 벽은 잘림). 잘린 선이 없으면 옛 동작
    bool zRangeFromProfile = false;
    double zBelowM = 0.1;
    double depthFade = 0;      // 입면 깊이 음영(0 = 끔). 먼 면일수록 옅게
    double meshRes = 0;        // >0: 미리보기 — 입면 영상은 이 해상도(m)에 충분한 거친 LOD(빠름). 0: 최고 해상도 잎(최종)
    /// 잘린 면(단면선)은 미리보기에서도 최고 해상도 잎으로 자름(단면 평면 ±1 mm 얇은 띠의 잎만 모음 — 뒤 3 m 배경과 무관).
    /// false 면 옛 동작(미리보기는 거친 LOD 로 자름) — 비교 시험용
    bool leafProfileAlways = true;
};

struct SectionOutput {
    SectionResult result;
    ElevationImage image;
    LeafStats stats;           // 입면 영상(두께 띠)에 쓴 메시
    LeafStats cutStats;        // 단면선(잘린 면)에 쓴 메시
    bool previewLod = false;   // 입면 영상을 거친 LOD 로 그린 미리보기
    bool cutFromLeaf = true;   // 단면선을 최고 해상도 잎으로 자름
    double msCollect = 0, msCut = 0, msImage = 0;
};

bool computeSection(MeshSource& src, const SectionRequest& rq, SectionOutput& out, std::string* err, const std::atomic<bool>* cancel = nullptr);

}  // namespace asec
