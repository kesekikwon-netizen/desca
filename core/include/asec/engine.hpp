// 단면 계산 파이프라인(앱·CLI·향후 MDL 공용): 최고 해상도 타일 수집 → 자르기 → 이어붙이기·정리 → 입면 영상
#pragma once
#include <chrono>
#include "asec/export.hpp"
#include "asec/lod.hpp"
#include "asec/raster.hpp"

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
    SrsInfo srs;
    Box3 bounds;  // 로컬
};

class TmxSource : public MeshSource {
public:
    TmxScene scene;
    std::shared_ptr<TileCache> cache = std::make_shared<TileCache>();
    bool open(const fs::path& p, std::string* err);
    bool leafMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) override;
    bool bandMeshes(const BandQuad& band, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) override;
    /// XY 상자 + 해상도(정사영상용, 모든 레이어)
    bool areaMeshes(const Box3& areaXY, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel = nullptr);
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
    double depthFade = 0;      // 입면 깊이 음영(0 = 끔). 먼 면일수록 옅게
    double meshRes = 0;        // >0: 미리보기 — 이 해상도(m)에 충분한 거친 LOD 로 계산(빠름). 0: 최고 해상도 잎(최종)
};

struct SectionOutput {
    SectionResult result;
    ElevationImage image;
    LeafStats stats;
    bool previewLod = false;   // 거친 LOD 로 계산한 미리보기
    double msCollect = 0, msCut = 0, msImage = 0;
};

bool computeSection(MeshSource& src, const SectionRequest& rq, SectionOutput& out, std::string* err, const std::atomic<bool>* cancel = nullptr);

}  // namespace asec
