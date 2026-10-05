#include "asec/lod.hpp"

#include <algorithm>
#include <queue>

namespace asec {

static size_t tileBytes(const TmxTile& t) {
    size_t b = t.bytes.size();
    for (auto& n : t.nodes) for (auto& m : n.meshes) b += m->pos.size() * 4 + m->uv.size() * 4 + m->idx.size() * 4;
    for (auto& tx : t.textures) b += tx.second->rgba.px.size();
    return b;
}

std::shared_ptr<TmxTile> TileCache::get(const fs::path& p, std::string* err) {
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = tiles_.find(p);
        if (it != tiles_.end()) { lastUse_[p] = ++clock_; return it->second; }
    }
    auto t = std::make_shared<TmxTile>();
    if (!readTmxTile(p, *t, err)) return nullptr;
    std::lock_guard<std::mutex> lk(mu_);
    auto it = tiles_.find(p);
    if (it != tiles_.end()) return it->second;
    tiles_[p] = t;
    lastUse_[p] = ++clock_;
    evictLocked();
    return t;
}

bool TileCache::decode(const std::shared_ptr<TmxTile>& t, size_t node, std::string* err) {
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (node < t->nodes.size() && t->nodes[node].decoded) return true;
    }
    // 같은 타일을 두 스레드가 동시에 디코드하지 않도록 타일 단위 잠금 대신 전체 잠금(디코드는 짧다)
    std::lock_guard<std::mutex> lk(mu_);
    if (!decodeNode(*t, node, err)) return false;
    if (textureDecoder)
        for (auto& m : t->nodes[node].meshes)
            if (m->texture && m->texture->rgba.empty() && !m->texture->encoded.empty()) textureDecoder(*m->texture);
    return true;
}

size_t TileCache::loadedBytes() const {
    std::lock_guard<std::mutex> lk(mu_);
    size_t b = 0;
    for (auto& kv : tiles_) b += tileBytes(*kv.second);
    return b;
}

void TileCache::clear() {
    std::lock_guard<std::mutex> lk(mu_);
    tiles_.clear(); lastUse_.clear();
}

void TileCache::evictLocked() {
    size_t total = 0;
    for (auto& kv : tiles_) total += tileBytes(*kv.second);
    while (total > budget_ && tiles_.size() > 1) {
        auto victim = tiles_.end(); uint64_t best = UINT64_MAX;
        for (auto it = tiles_.begin(); it != tiles_.end(); ++it) {
            if (it->second.use_count() > 1) continue;  // 사용 중
            uint64_t u = lastUse_[it->first];
            if (u < best) { best = u; victim = it; }
        }
        if (victim == tiles_.end()) break;
        total -= tileBytes(*victim->second);
        lastUse_.erase(victim->first);
        tiles_.erase(victim);
    }
}

static bool collectRec(TileCache& c, const fs::path& p, const BandQuad& band, std::vector<MeshPtr>& out, LeafStats& st, std::string* err, int depth,
                       const std::atomic<bool>* cancel) {
    if (cancel && cancel->load()) return false;
    auto t = c.get(p, err);
    if (!t) return false;
    st.tilesVisited++;
    st.maxDepth = std::max(st.maxDepth, depth);
    for (size_t i = 0; i < t->nodes.size(); ++i) {
        const TmxNode& n = t->nodes[i];
        if (n.bb.valid() && !bandIntersectsBox(band, n.bb)) continue;
        bool useSelf = n.isLeaf();
        if (!n.isLeaf()) {
            for (auto& ch : n.children) {
                std::string e2;
                if (!collectRec(c, ch, band, out, st, &e2, depth + 1, cancel)) {
                    if (cancel && cancel->load()) return false;
                    useSelf = true;  // 자식 파일 없음 → 이 노드가 가장 고해상도
                    st.fallbackNodes++;
                    if (err) *err = e2;
                }
            }
        }
        if (useSelf) {
            if (!c.decode(t, i, err)) return false;
            st.leafNodes++;
            for (auto& m : t->nodes[i].meshes) {
                if (m->bbox.valid() && !bandIntersectsBox(band, m->bbox)) continue;
                out.push_back(m); st.meshes++; st.triangles += m->triangleCount();
            }
        }
    }
    return true;
}

bool collectLeafMeshes(TileCache& c, const fs::path& root, const BandQuad& band, std::vector<MeshPtr>& out, LeafStats* st, std::string* err,
                       const std::atomic<bool>* cancel) {
    LeafStats s;
    bool ok = collectRec(c, root, band, out, s, err, 0, cancel);
    if (st) *st = s;
    return ok;
}

