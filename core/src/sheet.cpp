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

// ---- 위치도(평면 띠)
double planStripHalfWidth(double frontM, double backM) {
    double h = kPlanStripMinHalfM;
    if (std::isfinite(frontM)) h = std::max(h, frontM);
    if (std::isfinite(backM)) h = std::max(h, backM);
    return h;
}

namespace {
struct LineFrame { double ax, ay, ux, uy, nx, ny, L; };
LineFrame lineFrame(double ax, double ay, double bx, double by) {
    LineFrame f{ax, ay, 1, 0, 0, 1, 0};
    const double dx = bx - ax, dy = by - ay;
    f.L = std::sqrt(dx * dx + dy * dy);
    if (f.L > 0) { f.ux = dx / f.L; f.uy = dy / f.L; }
    f.nx = -f.uy; f.ny = f.ux;   // A→A′ 왼쪽 법선 = 단면의 '뒤' 쪽(facingKo 와 같은 규약)
    return f;
}
}  // namespace

PlanStripArea planStripArea(double ax, double ay, double bx, double by, double halfM, double marginM) {
    const LineFrame f = lineFrame(ax, ay, bx, by);
    PlanStripArea a;
    bool first = true;
    for (double s : {-marginM, f.L + marginM})
        for (double t : {-halfM, halfM}) {
            const double x = f.ax + f.ux * s + f.nx * t, y = f.ay + f.uy * s + f.ny * t;
            if (first) { a.x0 = a.x1 = x; a.y0 = a.y1 = y; first = false; }
            a.x0 = std::min(a.x0, x); a.x1 = std::max(a.x1, x); a.y0 = std::min(a.y0, y); a.y1 = std::max(a.y1, y);
        }
    return a;
}

PlanStripXf planStripTransform(double ax, double ay, double bx, double by, double imgX0, double imgY1, double res, double pageXAtS0, double pageYAtT0, double mmPerM) {
    // 픽셀 (i, j) → 로컬 (imgX0 + i·res, imgY1 − j·res) → s = (p−A)·u, t = (p−A)·n → x' = X0 + S·s, y' = Y0 − S·t (t 는 위로)
    const LineFrame f = lineFrame(ax, ay, bx, by);
    const double S = mmPerM;
    const double sC = (imgX0 - f.ax) * f.ux + (imgY1 - f.ay) * f.uy;
    const double tC = (imgX0 - f.ax) * f.nx + (imgY1 - f.ay) * f.ny;
    PlanStripXf T;
    T.m11 = S * res * f.ux;  T.m21 = -S * res * f.uy;  T.dx = pageXAtS0 + S * sC;
    T.m12 = -S * res * f.nx; T.m22 = S * res * f.ny;   T.dy = pageYAtT0 - S * tC;
    return T;
}

// ---- 평·단면도 한 장 조판
void ComboPlaced::toPaper(double x, double y, double &px, double &py) const {
    if (rotDeg == -90) { px = originX + y; py = originY - x; }        // rotate(-90): +x → 위, +y → 오른쪽
    else if (rotDeg == 90) { px = originX - y; py = originY + x; }    // rotate(90): +x → 아래, +y → 왼쪽
    else { px = originX + x; py = originY + y; }
}

double ComboLayout::northDeg() const {
    // 북 (0,1) 의 돌린 좌표: s = uy, t = ny → 종이 (uy, -ny)(y 아래). 위에서 시계 방향 각
    return std::atan2(uy, ny) * 180.0 / 3.14159265358979323846;
}

