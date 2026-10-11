// 3MX LOD 트리 탐색: 화면용 예산 선택, 단면용 최고 해상도(잎) 타일 수집. 스레드 안전 캐시.
#pragma once
#include <atomic>
#include <functional>
#include <map>
#include <mutex>
#include "asec/section.hpp"
#include "asec/tmx.hpp"

namespace asec {

/// 타일 파일 캐시(경로 → TmxTile). 디코드된 노드는 그대로 보관. 바이트 예산을 넘으면 오래된 것부터 버림.
class TileCache {
public:
    explicit TileCache(size_t byteBudget = size_t(1536) << 20) : budget_(byteBudget) {}
    std::shared_ptr<TmxTile> get(const fs::path& p, std::string* err);
    /// 노드 디코드(+선택: 텍스처 디코드 콜백). 스레드 안전: 타일 단위 잠금으로 지오메트리를 디코드하고,
    /// 텍스처(JPG) 디코드는 잠금 밖에서 한다. 돌려준 뒤에는 그 노드의 메시·텍스처가 완성되어 있다.
    bool decode(const std::shared_ptr<TmxTile>& t, size_t node, std::string* err);
    /// 지오메트리만(텍스처 JPG 디코드 없음) — 단면선(잘린 면) 계산용. 나중에 decode() 하면 텍스처만 이어서 채움
    bool decodeGeometry(const std::shared_ptr<TmxTile>& t, size_t node, std::string* err);
    std::function<void(Texture&)> textureDecoder;  // 앱이 JPG→RGBA 를 채움(여러 스레드에서 동시에 불릴 수 있음)
    /// 이번 수집이 필요로 하는 텍셀 크기(m/px, 예: 입면 영상 해상도). > 0 이면 디코드한 RGBA 를 그 절반 크기까지 2ⁿ 배로 줄여 둔다
    /// (10 cm 잎에 1024 px 사진 = 0.1 mm 텍셀을 1.5 mm/px 영상에 쓰면 메모리 수 GB → 예산 초과·퇴거 폭주). 더 촘촘한 요청이 오면 다시 디코드한다
    std::atomic<double> wantTexelM{0.0};
    size_t loadedBytes() const;
    void clear();
private:
    mutable std::mutex mu_;
    struct Entry { std::shared_ptr<TmxTile> tile; uint64_t lastUse = 0; size_t bytes = 0; };   // bytes = 디코드 때 갱신(다른 스레드가 디코드 중인 타일을 훑지 않으려고 따로 둠)
    std::map<fs::path, Entry> tiles_;
    size_t total_ = 0;                  // bytes 합(퇴거 판정을 매번 합산하지 않게)
    size_t evictSkip_ = 0;              // 지난 훑기에서 뺄 타일이 없었으면(모두 사용 중) 이만큼은 훑지 않는다 — 수집 중 O(n²) 방지
    uint64_t clock_ = 0;
    size_t budget_;
    void setBytesLocked(Entry& e, size_t b);
    void evictLocked();
};

struct LeafStats { size_t tilesVisited = 0, leafNodes = 0, meshes = 0, triangles = 0; int maxDepth = 0; size_t fallbackNodes = 0; double msRead = 0, msDecode = 0; size_t texBytes = 0; };   // msRead = 타일 읽기(직렬 순회), msDecode = 병렬 디코드, texBytes = 모은 메시의 RGBA 합

/// 진단: TileCache 가 지금까지 디코드한 텍스처 수·걸린 ms·퇴거 훑기 수(프로세스 누적, 명령줄 기록용)
struct TileCacheCounters { size_t texDecodes = 0; double texMs = 0; size_t evictScans = 0, evicted = 0; };
TileCacheCounters tileCacheCounters();

/// 띠와 겹치는 최고 해상도(자식 없는) 노드의 메시를 모은다. 자식 파일을 못 읽으면 그 노드 메시로 대체(fallbackNodes).
bool collectLeafMeshes(TileCache& cache, const fs::path& root, const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err,
                       const std::atomic<bool>* cancel = nullptr, bool textures = true, size_t maxThreads = 8);

/// 정사영상 내보내기용: XY 상자와 겹치는 노드 중 해상도 res(m/px)에 충분한 단계(3MX maxScreenDiameter 규칙:
/// 노드 대각선/res <= maxScreenDiameter 이면 그 노드, 아니면 자식으로). 잎은 항상 사용.
bool collectMeshesForResolution(TileCache& cache, const fs::path& root, const Box3& areaXY, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err,
                                const std::atomic<bool>* cancel = nullptr);
/// 단면 미리보기용: 띠와 겹치는 노드 중 해상도 res(m)에 충분한 단계(위와 같은 규칙). res<=0 이면 잎(collectLeafMeshes)
bool collectBandMeshesForResolution(TileCache& cache, const fs::path& root, const BandQuad& band, double res, std::vector<MeshPtr>& out, LeafStats* st,
                                    std::string* err, const std::atomic<bool>* cancel = nullptr);

/// 화면 표시용: 큰 노드부터 세분해 삼각형 예산 안에서 가장 고른 해상도 집합 선택.
bool selectDisplayMeshes(TileCache& cache, const fs::path& root, size_t triBudget, std::vector<MeshPtr>& out, Box3* bbox, std::string* err,
                         const std::function<void(double)>& progress = {});

}  // namespace asec
