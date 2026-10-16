#include "asec/export.hpp"
#include "asec/sheet.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include "json.hpp"

namespace asec {

Vec3 sectionToWorld(const SectionResult& r, double s, double z) {
    SectionFrame f(r.line);
    Vec2 p = f.planXY(s, 0);
    return r.srs.toWorld(Vec3(p.x, p.y, z));
}

bool checkDrawingDxfLayers(const std::string& txt, bool withLevels, std::string* report) {
    struct Lay { int lw = -99, color = 0; };
    std::map<std::string, Lay> layers;
    std::map<std::string, int> lines, polys, texts;   // 레이어별 개체 수
    std::istringstream in(txt);
    std::string c, v, kind, layName;
    auto trim = [](std::string& t) {
        while (!t.empty() && (t.back() == '\r' || t.back() == ' ')) t.pop_back();
        size_t i = 0;
        while (i < t.size() && t[i] == ' ') ++i;
        t.erase(0, i);
    };
    auto flush = [&] {
        if (kind == "LINE") ++lines[layName];
        else if (kind == "LWPOLYLINE" || kind == "POLYLINE") ++polys[layName];
        else if (kind == "TEXT" || kind == "MTEXT") ++texts[layName];
    };
    bool inLayerRec = false;
    std::string recName;
    while (std::getline(in, c) && std::getline(in, v)) {
        trim(c); trim(v);
        int code = 0;
        try { code = std::stoi(c); } catch (...) { continue; }
        if (code == 0) {
            flush();
            kind = v; layName.clear();
            inLayerRec = v == "LAYER";
            recName.clear();
            continue;
        }
        if (inLayerRec) {
            if (code == 2) { recName = v; layers.emplace(recName, Lay{}); }
            else if (code == 370 && !recName.empty()) layers[recName].lw = std::atoi(v.c_str());
            else if (code == 62 && !recName.empty()) layers[recName].color = std::atoi(v.c_str());
        } else if (code == 8) layName = v;
    }
    flush();
    const int wantCut = dxfLineWeight(kCutLinePt), wantMajor = dxfLineWeight(0.5), wantMinor = dxfLineWeight(0.2);
    auto lwOf = [&](const char* n) { auto it = layers.find(n); return it == layers.end() ? -99 : it->second.lw; };
    int old = 0;
    for (const char* n : {"SECTION_PROFILE", "LEVEL_10CM", "LEVEL_50CM", "LEVEL_1M"})
        old += int(layers.count(n)) + lines[n] + polys[n];
    const int cut = polys[kLayerCut], lv05 = lines[kLayerLevelMajor], lv02 = lines[kLayerLevelMinor];
    const int textOnLines = texts[kLayerCut] + texts[kLayerLevelMajor] + texts[kLayerLevelMinor];
    const bool ok = lwOf(kLayerCut) == wantCut && lwOf(kLayerLevelMajor) == wantMajor && lwOf(kLayerLevelMinor) == wantMinor &&
                    old == 0 && cut > 0 && textOnLines == 0 && (!withLevels || (lv05 > 0 && lv02 > 0));
    if (report) {
        char b[256];
        std::snprintf(b, sizeof b, "0.8=lw%d(%d) 0.5=lw%d(%d) 0.2=lw%d(%d) cut=%d lv05=%d lv02=%d old=%d text=%d",
                      lwOf(kLayerCut), wantCut, lwOf(kLayerLevelMajor), wantMajor, lwOf(kLayerLevelMinor), wantMinor,
                      cut, lv05, lv02, old, textOnLines);
        *report = b;
    }
    return ok;
}

bool exportSectionDxf(const SectionResult& r, const DxfExportOptions& o, const fs::path& out, std::string* err, std::string* dxfText) {
    SectionFrame f(r.line);
    const double oz = r.srs.origin.z;
    const bool w3 = o.mode == DxfCoordMode::World3D;
    const double th = o.textMm / 1000.0 * o.scaleDenom;
    const Vec3 ext(f.u.y, -f.u.x, 0);  // 관찰자 쪽 법선(−n) → OCS X축 = A→B
    auto P = [&](double s, double zAbs) -> Vec3 {
        if (!w3) return Vec3(s, zAbs, 0);
        Vec2 p = f.planXY(s, 0);
        return Vec3(p.x + r.srs.origin.x, p.y + r.srs.origin.y, zAbs);
    };
    auto T = [&](DxfWriter& d, const std::string& layer, double s, double zAbs, const std::string& txt, int ha, int va) {
        if (w3) d.textOnPlane(layer, P(s, zAbs), ext, th, txt, ha, va);
        else d.text(layer, P(s, zAbs), th, txt, ha, va);
    };
    DxfWriter d;
    d.addLayer("DRAW_SOIL", 7, 25);      // 빈 층 · 토층선 0.25 mm
    d.addLayer("DRAW_OUTLINE", 7, 35);   // 빈 층 · 유구 윤곽 0.35 mm
    // 레이어 이름 0.2 · 0.5 · 0.8(사용자 2026-10-16 「0.2 0.5 0.8로 레이어를 잡아 내보내기」, SVG 와 같음).
    // 디자인 v5 결정 L: 10 cm 0.2 pt(→ 0.09 mm) · 50 cm = 1 m 0.5 pt(→ 0.18 mm) — 1 m 선도 0.5 레이어
    d.addLayer(kLayerLevelMinor, 9, dxfLineWeight(o.levelMinorPt));
    d.addLayer(kLayerLevelMajor, 8, dxfLineWeight(o.levelMajorPt));
    d.addLayer("SECTION_IMAGE", 7, -3);
    d.addLayer(kLayerCut, 1, dxfLineWeight(kCutLinePt));   // 빨간 잘린 선 0.8 pt → 0.30 mm
    d.addLayer("LEVEL_TEXT", 7, 18);
    d.addLayer("SECTION_AXIS", 7, 18);
    d.addLayer("SECTION_TITLE", 7, 25);
    if (w3) d.addLayer("SECTION_LINE_PLAN", 1, 35);

    const double s0 = 0, s1 = f.L;
    const double zlo = r.zMin + oz, zhi = r.zMax + oz;
    // 그리기 순서 = 개체 순서: ① 레벨선(맨 아래) → ② 입면 영상 → ③ 빨간 단면선 → ④ 라벨·축
    // 숫자 라벨 50 cm 마다(사용자 2026-10-16 「0.5 m 마다 해발값은 꼭」): 좁으면 글자를 줄이고 · 왼쪽 · 오른쪽 번갈아(fitLevelLabels)
    const LevelLabelFit lf = fitLevelLabels(0.5, th * 1.8);
    const int lab = std::max(lf.labelCm, std::max(50, o.levelStepCm));
    const double lth = th * lf.scale;
    const auto levels = o.levels ? levelLines(zlo, zhi, o.levelStepCm) : std::vector<LevelLine>{};
    for (auto& lv : levels) {
        const char* layer = lv.cls == LevelClass::Minor ? kLayerLevelMinor : kLayerLevelMajor;
        if (o.levelsBelowGround) {   // 지면 위(공기)에는 없음 — 보고서 도면 관례(2026-10-16). 레벨선 z 는 로컬이므로 잘린 선(로컬 z)과 바로 비교
            for (const SGap& g : levelLineSpans(r.profile, lv.z - oz, s0, s1)) d.line(layer, P(g.s0, lv.z), P(g.s1, lv.z));
        } else d.line(layer, P(s0, lv.z), P(s1, lv.z));
    }
    if (o.image && !o.imageFile.empty() && o.imageW > 0 && o.imageH > 0) {
        if (!w3) d.image("SECTION_IMAGE", o.imageFile, o.imageW, o.imageH, Vec3(o.imageS0, o.imageZ0 + oz, 0), Vec3(o.imageRes, 0, 0), Vec3(0, o.imageRes, 0));
        else {
            Vec3 ins = P(o.imageS0, o.imageZ0 + oz);
            d.image("SECTION_IMAGE", o.imageFile, o.imageW, o.imageH, ins, Vec3(f.u.x * o.imageRes, f.u.y * o.imageRes, 0), Vec3(0, 0, o.imageRes));
        }
    }
    for (auto& pl : r.profile) {
        // 닫힌 고리(처음 = 끝)는 마지막 점을 빼고 닫힘 플래그로(DXF 에서 중복 꼭짓점 없음)
        const bool closed = pl.size() > 3 && (pl.front() - pl.back()).len() < 1e-9;
        const size_t n = closed ? pl.size() - 1 : pl.size();
        if (!w3) {
            std::vector<Vec2> v;
            for (size_t i = 0; i < n; ++i) v.emplace_back(pl[i].x, pl[i].y + oz);
            d.polyline2d(kLayerCut, v, closed);
        } else {
            std::vector<Vec3> v;
            for (size_t i = 0; i < n; ++i) v.push_back(P(pl[i].x, pl[i].y + oz));
            d.polyline3d(kLayerCut, v, closed);
        }
    }
    auto TL = [&](double s, double zAbs, const std::string& txt, int ha) {   // 표고 글자(줄인 높이 lth)
        if (w3) d.textOnPlane("LEVEL_TEXT", P(s, zAbs), ext, lth, txt, ha, 2);
        else d.text("LEVEL_TEXT", P(s, zAbs), lth, txt, ha, 2);
    };
    for (auto& lv : levels)
        if (lv.cm % lab == 0) {
            const long kth = lv.cm / lab;   // 번갈아 쓸 때: 짝수 번째는 왼쪽, 홀수 번째는 오른쪽
            if (!lf.alternate || kth % 2 == 0) TL(s0 - th * 0.6, lv.z, formatElevation(lv.z), 2);
            if (!lf.alternate || kth % 2 != 0) TL(s1 + th * 0.6, lv.z, formatElevation(lv.z), 0);
        }
    // 거리 축
    const double zb = zlo - th * 1.5;
    d.line("SECTION_AXIS", P(s0, zb), P(s1, zb));
    double st = niceStep(f.L, 10);
    for (long k = 0; k * st <= f.L + 1e-9; ++k) {
        double s = k * st;
        d.line("SECTION_AXIS", P(s, zb), P(s, zb - th * 0.5));
        char b[32]; std::snprintf(b, sizeof b, "%.*f", st < 1 ? 1 : 0, s);
        T(d, "SECTION_AXIS", s, zb - th * 0.8, b, 1, 3);
    }
    T(d, "SECTION_TITLE", s0, zhi + th * 1.2, "A", 1, 1);
    T(d, "SECTION_TITLE", s1, zhi + th * 1.2, "A'", 1, 1);
    char tb[256];
    std::snprintf(tb, sizeof tb, "%s  L=%.2fm  %s  1:%g", o.title.c_str(), f.L, r.srs.shortLabel().c_str(), o.scaleDenom);
    T(d, "SECTION_TITLE", (s0 + s1) / 2, zhi + th * 3.2, tb, 1, 1);
    if (w3) {
        Vec3 a = r.srs.toWorld(Vec3(r.line.a.x, r.line.a.y, r.zMin)), b = r.srs.toWorld(Vec3(r.line.b.x, r.line.b.y, r.zMin));
        d.line("SECTION_LINE_PLAN", a, b);
    }
    if (dxfText) *dxfText = d.str();
    if (out.empty()) return true;
    return d.save(out, err);
}

GeoRef planGeoRef(const SrsInfo& srs, double x0, double y1, double res) {
    GeoRef g;
    g.enabled = true;
    g.tieX = x0 + srs.origin.x; g.tieY = y1 + srs.origin.y;
    g.scaleX = g.scaleY = res;
    g.epsg = srs.epsg();
    g.vertEpsg = srs.verticalEpsg();
    g.citation = "Trench plan orthophoto " + srs.shortLabel();
    return g;
}

double sectionAzimuthDeg(const SectionLine& l) {
    double a = std::atan2(l.b.x - l.a.x, l.b.y - l.a.y) * 180.0 / 3.14159265358979323846;
    return a < 0 ? a + 360 : a;
}

GeoRef sectionGeoRef(const SectionResult& r, double s0, double zTop, double res) {
    GeoRef g;
    g.enabled = true;
    g.tieX = s0; g.tieY = zTop;
    g.scaleX = g.scaleY = res;
    g.epsg = 0;
    Vec3 A = r.srs.toWorld(Vec3(r.line.a.x, r.line.a.y, 0)), B = r.srs.toWorld(Vec3(r.line.b.x, r.line.b.y, 0));
    char b[512];
    std::snprintf(b, sizeof b, "Trench local section: X=distance from A (m), Y=elevation (m). %s A=(%.3f,%.3f) A'=(%.3f,%.3f) az=%.2f deg",
                  r.srs.shortLabel().c_str(), A.x, A.y, B.x, B.y, sectionAzimuthDeg(r.line));
    g.citation = b;
    return g;
}

bool writeSectionSidecar(const fs::path& p, const SectionResult& r, const GeoRef& g, double denom, double dpi, int w, int h, std::string* err) {
    using json = nlohmann::json;
    SectionFrame f(r.line);
    Vec3 A = r.srs.toWorld(Vec3(r.line.a.x, r.line.a.y, 0)), B = r.srs.toWorld(Vec3(r.line.b.x, r.line.b.y, 0));
    json j;
    j["type"] = "ExcavSection section image";
    j["srs"] = r.srs.srs;
    {
        SrsDesc sd = r.srs.describe();
        j["srs_label"] = sd.shortAscii();
        j["horizontal_epsg"] = sd.horizontalEpsg;
        j["vertical_reference"] = sd.vertKind == VertKind::Ellipsoidal ? "ellipsoidal" : sd.vertKind == VertKind::Gravity ? "gravity-related" : sd.vertKind == VertKind::LocalEnu ? "local-enu" : "unspecified";
        if (sd.verticalEpsg) j["vertical_epsg"] = sd.verticalEpsg;
        j["vertical_datum"] = vdatumKey(sd.vdatum);
        if (sd.heightDeclared) {
            j["vertical_declared_by_user"] = true;
            j["vertical_srs_original"] = sd.srsVerticalAscii;
            j["height_note"] = "Z = model values as-is (no conversion); height datum label declared by user";
        } else {
            j["height_note"] = "Z = model SRS height as-is (no geoid conversion)";
        }
    }
    j["A"] = {{"X", A.x}, {"Y", A.y}};
    j["A_prime"] = {{"X", B.x}, {"Y", B.y}};
    j["length_m"] = f.L;
    j["azimuth_deg"] = sectionAzimuthDeg(r.line);
    j["thickness_front_m"] = r.line.front;
    j["thickness_back_m"] = r.line.back;
    j["view_direction"] = "viewer looks along the left normal of A->A' (into the back band)";
    j["scale"] = "1:" + std::to_string(int(std::lround(denom)));
    j["dpi"] = dpi;
    j["pixel_size_m"] = g.scaleX;
    j["image_px"] = {w, h};
    j["pixel_to_section"] = {{"distance_m", "s = " + dxfNum(g.tieX) + " + (col + 0.5) * " + dxfNum(g.scaleX)},
                             {"elevation_m", "z = " + dxfNum(g.tieY) + " - (row + 0.5) * " + dxfNum(g.scaleY)}};
    j["section_to_world"] = "X = A.X + s*sin(az), Y = A.Y + s*cos(az), Z = z";
    std::ofstream o(p);
    if (!o) { if (err) *err = "쓸 수 없음: " + p.u8string(); return false; }
    o << j.dump(2);
    return bool(o);
}

}  // namespace asec
