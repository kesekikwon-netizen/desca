// Bentley 3MX (공개 명세 3MX 1.0) 읽기/쓰기.
//  .3mx  = JSON 루트(layers[].type=="meshPyramid", SRS, SRSOrigin, root)
//  .3mxb = "3MXBO" + uint32 헤더길이 + JSON 헤더(nodes, resources) + 리소스 버퍼(헤더 순서)
//  geometryBuffer 형식 "ctm" = OpenCTM.  자식(children)은 현재 .3mxb 기준 상대 경로.
#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include "asec/geom.hpp"

namespace asec {
namespace fs = std::filesystem;

struct SrsInfo {
    std::string srs;      // 예: "EPSG:5186" (없으면 빈 문자열)
    Vec3 origin;          // SRSOrigin. 실좌표 = 로컬 + origin
    bool hasOrigin = false;
    Vec3 toWorld(const Vec3& local) const { return local + origin; }
    Vec3 toLocal(const Vec3& world) const { return world - origin; }
    /// 수평 좌표계: "EPSG:5186" 또는 복합 "EPSG:5186+5711" → 5186, 아니면 0
    int epsg() const;
    /// 수직 기준(복합 좌표계의 '+' 뒤): "EPSG:5186+5711" / "EPSG:5186+EPSG:5711" → 5711, 없으면 0.
    /// 높이 값은 바꾸지 않는다(모델 SRS 그대로) — 이 번호는 메타데이터(GeoTIFF·LAS 키, 표시)로만 전달
    int verticalEpsg() const;
};

struct TmxScene {
    std::string name, description;
    SrsInfo srs;
    fs::path rootFile;  // 최상위 .3mxb 절대 경로
};

struct TmxNode {
    std::string id;
    Box3 bb;                         // 로컬 좌표
    double maxScreenDiameter = 0;
    std::vector<fs::path> children;  // 자식 .3mxb 절대 경로(비어 있으면 최고 해상도 = 잎)
    std::vector<std::string> resourceIds;
    std::vector<MeshPtr> meshes;     // decodeNode 후 채워짐
    bool decoded = false;            // 지오메트리 디코드됨
    bool ready = false;              // TileCache::decode 완료(텍스처까지). TmxTile::mu 아래에서만 읽고 쓴다
    bool isLeaf() const { return children.empty(); }
};

struct TmxTile {
    fs::path path;
    /// 타일 단위 잠금(TileCache 가 디코드 때 사용). 서로 다른 타일은 동시에 디코드된다
    std::shared_ptr<std::mutex> mu = std::make_shared<std::mutex>();
    std::vector<TmxNode> nodes;
    // 내부: 원본 바이트와 리소스 위치(지연 디코드용)
    std::vector<uint8_t> bytes;
    struct Res { std::string id, type, format, texId, file; size_t off = 0, size = 0; Box3 bb; };
    std::vector<Res> resources;
    std::vector<std::pair<std::string, TexturePtr>> textures;
};

/// OpenCTM 메모리 디코드/인코드
bool decodeCtm(const uint8_t* data, size_t size, Mesh& out, std::string* err);
bool encodeCtm(const Mesh& m, std::vector<uint8_t>& out, std::string* err, bool mg2 = true, double vertexPrecision = 0.0005);

bool readTmxScene(const fs::path& file3mx, TmxScene& out, std::string* err);
/// 헤더와 버퍼를 읽는다(지오메트리는 decodeNode 로 지연 디코드).
bool readTmxTile(const fs::path& file3mxb, TmxTile& out, std::string* err);
bool decodeNode(TmxTile& tile, size_t nodeIndex, std::string* err);
inline bool decodeAll(TmxTile& t, std::string* err) {
    for (size_t i = 0; i < t.nodes.size(); ++i) if (!decodeNode(t, i, err)) return false;
    return true;
}

// ---- 쓰기(합성 시험 데이터·변환용) ----
struct TmxWriteNode {
    std::string id;
    double maxScreenDiameter = 0;
    std::vector<std::string> childFiles;  // 이 .3mxb 기준 상대 경로
    std::vector<MeshPtr> meshes;          // 각 메시 texture->encoded(jpg) 를 버퍼로 씀
};
bool writeTmxTile(const fs::path& file3mxb, const std::vector<TmxWriteNode>& nodes, std::string* err);
bool writeTmxScene(const fs::path& file3mx, const std::string& name, const SrsInfo& srs, const std::string& rootRelative, std::string* err);

/// 파일 전체 읽기(유니코드 경로 안전)
bool readFileBytes(const fs::path& p, std::vector<uint8_t>& out);

}  // namespace asec
