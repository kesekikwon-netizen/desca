// 시험 전용: 합성 3SM(ScalableMesh SQLite) 작성기. 스키마·BLOB 인코딩은 research/3sm-format.md(실제 파일로 확인) 그대로.
//  - SMMasterHeader(GCS, RootNodeId), SMNodeHeader(Extent·SubNode·NodeCount·NbFaceIndexes·TexID·GeometryResolution·Resolution)
//  - SMPoint(PointData = zlib(double xyz), IndexData = zlib(int32, 1 기반)), SMUVs(UVData = zlib(double uv), UVIndexData = zlib(int32, 1 기반))
//  - SMTexture(TexData = 16 B 머리(int32 w,h,c,0) + JPEG)
#pragma once
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <zlib.h>
#include "sqlite3.h"
#include "stb_image_write.h"
#include "asec/geom.hpp"
#include "synth.hpp"

namespace asec::sm3synth {
namespace fs = std::filesystem;

// H:\3sm 파일들의 SMMasterHeader.GCS 원문(동경 129° 동부원점, 가산 600000, 타원체고)
inline const char* kBentley3smWkt =
    R"WKT(COMPD_CS["Korea 2000 / East Belt ",PROJCS["Korea 2000 / East Belt ",GEOGCS["Korean2000.LL",DATUM["Korean2000",SPHEROID["GRS1980",6378137.000,298.25722210],TOWGS84[0.0000,0.0000,0.0000,-0.000000,-0.000000,-0.000000,0.00000000]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]],PROJECTION["Transverse Mercator"],PARAMETER["False Easting",200000.000],PARAMETER["False Northing",600000.000],PARAMETER["Scale Reduction",1.000000000000],PARAMETER["Central Meridian",129.00000000000003],PARAMETER["Origin Latitude",38.00000000000001],UNIT["Meter",1.00000000000000]],VERT_CS["Ellipsoid Height",VERT_DATUM["Ellipsoid",2002],UNIT["Meter",1.000000]]])WKT";

struct Node {
    int64_t id = 0, parent = -1, texId = -1;
    int level = 0;
    double ext[6] = {0, 0, 0, 0, 0, 0};   // minx,miny,minz,maxx,maxy,maxz (실좌표)
    double geomRes = 0;
    std::vector<int64_t> children;
    std::vector<double> xyz;            // 실좌표
    std::vector<int32_t> idx1;          // 1 기반
    std::vector<double> uv;
    std::vector<int32_t> uvIdx1;        // 1 기반(비어 있으면 idx1 과 같게 씀). writeUvs=false 면 SMUVs 행 없음
    bool writeUvs = true;
    bool compress = true;               // false 면 원시(비압축) BLOB
    std::vector<uint8_t> rawPointBlob;  // 비어 있지 않으면 PointData 를 이 바이트로 덮어씀(깨진 BLOB 시험)
};
struct File {
    std::string gcs = kBentley3smWkt;
    int64_t rootId = 2;
    int depth = 2;
    std::vector<Node> nodes;
    std::map<int64_t, std::vector<uint8_t>> jpegByTexId;   // TexID → JPEG 바이트(머리 없음)
    std::map<int64_t, std::pair<int, int>> texSize;        // TexID → (w, h)
};

inline std::vector<uint8_t> zlibBytes(const void* p, size_t n, bool compress) {
    if (!compress) { auto b = static_cast<const uint8_t*>(p); return std::vector<uint8_t>(b, b + n); }
    uLongf cap = compressBound(uLong(n));
    std::vector<uint8_t> out(cap);
    if (::compress(out.data(), &cap, static_cast<const Bytef*>(p), uLong(n)) != Z_OK) return {};
    out.resize(cap);
    return out;
}

inline bool exec(sqlite3* db, const char* sql, std::string* err) {
    char* e = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &e) != SQLITE_OK) { if (err) *err = e ? e : "sqlite error"; sqlite3_free(e); return false; }
    return true;
}

