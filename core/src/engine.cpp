#include "asec/engine.hpp"

namespace asec {
using clk = std::chrono::steady_clock;
static double msSince(clk::time_point t) { return std::chrono::duration<double, std::milli>(clk::now() - t).count(); }

bool TmxSource::open(const fs::path& p, std::string* err) {
    if (!readTmxScene(p, scene, err)) return false;
    srs = scene.srs;
    auto t = cache->get(scene.rootFile, err);
    if (!t) return false;
    bounds = Box3();
    for (auto& n : t->nodes) bounds.add(n.bb);
    return true;
}

bool TmxSource::leafMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err, const std::atomic<bool>* cancel) {
    return collectLeafMeshes(*cache, scene.rootFile, band, out, st, err, cancel);
}

bool StaticSource::leafMeshes(const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string*, const std::atomic<bool>*) {
    LeafStats s;
    for (auto& m : meshes)
        if (!m->bbox.valid() || bandIntersectsBox(band, m->bbox)) { out.push_back(m); s.meshes++; s.triangles += m->triangleCount(); }
    s.leafNodes = s.meshes;
    if (st) *st = s;
    return true;
}

bool computeSection(MeshSource& src, const SectionRequest& rq, SectionOutput& out, std::string* err, const std::atomic<bool>* cancel) {
    out = SectionOutput();
    SectionFrame f(rq.line);
    if (f.L < 1e-3) { if (err) *err = "단면선이 너무 짧습니다"; return false; }
    auto t0 = clk::now();
    std::vector<MeshPtr> meshes;
    BandQuad band = sectionBand(rq.line, 0.0);
    // 단면 평면 자체(두께 0)도 포함되도록 띠가 비어 있으면 약간 넓힘
    if (rq.line.front + rq.line.back < 1e-6) band = sectionBand(SectionLine{rq.line.a, rq.line.b, 0.001, 0.001}, 0.0);
    if (!src.leafMeshes(band, meshes, &out.stats, err, cancel)) return false;
    out.msCollect = msSince(t0);
    if (cancel && cancel->load()) return false;

    auto t1 = clk::now();
    std::vector<CutSeg> segs;
    for (auto& m : meshes) cutMesh(*m, f, 0.0, 0.0, f.L, segs);
    out.result.line = rq.line;
    out.result.srs = src.srs;
    out.result.rawSegments = segs.size();
    out.result.triangles = out.stats.triangles;
    out.result.tiles = out.stats.leafNodes;
    out.result.profile = buildProfile(segs, rq.cleanup);
    snapEnds(out.result.profile, 0.0, f.L, rq.cleanup.weldTol * 2.5);
    out.msCut = msSince(t1);

    double zmin = 1e300, zmax = -1e300;
    for (auto& pl : out.result.profile) for (auto& p : pl) { zmin = std::min(zmin, p.y); zmax = std::max(zmax, p.y); }
    double bz0, bz1;
    if (bandZRange(meshes, f, -rq.line.front, rq.line.back, 0.0, f.L, bz0, bz1)) { zmin = std::min(zmin, bz0); zmax = std::max(zmax, bz1); }
    if (zmax < zmin) { if (err) *err = "단면선이 메시와 만나지 않습니다"; return false; }
    out.result.zMin = std::floor((zmin - rq.zMargin) * 10.0) / 10.0;
    out.result.zMax = std::ceil((zmax + rq.zMargin) * 10.0) / 10.0;

    if (rq.wantImage) {
        auto t2 = clk::now();
        RasterRequest rr;
        rr.s0 = 0; rr.s1 = f.L; rr.z0 = out.result.zMin; rr.z1 = out.result.zMax;
        rr.res = rq.imageRes; rr.dNear = -rq.line.front; rr.dFar = rq.line.back; rr.maxPixels = rq.maxImagePixels;
        if (rr.dFar - rr.dNear < 1e-4) rr.dFar = rr.dNear + 0.01;
        renderElevation(meshes, f, rr, out.image, cancel);
        out.msImage = msSince(t2);
    }
    return !(cancel && cancel->load());
}

}  // namespace asec