ComboLayout layoutCombo(const SheetSpec& spec, const std::vector<ComboSection>& secs, double planMarginM, double gapMm, double stackGapMm,
                        const ComboLayout* keepFrame) {
    ComboLayout c;
    c.L = layoutSheet(spec, 0.1, 0.1);
    c.mmPerM = 1000.0 / std::max(1e-9, spec.denom);
    if (secs.empty()) return c;
    if (keepFrame) {   // 끄는 동안: 틀을 붙든다(평면이 돌지 않음)
        c.ux = keepFrame->ux; c.uy = keepFrame->uy; c.nx = keepFrame->nx; c.ny = keepFrame->ny; c.ax = keepFrame->ax; c.ay = keepFrame->ay;
    } else {
        const LineFrame f0 = lineFrame(secs[0].ax, secs[0].ay, secs[0].bx, secs[0].by);
        c.ux = f0.ux; c.uy = f0.uy; c.nx = f0.nx; c.ny = f0.ny; c.ax = f0.ax; c.ay = f0.ay;
    }
    const double gL = spec.gutterLeftMm, gR = spec.gutterRightMm, gT = spec.gutterTopMm, gB = spec.gutterBottomMm;
    const double mg = std::max(0.0, planMarginM);
    bool first = true, firstT = true;
    for (const ComboSection& sc : secs) {
        ComboPlaced p;
        const LineFrame f = lineFrame(sc.ax, sc.ay, sc.bx, sc.by);
        p.lenM = f.L;
        p.heightM = std::max(0.0, sc.heightM);
        c.toST(sc.ax, sc.ay, p.sA, p.tA);
        c.toST(sc.bx, sc.by, p.sB, p.tB);
        for (double s : {p.sA, p.sB}) { c.sMin = first ? s : std::min(c.sMin, s); c.sMax = first ? s : std::max(c.sMax, s); first = false; }
        for (double t : {p.tA, p.tB}) { c.tMin = firstT ? t : std::min(c.tMin, t); c.tMax = firstT ? t : std::max(c.tMax, t); firstT = false; }
        // 돌린 평면에서의 방향: 바라보는 쪽(왼쪽 법선)이 위면 Top …
        const double dirS = f.ux * c.ux + f.uy * c.uy, dirT = f.ux * c.nx + f.uy * c.ny;
        const double ang = std::atan2(dirT, dirS) * 180.0 / 3.14159265358979323846;   // 0 = →, 90 = ↑
        if (std::fabs(ang) <= 45.0) { p.slot = ComboSlot::Top; p.rotDeg = 0; }
        else if (std::fabs(ang - 90.0) <= 45.0) { p.slot = ComboSlot::Left; p.rotDeg = -90; }
        else if (std::fabs(ang + 90.0) <= 45.0) { p.slot = ComboSlot::Right; p.rotDeg = 90; }
        else { p.slot = ComboSlot::Bottom; p.rotDeg = 0; }
        p.plotW = p.lenM * c.mmPerM;
        p.plotH = p.heightM * c.mmPerM;
        p.areaX = -gL; p.areaY = -gT; p.areaW = p.plotW + gL + gR; p.areaH = p.plotH + gT + gB;
        c.sections.push_back(p);
    }
    if (c.sections.size() == 1 && c.sections[0].lenM <= 0) return c;
    c.sMin -= mg; c.sMax += mg; c.tMin -= mg; c.tMax += mg;
    c.planW = (c.sMax - c.sMin) * c.mmPerM;
    c.planH = (c.tMax - c.tMin) * c.mmPerM;
    // 띠 크기
    double band[4] = {0, 0, 0, 0};
    int cnt[4] = {0, 0, 0, 0};
    double bottomW = 0;
    for (const ComboPlaced& p : c.sections) {
        const int k = int(p.slot);
        band[k] += p.areaH + (cnt[k] ? stackGapMm : gapMm);
        ++cnt[k];
        if (p.slot == ComboSlot::Bottom) bottomW = std::max(bottomW, p.areaW);
    }
    const bool horiz = cnt[0] + cnt[1] > 0, vert = cnt[2] + cnt[3] > 0;
    const double extraL = std::max(band[2], horiz ? gL : 0.0), extraR = std::max(band[3], horiz ? gR : 0.0);
    const double extraT = std::max(band[0], vert ? gR : 0.0), extraB = std::max(band[1], vert ? gL : 0.0);
    c.contentW = std::max(extraL + c.planW + extraR, bottomW);
    c.contentH = extraT + c.planH + extraB;
    c.fits = c.contentW <= c.L.plotW + 1e-6 && c.contentH <= c.L.plotH + 1e-6;
    const double cx = c.L.plotX + (c.L.plotW - c.contentW) * 0.5, cy = c.L.plotY + (c.L.plotH - c.contentH) * 0.5;
    c.planX = cx + extraL;
    c.planY = cy + extraT;
    double curTop = c.planY - gapMm, curBot = c.planY + c.planH + gapMm;
    double curLeft = c.planX - gapMm, curRight = c.planX + c.planW + gapMm;
    for (ComboPlaced& p : c.sections) {
        switch (p.slot) {
        case ComboSlot::Top:
            p.originX = c.xMm(p.sA); p.originY = curTop - p.areaH + gT;   // 단면 좌표 (0,0) = A · 높이 위
            curTop -= p.areaH + stackGapMm;
            break;
        case ComboSlot::Bottom: {
            const double mid = 0.5 * (c.xMm(p.sA) + c.xMm(p.sB));
            p.originX = mid - p.plotW * 0.5; p.originY = curBot + gT;
            curBot += p.areaH + stackGapMm;
            break;
        }
        case ComboSlot::Left:   // rotate(-90): 종이 x = originX + y(높이 아래로) → 오른쪽 끝(= 낮은 높이 + 아래 여백)이 평면에 닿음
            p.originX = curLeft - (p.plotH + gB); p.originY = c.yMm(p.tA);
            curLeft -= p.areaH + stackGapMm;
            break;
        case ComboSlot::Right:  // rotate(90): 종이 x = originX - y → 왼쪽 끝이 평면에 닿음
            p.originX = curRight + (p.plotH + gB); p.originY = c.yMm(p.tA);
            curRight += p.areaH + stackGapMm;
            break;
        }
    }
    return c;
}