/// 32×32 JPEG(그러데이션) 만들기
inline std::vector<uint8_t> makeJpeg(int w = 32, int h = 32) {
    std::vector<uint8_t> rgb(size_t(w) * h * 3);
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) { auto* q = &rgb[(size_t(y) * w + x) * 3]; q[0] = uint8_t(255 * x / (w - 1)); q[1] = uint8_t(255 * y / (h - 1)); q[2] = 100; }
    std::vector<uint8_t> out;
    stbi_write_jpg_to_func([](void* ctx, void* data, int n) { auto* v = static_cast<std::vector<uint8_t>*>(ctx); v->insert(v->end(), static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + n); }, &out, w, h, 3, rgb.data(), 90);
    return out;
}

inline bool write(const fs::path& out, const File& f, std::string* err) {
    std::error_code ec; fs::remove(out, ec);
    sqlite3* db = nullptr;
    const std::string u8 = out.u8string();
    if (sqlite3_open_v2(u8.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) { if (err) *err = sqlite3_errmsg(db); sqlite3_close(db); return false; }
    bool ok = exec(db, "PRAGMA journal_mode=DELETE; BEGIN;", err) &&
        exec(db, "CREATE TABLE SMFileMetadata (Version TEXT, Properties TEXT)", err) &&
        exec(db, "CREATE TABLE SMMasterHeader (MasterHeaderId INTEGER PRIMARY KEY,Balanced INTEGER,SplitTreshold INTEGER,RootNodeId INTEGER,Depth INTEGER,IsTextured INTEGER,SingleFile INTEGER,MeshDataDepth INTEGER,IsTerrain INTEGER,GCS STRING,LastModifiedTime INTEGER,LastSyncTime INTEGER,CheckTime INTEGER,DataResolution REAL)", err) &&
        exec(db, "CREATE TABLE SMNodeHeader (NodeId INTEGER PRIMARY KEY,ParentNodeId INTEGER,Resolution INTEGER,Filtered INTEGER,Extent BLOB,ContentExtent BLOB,TotalCount INTEGER,NodeCount INTEGER,ArePoints3d INTEGER,NbFaceIndexes INTEGER,NumberOfMeshComponents INTEGER,AllComponent BLOB,IsTextured INTEGER,TexID INTEGER,SubNode BLOB,Neighbor BLOB,GeometryResolution REAL,TextureResolution REAL)", err) &&
        exec(db, "CREATE TABLE SMPoint (NodeId INTEGER PRIMARY KEY AUTOINCREMENT,PointData BLOB,IndexData BLOB,SizePts INTEGER,SizeIndices INTEGER)", err) &&
        exec(db, "CREATE TABLE SMTexture (NodeId INTEGER PRIMARY KEY,TexData BLOB,SizeTexture INTEGER,Codec INTEGER,NOfChannels INTEGER)", err) &&
        exec(db, "CREATE TABLE SMUVs (NodeId INTEGER PRIMARY KEY,UVData BLOB,SizeUVs INTEGER,UVIndexData BLOB,SizeUVIndex INTEGER)", err) &&
        exec(db, "INSERT INTO SMFileMetadata VALUES ('{\"major\":1,\"minor\":1,\"sub1\":0,\"sub2\":5}','')", err);
    if (ok) {
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db, "INSERT INTO SMMasterHeader VALUES (0,0,10000,?,?,0,1,?,0,?,0,0,0,0.0)", -1, &st, nullptr);
        sqlite3_bind_int64(st, 1, f.rootId); sqlite3_bind_int(st, 2, f.depth); sqlite3_bind_int(st, 3, f.depth); sqlite3_bind_text(st, 4, f.gcs.c_str(), -1, SQLITE_TRANSIENT);
        ok = sqlite3_step(st) == SQLITE_DONE; sqlite3_finalize(st);
        if (!ok && err) *err = sqlite3_errmsg(db);
    }
    for (size_t k = 0; ok && k < f.nodes.size(); ++k) {
        const Node& n = f.nodes[k];
        const int64_t npts = int64_t(n.xyz.size() / 3), nidx = int64_t(n.idx1.size());
        std::vector<int32_t> sub(n.children.begin(), n.children.end());
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db, "INSERT INTO SMNodeHeader (NodeId,ParentNodeId,Resolution,Filtered,Extent,ContentExtent,TotalCount,NodeCount,ArePoints3d,NbFaceIndexes,NumberOfMeshComponents,AllComponent,IsTextured,TexID,SubNode,Neighbor,GeometryResolution,TextureResolution) VALUES (?,?,?,0,?,?,?,?,?,?,0,NULL,?,?,?,NULL,?,?)", -1, &st, nullptr);
        sqlite3_bind_int64(st, 1, n.id); sqlite3_bind_int64(st, 2, n.parent); sqlite3_bind_int(st, 3, n.level);
        sqlite3_bind_blob(st, 4, n.ext, 48, SQLITE_TRANSIENT); sqlite3_bind_blob(st, 5, n.ext, 48, SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 6, npts); sqlite3_bind_int64(st, 7, npts); sqlite3_bind_int(st, 8, npts > 0); sqlite3_bind_int64(st, 9, nidx);
        sqlite3_bind_int(st, 10, n.texId >= 0); sqlite3_bind_int64(st, 11, n.texId);
        if (sub.empty()) sqlite3_bind_null(st, 12); else sqlite3_bind_blob(st, 12, sub.data(), int(sub.size() * 4), SQLITE_TRANSIENT);
        sqlite3_bind_double(st, 13, n.geomRes); sqlite3_bind_double(st, 14, n.geomRes);
        ok = sqlite3_step(st) == SQLITE_DONE; if (!ok && err) *err = sqlite3_errmsg(db); sqlite3_finalize(st);
        if (!ok || npts == 0) continue;
        auto pb = n.rawPointBlob.empty() ? zlibBytes(n.xyz.data(), n.xyz.size() * 8, n.compress) : n.rawPointBlob;
        auto ib = zlibBytes(n.idx1.data(), n.idx1.size() * 4, n.compress);
        sqlite3_prepare_v2(db, "INSERT INTO SMPoint (NodeId,PointData,IndexData,SizePts,SizeIndices) VALUES (?,?,?,?,?)", -1, &st, nullptr);
        sqlite3_bind_int64(st, 1, n.id); sqlite3_bind_blob(st, 2, pb.data(), int(pb.size()), SQLITE_TRANSIENT); sqlite3_bind_blob(st, 3, ib.data(), int(ib.size()), SQLITE_TRANSIENT);
        sqlite3_bind_int64(st, 4, int64_t(n.xyz.size() * 8)); sqlite3_bind_int64(st, 5, int64_t(n.idx1.size() * 4));
        ok = sqlite3_step(st) == SQLITE_DONE; if (!ok && err) *err = sqlite3_errmsg(db); sqlite3_finalize(st);
        if (!ok || !n.writeUvs || n.uv.empty()) continue;
        const std::vector<int32_t>& ui = n.uvIdx1.empty() ? n.idx1 : n.uvIdx1;
        auto ub = zlibBytes(n.uv.data(), n.uv.size() * 8, n.compress);
        auto uib = zlibBytes(ui.data(), ui.size() * 4, n.compress);
        sqlite3_prepare_v2(db, "INSERT INTO SMUVs (NodeId,UVData,SizeUVs,UVIndexData,SizeUVIndex) VALUES (?,?,?,?,?)", -1, &st, nullptr);
        sqlite3_bind_int64(st, 1, n.id); sqlite3_bind_blob(st, 2, ub.data(), int(ub.size()), SQLITE_TRANSIENT); sqlite3_bind_int64(st, 3, int64_t(n.uv.size() * 8));
        sqlite3_bind_blob(st, 4, uib.data(), int(uib.size()), SQLITE_TRANSIENT); sqlite3_bind_int64(st, 5, int64_t(ui.size() * 4));
        ok = sqlite3_step(st) == SQLITE_DONE; if (!ok && err) *err = sqlite3_errmsg(db); sqlite3_finalize(st);
    }
    for (auto it = f.jpegByTexId.begin(); ok && it != f.jpegByTexId.end(); ++it) {
        auto sz = f.texSize.count(it->first) ? f.texSize.at(it->first) : std::make_pair(32, 32);
        std::vector<uint8_t> blob(16, 0);
        const int32_t hdr[4] = {sz.first, sz.second, 3, 0};
        std::memcpy(blob.data(), hdr, 16);
        blob.insert(blob.end(), it->second.begin(), it->second.end());
        sqlite3_stmt* st = nullptr;
        sqlite3_prepare_v2(db, "INSERT INTO SMTexture (NodeId,TexData,SizeTexture,Codec,NOfChannels) VALUES (?,?,?,NULL,NULL)", -1, &st, nullptr);
        sqlite3_bind_int64(st, 1, it->first); sqlite3_bind_blob(st, 2, blob.data(), int(blob.size()), SQLITE_TRANSIENT); sqlite3_bind_int64(st, 3, int64_t(sz.first) * sz.second * 3);
        ok = sqlite3_step(st) == SQLITE_DONE; if (!ok && err) *err = sqlite3_errmsg(db); sqlite3_finalize(st);
    }
    if (ok) ok = exec(db, "COMMIT;", err);
    sqlite3_close(db);
    return ok;
}

