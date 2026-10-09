#include "asec/sheet.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace asec {

void paperSizeMm(Paper p, bool landscape, double& w, double& h) {
    double a = p == Paper::A3 ? 420 : 297, b = p == Paper::A3 ? 297 : 210;
    w = landscape ? a : b;
    h = landscape ? b : a;
}

const char* paperName(Paper p) { return p == Paper::A3 ? "A3" : "A4"; }

SheetLayout layoutSheet(const SheetSpec& s, double lenM, double heightM) {
    SheetLayout L;
    paperSizeMm(s.paper, s.landscape, L.paperW, L.paperH);
    L.frameX = s.marginMm; L.frameY = s.marginMm;
    L.frameW = L.paperW - 2 * s.marginMm; L.frameH = L.paperH - 2 * s.marginMm;
    L.titleY = L.frameY + L.frameH - s.titleHmm;
    L.plotX = L.frameX + s.gutterLeftMm;
    L.plotY = L.frameY + s.gutterTopMm;
    L.plotW = std::max(1.0, L.frameW - s.gutterLeftMm - s.gutterRightMm);
    L.plotH = std::max(1.0, L.titleY - s.gutterBottomMm - L.plotY);
    const double k = 1000.0 / std::max(1e-9, s.denom);   // m → mm
    L.contentW = std::max(0.0, lenM) * k;
    L.contentH = std::max(0.0, heightM) * k;
    L.fitLenM = L.plotW / k;
    L.fitHeightM = L.plotH / k;
    const double tol = 1e-6;
    L.cols = std::max(1, int(std::ceil(L.contentW / L.plotW - tol)));
    L.rows = std::max(1, int(std::ceil(L.contentH / L.plotH - tol)));
    L.fits = L.cols == 1 && L.rows == 1;
    return L;
}

double fitDenominator(const SheetSpec& s, double lenM, double heightM) {
    SheetSpec t = s; t.denom = 1000;
    SheetLayout L = layoutSheet(t, 1, 1);
    double d = std::max(lenM * 1000.0 / L.plotW, heightM * 1000.0 / L.plotH);
    return std::max(1.0, std::ceil(d - 1e-9));
}

SheetAdvice adviseSheet(const SheetSpec& s, double lenM, double heightM, const std::vector<double>& stdDenoms) {
    SheetAdvice a;
    SheetLayout L = layoutSheet(s, lenM, heightM);
    a.overflow = !L.fits;
    a.fitLenM = L.fitLenM;
    a.splitCols = L.cols; a.splitRows = L.rows; a.splitSheets = L.cols * L.rows;
    if (!a.overflow) return a;
    std::vector<double> ds = stdDenoms;
    std::sort(ds.begin(), ds.end());
    for (double d : ds) {
        if (d <= s.denom) continue;
        SheetSpec t = s; t.denom = d;
        if (layoutSheet(t, lenM, heightM).fits) { a.smallerDenom = d; break; }
    }
    if (s.paper == Paper::A4) { SheetSpec t = s; t.paper = Paper::A3; a.a3Fits = layoutSheet(t, lenM, heightM).fits; }
    { SheetSpec t = s; t.landscape = !s.landscape; a.rotateFits = layoutSheet(t, lenM, heightM).fits; }
    return a;
}

double snapScaleDenom10(double denom) {
    if (!(denom > 0) || !std::isfinite(denom)) return 10;
    double n = std::round(denom / 10.0) * 10.0;
    if (n < 10) n = 10;
    return n;
}

double fitDenomStep10(const SheetSpec& s, double widthM, double heightM) {
    widthM = std::max(0.0, widthM);
    heightM = std::max(0.0, heightM);
    SheetSpec probe = s;
    probe.denom = 1000;
    SheetLayout base = layoutSheet(probe, 1, 1);
    double d = std::max(widthM * 1000.0 / base.plotW, heightM * 1000.0 / base.plotH);
    double n = std::ceil((d - 0.5) / 10.0) * 10.0;
    if (!(n >= 10) || !std::isfinite(n)) n = 10;
    for (int guard = 0; guard < 1000000; ++guard) {
        SheetSpec t = s;
        t.denom = n;
        SheetLayout L = layoutSheet(t, widthM, heightM);
        if (L.contentW <= L.plotW + 1.0 && L.contentH <= L.plotH + 1.0) return n;
        n += 10;
    }
    return n;
}