double normDeg180(double deg) {
    if (!std::isfinite(deg)) return 0;
    double d = std::fmod(deg, 360.0);
    if (d <= -180.0) d += 360.0;
    if (d > 180.0) d -= 360.0;
    return d;
}

ComboLayout comboFrameForRotation(double rotDeg, double ax, double ay) {
    ComboLayout c;
    const double r = normDeg180(rotDeg) * 3.14159265358979323846 / 180.0;
    c.ux = std::cos(r); c.uy = std::sin(r);
    c.nx = -c.uy; c.ny = c.ux;
    c.ax = ax; c.ay = ay;
    return c;
}

double rotationToAlign(double px, double py, double qx, double qy, bool vertical) {
    const double dx = qx - px, dy = qy - py;
    if (std::hypot(dx, dy) < 1e-12) return 0;
    const double phi = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;   // p → q 의 방향(동 0°, 반시계)
    return normDeg180(vertical ? phi - 90.0 : phi);   // 종이 오른쪽 u 의 방향 = rotDeg, 종이 위 n = rotDeg + 90
}

AngleSnap snapAngle(double dirDeg, const std::vector<double> &refsDeg, double tolDeg) {
    AngleSnap best;
    double bestErr = std::max(0.0, tolDeg) + 1e-12;
    for (size_t i = 0; i < refsDeg.size(); ++i) {
        const double rel = normDeg180(dirDeg - refsDeg[i]);
        const double k = std::round(rel / 90.0);
        const double err = std::fabs(rel - k * 90.0);
        if (err < bestErr) {
            bestErr = err;
            best.snapped = true;
            best.angleDeg = normDeg180(refsDeg[i] + k * 90.0);
            best.source = int(i);
            best.perpendicular = (long(k) % 2) != 0;
        }
    }
    if (!best.snapped) best.angleDeg = normDeg180(dirDeg);
    return best;
}

double fitComboDenomStep10(const SheetSpec& spec, const std::vector<ComboSection>& secs, double planMarginM) {
    SheetSpec t = spec;
    for (double d = 10; d <= 2000; d += 10) {
        t.denom = d;
        if (layoutCombo(t, secs, planMarginM).fits) return d;
    }
    return 2000;
}