/// 로컬 격자 메시(synth::gridMesh, 로컬 좌표) → 노드(실좌표 = 로컬 + origin, 1 기반 인덱스, 범위)
inline Node nodeFromMesh(int64_t id, int64_t parent, int level, const MeshPtr& m, const Vec3& origin, double geomRes, int64_t texId) {
    Node n; n.id = id; n.parent = parent; n.level = level; n.geomRes = geomRes; n.texId = texId;
    Box3 b;
    for (size_t i = 0; i < m->vertexCount(); ++i) {
        Vec3 w = Vec3(m->pos[3 * i], m->pos[3 * i + 1], m->pos[3 * i + 2]) + origin;
        n.xyz.push_back(w.x); n.xyz.push_back(w.y); n.xyz.push_back(w.z); b.add(w);
        if (!m->uv.empty()) { n.uv.push_back(m->uv[2 * i]); n.uv.push_back(m->uv[2 * i + 1]); }
    }
    for (uint32_t k : m->idx) n.idx1.push_back(int32_t(k) + 1);
    n.ext[0] = b.mn.x; n.ext[1] = b.mn.y; n.ext[2] = b.mn.z; n.ext[3] = b.mx.x; n.ext[4] = b.mx.y; n.ext[5] = b.mx.z;
    return n;
}

