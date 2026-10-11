// Bentley ScalableMesh(.3sm, SQLite) 읽기 전용 판독. 구조는 research/3sm-format.md(실제 파일로 확인) 를 따른다.
//  열 때는 SMMasterHeader(GCS, RootNodeId) 와 SMNodeHeader(노드 색인) 만 읽는다. 기하·UV·텍스처는 노드 단위로 나중에(지연) 읽는다.
//  노드 하나 = "가상 .3mxb 타일" 경로 `<x.3sm>/n<NodeId>` → readTmxTile 이 이 경로를 알아보고 readSmVirtualTile 로 넘긴다.
//  그래서 TileCache·LodStreamer·collectLeafMeshes·점군 내보내기는 3MX 와 같은 코드로 동작한다.
//  원본은 SQLITE_OPEN_READONLY + URI immutable=1 로 연다(잠금·저널·WAL 없음 → 부속 파일을 만들지 않는다).
//  좌표: 3SM 점은 절대 실좌표(double). origin = 뿌리 Extent 중심 xy 를 1 m 로 반올림, z = 0 → 로컬 = 실좌표 − origin(float32). Z 값은 그대로.
#pragma once
#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "asec/tmx.hpp"

struct sqlite3;

namespace asec {

struct SmNodeInfo {
    int64_t id = 0, parent = -1, texId = -1;
    int level = 0;                  // SMNodeHeader.Resolution
    Box3 extentWorld;               // SMNodeHeader.Extent(실좌표)
    double geomRes = 0;             // GeometryResolution(m). 0 = 모름(잎으로 취급)
    std::vector<int64_t> children;  // SubNode
    bool hasMesh = false;           // NodeCount > 0
};

class SmFile {
public:
    /// 읽기 전용으로 열고 노드 색인을 읽는다. 실패하면 nullptr(err 한국어). 성공하면 가상 타일 등록부에 등록된다.
    static std::shared_ptr<SmFile> open(const fs::path& p, std::string* err);
    ~SmFile();
    SmFile(const SmFile&) = delete;
    SmFile& operator=(const SmFile&) = delete;

    const fs::path& path() const { return path_; }
    const std::string& gcs() const { return gcs_; }   // SMMasterHeader.GCS 원문(WKT1 COMPD_CS)
    int64_t rootId() const { return rootId_; }
    Vec3 origin() const { return origin_; }
    Box3 boundsLocal() const { return boundsLocal_; }
    const std::map<int64_t, SmNodeInfo>& nodes() const { return nodes_; }
    /// 가상 타일 경로: path()/n<id>
    fs::path virtualTilePath(int64_t id) const;
    /// 노드 하나 → TmxTile(노드 1개, 메시 디코드 완료·decoded=true, 텍스처는 JPEG 바이트(ready=false → TileCache 가 디코드)).
    /// 메시 없는 노드는 meshes 가 비어 있다. 자식은 가상 경로. 여러 스레드에서 동시에 불러도 된다(연결 풀).
    bool readNodeTile(int64_t id, TmxTile& out, std::string* err);
    /// readNodeTile 호출 수(지연 읽기 확인용)
    size_t geometryReads() const;

private:
    SmFile() = default;
    struct Conn;                                  // sqlite3* 와 준비된 문장
    std::shared_ptr<Conn> acquire(std::string* err);   // 풀에서 하나 빌림(없으면 새로 연다, 최대 8)
    void release(std::shared_ptr<Conn> c);
    bool openConnection(sqlite3** db, std::string* err) const;

    fs::path path_;
    std::string uri_;
    std::string gcs_;
    int64_t rootId_ = -1;
    Vec3 origin_;
    Box3 boundsLocal_;
    std::map<int64_t, SmNodeInfo> nodes_;
    mutable std::mutex poolMu_;
    std::vector<std::shared_ptr<Conn>> pool_;
    size_t poolSize_ = 0;
    std::atomic<size_t> reads_{0};
};

/// 파일 첫 16 바이트가 "SQLite format 3\0" 인가
bool isSqliteFile(const fs::path& p);
/// `<x.3sm>/n<id>` 꼴 가상 경로인가(부모 확장자 .3sm, 이름 n + 숫자). 맞으면 file3sm·id 를 채운다
bool parseSmVirtualPath(const fs::path& p, fs::path* file3sm, int64_t* id);
/// 등록부에서 열린 SmFile 을 찾아 readNodeTile. 그 3SM 이 열려 있지 않으면(닫힘) false
bool readSmVirtualTile(const fs::path& p, TmxTile& out, std::string* err);

}  // namespace asec