double outsideStepMeters(double mPerMm, double minMm) {
    if (!(mPerMm > 0) || !std::isfinite(mPerMm)) return 1;
    if (!(minMm > 0)) minMm = 28;
    double step = 1;
    for (int n = 0; n < 15; ++n) {
        double p = std::pow(10.0, double(n));
        for (double m : {1.0, 2.0, 5.0}) {
            double c = m * p;
            if (c / mPerMm >= minMm) return c;
            step = c;
        }
    }
    return step;
}

static bool rectContains(double ax, double ay, double aw, double ah, double bx, double by, double bw, double bh, double eps) {
    return ax <= bx + eps && ay <= by + eps && ax + aw + eps >= bx + bw && ay + ah + eps >= by + bh;
}

PlanPlace placePlan(const SheetSpec& spec, double x0, double x1, double y0, double y1, double viewZoom, double dxMm, double dyMm, int col, int row, bool split) {
    PlanPlace p;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    double zoom = (viewZoom > 1e-6 && std::isfinite(viewZoom)) ? viewZoom : 1;
    double rw = std::max(0.0, x1 - x0), rh = std::max(0.0, y1 - y0);
    SheetLayout L = layoutSheet(spec, rw, rh);
    p.plotX = L.plotX; p.plotY = L.plotY; p.plotW = L.plotW; p.plotH = L.plotH;
    p.mPerMm = std::max(1e-12, spec.denom) / 1000.0 / zoom;
    p.dxMm = dxMm; p.dyMm = dyMm;
    if (split) {
        double base = std::max(1e-12, spec.denom) / 1000.0;
        p.cX = x0 + (col + 0.5) * L.plotW * base;
        p.cY = y1 - (row + 0.5) * L.plotH * base;
    } else {
        p.cX = 0.5 * (x0 + x1);
        p.cY = 0.5 * (y0 + y1);
    }
    p.visX0 = p.cX - (p.plotW * 0.5 + dxMm) * p.mPerMm;
    p.visX1 = p.cX + (p.plotW * 0.5 - dxMm) * p.mPerMm;
    p.visYTop = p.cY + (p.plotH * 0.5 + dyMm) * p.mPerMm;
    p.visYBot = p.cY - (p.plotH * 0.5 - dyMm) * p.mPerMm;
    double rx0 = split ? p.visX0 : x0, rx1 = split ? p.visX1 : x1;
    double ry0 = split ? p.visYBot : y0, ry1 = split ? p.visYTop : y1;
    p.worldX0 = std::min(p.visX0, rx0);
    p.worldX1 = std::max(p.visX1, rx1);
    p.worldYBot = std::min(p.visYBot, ry0);
    p.worldYTop = std::max(p.visYTop, ry1);
    p.imgX = p.xMm(p.worldX0);
    p.imgY = p.yMm(p.worldYTop);
    p.imgW = std::max(0.0, p.worldX1 - p.worldX0) / p.mPerMm;
    p.imgH = std::max(0.0, p.worldYTop - p.worldYBot) / p.mPerMm;
    p.imageFillsPlot = rectContains(p.imgX, p.imgY, p.imgW, p.imgH, p.plotX, p.plotY, p.plotW, p.plotH, 0.05);
    p.rangeInsideImage = rectContains(p.imgX, p.imgY, p.imgW, p.imgH, p.xMm(x0), p.yMm(y1), rw / p.mPerMm, rh / p.mPerMm, 0.05);
    p.tickStep = outsideStepMeters(p.mPerMm);
    return p;
}

