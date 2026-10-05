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
    SrsInfo srs;
    Box3 bounds;  // 로컬
};

class TmxSource : public MeshSource {
public:
    TmxScene scene;
    std::shared_ptr<TileCache> cache = std::make_shared<TileCache>();
    bool open(const fs::path& p, std::string* err);
    bool leafMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) override;
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
};

struct SectionOutput {
    SectionResult result;
    ElevationImage image;
    LeafStats stats;
    double msCollect = 0, msCut = 0, msImage = 0;
};

bool computeSection(MeshSource& src, const SectionRequest& rq, SectionOutput& out, std::string* err, const std::atomic<bool>* cancel = nullptr);

}  // namespace asec