static bool boxOverlapXY(const Box3& a, const Box3& b) {
    return a.valid() && b.valid() && !(a.mx.x < b.mn.x || a.mn.x > b.mx.x || a.mx.y < b.mn.y || a.mn.y > b.mx.y);
}

static bool resRec(TileCache& c, const fs::path& p, const Box3& area, double res, std::vector<MeshPtr>& out, LeafStats& st, std::string* err, int depth,
                   const std::atomic<bool>* cancel) {
    if (cancel && cancel->load()) return false;
    auto t = c.get(p, err);
    if (!t) return false;
    st.tilesVisited++;
    st.maxDepth = std::max(st.maxDepth, depth);
    for (size_t i = 0; i < t->nodes.size(); ++i) {
        const TmxNode& n = t->nodes[i];
        if (n.bb.valid() && !boxOverlapXY(n.bb, area)) continue;
        bool enough = n.isLeaf() || (n.maxScreenDiameter > 0 && n.bb.diag() / res <= n.maxScreenDiameter);
        bool useSelf = enough;
        if (!enough) {
            for (auto& ch : n.children) {
                std::string e2;
                if (!resRec(c, ch, area, res, out, st, &e2, depth + 1, cancel)) {
                    if (cancel && cancel->load()) return false;
                    useSelf = true; st.fallbackNodes++;
                }
            }
        }
        if (useSelf) {
            if (!c.decode(t, i, err)) return false;
            st.leafNodes++;
            for (auto& m : t->nodes[i].meshes) { out.push_back(m); st.meshes++; st.triangles += m->triangleCount(); }
        }
    }
    return true;
}

bool collectMeshesForResolution(TileCache& c, const fs::path& root, const Box3& area, double res, std::vector<MeshPtr>& out, LeafStats* st, std::string* err,
                                const std::atomic<bool>* cancel) {
    LeafStats s;
    bool ok = resRec(c, root, area, res, out, s, err, 0, cancel);
    if (st) *st = s;
    return ok;
}

namespace {
struct DNode {
    std::shared_ptr<TmxTile> tile; size_t idx; double diag; size_t tris;
};
}

bool selectDisplayMeshes(TileCache& c, const fs::path& root, size_t budget, std::vector<MeshPtr>& out, Box3* bbox, std::string* err,
                         const std::function<void(double)>& progress) {
    auto rt = c.get(root, err);
    if (!rt) return false;
    auto cmp = [](const DNode& a, const DNode& b) { return a.diag < b.diag; };
    std::priority_queue<DNode, std::vector<DNode>, decltype(cmp)> open(cmp);  // 세분 후보(큰 것 먼저)
    std::vector<DNode> final;
    size_t total = 0;
    auto triCount = [&](const std::shared_ptr<TmxTile>& t, size_t i) {
        size_t n = 0; for (auto& m : t->nodes[i].meshes) n += m->triangleCount(); return n;
    };
    for (size_t i = 0; i < rt->nodes.size(); ++i) {
        if (!c.decode(rt, i, err)) return false;
        DNode d{rt, i, rt->nodes[i].bb.diag(), triCount(rt, i)};
        total += d.tris;
        open.push(d);
    }
    int steps = 0;
    while (!open.empty()) {
        DNode d = open.top(); open.pop();
        const TmxNode& n = d.tile->nodes[d.idx];
        if (n.isLeaf()) { final.push_back(d); continue; }
        std::vector<DNode> kids; size_t kt = 0; bool ok = true;
        for (auto& ch : n.children) {
            std::string e2;
            auto t = c.get(ch, &e2);
            if (!t) { ok = false; break; }
            for (size_t i = 0; i < t->nodes.size(); ++i) {
                if (!c.decode(t, i, &e2)) { ok = false; break; }
                DNode k{t, i, t->nodes[i].bb.diag(), triCount(t, i)};
                kt += k.tris; kids.push_back(k);
            }
            if (!ok) break;
        }
        if (!ok || total - d.tris + kt > budget) { final.push_back(d); continue; }
        total = total - d.tris + kt;
        for (auto& k : kids) open.push(k);
        if (progress && (++steps % 4) == 0) progress(std::min(1.0, double(total) / double(budget)));
    }
    Box3 bb;
    for (auto& d : final)
        for (auto& m : d.tile->nodes[d.idx].meshes) { out.push_back(m); bb.add(m->bbox); }
    if (bbox) *bbox = bb;
    return true;
}

}  // namespace asec