SectionPaperWindow sectionPaperWindow(double lenM, double heightM, double zTopAbs, double plotWmm, double plotHmm, double denom, double viewZoom, double dxMm, double dyMm, int col, int row, bool split) {
    SectionPaperWindow w;
    double zoom = (viewZoom > 1e-6 && std::isfinite(viewZoom)) ? viewZoom : 1;
    double base = std::max(1e-12, denom) / 1000.0;
    double baseWm = plotWmm * base, baseHm = plotHmm * base;
    double sCenter, zCenter;
    if (split) {
        sCenter = (col + 0.5) * baseWm;
        zCenter = zTopAbs - (row + 0.5) * baseHm;
    } else {
        sCenter = lenM <= baseWm ? lenM * 0.5 : baseWm * 0.5;
        zCenter = heightM <= baseHm ? zTopAbs - heightM * 0.5 : zTopAbs - baseHm * 0.5;
    }
    w.mPerMm = base / zoom;
    sCenter -= dxMm * w.mPerMm;
    zCenter += dyMm * w.mPerMm;
    w.visLen = plotWmm * w.mPerMm;
    w.visHeight = plotHmm * w.mPerMm;
    w.s0 = sCenter - w.visLen * 0.5;
    w.zTop = zCenter + w.visHeight * 0.5;
    return w;
}

OutsideTicks outsideTicks(double lo, double hi, int maxCount) {
    OutsideTicks t;
    if (!std::isfinite(lo) || !std::isfinite(hi)) return t;
    if (hi < lo) std::swap(lo, hi);
    if (maxCount < 1) maxCount = 1;
    if (hi - lo < 1e-9) { t.first = lo; t.step = 1; t.count = 1; return t; }
    if (maxCount < 2) maxCount = 2;
    const double raw = (hi - lo) / double(maxCount);
    const double p = std::pow(10.0, std::floor(std::log10(std::max(raw, 1e-12))));
    double step = p;
    for (double m : {1.0, 2.0, 5.0, 10.0}) {
        if (m * p + 1e-15 >= raw) { step = m * p; break; }
    }
    const double first = std::ceil(lo / step - 1e-9) * step;
    int n = 0;
    for (double x = first; x <= hi + step * 1e-8 && n < 10000; x += step) ++n;
    t.step = step;
    t.first = first;
    t.count = n;
    return t;
}

std::string formatAzimuth(double az) {
    az = std::fmod(az, 360.0); if (az < 0) az += 360.0;
    char b[48];
    if (az <= 180.0) std::snprintf(b, sizeof b, "N %.1f\xC2\xB0 E", az);
    else std::snprintf(b, sizeof b, "N %.1f\xC2\xB0 W", 360.0 - az);
    return b;
}

std::string facingKo(double lineAz) {
    double f = std::fmod(lineAz - 90.0, 360.0); if (f < 0) f += 360.0;
    static const char* names[8] = {"북쪽", "북동쪽", "동쪽", "남동쪽", "남쪽", "남서쪽", "서쪽", "북서쪽"};
    int k = int(std::floor((f + 22.5) / 45.0)) % 8;
    return std::string(names[k]) + "을 봄";
}

std::string sectionLetterName(int i) {
    if (i < 0) i = 0;
    std::string base(1, char('A' + i % 26));
    if (i >= 26) base += std::to_string(i / 26);
    return base + "\xE2\x80\x93" + base + "\xE2\x80\xB2";   // – , ′
}

static std::string normPath(const std::string& p) {
    std::string r = p;
    for (auto& c : r) { c = char(std::tolower(static_cast<unsigned char>(c))); if (c == '\\') c = '/'; }
    return r;
}

std::vector<std::string> pushRecent(const std::vector<std::string>& list, const std::string& item, size_t maxN) {
    std::vector<std::string> out;
    if (!item.empty()) out.push_back(item);
    const std::string key = normPath(item);
    for (auto& x : list) {
        if (out.size() >= maxN) break;
        if (x.empty() || normPath(x) == key) continue;
        out.push_back(x);
    }
    if (out.size() > maxN) out.resize(maxN);
    return out;
}

}  // namespace asec

