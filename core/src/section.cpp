#include "asec/section.hpp"

#include <algorithm>
#include <cstdio>
#include <unordered_map>
#include <unordered_set>

namespace asec {

size_t cutMesh(const Mesh& m, const SectionFrame& f, double dOff, double sMin, double sMax, std::vector<CutSeg>& out) {
    const float* P = m.pos.data();
    const size_t nt = m.triangleCount();
    size_t added = 0;
    auto D = [&](uint32_t i) { return f.d(P[3 * i], P[3 * i + 1]) - dOff; };
    for (size_t t = 0; t < nt; ++t) {
        uint32_t v[3] = {m.idx[3 * t], m.idx[3 * t + 1], m.idx[3 * t + 2]};
        double d[3] = {D(v[0]), D(v[1]), D(v[2])};
        bool pos[3] = {d[0] >= 0, d[1] >= 0, d[2] >= 0};
        if (pos[0] == pos[1] && pos[1] == pos[2]) continue;
        double sv[3], smin = 1e300, smax = -1e300;
        for (int k = 0; k < 3; ++k) { sv[k] = f.s(P[3 * v[k]], P[3 * v[k] + 1]); smin = std::min(smin, sv[k]); smax = std::max(smax, sv[k]); }
        if (smax < sMin || smin > sMax) continue;
        double ps[2], pz[2];
        int c = 0;
        for (int e = 0; e < 3 && c < 2; ++e) {
            int i = e, j = (e + 1) % 3;
            if (pos[i] == pos[j]) continue;
            // 정준 순서: 꼭짓점 번호가 작은 쪽 기준 → 이웃 삼각형과 동일 결과
            int lo = v[i] < v[j] ? i : j, hi = lo == i ? j : i;
            double tt = d[lo] / (d[lo] - d[hi]);
            const float* A = P + 3 * v[lo];
            const float* B = P + 3 * v[hi];
            double x = A[0] + tt * (double(B[0]) - A[0]);
            double y = A[1] + tt * (double(B[1]) - A[1]);
            double z = A[2] + tt * (double(B[2]) - A[2]);
            ps[c] = f.s(x, y); pz[c] = z; ++c;
        }
        if (c != 2) continue;
        // [sMin,sMax] 로 자르기
        double s0 = ps[0], z0 = pz[0], s1 = ps[1], z1 = pz[1];
        if (s0 > s1) { std::swap(s0, s1); std::swap(z0, z1); }
        if (s1 < sMin || s0 > sMax) continue;
        if (s0 < sMin && s1 > s0) { z0 = z0 + (z1 - z0) * (sMin - s0) / (s1 - s0); s0 = sMin; }
        if (s1 > sMax && s1 > s0) { z1 = z0 + (z1 - z0) * (sMax - s0) / (s1 - s0); s1 = sMax; }
        out.push_back({s0, z0, s1, z1});
        ++added;
    }
    return added;
}

// ---------------- 이어 붙이기 ----------------
namespace {
struct WeldGrid {
    double cell;
    std::unordered_map<uint64_t, std::vector<uint32_t>> map;
    std::vector<Vec2> pts;
    explicit WeldGrid(double c) : cell(c > 0 ? c : 1e-6) {}
    static uint64_t key(int64_t ix, int64_t iy) { return (uint64_t(ix) * 0x9E3779B97F4A7C15ULL) ^ (uint64_t(iy) + 0x632BE59BD9B4E019ULL + (uint64_t(ix) << 6)); }
    uint32_t add(double x, double y) {
        int64_t ix = int64_t(std::floor(x / cell)), iy = int64_t(std::floor(y / cell));
        double t2 = cell * cell;
        for (int64_t dx = -1; dx <= 1; ++dx)
            for (int64_t dy = -1; dy <= 1; ++dy) {
                auto it = map.find(key(ix + dx, iy + dy));
                if (it == map.end()) continue;
                for (uint32_t id : it->second) {
                    double ex = pts[id].x - x, ey = pts[id].y - y;
                    if (ex * ex + ey * ey <= t2) return id;
                }
            }
        uint32_t id = uint32_t(pts.size());
        pts.emplace_back(x, y);
        map[key(ix, iy)].push_back(id);
        return id;
    }
};
}  // namespace

std::vector<Polyline> stitchSegments(const std::vector<CutSeg>& segs, double tol) {
    WeldGrid g(tol);
    std::vector<std::pair<uint32_t, uint32_t>> edges;
    edges.reserve(segs.size());
    std::unordered_set<uint64_t> seen;
    for (auto& s : segs) {
        uint32_t a = g.add(s.s0, s.z0), b = g.add(s.s1, s.z1);
        if (a == b) continue;  // 퇴화
        uint64_t k = a < b ? (uint64_t(a) << 32 | b) : (uint64_t(b) << 32 | a);
        if (!seen.insert(k).second) continue;  // 겹선
        edges.emplace_back(a, b);
    }
    const size_t nv = g.pts.size();
    std::vector<std::vector<uint32_t>> adj(nv);
    for (uint32_t e = 0; e < edges.size(); ++e) { adj[edges[e].first].push_back(e); adj[edges[e].second].push_back(e); }
    std::vector<char> used(edges.size(), 0);
    std::vector<Polyline> lines;
    auto other = [&](uint32_t e, uint32_t v) { return edges[e].first == v ? edges[e].second : edges[e].first; };
    auto walk = [&](uint32_t start, uint32_t e0) {
        Polyline pl;
        pl.push_back(g.pts[start]);
        uint32_t cur = start, e = e0;
        while (true) {
            used[e] = 1;
            uint32_t nx = other(e, cur);
            pl.push_back(g.pts[nx]);
            cur = nx;
            if (adj[cur].size() != 2) break;
            uint32_t ne = UINT32_MAX;
            for (uint32_t c : adj[cur]) if (!used[c]) { ne = c; break; }
            if (ne == UINT32_MAX) break;
            e = ne;
        }
        lines.push_back(std::move(pl));
    };
    for (uint32_t v = 0; v < nv; ++v)
        if (adj[v].size() != 2)
            for (uint32_t e : adj[v]) if (!used[e]) walk(v, e);
    for (uint32_t e = 0; e < edges.size(); ++e)  // 남은 것 = 닫힌 고리
        if (!used[e]) walk(edges[e].first, e);
    return lines;
}

// ---------------- 정리 ----------------
double polylineLength(const Polyline& pl) {
    double L = 0;
    for (size_t i = 1; i < pl.size(); ++i) L += (pl[i] - pl[i - 1]).len();
    return L;
}

void removeDuplicatePoints(Polyline& pl, double tol) {
    if (pl.size() < 2) return;
    Polyline o;
    o.reserve(pl.size());
    o.push_back(pl[0]);
    for (size_t i = 1; i < pl.size(); ++i)
        if ((pl[i] - o.back()).len() > tol) o.push_back(pl[i]);
    // 마지막 원래 점(끝점 A' 등)은 항상 보존: 가까워서 빠졌다면 직전 점을 끝점으로 바꿈
    if ((o.back() - pl.back()).len() > 0) {
        if (o.size() >= 2) o.back() = pl.back();
        else if ((pl.back() - o.back()).len() > 0) o.push_back(pl.back());
    }
    pl.swap(o);
}

static bool isClosed(const Polyline& pl) { return pl.size() > 3 && (pl.front() - pl.back()).len() < 1e-12; }

int removeSpikes(Polyline& pl, double maxLen, double turnDeg) {
    const bool closed = isClosed(pl);
    const double cosLim = std::cos(turnDeg * 3.14159265358979323846 / 180.0);
    int removed = 0;
    bool changed = true;
    while (changed && pl.size() >= 3) {
        changed = false;
        for (size_t i = 1; i + 1 < pl.size(); ++i) {
            Vec2 a = pl[i] - pl[i - 1], b = pl[i + 1] - pl[i];
            double la = a.len(), lb = b.len();
            if (la <= 0 || lb <= 0) continue;
            double c = a.dot(b) / (la * lb);  // 진행 방향 변화의 cos (−1 = 완전 되돌아감)
            if (c <= cosLim && std::min(la, lb) <= maxLen) {
                pl.erase(pl.begin() + long(i));
                ++removed; changed = true;
                if (i > 1) --i;
            }
        }
        // 끝의 짧은 갈고리: 마지막 두 선분이 되돌아가면 끝점 버림
        for (int pass = 0; pass < 2 && pl.size() >= 3 && !closed; ++pass) {
            size_t n = pl.size();
            Vec2 a = pl[n - 2] - pl[n - 3], b = pl[n - 1] - pl[n - 2];
            double la = a.len(), lb = b.len();
            if (la > 0 && lb > 0 && a.dot(b) / (la * lb) <= cosLim && lb <= maxLen) { pl.pop_back(); ++removed; changed = true; }
            std::reverse(pl.begin(), pl.end());
        }
    }
    return removed;
}

void joinGaps(std::vector<Polyline>& L, double gapTol) {
    while (true) {
        double best = gapTol;
        int bi = -1, bj = -1, mode = 0;  // mode: 0 = i끝-j처음, 1 = i끝-j끝, 2 = i처음-j처음, 3 = i처음-j끝
        for (size_t i = 0; i < L.size(); ++i) {
            if (L[i].size() < 2 || isClosed(L[i])) continue;  // 닫힌 고리(나무·돌 등)는 다른 선과 잇지 않음
            for (size_t j = i + 1; j < L.size(); ++j) {
                if (L[j].size() < 2 || isClosed(L[j])) continue;
                const Vec2 ends[4][2] = {{L[i].back(), L[j].front()}, {L[i].back(), L[j].back()}, {L[i].front(), L[j].front()}, {L[i].front(), L[j].back()}};
                for (int k = 0; k < 4; ++k) {
                    double d = (ends[k][0] - ends[k][1]).len();
                    if (d <= best) { best = d; bi = int(i); bj = int(j); mode = k; }
                }
            }
        }
        if (bi < 0) break;
        Polyline& A = L[size_t(bi)];
        Polyline B = std::move(L[size_t(bj)]);
        L.erase(L.begin() + bj);
        if (mode == 1) std::reverse(B.begin(), B.end());
        if (mode == 2) std::reverse(A.begin(), A.end());
        if (mode == 3) { std::reverse(A.begin(), A.end()); std::reverse(B.begin(), B.end()); }
        // 이제 A끝 → B처음
        size_t startB = ((A.back() - B.front()).len() <= 1e-12) ? 1 : 0;
        A.insert(A.end(), B.begin() + long(startB), B.end());
    }
    // 거의 닫힌 고리(끝과 처음 사이 틈 <= gapTol)는 닫는다
    for (auto& pl : L) {
        if (pl.size() < 4 || isClosed(pl)) continue;
        double gap = (pl.front() - pl.back()).len();
        if (gap <= gapTol && polylineLength(pl) > 4 * std::max(gap, 1e-6)) {
            if (gap < 1e-9) pl.back() = pl.front(); else pl.push_back(pl.front());
        }
    }
}

Polyline simplifyDP(const Polyline& pl, double tol) {
    const size_t n = pl.size();
    if (n < 3 || tol <= 0) return pl;
    std::vector<char> keep(n, 0);
    keep[0] = keep[n - 1] = 1;
    std::vector<std::pair<size_t, size_t>> st{{0, n - 1}};
    while (!st.empty()) {
        auto [i, j] = st.back(); st.pop_back();
        if (j <= i + 1) continue;
        Vec2 a = pl[i], ab = pl[j] - pl[i];
        double L2 = ab.dot(ab);
        double md = -1; size_t mk = i;
        for (size_t k = i + 1; k < j; ++k) {
            Vec2 ap = pl[k] - a;
            double d;
            if (L2 <= 0) d = ap.len();
            else {
                double t = std::clamp(ap.dot(ab) / L2, 0.0, 1.0);
                d = (ap - ab * t).len();
            }
            if (d > md) { md = d; mk = k; }
        }
        if (md > tol) { keep[mk] = 1; st.push_back({i, mk}); st.push_back({mk, j}); }
    }
    Polyline o;
    for (size_t k = 0; k < n; ++k) if (keep[k]) o.push_back(pl[k]);
    return o;
}

void smoothTaubin(Polyline& pl, int iters) {
    if (pl.size() < 3) return;
    bool closed = (pl.front() - pl.back()).len() < 1e-12;
    auto pass = [&](double f) {
        Polyline q = pl;
        size_t n = pl.size();
        for (size_t i = 1; i + 1 < n; ++i) q[i] = pl[i] + ((pl[i - 1] + pl[i + 1]) * 0.5 - pl[i]) * f;
        if (closed && n > 3) { q[0] = pl[0] + ((pl[n - 2] + pl[1]) * 0.5 - pl[0]) * f; q[n - 1] = q[0]; }
        pl.swap(q);
    };
    for (int k = 0; k < iters; ++k) { pass(0.5); pass(-0.53); }
}

size_t countDuplicateVertices(const std::vector<Polyline>& L, double tol) {
    size_t c = 0;
    for (auto& pl : L)
        for (size_t i = 1; i < pl.size(); ++i) if ((pl[i] - pl[i - 1]).len() < tol) ++c;
    return c;
}

void cleanupPolylines(std::vector<Polyline>& L, const CleanupParams& p) {
    for (auto& pl : L) { removeDuplicatePoints(pl, p.weldTol); removeSpikes(pl, p.spikeLen, p.spikeTurnDeg); }
    // 아주 작은 부스러기 먼저 버림(용접 공차 4배 미만) → 잇기에서 끼어들지 않게
    L.erase(std::remove_if(L.begin(), L.end(), [&](const Polyline& q) { return q.size() < 2 || polylineLength(q) < p.weldTol * 4; }), L.end());
    joinGaps(L, p.gapTol);
    for (auto& pl : L) { removeDuplicatePoints(pl, p.weldTol); removeSpikes(pl, p.spikeLen, p.spikeTurnDeg); }
    L.erase(std::remove_if(L.begin(), L.end(), [&](const Polyline& q) { return q.size() < 2 || polylineLength(q) < p.minFragment; }), L.end());
    for (auto& pl : L) {
        pl = simplifyDP(pl, p.simplifyTol);
        if (p.smooth) smoothTaubin(pl, p.smoothIter);
        removeDuplicatePoints(pl, std::max(p.weldTol, 1e-9));
        bool closed = pl.size() > 2 && (pl.front() - pl.back()).len() < 1e-12;
        if (!closed && pl.front().x > pl.back().x) std::reverse(pl.begin(), pl.end());
    }
    std::sort(L.begin(), L.end(), [](const Polyline& a, const Polyline& b) {
        double ma = 1e300, mb = 1e300;
        for (auto& v : a) ma = std::min(ma, v.x);
        for (auto& v : b) mb = std::min(mb, v.x);
        return ma < mb;
    });
}

void snapEnds(std::vector<Polyline>& L, double s0, double s1, double tol) {
    auto snap = [&](Vec2& e, const Vec2& prev) {
        for (double target : {s0, s1}) {
            if (std::fabs(e.x - target) > tol || std::fabs(e.x - target) == 0) continue;
            double dx = e.x - prev.x;
            if (std::fabs(dx) > 1e-12) e.y = prev.y + (e.y - prev.y) * (target - prev.x) / dx;
            e.x = target;
            return;
        }
    };
    for (auto& pl : L) {
        if (pl.size() < 2) continue;
        snap(pl.front(), pl[1]);
        snap(pl.back(), pl[pl.size() - 2]);
    }
}

// ---------------- 레벨선 ----------------
std::vector<LevelLine> levelLines(double zmin, double zmax, int step, int major, int master) {
    std::vector<LevelLine> out;
    if (!(zmax >= zmin) || step <= 0) return out;
    long k0 = long(std::ceil(zmin * 100.0 / step - 1e-7)), k1 = long(std::floor(zmax * 100.0 / step + 1e-7));
    if (k1 - k0 > 200000) return out;
    auto mod0 = [](long a, long m) { return m > 0 && ((a % m) + m) % m == 0; };
    for (long k = k0; k <= k1; ++k) {
        long cm = k * step;
        LevelClass c = mod0(cm, master) ? LevelClass::Master : mod0(cm, major) ? LevelClass::Major : LevelClass::Minor;
        out.push_back({cm, cm / 100.0, c});
    }
    return out;
}

std::string formatElevation(double z, int decimals) {
    decimals = std::clamp(decimals, 0, 3);
    long long scale = decimals == 0 ? 1 : decimals == 1 ? 10 : decimals == 2 ? 100 : 1000;
    long long c = std::llround(z * double(scale));
    bool neg = c < 0;
    if (neg) c = -c;
    char b[48];
    if (decimals == 0) std::snprintf(b, sizeof b, "%s%lld", neg ? "-" : "", c);
    else std::snprintf(b, sizeof b, "%s%lld.%0*lld", neg ? "-" : "", c / scale, decimals, c % scale);
    return b;
}

int labelStepCm(double pxPerMeter, double minPx) {
    // 숫자 라벨은 50 cm 마다(78.5 / 79.0), 너무 촘촘하면 1 m · 5 m · 10 m. 10 cm 선에는 라벨 없음
    for (int s : {50, 100, 500, 1000}) if (s / 100.0 * pxPerMeter >= minPx) return s;
    return 1000;
}

double niceStep(double range, int target) {
    if (range <= 0 || target <= 0) return 1;
    double raw = range / target, mag = std::pow(10.0, std::floor(std::log10(raw))), r = raw / mag;
    return (r < 1.5 ? 1 : r < 3.5 ? 2 : r < 7.5 ? 5 : 10) * mag;
}

// ---------------- 띠 ----------------
BandQuad sectionBand(const SectionLine& l, double m) {
    SectionFrame f(l);
    BandQuad q;
    q.p[0] = f.planXY(-m, -l.front - m);
    q.p[1] = f.planXY(f.L + m, -l.front - m);
    q.p[2] = f.planXY(f.L + m, l.back + m);
    q.p[3] = f.planXY(-m, l.back + m);
    return q;
}

bool bandIntersectsBox(const BandQuad& q, const Box3& b) {
    if (!b.valid()) return false;
    Vec2 bc[4] = {{b.mn.x, b.mn.y}, {b.mx.x, b.mn.y}, {b.mx.x, b.mx.y}, {b.mn.x, b.mx.y}};
    Vec2 axes[4] = {{1, 0}, {0, 1}, q.p[1] - q.p[0], q.p[3] - q.p[0]};
    for (auto ax : axes) {
        double l = ax.len();
        if (l <= 0) continue;
        ax = ax * (1.0 / l);
        double a0 = 1e300, a1 = -1e300, b0 = 1e300, b1 = -1e300;
        for (auto& p : q.p) { double t = p.dot(ax); a0 = std::min(a0, t); a1 = std::max(a1, t); }
        for (auto& p : bc) { double t = p.dot(ax); b0 = std::min(b0, t); b1 = std::max(b1, t); }
        if (a1 < b0 || b1 < a0) return false;
    }
    return true;
}

}  // namespace asec