void freeComboPage(ComboLayout& c, double marginMm, double titleMm) {
    double x0 = c.planX, y0 = c.planY, x1 = c.planX + c.planW, y1 = c.planY + c.planH;
    for (const ComboPlaced& p : c.sections)
        for (double x : {p.areaX, p.areaX + p.areaW})
            for (double y : {p.areaY, p.areaY + p.areaH}) {
                double px, py;
                p.toPaper(x, y, px, py);
                x0 = std::min(x0, px); x1 = std::max(x1, px); y0 = std::min(y0, py); y1 = std::max(y1, py);
            }
    const double dx = marginMm - x0, dy = marginMm - y0;
    c.planX += dx; c.planY += dy;
    for (ComboPlaced& p : c.sections) { p.originX += dx; p.originY += dy; }
    const double w = x1 - x0, h = y1 - y0;
    SheetLayout& L = c.L;
    L.paperW = w + 2 * marginMm;
    L.paperH = h + 2 * marginMm + titleMm;
    L.frameX = marginMm * 0.5; L.frameY = marginMm * 0.5;
    L.frameW = L.paperW - marginMm; L.frameH = L.paperH - marginMm;
    L.plotX = marginMm; L.plotY = marginMm; L.plotW = w; L.plotH = h;
    L.titleY = marginMm + h + marginMm * 0.5;
    L.contentW = w; L.contentH = h;
    L.fits = true; L.cols = L.rows = 1;
    c.contentW = w; c.contentH = h;
    c.fits = true;
}

PlanStripArea comboPlanArea(const ComboLayout& c) {
    PlanStripArea a;
    bool first = true;
    for (double s : {c.sMin, c.sMax})
        for (double t : {c.tMin, c.tMax}) {
            const double x = c.ax + c.ux * s + c.nx * t, y = c.ay + c.uy * s + c.ny * t;
            if (first) { a.x0 = a.x1 = x; a.y0 = a.y1 = y; first = false; }
            a.x0 = std::min(a.x0, x); a.x1 = std::max(a.x1, x); a.y0 = std::min(a.y0, y); a.y1 = std::max(a.y1, y);
        }
    return a;
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

PrintPaperCheck checkPrintPaper(const SheetLayout& L, double pw, double ph, double ml, double mt, double mr, double mb, double tol) {
    PrintPaperCheck c;
    c.sizeOk = std::fabs(pw - L.paperW) <= tol && std::fabs(ph - L.paperH) <= tol;
    const double fl = L.frameX, ft = L.frameY, fr = L.paperW - L.frameX - L.frameW, fb = L.paperH - L.frameY - L.frameH;
    c.frameMarginMm = std::min(std::min(fl, ft), std::min(fr, fb));
    c.maxPrinterMarginMm = std::max(std::max(ml, mt), std::max(mr, mb));
    c.marginOk = ml <= fl + 1e-9 && mt <= ft + 1e-9 && mr <= fr + 1e-9 && mb <= fb + 1e-9;
    return c;
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

// ---- 단계 12: 위에서 본 범위 ----
TopDownRange topDownRange(double cX, double cY, double mPerMm, double plotWmm, double plotHmm) {
    TopDownRange r;
    double k = std::max(0.0, mPerMm) * 0.5;
    double hw = std::max(0.0, plotWmm) * k, hh = std::max(0.0, plotHmm) * k;
    r.x0 = cX - hw;
    r.x1 = cX + hw;
    r.yBot = cY - hh;
    r.yTop = cY + hh;
    return r;
}
}  // namespace asec

// ---- 레벨선 굵기 pt(디자인 v5 결정 L)
namespace asec {
double ptToMm(double pt) { return pt * 25.4 / 72.0; }

double clampLineWeightPt(double pt) {
    if (std::isnan(pt)) return 0.2;
    return std::min(3.0, std::max(0.05, pt));
}

LineWeights defaultLineWeights() { return LineWeights{}; }
int dxfLineWeight(double pt) {
    static const int kValid[] = {0, 5, 9, 13, 15, 18, 20, 25, 30, 35, 40, 50, 53, 60, 70, 80, 90, 100, 106, 120, 140, 158, 200, 211};
    const double hmm = ptToMm(clampLineWeightPt(pt)) * 100.0;   // 1/100 mm
    int best = kValid[0]; double bd = std::abs(hmm - kValid[0]);
    for (int v : kValid) { const double dv = std::abs(hmm - v); if (dv < bd) { bd = dv; best = v; } }
    return best;
}
}  // namespace asec