#include "json.hpp"
namespace asec {
double resolveBackDepth(bool hasStored, double stored, bool userSet) {
    if (!hasStored || !(stored >= 0)) return kDefaultBackDepth;
    stored = std::min(stored, kMaxBackDepth);
    if (userSet) return stored;
    if (std::fabs(stored - kLegacyDefaultBackDepth) < 1e-6) return kDefaultBackDepth;   // 옛 기본값 → 새 기본값
    return stored;
}

std::string sectionsToJson(const std::vector<SavedSection>& v, const std::string& model, const std::string& srsLabel, int current) {
    nlohmann::json j;
    j["format"] = "ExcavSection.sections";
    j["version"] = 1;
    j["model"] = model;
    j["srs"] = srsLabel;
    j["current"] = current;
    j["coords"] = "world (SRSOrigin applied), metres";
    auto& a = j["sections"] = nlohmann::json::array();
    for (auto& s : v)
        a.push_back({{"name", s.name}, {"a", {s.ax, s.ay}}, {"b", {s.bx, s.by}}, {"front", s.front}, {"back", s.back}, {"backUserSet", s.backUserSet}, {"note", s.note}});
    return j.dump(2);
}

bool sectionsFromJson(const std::string& text, std::vector<SavedSection>& out, int* current, std::string* err) {
    out.clear();
    try {
        auto j = nlohmann::json::parse(text);
        if (!j.is_object() || !j.contains("sections") || !j["sections"].is_array()) { if (err) *err = "sections 배열이 없습니다"; return false; }
        for (auto& e : j["sections"]) {
            SavedSection s;
            s.name = e.value("name", std::string());
            auto A = e.at("a"), B = e.at("b");
            s.ax = A.at(0).get<double>(); s.ay = A.at(1).get<double>();
            s.bx = B.at(0).get<double>(); s.by = B.at(1).get<double>();
            s.front = e.value("front", 0.0);
            s.backUserSet = e.value("backUserSet", false);
            s.back = resolveBackDepth(e.contains("back") && e["back"].is_number(), e.value("back", kDefaultBackDepth), s.backUserSet);
            s.note = e.value("note", std::string());
            out.push_back(s);
        }
        if (current) *current = j.value("current", out.empty() ? -1 : 0);
        return true;
    } catch (const std::exception& ex) {
        if (err) *err = ex.what();
        return false;
    }
}

// ---- 단계 14: 도면 파일 이름 ----
namespace {
std::string sanitizeFilePart(const std::string& s) {
    static const std::string bad = "\\/:*?\"<>|";
    std::string o;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        // en dash(U+2013, UTF-8 E2 80 93) → '-'. ′(U+2032, E2 80 B2)은 그대로 둔다.
        if (c == 0xE2 && i + 2 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0x80 &&
            static_cast<unsigned char>(s[i + 2]) == 0x93) {
            o += '-';
            i += 3;
            continue;
        }
        o += (bad.find(static_cast<char>(c)) == std::string::npos ? static_cast<char>(c) : '_');
        ++i;
    }
    size_t a = 0, b = o.size();
    while (a < b && (o[a] == ' ' || o[a] == '.')) ++a;
    while (b > a && (o[b - 1] == ' ' || o[b - 1] == '.')) --b;
    return o.substr(a, b - a);
}
std::string denomText(double denom) {
    long long d = std::llround(denom);
    if (std::fabs(denom - double(d)) < 1e-9) return std::to_string(d);
    char b[32];
    std::snprintf(b, sizeof b, "%.6f", denom);
    std::string s = b;
    while (s.size() > 1 && s.back() == '0') s.pop_back();
    return s;
}
}  // namespace

std::string sheetFileName(const std::string& model, const std::string& sheet, double denom) {
    return sanitizeFilePart(model) + "_" + sanitizeFilePart(sheet) + "_1-" + denomText(denom) + ".svg";
}

std::string uniqueSheetFileName(const std::string& base, const std::vector<std::string>& existing) {
    auto has = [&](const std::string& n) { return std::find(existing.begin(), existing.end(), n) != existing.end(); };
    if (!has(base)) return base;
    size_t dot = base.rfind('.');
    std::string stem = dot == std::string::npos ? base : base.substr(0, dot);
    std::string ext = dot == std::string::npos ? std::string() : base.substr(dot);
    for (int k = 2;; ++k) {
        std::string c = stem + "_" + std::to_string(k) + ext;
        if (!has(c)) return c;
    }
}
}  // namespace asec