/// 지면 모델: 뿌리 2(메시 없음) → 3(거친 격자 leaf*4) → 잎 4(왼쪽 반)·5(오른쪽 반, 간격 leaf). 텍스처 1장(TexID 1) 을 세 노드가 공유.
/// 좌표는 synth::height 의 지면(로컬) + origin.
inline File groundFile(const Vec3& origin, double size, double leaf) {
    File f;
    auto coarse = synth::gridMesh(0, 0, size, size, leaf * 4);
    auto left = synth::gridMesh(0, 0, size / 2, size, leaf);
    auto right = synth::gridMesh(size / 2, 0, size, size, leaf);
    Node n3 = nodeFromMesh(3, 2, 1, coarse, origin, leaf * 4, 1); n3.children = {4, 5};
    Node n4 = nodeFromMesh(4, 3, 2, left, origin, leaf, 1);
    Node n5 = nodeFromMesh(5, 3, 2, right, origin, leaf, 1);
    Node root; root.id = 2; root.parent = -1; root.level = 0; root.geomRes = leaf * 16; root.children = {3};
    std::memcpy(root.ext, n3.ext, 48);
    f.nodes = {root, n3, n4, n5};
    f.jpegByTexId[1] = makeJpeg();
    f.texSize[1] = {32, 32};
    return f;
}

}  // namespace asec::sm3synth
