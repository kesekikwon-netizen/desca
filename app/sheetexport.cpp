// 도면(용지) 내보내기: 축척·용지 미리보기, 넘침 경고·추천, 표제란(높이 기준 포함), PDF/DXF/PNG/TIFF 한 창(Ctrl+P).
// QtPrintSupport 없이 QPdfWriter(QtGui)로 PDF. 인쇄는 PDF 를 열어서.
#include "mainwindow.hpp"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>
#include <QSvgGenerator>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <cstring>
#include "asec/dxf.hpp"
#include "guideband.hpp"
#include "icons.hpp"
#include <QDateTime>
#include <QUrl>
#include <QDesktopServices>
#include <QFileDialog>
#include <QScrollArea>
#include "asec/raster.hpp"

using namespace asec;
namespace fs = std::filesystem;
extern const char* const kVersion;

namespace {
QString qs8(const std::string& s) { return QString::fromUtf8(s.c_str()); }
fs::path toFsS(const QString& q) {
#ifdef _WIN32
    return fs::path(q.toStdWString());
#else
    return fs::u8path(q.toStdString());
#endif
}
QImage toQImageS(const RgbaImage& im) {
    if (im.empty()) return {};
    return QImage(im.px.data(), im.w, im.h, im.w * 4, QImage::Format_RGBA8888).copy();
}
RgbaImage fromQImageS(const QImage& q0) {
    QImage q = q0.convertToFormat(QImage::Format_RGBA8888);
    RgbaImage r; r.w = q.width(); r.h = q.height(); r.px.resize(size_t(r.w) * r.h * 4);
    for (int y = 0; y < r.h; ++y) std::memcpy(&r.px[size_t(y) * r.w * 4], q.constScanLine(y), size_t(r.w) * 4);
    return r;
}

/// 단면 → 용지 배치(높이는 위아래 4% 여유)
struct SheetGeom { SheetLayout L; double lenM = 0, heightM = 0, zTopAbs = 0; };
SheetGeom sheetGeom(const SectionDoc& doc, const SheetSpec& spec) {
    SheetGeom g;
    const double oz = doc.r.srs.origin.z;
    double zr = std::max(0.2, doc.r.zMax - doc.r.zMin);
    double pad = std::max(0.05, zr * 0.04);
    g.lenM = SectionFrame(doc.r.line).L;
    g.heightM = zr + 2 * pad;
    g.zTopAbs = doc.r.zMax + oz + pad;
    g.L = layoutSheet(spec, g.lenM, g.heightM);
    return g;
}

double niceLen(double target) {
    double p = std::pow(10.0, std::floor(std::log10(std::max(1e-9, target))));
    for (double m : {1.0, 2.0, 5.0, 10.0}) if (m * p >= target * 0.999) return m * p;
    return 10 * p;
}

QString paperLabel(const SheetSpec& s) { return QStringLiteral("%1 %2").arg(QString::fromLatin1(paperName(s.paper)), s.landscape ? QStringLiteral("가로") : QStringLiteral("세로")); }
QString denomText(double d) { return QStringLiteral("1:%1").arg(d, 0, 'f', 0); }
const char* kFormatExt[4] = {"pdf", "dxf", "png", "tif"};
const char* kFormatName[4] = {"PDF", "DXF", "PNG", "TIFF"};

/// 평면·단면 조판이 같이 쓰는 바깥 자리(mm, 종이 왼쪽 위가 원점, y 아래).
struct SheetMarks {
    double compassX = 0, compassY = 0, compassR = 3.2;
    double legendX = 0, legendY = 0, legendW = 40, legendH = 18;
    double titleX = 0, titleY = 0, titleW = 120, titleH = 21;
    double scaleX = 0, scaleY = 0;
};
SheetMarks sheetMarks(const SheetLayout& L) {
    SheetMarks m;
    const double bandTop = L.titleY;
    const double frameBottom = L.frameY + L.frameH;
    const double band = frameBottom - bandTop;
    m.compassR = 3.2;
    m.compassX = L.frameX + L.frameW - 8;
    m.compassY = L.frameY + 5;
    m.titleW = std::min(120.0, L.frameW * 0.45);
    m.titleH = std::max(8.0, band - 3);
    m.titleX = L.frameX + L.frameW - m.titleW;
    m.titleY = frameBottom - m.titleH;
    m.legendX = L.frameX + 58;
    m.legendY = bandTop + 2;
    m.legendW = std::max(24.0, m.titleX - 4 - m.legendX);
    m.legendH = std::max(8.0, band - 4);
    m.scaleX = L.frameX + 6;
    m.scaleY = frameBottom - 9;
    return m;
}

QString coordText(double v, double step) {
    int dec = step >= 1 ? 0 : step >= 0.1 ? 1 : 2;
    return QString::number(v, 'f', dec);
}

struct SheetChrome {
    QString title, srs, height, date, facing, kind, paper, pageTag;
    double denom = 40;
    bool showTitle = true;
};

void paintCompass(QPainter& p, double cx, double cy, double r, double k) {
    QPointF c(cx * k, cy * k);
    const double R = r * k;
    p.setPen(QPen(theme::Ink, std::max(0.6, 0.25 * k)));
    p.setBrush(Qt::white);
    p.drawEllipse(c, R, R);
    QPolygonF arrow;
    arrow << QPointF(c.x(), c.y() - R * 0.78) << QPointF(c.x() - R * 0.28, c.y() + R * 0.15) << QPointF(c.x(), c.y() - R * 0.05)
          << QPointF(c.x() + R * 0.28, c.y() + R * 0.15);
    p.setBrush(theme::Ink);
    p.drawPolygon(arrow);
    p.setPen(theme::Ink);
    QFont f = theme::uiFont(std::max(6, int(std::lround(2.2 * k))), true);
    p.setFont(f);
    p.drawText(QRectF(c.x() - R, c.y() - R - 3.2 * k, R * 2, 3.0 * k), Qt::AlignHCenter | Qt::AlignBottom, QStringLiteral("진북"));
}

void paintSheetChrome(QPainter& p, const SheetLayout& L, double k, const SheetChrome& c) {
    const SheetMarks m = sheetMarks(L);
    const QRectF frame(L.frameX * k, L.frameY * k, L.frameW * k, L.frameH * k);
    p.setPen(QPen(theme::Ink, 0.35 * k));
    p.setBrush(Qt::NoBrush);
    p.drawRect(frame);
    paintCompass(p, m.compassX, m.compassY, m.compassR, k);
    const double ui = k * 25.4 / 96.0;
    QFont fName = theme::uiFont(std::max(6, int(std::lround(15 * ui))), true);
    QFont fSmall = theme::uiFont(std::max(5, int(std::lround(10.5 * ui))));
    QFont fMono = theme::monoFont(std::max(5, int(std::lround(10 * ui))));
    QRectF lg(m.legendX * k, m.legendY * k, m.legendW * k, m.legendH * k);
    p.setPen(QPen(theme::Ink, 0.25 * k));
    p.setBrush(Qt::white);
    p.drawRect(lg);
    p.setFont(fSmall);
    p.setPen(theme::Ink);
    const double row = lg.height() / 4.0;
    p.drawText(QRectF(lg.left() + 1.5 * k, lg.top(), lg.width() - 3 * k, row), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("범례  ·  %1").arg(c.kind));
    p.drawText(QRectF(lg.left() + 1.5 * k, lg.top() + row, lg.width() - 3 * k, row), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("축척  %1").arg(denomText(c.denom)));
    p.setFont(fMono);
    p.drawText(QRectF(lg.left() + 1.5 * k, lg.top() + row * 2, lg.width() - 3 * k, row), Qt::AlignLeft | Qt::AlignVCenter, c.srs);
    p.drawText(QRectF(lg.left() + 1.5 * k, lg.top() + row * 3, lg.width() - 3 * k, row), Qt::AlignLeft | Qt::AlignVCenter, c.height);
    if (!c.showTitle) return;
    QRectF tb(m.titleX * k, m.titleY * k, m.titleW * k, m.titleH * k);
    p.setPen(QPen(theme::Ink, 0.25 * k));
    p.setBrush(Qt::white);
    p.drawRect(tb);
    const double rowH = tb.height() / 3.0;
    p.drawLine(QPointF(tb.left(), tb.top() + rowH * 1.3), QPointF(tb.right(), tb.top() + rowH * 1.3));
    p.setFont(fName);
    p.setPen(theme::Ink);
    p.drawText(tb.adjusted(2 * k, 0, -2 * k, -(tb.height() - rowH * 1.3)), Qt::AlignLeft | Qt::AlignVCenter, c.title);
    if (!c.pageTag.isEmpty()) {
        p.setFont(fMono);
        p.drawText(tb.adjusted(2 * k, 0, -2 * k, -(tb.height() - rowH * 1.3)), Qt::AlignRight | Qt::AlignVCenter, c.pageTag);
        p.setFont(fName);
    }
    p.setFont(fSmall);
    p.setPen(theme::Ink2);
    QString l1 = QStringLiteral("축척 %1 · %2 · %3 · %4").arg(denomText(c.denom), c.paper, c.facing, c.date);
    QString l2 = QStringLiteral("%1 · %2").arg(c.srs, c.height);
    p.drawText(QRectF(tb.left() + 2 * k, tb.top() + rowH * 1.3, tb.width() - 4 * k, rowH * 0.85), Qt::AlignLeft | Qt::AlignVCenter, l1);
    p.drawText(QRectF(tb.left() + 2 * k, tb.top() + rowH * 2.15, tb.width() - 4 * k, rowH * 0.85), Qt::AlignLeft | Qt::AlignVCenter, l2);
    // 막대는 범례 상자 왼쪽 공간(≈ 48 mm)에 들어가는 가장 긴 1·2·5×10ⁿ m
    double lenM = 0;
    {
        const double maxM = 48.0 * c.denom / 1000.0, pw = std::pow(10.0, std::floor(std::log10(std::max(1e-9, maxM))));
        for (double mlt : {1.0, 2.0, 5.0, 10.0}) if (mlt * pw <= maxM * 1.0001) lenM = mlt * pw;
        if (lenM <= 0) lenM = pw;
    }
    double barMm = lenM * 1000.0 / c.denom;
    QRectF sb(m.scaleX * k, m.scaleY * k, barMm * k, 1.6 * k);
    p.setPen(QPen(theme::Ink, 0.2 * k));
    p.setBrush(Qt::white);
    p.drawRect(sb);
    p.setBrush(theme::Ink);
    for (int i = 0; i < 4; i += 2) p.drawRect(QRectF(sb.x() + sb.width() * i / 4, sb.y(), sb.width() / 4, sb.height()));
    p.setFont(fMono);
    p.setPen(theme::Ink);
    auto ml = [](double meters) { return meters >= 1 ? QString::number(meters, 'g', 4) + " m" : QString::number(meters * 100, 'g', 4) + " cm"; };
    p.drawText(QRectF(sb.left() - 5 * k, sb.bottom() + 0.6 * k, 10 * k, 3.2 * k), Qt::AlignHCenter | Qt::AlignTop, "0");
    p.drawText(QRectF(sb.right() - 10 * k, sb.bottom() + 0.6 * k, 20 * k, 3.2 * k), Qt::AlignHCenter | Qt::AlignTop, ml(lenM));
    p.setFont(fSmall);
    p.setPen(theme::Muted);
    p.drawText(QRectF(sb.left(), sb.top() - 3.6 * k, 70 * k, 3.4 * k), Qt::AlignLeft | Qt::AlignBottom, denomText(c.denom));
}

bool renderPlanOrtho(MeshSource& src, const Box3& areaIn, double res, QImage& out, double& x0, double& y1, QString* msg, const std::atomic<bool>* cancel) {
    Box3 area = areaIn;
    Box3 b = src.bounds;
    area.mn.x = std::max(area.mn.x, b.mn.x); area.mn.y = std::max(area.mn.y, b.mn.y);
    area.mx.x = std::min(area.mx.x, b.mx.x); area.mx.y = std::min(area.mx.y, b.mx.y);
    if (!area.valid() || area.mx.x <= area.mn.x || area.mx.y <= area.mn.y) { if (msg) *msg = QStringLiteral("내보낼 범위가 비어 있습니다"); return false; }
    if (!(res > 0)) { if (msg) *msg = QStringLiteral("해상도가 없습니다"); return false; }
    x0 = std::floor(area.mn.x / res) * res;
    y1 = std::ceil(area.mx.y / res) * res;
    int W = int(std::ceil((area.mx.x - x0) / res)), H = int(std::ceil((y1 - area.mn.y) / res));
    if (W < 1 || H < 1) { if (msg) *msg = QStringLiteral("내보낼 범위가 비어 있습니다"); return false; }
    if (double(W) * H > 200e6) {
        if (msg) *msg = QStringLiteral("출력 영상이 너무 큽니다(%1 × %2 px). 축척 분모를 키우거나 범위를 줄이세요.").arg(W).arg(H);
        return false;
    }
    std::vector<MeshPtr> meshes;
    std::string e;
    if (auto* t = dynamic_cast<TmxSource*>(&src)) {
        LeafStats st;
        if (!t->areaMeshes(area, res, meshes, &st, &e, cancel)) { if (msg) *msg = qs8(e); return false; }
    } else if (auto* s = dynamic_cast<StaticSource*>(&src)) {
        for (auto& m : s->meshes)
            if (!m->bbox.valid() || !(m->bbox.mx.x < area.mn.x || m->bbox.mn.x > area.mx.x || m->bbox.mx.y < area.mn.y || m->bbox.mn.y > area.mx.y)) meshes.push_back(m);
    }
    RgbaImage img;
    if (!renderPlan(meshes, x0, y1, res, W, H, img, cancel)) { if (msg) *msg = QStringLiteral("평면 영상 생성 실패(취소 또는 메모리)"); return false; }
    out = toQImageS(img);
    return true;
}
/// 조판 미리보기 영상: 지금 보이는 범위(주변 30 % 포함)만, 가장 촘촘하게(긴 변 6000 px, 최소 1 mm/px)
bool renderPlanPreview(MeshSource& src, const SheetSpec& spec, const Box3& area, double zoom, double dxMm, double dyMm,
                       QImage& out, double& x0, double& y1, double& res) {
    PlanPlace v = placePlan(spec, area.mn.x, area.mx.x, area.mn.y, area.mx.y, zoom, dxMm, dyMm);
    const double mx = (v.visX1 - v.visX0) * 0.3, my = (v.visYTop - v.visYBot) * 0.3;
    Box3 box;
    box.mn.x = v.visX0 - mx; box.mx.x = v.visX1 + mx; box.mn.y = v.visYBot - my; box.mx.y = v.visYTop + my;
    box.mn.z = src.bounds.mn.z; box.mx.z = src.bounds.mx.z;
    const double w = std::max(0.0, box.mx.x - box.mn.x), h = std::max(0.0, box.mx.y - box.mn.y);
    res = std::max({1e-4, 0.001, std::max(w, h) / 6000.0});
    QString em;
    return renderPlanOrtho(src, box, res, out, x0, y1, &em, nullptr);
}
}  // namespace

void MainWindow::paintSheet(QPainter& p, const SectionDoc& doc0, const SheetParams& sp, double pxPerMm, int page, const QImage& img0, const SectionImgGeo& geo0,
                            double viewZoom, double viewPanX, double viewPanY, SheetPaintProbe* probe) {
    const SheetGeom G = sheetGeom(doc0, sp.spec);
    const SheetLayout& L = G.L;
    const double k = pxPerMm;                         // mm → 장치 픽셀
    const double ui = pxPerMm * 25.4 / 96.0;          // 96 dpi 기준 글자·선 배율 → 실제 mm. 휠과 무관
    (void)viewPanX; (void)viewPanY;                   // 왼쪽 끌기는 종이 전체를 옮긴다. 그림 좌표는 imgDx/imgDy
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.fillRect(QRectF(0, 0, L.paperW * k, L.paperH * k), Qt::white);
    const int cols = sp.split ? L.cols : 1;
    const int col = sp.split ? page % cols : 0, row = sp.split ? page / cols : 0;
    const SectionPaperWindow W = sectionPaperWindow(G.lenM, G.heightM, G.zTopAbs, L.plotW, L.plotH, sp.spec.denom, viewZoom, sp.imgDxMm, sp.imgDyMm, col, row, sp.split);
    SectionXf xf;
    xf.ppm = k / W.mPerMm;
    xf.plot = QRectF(L.plotX * k, L.plotY * k, L.plotW * k, L.plotH * k);
    xf.s0 = W.s0;
    xf.zTop = W.zTop;
    // 문서 스타일: 넣을 것 선택 그대로(그리는 순서 레벨선 → 영상 → 단면선은 paintSectionDoc 이 지킴)
    SectionDoc doc = doc0;
    doc.st.showImage = sp.withImage; doc.st.showLine = sp.withLine; doc.st.showLevels = sp.withLevels;
    doc.st.imageOpacity = 1.0; doc.st.showBaseline = sp.showBaseline; doc.st.baselineEl = sp.baselineEl;
    doc.st.plotScaleBar = !sp.withTitle;
    doc.st.levelMinorPt = clampLineWeightPt(sp.levelMinorPt); doc.st.levelMajorPt = clampLineWeightPt(sp.levelMajorPt);
    // 영상: 이 쪽에 보이는 거리 범위만 잘라서(PDF 에 쪽마다 전체 영상이 들어가지 않게)
    QImage img = img0; SectionImgGeo geo = geo0;
    if (!img.isNull() && geo.res > 0) {
        double sa = xf.s0, sb = xf.s0 + W.visLen;
        int x0 = std::max(0, int(std::floor((sa - geo.s0) / geo.res)) - 2), x1 = std::min(img.width(), int(std::ceil((sb - geo.s0) / geo.res)) + 2);
        if (x1 > x0 && (x0 > 0 || x1 < img.width())) { img = img.copy(x0, 0, x1 - x0, img.height()); geo.s0 += x0 * geo.res; }
        else if (x1 <= x0) img = QImage();
    }
    const QRectF frame(L.frameX * k, L.frameY * k, L.frameW * k, L.frameH * k);
    const QRectF area(frame.left(), frame.top(), frame.width(), (L.titleY - L.frameY) * k);
    paintSectionDoc(p, doc, area, xf, ui, img, true, QString(), &geo, false, false, 0, 0, 1, 0, 0, probe);
    SheetChrome ch;
    ch.title = sp.title; ch.srs = sp.srsLabel; ch.height = sp.heightLabel; ch.date = sp.date; ch.facing = sp.facing;
    ch.kind = QStringLiteral("단면도"); ch.paper = paperLabel(sp.spec); ch.denom = sp.spec.denom; ch.showTitle = sp.withTitle;
    if (sp.split && G.L.cols * G.L.rows > 1) ch.pageTag = QStringLiteral("%1 / %2").arg(page + 1).arg(G.L.cols * G.L.rows);
    paintSheetChrome(p, L, k, ch);
    if (probe) {
        SheetMarks m = sheetMarks(L);
        probe->compass = QRectF((m.compassX - m.compassR) * k, (m.compassY - m.compassR) * k, m.compassR * 2 * k, m.compassR * 2 * k);
        probe->legend = QRectF(m.legendX * k, m.legendY * k, m.legendW * k, m.legendH * k);
        probe->plot = xf.plot;
    }
    p.restore();
}

SheetParams MainWindow::defaultSheetParams() const {
    SheetParams sp;
    QSettings st;
    sp.spec.paper = st.value("sheet/paper", 0).toInt() == 1 ? Paper::A3 : Paper::A4;
    sp.spec.landscape = st.value("sheet/landscape", true).toBool();
    sp.spec.denom = st.value("sheet/denom", 40.0).toDouble();
    sp.format = std::clamp(st.value("sheet/format", 4).toInt(), 0, 4);   // 기본 SVG(4) — 일러스트레이터로(스펙 §8)
    sp.dpi = std::max(600.0, st.value("sheet/dpi", 600.0).toDouble());
    sp.withImage = st.value("sheet/withImage", true).toBool();
    sp.withLine = st.value("sheet/withLine", true).toBool();
    sp.withLevels = st.value("sheet/withLevels", true).toBool();
    sp.withTitle = st.value("sheet/withTitle", true).toBool();
    sp.levelMinorPt = clampLineWeightPt(st.value("sheet/levelMinorPt", 0.2).toDouble());
    sp.levelMajorPt = clampLineWeightPt(st.value("sheet/levelMajorPt", 0.5).toDouble());
    sp.hatchKind = std::clamp(st.value("sheet/hatchKind", 0).toInt(), 0, 2); sp.hatchMm = std::clamp(st.value("sheet/hatchMm", 1.0).toDouble(), 0.2, 5.0);
    sp.title = QStringLiteral("%1 단면도").arg(sectionName());
    if (current_ >= 0 && current_ < int(sections_.size()) && !sections_[size_t(current_)].note.empty())
        sp.title += QStringLiteral(" · ") + qs8(sections_[size_t(current_)].note);
    const SrsDesc& d = srsReport_.desc;
    sp.srsLabel = d.horizontalEpsg ? QStringLiteral("수평 EPSG:%1").arg(d.horizontalEpsg) : (d.known() ? qs8(d.shortAscii()) : QStringLiteral("좌표계 없음"));
    const VDatumInfo& vi = vdatumInfo(d.vdatum);
    int ve = d.verticalEpsg ? d.verticalEpsg : vi.epsg;
    const bool named = d.vdatum == VDatum::EGM96 || d.vdatum == VDatum::EGM2008 || d.vdatum == VDatum::KVD1964 || d.vdatum == VDatum::KNGeoid;
    QString vn = named && vi.shortName && *vi.shortName ? QString::fromUtf8(vi.shortName) : qs8(d.verticalKo());
    sp.heightLabel = QStringLiteral("높이 %1%2%3").arg(vn, ve ? QStringLiteral(" (EPSG:%1)").arg(ve) : QString(), d.heightDeclared ? QStringLiteral(" · 지정") : QString());
    if (section_->hasResult()) {
        sp.facing = qs8(facingKo(sectionAzimuthDeg(section_->doc().r.line)));
        SectionDoc doc = section_->doc();
        sp.baselineEl = std::round((0.5 * (doc.r.zMin + doc.r.zMax) + doc.r.srs.origin.z) * 2.0) / 2.0;   // 가운데쯤 0.5 m 단위
        const SheetGeom G0 = sheetGeom(doc, sp.spec);   // 열 때는 단면이 그림 칸에 다 들어가는 가장 작은 10 단위(맞춤)
        if (G0.lenM > 0 && G0.heightM > 0) sp.spec.denom = fitDenomStep10(sp.spec, G0.lenM, G0.heightM);
    }
    sp.date = QDate::currentDate().toString("yyyy-MM-dd");
    return sp;
}

namespace {
struct PlanWindow {
    SheetLayout L;
    PlanPlace place;
};
PlanWindow planWindow(const PlanSheetParams& sp, int page, double viewZoom = 1) {
    PlanWindow o;
    double w = std::max(0.0, sp.area.mx.x - sp.area.mn.x), h = std::max(0.0, sp.area.mx.y - sp.area.mn.y);
    o.L = layoutSheet(sp.spec, w, h);
    const int cols = sp.split ? std::max(1, o.L.cols) : 1;
    const int col = sp.split ? page % cols : 0;
    const int row = sp.split ? page / cols : 0;
    o.place = placePlan(sp.spec, sp.area.mn.x, sp.area.mx.x, sp.area.mn.y, sp.area.mx.y, viewZoom, sp.imgDxMm, sp.imgDyMm, col, row, sp.split);
    return o;
}
QString groupedCoord(double v, double step) {
    QLocale loc(QLocale::English);
    if (step >= 1) return loc.toString(qint64(std::llround(v)));
    return loc.toString(v, 'f', 1);
}
void noteProbeTransform(QPainter& p, SheetPaintProbe* probe) {
    if (!probe) return;
    const QTransform tr = p.worldTransform();
    probe->uniform = probe->uniform && std::abs(tr.m11() - tr.m22()) < 1e-4 && std::abs(tr.m12()) < 1e-4 && std::abs(tr.m21()) < 1e-4;
}
}  // namespace

void MainWindow::paintPlanSheet(QPainter& p, const PlanSheetParams& sp, double pxPerMm, int page, const QImage& img,
                                double imgX0, double imgY1, double imgRes, const Vec3& origin, double viewZoom, double viewPanX, double viewPanY,
                                SheetPaintProbe* probe) {
    (void)viewPanX; (void)viewPanY;
    const PlanWindow W = planWindow(sp, page, viewZoom);
    const SheetLayout& L = W.L;
    const PlanPlace& pl = W.place;
    const double k = pxPerMm;
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    noteProbeTransform(p, probe);
    p.fillRect(QRectF(0, 0, L.paperW * k, L.paperH * k), Qt::white);
    const QRectF plot(L.plotX * k, L.plotY * k, L.plotW * k, L.plotH * k);
    p.save();
    p.setClipRect(plot);
    p.fillRect(plot, QColor(0xF7, 0xF5, 0xEE));
    if (!img.isNull() && imgRes > 0) {
        const double worldW = img.width() * imgRes, worldH = img.height() * imgRes;
        const QRectF dest(pl.xMm(imgX0) * k, pl.yMm(imgY1) * k, worldW / pl.mPerMm * k, worldH / pl.mPerMm * k);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.drawImage(dest, img);
        if (probe) probe->image = dest;
    }
    p.restore();
    p.setPen(QPen(theme::Ink, std::max(0.6, 0.35 * k)));
    p.setBrush(Qt::NoBrush);
    p.drawRect(plot);
    QFont fMono = theme::monoFont(std::max(6, int(std::lround(2.6 * k))));
    QFont fCap = theme::uiFont(std::max(6, int(std::lround(2.3 * k))));
    if (probe) {
        probe->plot = plot;
        probe->fontPx = fMono.pixelSize();
        probe->worldStep = pl.tickStep;
        probe->frame = QRectF(L.frameX * k, L.frameY * k, L.frameW * k, L.frameH * k);
    }
    const QRectF gutterL(L.frameX * k, plot.top(), std::max(0.0, plot.left() - L.frameX * k), plot.height());
    const QRectF gutterR(plot.right(), plot.top(), std::max(0.0, (L.frameX + L.frameW) * k - plot.right()), plot.height());
    const QRectF gutterB(plot.left(), plot.bottom(), plot.width(), std::max(0.0, L.titleY * k - plot.bottom()));
    auto markLabel = [&](const QRectF& r, const QString& t) {
        if (!probe) return;
        probe->labels.push_back(r);
        probe->labelText << t;
    };
    p.setFont(fMono);
    p.setPen(theme::Ink2);
    QVector<double> xs;
    p.save();
    p.setClipRect(gutterB);
    p.fillRect(gutterB, Qt::white);
    for (double X = std::ceil(pl.visX0 / pl.tickStep - 1e-9) * pl.tickStep; X <= pl.visX1 + pl.tickStep * 1e-8; X += pl.tickStep) {
        double x = pl.xMm(X) * k;
        if (x < plot.left() - 0.2 || x > plot.right() + 0.2) continue;
        p.setPen(QPen(theme::Ink, std::max(0.4, 0.2 * k)));
        p.drawLine(QPointF(x, plot.bottom()), QPointF(x, plot.bottom() + 1.6 * k));
        QString t = groupedCoord(X + origin.x, pl.tickStep);
        QRectF tr(x - 14 * k, plot.bottom() + 2.2 * k, 28 * k, 2.8 * k);
        p.setPen(theme::Ink2);
        p.setFont(fMono);
        p.drawText(tr, Qt::AlignHCenter | Qt::AlignTop, t);
        markLabel(tr, t);
        xs.push_back(x);
    }
    p.setFont(fCap);
    p.setPen(theme::Muted);
    p.drawText(QRectF(plot.right() - 70 * k, plot.bottom() + 6.2 * k, 70 * k, 2.6 * k), Qt::AlignRight | Qt::AlignTop,
               QStringLiteral("X (동) · 1 칸 %1 m").arg(pl.tickStep, 0, 'f', 0));
    p.restore();
    auto drawY = [&](const QRectF& gutter, bool left) {
        p.save();
        p.setClipRect(gutter);
        p.fillRect(gutter, Qt::white);
        p.setFont(fMono);
        for (double Y = std::ceil(pl.visYBot / pl.tickStep - 1e-9) * pl.tickStep; Y <= pl.visYTop + pl.tickStep * 1e-8; Y += pl.tickStep) {
            double y = pl.yMm(Y) * k;
            if (y < plot.top() - 0.2 || y > plot.bottom() + 0.2) continue;
            p.setPen(QPen(theme::Ink, std::max(0.4, 0.2 * k)));
            if (left) p.drawLine(QPointF(plot.left() - 1.6 * k, y), QPointF(plot.left(), y));
            else p.drawLine(QPointF(plot.right(), y), QPointF(plot.right() + 1.6 * k, y));
            QString t = groupedCoord(Y + origin.y, pl.tickStep);
            QRectF tr = left ? QRectF(gutter.left(), y - 1.4 * k, std::max(0.0, gutter.width() - 0.8 * k), 2.8 * k)
                             : QRectF(gutter.left() + 0.8 * k, y - 1.4 * k, std::max(0.0, gutter.width() - 0.8 * k), 2.8 * k);
            p.setPen(theme::Ink2);
            p.setFont(fMono);
            p.drawText(tr, (left ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter, t);
            markLabel(tr, t);
        }
        p.restore();
    };
    drawY(gutterL, true);
    drawY(gutterR, false);
    p.save();
    p.setClipRect(QRectF(L.frameX * k, L.frameY * k, std::max(0.0, plot.left() - L.frameX * k), std::max(0.0, plot.top() - L.frameY * k)));
    p.setFont(fCap);
    p.setPen(theme::Muted);
    p.drawText(QRectF(L.frameX * k, plot.top() - 4.2 * k, std::max(0.0, plot.left() - L.frameX * k), 3.2 * k), Qt::AlignRight | Qt::AlignBottom, QStringLiteral("Y (북)"));
    p.restore();
    if (probe && xs.size() >= 2) probe->neighborPx = xs[1] - xs[0];
    p.setPen(QPen(theme::Ink, std::max(0.6, 0.35 * k)));
    p.setBrush(Qt::NoBrush);
    p.drawRect(plot);
    SheetChrome ch;
    ch.title = sp.title; ch.srs = sp.srsLabel; ch.height = sp.heightLabel; ch.date = sp.date;
    ch.facing = QStringLiteral("북이 위"); ch.kind = QStringLiteral("평면도"); ch.paper = paperLabel(sp.spec);
    ch.denom = sp.spec.denom; ch.showTitle = true;
    if (sp.split && L.cols * L.rows > 1) ch.pageTag = QStringLiteral("%1 / %2").arg(page + 1).arg(L.cols * L.rows);
    paintSheetChrome(p, L, k, ch);
    if (probe) {
        SheetMarks m = sheetMarks(L);
        probe->compass = QRectF((m.compassX - m.compassR) * k, (m.compassY - m.compassR) * k, m.compassR * 2 * k, m.compassR * 2 * k);
        probe->legend = QRectF(m.legendX * k, m.legendY * k, m.legendW * k, m.legendH * k);
        noteProbeTransform(p, probe);
    }
    p.restore();
}

namespace {
bool writePlanSheetDxf(const PlanSheetParams& sp, const QImage& img, double imgX0, double imgY1, double imgRes, const Vec3& origin, const QString& path, QString* msg) {
    const PlanWindow W = planWindow(sp, 0);
    const SheetLayout& L = W.L;
    QString png = QFileInfo(path).completeBaseName() + "_image.png";
    QString pngPath = QFileInfo(path).absolutePath() + "/" + png;
    const PlanPlace& pl = W.place;
    QImage crop = img;
    DxfWriter d;
    d.addLayer("DRAW_SOIL", 7, 25);
    d.addLayer("DRAW_OUTLINE", 7, 35);
    d.addLayer("FRAME", 7);
    d.addLayer("COMPASS", 7);
    d.addLayer("COORD", 7);
    d.addLayer("LEGEND", 7);
    d.addLayer("IMAGE", 7);
    auto P = [&](double xMm, double yTopMm) { return Vec3(xMm, L.paperH - yTopMm, 0); };
    d.polyline2d("FRAME", {Vec2(L.frameX, L.paperH - L.frameY), Vec2(L.frameX + L.frameW, L.paperH - L.frameY), Vec2(L.frameX + L.frameW, L.paperH - (L.frameY + L.frameH)), Vec2(L.frameX, L.paperH - (L.frameY + L.frameH))}, true);
    SheetMarks m = sheetMarks(L);
    Vec3 c = P(m.compassX, m.compassY);
    d.line("COMPASS", Vec3(c.x, c.y - m.compassR, 0), Vec3(c.x, c.y + m.compassR, 0));
    d.line("COMPASS", Vec3(c.x, c.y + m.compassR, 0), Vec3(c.x - m.compassR * 0.35, c.y + m.compassR * 0.2, 0));
    d.line("COMPASS", Vec3(c.x, c.y + m.compassR, 0), Vec3(c.x + m.compassR * 0.35, c.y + m.compassR * 0.2, 0));
    d.text("COMPASS", Vec3(c.x, c.y + m.compassR + 1.2, 0), 2.2, "N", 1, 0);
    d.text("LEGEND", P(m.legendX + 1.5, m.legendY + 4), 2.4, "PLAN", 0, 0);
    d.text("LEGEND", P(m.legendX + 1.5, m.legendY + 8), 2.2, ("1:" + std::to_string(int(std::lround(sp.spec.denom)))), 0, 0);
    for (double X = std::ceil(pl.visX0 / pl.tickStep - 1e-9) * pl.tickStep; X <= pl.visX1 + pl.tickStep * 1e-8; X += pl.tickStep) {
        double xmm = pl.xMm(X);
        if (xmm < L.plotX || xmm > L.plotX + L.plotW) continue;
        d.text("COORD", P(xmm, L.plotY + L.plotH + 3.2), 1.8, groupedCoord(X + origin.x, pl.tickStep).toStdString(), 1, 0);
    }
    for (double Y = std::ceil(pl.visYBot / pl.tickStep - 1e-9) * pl.tickStep; Y <= pl.visYTop + pl.tickStep * 1e-8; Y += pl.tickStep) {
        double ymm = pl.yMm(Y);
        if (ymm < L.plotY || ymm > L.plotY + L.plotH) continue;
        d.text("COORD", P(L.plotX - 1.2, ymm), 1.8, groupedCoord(Y + origin.y, pl.tickStep).toStdString(), 2, 2);
    }
    if (!crop.isNull() && imgRes > 0) {
        crop = crop.mirrored(false, true);   // DXF 는 삽입점(왼쪽 아래)에서 +Y 가 다음 행
        if (!crop.save(pngPath, "PNG")) { if (msg) *msg = QStringLiteral("PNG 저장 실패: %1").arg(pngPath); return false; }
        double destW = crop.width() * imgRes / pl.mPerMm, destH = crop.height() * imgRes / pl.mPerMm;
        double top = pl.yMm(imgY1);
        Vec3 ins = P(pl.xMm(imgX0), top + destH);
        Vec3 u(destW / crop.width(), 0, 0);
        Vec3 v(0, destH / crop.height(), 0);
        d.image("IMAGE", png.toStdString(), crop.width(), crop.height(), ins, u, v);
    }
    std::string err;
    if (!d.save(toFsS(path), &err)) { if (msg) *msg = qs8(err); return false; }
    if (msg) *msg = QStringLiteral("%1%2\n\n평면도 DXF (용지 mm, 단면선 없음, 나침반·좌표·범례)").arg(QFileInfo(path).fileName(), crop.isNull() ? QString() : ", " + png);
    return true;
}
}  // namespace

bool MainWindow::exportPlanSheet(MeshSource& src, const PlanSheetParams& p, const QString& path, QString* msg,
                                 const std::atomic<bool>* cancel, const std::function<void(double)>& progress) {
    PlanSheetParams sp = p;
    sp.spec.denom = snapScaleDenom10(sp.spec.denom);
    Box3 area = sp.wholeModel ? src.bounds : sp.area;
    if (src.bounds.valid()) {
        area.mn.x = std::max(area.mn.x, src.bounds.mn.x); area.mn.y = std::max(area.mn.y, src.bounds.mn.y);
        area.mx.x = std::min(area.mx.x, src.bounds.mx.x); area.mx.y = std::min(area.mx.y, src.bounds.mx.y);
        area.mn.z = src.bounds.mn.z; area.mx.z = src.bounds.mx.z;
        if (!(area.mx.x > area.mn.x && area.mx.y > area.mn.y)) area = src.bounds;
    }
    sp.area = area;
    const double imgDpi = sp.dpi;   // PDF 도 제한 없이 지정한 dpi 그대로(가장 선명)
    const PlanPlace cover = placePlan(sp.spec, area.mn.x, area.mx.x, area.mn.y, area.mx.y, 1, sp.imgDxMm, sp.imgDyMm);
    Box3 render;
    render.mn.x = cover.worldX0; render.mx.x = cover.worldX1; render.mn.y = cover.worldYBot; render.mx.y = cover.worldYTop;
    render.mn.z = src.bounds.mn.z; render.mx.z = src.bounds.mx.z;
    double res = groundResolution(sp.spec.denom, imgDpi);
    double pix = (render.mx.x - render.mn.x) / res * (render.mx.y - render.mn.y) / res;
    if (pix > 40e6) res *= std::sqrt(pix / 40e6);
    QImage img; double x0 = 0, y1 = 0;
    if (progress) progress(0.05);
    if (!renderPlanOrtho(src, render, res, img, x0, y1, msg, cancel)) return false;
    if (cancel && cancel->load()) { if (msg) *msg = QStringLiteral("취소됨"); return false; }
    if (sp.format == 1) return writePlanSheetDxf(sp, img, x0, y1, res, src.srs.origin, path, msg);
    if (sp.format == 4) {
        const PlanWindow Wsvg = planWindow(sp, 0);
        const double dpi = 96.0, k = dpi / 25.4;
        QSvgGenerator gen(QSvgGenerator::SvgVersion::Svg11);
        gen.setFileName(path);
        gen.setResolution(int(dpi));
        gen.setTitle(sp.title);
        gen.setDescription(QStringLiteral("DRAW_SOIL and DRAW_OUTLINE are empty drawing layers"));
        gen.setSize(QSize(std::max(1, int(std::lround(Wsvg.L.paperW * k))), std::max(1, int(std::lround(Wsvg.L.paperH * k)))));
        gen.setViewBox(QRectF(0, 0, Wsvg.L.paperW * k, Wsvg.L.paperH * k));
        QPainter pt;
        if (!pt.begin(&gen)) { if (msg) *msg = QStringLiteral("SVG 를 만들 수 없습니다: %1").arg(path); return false; }
        paintPlanSheet(pt, sp, k, 0, img, x0, y1, res, src.srs.origin);
        pt.end();
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) { if (msg) *msg = QStringLiteral("SVG 를 읽을 수 없습니다"); return false; }
        QString svg = QString::fromUtf8(f.readAll());
        f.close();
        int at = svg.indexOf(QStringLiteral("<svg"));
        int gt = at < 0 ? -1 : svg.indexOf('>', at);
        if (gt > 0) svg.insert(gt + 1, QStringLiteral("\n<g id=\"DRAW_SOIL\"/>\n<g id=\"DRAW_OUTLINE\"/>\n"));
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { if (msg) *msg = QStringLiteral("SVG 를 쓸 수 없습니다"); return false; }
        f.write(svg.toUtf8());
        if (msg) *msg = QStringLiteral("%1\n평면도 SVG · 빈 층 DRAW_SOIL · DRAW_OUTLINE · 단면선 없음").arg(QFileInfo(path).fileName());
        return true;
    }
    const PlanWindow W0 = planWindow(sp, 0);
    const int pages = sp.split ? W0.L.cols * W0.L.rows : 1;
    const Vec3 origin = src.srs.origin;
    QStringList files;
    if (sp.format == 0) {
        QPdfWriter w(path);
        w.setResolution(int(std::lround(sp.dpi)));
        w.setTitle(QStringLiteral("%1 — %2 · %3").arg(sp.title, sp.srsLabel, sp.heightLabel));
        w.setCreator(QStringLiteral("Kerf %1").arg(QString::fromUtf8(kVersion)));
        QPageSize ps(sp.spec.paper == Paper::A3 ? QPageSize::A3 : QPageSize::A4);
        w.setPageLayout(QPageLayout(ps, sp.spec.landscape ? QPageLayout::Landscape : QPageLayout::Portrait, QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter));
        QPainter pt;
        if (!pt.begin(&w)) { if (msg) *msg = QStringLiteral("PDF 를 만들 수 없습니다: %1").arg(path); return false; }
        const double pxPerMm = w.resolution() / 25.4;
        for (int i = 0; i < pages; ++i) {
            if (i > 0) w.newPage();
            paintPlanSheet(pt, sp, pxPerMm, i, img, x0, y1, res, origin);
            if (progress) progress(0.3 + 0.7 * (i + 1) / pages);
            if (cancel && cancel->load()) break;
        }
        pt.end();
        files << QFileInfo(path).fileName();
    } else {
        const double pxPerMm = sp.dpi / 25.4;
        const QSize sz(int(std::lround(W0.L.paperW * pxPerMm)), int(std::lround(W0.L.paperH * pxPerMm)));
        QFileInfo fi(path);
        for (int i = 0; i < pages; ++i) {
            QImage canvas(sz, QImage::Format_RGB32);
            canvas.fill(Qt::white);
            { QPainter pt(&canvas); paintPlanSheet(pt, sp, pxPerMm, i, img, x0, y1, res, origin); }
            const int dpm = int(std::lround(sp.dpi / 0.0254));
            canvas.setDotsPerMeterX(dpm); canvas.setDotsPerMeterY(dpm);
            QString f = pages > 1 ? fi.absolutePath() + "/" + fi.completeBaseName() + QStringLiteral("_%1.").arg(i + 1) + fi.suffix() : path;
            if (sp.format == 2) {
                if (!canvas.save(f, "PNG")) { if (msg) *msg = QStringLiteral("PNG 저장 실패: %1").arg(f); return false; }
            } else {
                TiffOptions o; o.dpi = sp.dpi; o.alpha = false;
                o.description = "Plan sheet " + src.srs.shortLabel();
                std::string err;
                if (!writeTiff(toFsS(f), fromQImageS(canvas), o, &err)) { if (msg) *msg = qs8(err); return false; }
            }
            files << QFileInfo(f).fileName();
            if (progress) progress(0.3 + 0.7 * (i + 1) / pages);
            if (cancel && cancel->load()) { if (msg) *msg = QStringLiteral("취소됨"); return false; }
        }
    }
    if (msg) {
        *msg = QStringLiteral("%1\n\n평면도 · %2 · %3 · %4쪽\n단면선 없음 · 나침반 · 바깥 좌표 · 범례 · 자\n%5 · %6")
                   .arg(files.join(", "), paperLabel(sp.spec), denomText(sp.spec.denom)).arg(pages).arg(sp.srsLabel, sp.heightLabel);
    }
    return true;
}

bool MainWindow::exportSectionSvg(const SectionDoc& doc0, MeshSource& src, const SheetParams& p, const QString& path, QString* msg) {
    SheetParams sp = p;
    sp.withImage = true;
    const SheetGeom G = sheetGeom(doc0, sp.spec);
    QImage img; SectionImgGeo geo{doc0.imgS0, doc0.imgZ1, doc0.imgRes};
    if (sp.withImage && (img.isNull() || geo.res <= 0)) {
        SectionRequest rq;
        rq.line = doc0.r.line;
        rq.depthFade = doc0.st.depthFade ? kDepthFadeStrength : 0.0;
        rq.imageRes = groundResolution(sp.spec.denom, 300);
        rq.maxImagePixels = size_t(60) << 20;
        SectionOutput out; std::string e;
        if (!computeSection(src, rq, out, &e, nullptr)) { if (msg) *msg = qs8(e); return false; }
        img = toQImageS(out.image.img);
        geo = SectionImgGeo{out.image.s0, out.image.z1, out.image.res};
    }
    const double dpi = 96.0;
    const double k = dpi / 25.4;
    QSvgGenerator gen(QSvgGenerator::SvgVersion::Svg11);
    gen.setFileName(path);
    gen.setResolution(int(dpi));
    gen.setTitle(sp.title);
    gen.setDescription(QStringLiteral("DRAW_SOIL and DRAW_OUTLINE are empty drawing layers"));
    gen.setSize(QSize(std::max(1, int(std::lround(G.L.paperW * k))), std::max(1, int(std::lround(G.L.paperH * k)))));
    gen.setViewBox(QRectF(0, 0, G.L.paperW * k, G.L.paperH * k));
    QPainter pt;
    if (!pt.begin(&gen)) { if (msg) *msg = QStringLiteral("SVG 를 만들 수 없습니다: %1").arg(path); return false; }
    paintSheet(pt, doc0, sp, k, 0, img, geo);
    pt.end();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) { if (msg) *msg = QStringLiteral("SVG 를 읽을 수 없습니다"); return false; }
    QString svg = QString::fromUtf8(f.readAll());
    f.close();
    int at = svg.indexOf(QStringLiteral("<svg"));
    int gt = at < 0 ? -1 : svg.indexOf('>', at);
    if (gt > 0) svg.insert(gt + 1, QStringLiteral("\n<g id=\"DRAW_SOIL\"/>\n<g id=\"DRAW_OUTLINE\"/>\n"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { if (msg) *msg = QStringLiteral("SVG 를 쓸 수 없습니다"); return false; }
    f.write(svg.toUtf8());
    f.close();
    if (msg) *msg = QStringLiteral("%1\n빈 층 DRAW_SOIL · DRAW_OUTLINE").arg(QFileInfo(path).fileName());
    return true;
}

bool MainWindow::exportSheet(const SectionDoc& doc0, MeshSource& src, const SheetParams& p, const QString& path, QString* msg,
                             const std::atomic<bool>* cancel, const std::function<void(double)>& progress) {
    const SheetGeom G = sheetGeom(doc0, p.spec);
    const int pages = p.split ? G.L.cols * G.L.rows : 1;
    if (p.format == 1) {   // DXF: 모델 공간 1:N(용지 배치 없음)
        DxfParams dp; dp.world3d = false; dp.denom = p.spec.denom; dp.image = p.withImage; dp.imageDpi = std::min(p.dpi, 200.0);
        dp.levelMinorPt = p.levelMinorPt; dp.levelMajorPt = p.levelMajorPt;
        SectionDoc doc = doc0; doc.st.showLevels = p.withLevels;
        return exportSectionDxf(doc, src, dp, path, msg, cancel);
    }
    // 입면 영상: 출력 해상도로 다시 계산(같은 단면선 → 같은 윤곽은 스냅숏 것을 유지)
    QImage img; SectionImgGeo geo{doc0.imgS0, doc0.imgZ1, doc0.imgRes};
    double usedRes = 0;
    if (p.withImage) {
        const double imgDpi = p.dpi;
        SectionRequest rq;
        rq.line = doc0.r.line;
        rq.depthFade = doc0.st.depthFade ? kDepthFadeStrength : 0.0;
        rq.imageRes = groundResolution(p.spec.denom, imgDpi);
        rq.maxImagePixels = size_t(120) << 20;
        SectionOutput out; std::string e;
        if (progress) progress(0.05);
        if (!computeSection(src, rq, out, &e, cancel)) { if (msg) *msg = qs8(e); return false; }
        img = toQImageS(out.image.img);
        geo = SectionImgGeo{out.image.s0, out.image.z1, out.image.res};
        usedRes = out.image.res;
    }
    if (cancel && cancel->load()) { if (msg) *msg = QStringLiteral("취소됨"); return false; }
    QStringList files;
    if (p.format == 0) {
        QPdfWriter w(path);
        w.setResolution(int(std::lround(p.dpi)));
        w.setTitle(QStringLiteral("%1 — %2 · %3").arg(p.title, p.srsLabel, p.heightLabel));
        w.setCreator(QStringLiteral("Kerf %1").arg(QString::fromUtf8(kVersion)));
        QPageSize ps(p.spec.paper == Paper::A3 ? QPageSize::A3 : QPageSize::A4);
        w.setPageLayout(QPageLayout(ps, p.spec.landscape ? QPageLayout::Landscape : QPageLayout::Portrait, QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter));
        QPainter pt;
        if (!pt.begin(&w)) { if (msg) *msg = QStringLiteral("PDF 를 만들 수 없습니다: %1").arg(path); return false; }
        const double pxPerMm = w.resolution() / 25.4;
        for (int i = 0; i < pages; ++i) {
            if (i > 0) w.newPage();
            paintSheet(pt, doc0, p, pxPerMm, i, img, geo);
            if (progress) progress(0.3 + 0.7 * (i + 1) / pages);
            if (cancel && cancel->load()) break;
        }
        pt.end();
        files << QFileInfo(path).fileName();
    } else {
        const double pxPerMm = p.dpi / 25.4;
        const QSize sz(int(std::lround(G.L.paperW * pxPerMm)), int(std::lround(G.L.paperH * pxPerMm)));
        QFileInfo fi(path);
        for (int i = 0; i < pages; ++i) {
            QImage canvas(sz, QImage::Format_RGB32);
            canvas.fill(Qt::white);
            { QPainter pt(&canvas); paintSheet(pt, doc0, p, pxPerMm, i, img, geo); }
            const int dpm = int(std::lround(p.dpi / 0.0254));
            canvas.setDotsPerMeterX(dpm); canvas.setDotsPerMeterY(dpm);
            QString f = pages > 1 ? fi.absolutePath() + "/" + fi.completeBaseName() + QStringLiteral("_%1.").arg(i + 1) + fi.suffix() : path;
            if (p.format == 2) {
                if (!canvas.save(f, "PNG")) { if (msg) *msg = QStringLiteral("PNG 저장 실패: %1").arg(f); return false; }
            } else {
                TiffOptions o; o.dpi = p.dpi; o.alpha = false;
                o.description = "Section sheet " + doc0.r.srs.shortLabel();
                std::string err;
                if (!writeTiff(toFsS(f), fromQImageS(canvas), o, &err)) { if (msg) *msg = qs8(err); return false; }
            }
            files << QFileInfo(f).fileName();
            if (progress) progress(0.3 + 0.7 * (i + 1) / pages);
            if (cancel && cancel->load()) { if (msg) *msg = QStringLiteral("취소됨"); return false; }
        }
    }
    if (msg) {
        *msg = QStringLiteral("%1\n\n%2 · %3 · %4쪽%5\n%6 · %7")
                   .arg(files.join(", "), paperLabel(p.spec), denomText(p.spec.denom)).arg(pages)
                   .arg(!G.L.fits && !p.split ? QStringLiteral(" (넘친 부분은 잘림)") : QString())
                   .arg(p.srsLabel, p.heightLabel);
        if (usedRes > 0) *msg += QStringLiteral("\n입면 영상 %1 mm/px").arg(usedRes * 1000, 0, 'f', 2);
    }
    return true;
}

// ---------------------------------------------------------------- 도면 창
namespace {
class SheetPreview : public QWidget {
public:
    std::function<void(QPainter&, const QRect&)> paintFn;
    std::function<void(QPointF)> onImageDrag;
    std::function<void()> onSettled;   // 확대·그림 이동이 멈춘 0.25 초 뒤(보이는 범위를 더 촘촘히 다시 그리는 데 씀)
    bool editing = false;
    double zoom = 1;          // 조판편집 중 휠로 바꾸는 칸 안 배율(멈추면 축척 값으로 바뀌고 1 로 돌아감)
    double pageZoom = 1;      // 편집 아닐 때 휠: 종이 전체를 크게·작게 보기만(도면 내용·축척은 그대로)
    double fitPxPerMm = 1;
    QPointF lastPaperO, lastPaperSize;   // 마지막으로 그린 종이 자리·크기(px) — 커서 기준 확대용
    QPointF viewPan;
    QPointF zoomCenter;
    SheetPreview() { setMouseTracking(false); setFocusPolicy(Qt::WheelFocus); setObjectName(QStringLiteral("sheetPreview")); }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), theme::Wash);
        if (paintFn) paintFn(p, rect());
    }
    void wheelEvent(QWheelEvent* e) override {
        double steps = e->angleDelta().y() / 120.0;
        if (steps == 0 && !e->pixelDelta().isNull()) steps = e->pixelDelta().y() / 60.0;
        if (steps == 0) { e->ignore(); return; }
        const double factor = std::exp(-std::log(0.85) * steps);
        if (editing) {   // 조판편집: 칸 안 그림 배율 → 멈추면 축척 값으로
            zoom = std::clamp(zoom * factor, 0.35, 8.0);
            update();
            scheduleSettled();
        } else {         // 보기만: 커서 아래 종이 점을 고정하고 종이 전체를 확대·축소
            const double nz = std::clamp(pageZoom * factor, 0.5, 8.0), f = nz / pageZoom;
            const QPointF cur = e->position();
            const QPointF newO = cur - (cur - lastPaperO) * f;
            const QPointF newSize = lastPaperSize * f;
            const QPointF center(width() / 2.0, height() / 2.0);
            viewPan = newO - (center - newSize / 2);
            pageZoom = nz;
            update();
        }
        e->accept();
    }
    void mouseDoubleClickEvent(QMouseEvent* e) override { pageZoom = 1; viewPan = QPointF(); update(); e->accept(); }   // 종이 보기 처음 크기로
    void mousePressEvent(QMouseEvent* e) override {
        last_ = e->position();
        const bool image = editing && e->button() == Qt::MiddleButton;
        const bool view = e->button() == Qt::LeftButton;
        if (image || view) { drag_ = e->button(); grabMouse(); e->accept(); }
        else e->ignore();
    }
    void mouseMoveEvent(QMouseEvent* e) override {
        const QPointF d = e->position() - last_;
        last_ = e->position();
        if (editing && (e->buttons() & Qt::MiddleButton) && onImageDrag) { onImageDrag(d); e->accept(); return; }
        if (e->buttons() & Qt::LeftButton) { viewPan += d; update(); e->accept(); }
    }
    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == drag_) { const bool img = drag_ == Qt::MiddleButton; releaseMouse(); drag_ = Qt::NoButton; e->accept(); if (img) scheduleSettled(); }
    }
    void scheduleSettled() {
        if (!onSettled) return;
        if (!settle_) { settle_ = new QTimer(this); settle_->setSingleShot(true); settle_->setInterval(250); QObject::connect(settle_, &QTimer::timeout, this, [this] { if (onSettled) onSettled(); }); }
        settle_->start();
    }
private:
    QTimer* settle_ = nullptr;
    QPointF last_;
    Qt::MouseButton drag_ = Qt::NoButton;
};
QSpinBox* scaleSpin(double denom) {
    auto* scale = new QSpinBox;
    scale->setRange(10, 2000000000);
    scale->setSingleStep(10);
    scale->setPrefix(QStringLiteral("1:"));
    scale->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
    scale->setValue(int(std::lround(snapScaleDenom10(denom))));
    scale->setToolTip(QStringLiteral("10 단위. 위·아래 화살표로 바꿉니다"));
    return scale;
}
QWidget* scaleWithArrows(QSpinBox* spin) {
    auto* w = new QWidget;
    auto* h = new QHBoxLayout(w); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(2);
    h->addWidget(spin, 1);
    auto* col = new QVBoxLayout; col->setContentsMargins(0, 0, 0, 0); col->setSpacing(1);
    for (int up = 1; up >= 0; --up) {
        auto* b = new QToolButton; b->setObjectName("scaleArrow");
        b->setText(up ? QString::fromUtf8("▲") : QString::fromUtf8("▼"));
        b->setToolTip(up ? QStringLiteral("축척 분모 +10") : QStringLiteral("축척 분모 −10"));
        b->setFocusPolicy(Qt::NoFocus); b->setAutoRepeat(true); b->setFixedSize(22, 14);
        b->setStyleSheet(QStringLiteral("QToolButton{font-size:8px;padding:0;border:1px solid #C2C0B6;border-radius:4px;background:#FFFFFF;color:#3D3D3A}QToolButton:hover{background:#F5F4ED}QToolButton:pressed{background:#E8E6DC}"));
        QObject::connect(b, &QToolButton::clicked, spin, [spin, up] { if (up) spin->stepUp(); else spin->stepDown(); });
        col->addWidget(b);
    }
    h->addLayout(col);
    return w;
}
void bindImageDrag(SheetPreview* pv, double* dx, double* dy) {
    pv->onImageDrag = [pv, dx, dy](QPointF d) {
        const double px = std::max(1e-6, pv->fitPxPerMm);
        *dx += d.x() / px;
        *dy += d.y() / px;
        pv->update();
    };
}
QToolButton* chipBtn(const QString& t, QButtonGroup* g, int id, const char* pos = nullptr, int minW = 56) {
    auto* b = new QToolButton; b->setObjectName("chip"); b->setCheckable(true); b->setText(t); b->setFocusPolicy(Qt::NoFocus);
    b->setMinimumWidth(minW);
    if (pos) b->setProperty("pos", pos);
    g->addButton(b, id);
    return b;
}
QLabel* sideHead(const QString& t) { auto* l = new QLabel(t); l->setObjectName("sideHead"); return l; }
QLabel* lbl(const QString& t, const char* obj = nullptr) { auto* l = new QLabel(t); if (obj) l->setObjectName(obj); return l; }

// ---- 디자인 v5 §8 조판 판 부품(Strata 「항목 타일 · 축척 격자 · 도면 점검」과 같은 모양). Q_OBJECT 없음
QWidget* sideCaption(const QString& t) {   // 절 이름 11/700 Muted + 1 px Line 리더
    auto* w = new QWidget; auto* h = new QHBoxLayout(w); h->setContentsMargins(0, 6, 0, 0); h->setSpacing(8);
    h->addWidget(sideHead(t));
    auto* rule = new QFrame; rule->setObjectName("sideRule"); rule->setFixedHeight(1); h->addWidget(rule, 1);
    return w;
}
QToolButton* sheetTile(const QString& icon, const QString& text, const QString& tip) {   // 72×56 켜짐 = Oat + Ring
    auto* b = new QToolButton; b->setObjectName("sheetTile"); b->setCheckable(true); b->setFocusPolicy(Qt::NoFocus);
    b->setIcon(kerf::icon(icon, 20, theme::Hand)); b->setIconSize(QSize(20, 20)); b->setText(text); b->setToolTip(tip);
    b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    return b;
}
QDoubleSpinBox* ptSpin(double v, const QString& tip) {   // 레벨선 굵기 pt 칸(모노 72, 0.05–3.0)
    auto* sp = new QDoubleSpinBox; sp->setRange(0.05, 3.0); sp->setDecimals(2); sp->setSingleStep(0.05); sp->setSuffix(QStringLiteral(" pt")); sp->setValue(v);
    sp->setFixedWidth(72); sp->setAlignment(Qt::AlignRight); sp->setToolTip(tip);
    return sp;
}
QWidget* scaleGridWidget(QSpinBox* spin) {   // 축척 격자 4열 × 2줄 — 누르면 칸에 값, 칸 값과 같은 칸이 켜짐
    auto* w = new QWidget; auto* g = new QGridLayout(w); g->setContentsMargins(0, 0, 0, 0); g->setHorizontalSpacing(4); g->setVerticalSpacing(4);
    static const int vals[8] = {10, 20, 30, 40, 50, 100, 200, 500};
    QList<QToolButton*> btns;
    for (int i = 0; i < 8; ++i) {
        auto* b = new QToolButton; b->setObjectName("scaleGrid"); b->setCheckable(true); b->setFocusPolicy(Qt::NoFocus); b->setText(QString::number(vals[i]));
        b->setToolTip(QStringLiteral("1:%1").arg(vals[i])); b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        b->setProperty("denom", vals[i]);
        QObject::connect(b, &QToolButton::clicked, spin, [spin, v = vals[i]] { spin->setValue(v); });
        g->addWidget(b, i / 4, i % 4); btns << b;
    }
    auto sync = [btns](int v) { for (auto* b : btns) { QSignalBlocker bl(b); b->setChecked(b->property("denom").toInt() == v); } };
    sync(spin->value());
    QObject::connect(spin, &QSpinBox::valueChanged, w, sync);
    return w;
}
struct PaperChips { QWidget* w = nullptr; QButtonGroup* size = nullptr; QButtonGroup* orient = nullptr; };
PaperChips paperChips(const SheetSpec& s) {   // 용지 [A4 | A3] [가로 | 세로]
    PaperChips pc; pc.w = new QWidget; auto* h = new QHBoxLayout(pc.w); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(8);
    pc.size = new QButtonGroup(pc.w); pc.size->setExclusive(true);
    pc.orient = new QButtonGroup(pc.w); pc.orient->setExclusive(true);
    auto* a = new QWidget; { auto* hh = new QHBoxLayout(a); hh->setContentsMargins(0, 0, 0, 0); hh->setSpacing(2); hh->addWidget(chipBtn(QStringLiteral("A4"), pc.size, 0, "first")); hh->addWidget(chipBtn(QStringLiteral("A3"), pc.size, 1, "last")); }
    auto* o = new QWidget; { auto* hh = new QHBoxLayout(o); hh->setContentsMargins(0, 0, 0, 0); hh->setSpacing(2); hh->addWidget(chipBtn(QStringLiteral("가로"), pc.orient, 0, "first")); hh->addWidget(chipBtn(QStringLiteral("세로"), pc.orient, 1, "last")); }
    h->addWidget(a); h->addWidget(o); h->addStretch();
    pc.size->button(s.paper == Paper::A3 ? 1 : 0)->setChecked(true);
    pc.orient->button(s.landscape ? 0 : 1)->setChecked(true);
    return pc;
}
void syncPaperChips(const PaperChips& pc, const SheetSpec& s) {
    if (auto* b = pc.size->button(s.paper == Paper::A3 ? 1 : 0)) { QSignalBlocker bl(b); b->setChecked(true); }
    if (auto* b = pc.orient->button(s.landscape ? 0 : 1)) { QSignalBlocker bl(b); b->setChecked(true); }
}
struct CheckItem { bool ok = true; QString text; QString link; std::function<void()> fn; };
class SheetCheckBox : public QFrame {   // 「도면 점검」 상자: 항목마다 ✓(Done) 또는 ▲(Caution) + 문장 + 고치는 곳 링크. 저장을 막지 않는다
public:
    SheetCheckBox() {
        setObjectName(QStringLiteral("sheetCheck")); setAttribute(Qt::WA_StyledBackground);
        lay_ = new QVBoxLayout(this); lay_->setContentsMargins(12, 10, 12, 10); lay_->setSpacing(6);
    }
    void setItems(const std::vector<CheckItem>& items) {
        while (QLayoutItem* it = lay_->takeAt(0)) { if (QWidget* w = it->widget()) { w->hide(); w->deleteLater(); } delete it; }
        for (const CheckItem& c : items) {
            auto* row = new QWidget(this); auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(8);
            auto* mark = new QLabel(c.ok ? QStringLiteral("✓") : QStringLiteral("▲")); mark->setObjectName(c.ok ? "checkOk" : "checkWarn"); mark->setFixedWidth(14); mark->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
            auto* tx = new QLabel(c.text); tx->setObjectName("checkText"); tx->setWordWrap(true);
            h->addWidget(mark, 0, Qt::AlignTop); h->addWidget(tx, 1);
            if (!c.link.isEmpty()) {
                auto* b = new QPushButton(c.link); b->setObjectName("quietLink"); b->setFlat(true); b->setFocusPolicy(Qt::NoFocus); b->setCursor(Qt::PointingHandCursor);
                if (c.fn) QObject::connect(b, &QPushButton::clicked, b, [fn = c.fn] { fn(); });
                h->addWidget(b, 0, Qt::AlignTop);
            }
            lay_->addWidget(row); row->show();
        }
    }
private:
    QVBoxLayout* lay_ = nullptr;
};
QWidget* sideBottom(QPushButton* def, QPushButton* primary, QPushButton* secondary, QLabel** done) {   // 맨 아래 Line 위: 「n장 저장됨 · 폴더 열기」(저장 뒤) / 조용한 「☆ 기본으로」 | 보조 | 흙색 주 단추
    auto* w = new QWidget; w->setObjectName(QStringLiteral("sideBottom")); w->setAttribute(Qt::WA_StyledBackground);
    auto* v = new QVBoxLayout(w); v->setContentsMargins(12, 8, 12, 10); v->setSpacing(6);
    auto* d = new QLabel; d->setObjectName(QStringLiteral("sheetDone")); d->setTextFormat(Qt::RichText); d->setOpenExternalLinks(false); d->hide();
    QObject::connect(d, &QLabel::linkActivated, d, [](const QString& href) { QDesktopServices::openUrl(QUrl(href)); });
    v->addWidget(d);
    if (done) *done = d;
    auto* h = new QHBoxLayout; h->setContentsMargins(0, 0, 0, 0); h->setSpacing(6);
    h->addWidget(def); h->addStretch(); if (secondary) h->addWidget(secondary); h->addWidget(primary);
    v->addLayout(h);
    return w;
}
// 책상 위 떠 있는 것(스펙 §8): 왼쪽 위 「도면 보기」 안내 칩 + 아래 가운데 도구 줄 6칸 64×52
void addDeskFloats(SheetPreview* pv, QAction* editAct, std::function<void()> paperCycle, std::function<void()> listToggle) {
    auto* g = new GuideBand(pv); g->setReserveRight(0);
    g->setTool(QStringLiteral("sheet"), QStringLiteral("도면 보기"));
    g->setHint(QStringLiteral("휠 = 종이 확대 · 왼쪽 끌기 = 옮기기 · 두 번 = 처음 크기"));
    g->show(); g->place();
    auto* tools = new FloatButtons(pv, Qt::Horizontal, Qt::AlignHCenter | Qt::AlignBottom, 12);
    auto zoomBy = [pv](double f) { pv->pageZoom = std::clamp(pv->pageZoom * f, 0.5, 8.0); pv->update(); };
    QObject::connect(tools->add(QStringLiteral("square-dashed"), QStringLiteral("맞춤 — 종이를 처음 크기로"), nullptr, QStringLiteral("맞춤")), &QToolButton::clicked, pv, [pv] { pv->pageZoom = 1; pv->viewPan = QPointF(); pv->update(); });
    QObject::connect(tools->add(QStringLiteral("zoom-in"), QStringLiteral("확대"), nullptr, QStringLiteral("확대")), &QToolButton::clicked, pv, [zoomBy] { zoomBy(1.25); });
    QObject::connect(tools->add(QStringLiteral("zoom-out"), QStringLiteral("축소"), nullptr, QStringLiteral("축소")), &QToolButton::clicked, pv, [zoomBy] { zoomBy(0.8); });
    tools->add(QStringLiteral("edit-move"), QStringLiteral("조판편집 — 켜면 가운데 버튼으로 칸 안 그림을 옮기고, 휠은 축척을 바꿉니다"), editAct, QStringLiteral("조판편집"));
    QObject::connect(tools->add(QStringLiteral("paper"), QStringLiteral("용지 · 방향 바꾸기 (A4 가로 → A4 세로 → A3 가로 → A3 세로)"), nullptr, QStringLiteral("용지/방향")), &QToolButton::clicked, pv, [paperCycle] { paperCycle(); });
    QObject::connect(tools->add(QStringLiteral("list"), QStringLiteral("도면 목록 보이기 · 숨기기"), nullptr, QStringLiteral("도면 목록")), &QToolButton::clicked, pv, [listToggle] { listToggle(); });
    tools->show(); tools->place();
}
// 종이: Card · Edge 1 px · 둥글기 0 · 그림자 없음(스펙 §8)
void paintPaperFrame(QPainter& p, const QPointF& o, const QSizeF& pz) {
    p.setPen(QPen(theme::Edge, 1)); p.setBrush(Qt::NoBrush); p.drawRoundedRect(QRectF(o, pz), 4, 4);   // 화면 테두리만 둥글게(종이 위 도면 · 인쇄물은 그대로)
}
}  // namespace

QDialog* MainWindow::buildSheetDialog(SheetParams& io, bool& accepted) {
    accepted = false;
    auto* d = new QDialog(this);
    d->setWindowTitle(QStringLiteral("단면도"));
    d->setObjectName("sheetDialog");
    auto st = std::make_shared<SheetParams>(io);
    const SectionDoc doc = section_->doc();
    QImage screenImg = doc.img;
    SectionImgGeo screenGeo{doc.imgS0, doc.imgZ1, doc.imgRes};
    if (src_ && doc.imgRes > 0.0016) {   // 화면 해상도 영상은 조판에서 흐리므로 1.5 mm/px 로 다시 계산
        SectionRequest rq; rq.line = doc.r.line; rq.depthFade = doc.st.depthFade ? kDepthFadeStrength : 0.0;
        rq.imageRes = 0.0015; rq.maxImagePixels = size_t(60) << 20;
        SectionOutput out; std::string e;
        if (computeSection(*src_, rq, out, &e, nullptr)) { screenImg = toQImageS(out.image.img); screenGeo = SectionImgGeo{out.image.s0, out.image.z1, out.image.res}; }
    }
    auto* outer = new QHBoxLayout(d); outer->setContentsMargins(0, 0, 0, 0); outer->setSpacing(0);
    // ---- 책상(Desk): 종이 가운데, 떠 있는 안내 · 도구 줄은 아래에서
    auto* pv = new SheetPreview; pv->setMinimumSize(470, 330);
    bindImageDrag(pv, &st->imgDxMm, &st->imgDyMm);
    outer->addWidget(pv, 1);
    // ---- 오른쪽 판 320: 머리 · 도면 항목 · 도면 정보 · 선 글자 · 형식 · 도면 점검 | 바닥
    auto* side = new QWidget; side->setObjectName("dialogSide"); side->setAttribute(Qt::WA_StyledBackground); side->setFixedWidth(320);
    auto* sv = new QVBoxLayout(side); sv->setContentsMargins(0, 0, 0, 0); sv->setSpacing(0);
    auto* scroll = new QScrollArea; scroll->setObjectName("sideScroll"); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* body = new QWidget; body->setObjectName("sideBody"); body->setAttribute(Qt::WA_StyledBackground);
    auto* v = new QVBoxLayout(body); v->setContentsMargins(12, 10, 12, 10); v->setSpacing(6);
    const int nSec = int(sections_.size()), kSec = current_ + 1;
    v->addWidget(lbl(nSec > 1 ? QStringLiteral("고른 도면 · 단면 %1 / %2").arg(kSec).arg(nSec) : QStringLiteral("고른 도면 · 단면"), "cap"));
    auto* nameLab = lbl(st->title, "sheetName"); nameLab->setWordWrap(true); v->addWidget(nameLab);
    auto* subLab = lbl(QString(), "hint"); subLab->setWordWrap(true); v->addWidget(subLab);
    // 도면 항목(단면도의 층): 입면 영상 · 단면선 · 레벨선 · 표제란(자 포함). 진북 · 범례는 평면도에만
    v->addWidget(sideCaption(QStringLiteral("도면 항목")));
    auto* tImg = sheetTile(QStringLiteral("image-layer"), QStringLiteral("입면 영상"), QStringLiteral("입면 영상 (뒤 %1 m) — 레벨선 위, 단면선 아래").arg(doc.r.line.back, 0, 'g', 3));
    auto* tLine = sheetTile(QStringLiteral("pen-line"), QStringLiteral("단면선"), QStringLiteral("잘린 선 — 맨 위 · 빨강 0.35 mm"));
    auto* tLev = sheetTile(QStringLiteral("levels"), QStringLiteral("레벨선"), QStringLiteral("레벨선과 표고 숫자 — 맨 뒤"));
    auto* tTitle = sheetTile(QStringLiteral("title-block"), QStringLiteral("표제란"), QStringLiteral("표제란 · 축척 막대(끄면 그림 칸 안에 작은 축척 막대)"));
    { auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(0);
        h->setSpacing(8); for (auto* tb : {tImg, tLine, tLev, tTitle}) h->addWidget(tb);
        h->addStretch(); v->addWidget(row); }
    // 도면 정보: 도면명 · 용지 · 축척 1 : [칸] 맞춤 · 격자
    v->addWidget(sideCaption(QStringLiteral("도면 정보")));
    auto* g = new QGridLayout; g->setContentsMargins(0, 0, 0, 0); g->setHorizontalSpacing(10); g->setVerticalSpacing(8); g->setColumnMinimumWidth(0, 40); g->setColumnStretch(1, 1);
    int r = 0;
    auto* name = new QLineEdit(st->title);
    g->addWidget(lbl(QStringLiteral("도면명"), "hint"), r, 0); g->addWidget(name, r++, 1);
    PaperChips pc = paperChips(st->spec);
    g->addWidget(lbl(QStringLiteral("용지"), "hint"), r, 0); g->addWidget(pc.w, r++, 1);
    auto* scale = scaleSpin(st->spec.denom);
    auto* fitLink = new QPushButton; fitLink->setObjectName("quietLink"); fitLink->setFlat(true); fitLink->setFocusPolicy(Qt::NoFocus); fitLink->setCursor(Qt::PointingHandCursor);
    fitLink->setToolTip(QStringLiteral("단면이 그림 칸에 다 들어가는 가장 작은 10 단위"));
    { auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(6);
        h->addWidget(scaleWithArrows(scale), 1); h->addWidget(fitLink);
        g->addWidget(lbl(QStringLiteral("축척"), "hint"), r, 0); g->addWidget(row, r++, 1); }
    g->addWidget(scaleGridWidget(scale), r++, 1);
    v->addLayout(g);
    // 선 · 글자: 레벨선 pt 둘 · 숫자 · 빗금 · 기준선
    v->addWidget(sideCaption(QStringLiteral("선 · 글자")));
    auto* g2 = new QGridLayout; g2->setContentsMargins(0, 0, 0, 0); g2->setHorizontalSpacing(10); g2->setVerticalSpacing(8); g2->setColumnMinimumWidth(0, 40); g2->setColumnStretch(1, 1);
    r = 0;
    auto* ptMinor = ptSpin(st->levelMinorPt, QStringLiteral("10 cm 레벨선 굵기 — 일러스트레이터 pt. 종이 위 mm = pt × 25.4 / 72"));
    auto* ptMajor = ptSpin(st->levelMajorPt, QStringLiteral("50 cm(1 m 포함) 레벨선 굵기 — pt"));
    { auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(6);
        h->addWidget(lbl(QStringLiteral("10 cm"), "faint")); h->addWidget(ptMinor); h->addSpacing(6); h->addWidget(lbl(QStringLiteral("50 cm"), "faint")); h->addWidget(ptMajor); h->addStretch();
        g2->addWidget(lbl(QStringLiteral("레벨선"), "hint"), r, 0); g2->addWidget(row, r++, 1); }
    auto* numLab = lbl(QString(), "faint");
    g2->addWidget(lbl(QStringLiteral("숫자"), "hint"), r, 0); g2->addWidget(numLab, r++, 1);
    auto* hatchG = new QButtonGroup(d); hatchG->setExclusive(true);
    auto* hatchMm = new QDoubleSpinBox; hatchMm->setRange(0.2, 5.0); hatchMm->setDecimals(1); hatchMm->setSingleStep(0.1); hatchMm->setSuffix(QStringLiteral(" mm")); hatchMm->setValue(st->hatchMm); hatchMm->setFixedWidth(64); hatchMm->setAlignment(Qt::AlignRight);
    hatchMm->setToolTip(QStringLiteral("빗금 간격(종이 위 mm) — 잘린 돌(H)에 씀"));
    { auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(6);
        auto* chips = new QWidget; { auto* hh = new QHBoxLayout(chips); hh->setContentsMargins(0, 0, 0, 0); hh->setSpacing(2);
            hh->addWidget(chipBtn(QStringLiteral("45°"), hatchG, 0, "first", 40)); hh->addWidget(chipBtn(QStringLiteral("검은 칠"), hatchG, 1, nullptr, 56)); hh->addWidget(chipBtn(QStringLiteral("점묘"), hatchG, 2, "last", 40)); }
        h->addWidget(chips); h->addWidget(hatchMm); h->addStretch();
        g2->addWidget(lbl(QStringLiteral("빗금"), "hint"), r, 0); g2->addWidget(row, r++, 1); }
    auto* cbBase = new QCheckBox(QStringLiteral("기준선")); cbBase->setChecked(st->showBaseline); cbBase->setFocusPolicy(Qt::NoFocus);
    auto* baseEl = new QDoubleSpinBox; baseEl->setRange(-1000, 10000); baseEl->setDecimals(2); baseEl->setSingleStep(0.1); baseEl->setPrefix("EL. "); baseEl->setSuffix(" m"); baseEl->setValue(st->baselineEl);
    { auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(6); h->addWidget(cbBase); h->addWidget(baseEl, 1);
        g2->addWidget(row, r++, 0, 1, 2); }
    v->addLayout(g2);
    // 형식: SVG 가 먼저(일러스트레이터)
    v->addWidget(sideCaption(QStringLiteral("형식")));
    auto* fmtG = new QButtonGroup(d); fmtG->setExclusive(true);
    auto* fmtRow = new QWidget;
    { auto* h = new QHBoxLayout(fmtRow); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(2);
        h->addWidget(chipBtn(QStringLiteral("SVG"), fmtG, 4, "first", 48)); h->addWidget(chipBtn(QStringLiteral("DXF"), fmtG, 1, nullptr, 48)); h->addWidget(chipBtn(QStringLiteral("PDF"), fmtG, 0, nullptr, 48));
        h->addWidget(chipBtn(QStringLiteral("PNG"), fmtG, 2, nullptr, 48)); h->addWidget(chipBtn(QStringLiteral("TIFF"), fmtG, 3, "last", 48)); h->addStretch();
        v->addWidget(fmtRow); }
    // 도면 점검
    v->addWidget(sideCaption(QStringLiteral("도면 점검 · 저장을 막지 않음")));
    auto* check = new SheetCheckBox; v->addWidget(check);
    v->addStretch(1);
    scroll->setWidget(body);
    sv->addWidget(scroll, 1);
    auto* bDefault = new QPushButton(QStringLiteral("☆ 기본으로")); bDefault->setObjectName("quiet"); bDefault->setFocusPolicy(Qt::NoFocus); bDefault->setToolTip(QStringLiteral("이 설정(용지 · 축척 · 항목 · 형식 · 선 굵기)을 다음 도면의 기본으로"));
    auto* bSave = new QPushButton; bSave->setObjectName("primary"); bSave->setDefault(true);
    auto* bPdf = new QPushButton(QStringLiteral("PDF")); bPdf->setFocusPolicy(Qt::NoFocus); bPdf->setToolTip(QStringLiteral("같은 도면을 PDF 로"));
    { QLabel* done = nullptr; sv->addWidget(sideBottom(bDefault, bSave, bPdf, &done)); sheetDoneLabel_ = done; }
    outer->addWidget(side);

    // ---- 갱신
    const QString modelName = QFileInfo(path_).completeBaseName();
    auto refresh = std::make_shared<std::function<void()>>();
    *refresh = [=] {
        st->spec.denom = snapScaleDenom10(st->spec.denom);
        if (scale->value() != int(std::lround(st->spec.denom))) { QSignalBlocker bl(scale); scale->setValue(int(std::lround(st->spec.denom))); }
        syncPaperChips(pc, st->spec);
        SheetGeom G = sheetGeom(doc, st->spec);
        SheetAdvice a = adviseSheet(st->spec, G.lenM, G.heightM);
        if (auto* b = fmtG->button(st->format)) { QSignalBlocker bl(b); b->setChecked(true); }
        if (auto* b = hatchG->button(st->hatchKind)) { QSignalBlocker bl(b); b->setChecked(true); }
        for (auto& pr : {std::make_pair(tImg, st->withImage), std::make_pair(tLine, st->withLine), std::make_pair(tLev, st->withLevels), std::make_pair(tTitle, st->withTitle)}) { QSignalBlocker bl(pr.first); pr.first->setChecked(pr.second); }
        const double pxPerMm = 300 / 25.4, ui = pxPerMm * 25.4 / 96.0;
        LevelPlan lp = sectionLevelPlan(pxPerMm * 1000.0 / st->spec.denom, ui, true);
        numLab->setText(QStringLiteral("%1 cm마다 · 해발 EL. · 선 %2 cm").arg(lp.labelCm).arg(lp.lineCm));
        const double fitD = G.lenM > 0 && G.heightM > 0 ? fitDenomStep10(st->spec, G.lenM, G.heightM) : st->spec.denom;
        fitLink->setText(QStringLiteral("맞춤 %1").arg(denomText(fitD)));
        nameLab->setText(st->title);
        const int pages = st->split ? G.L.cols * G.L.rows : 1;
        subLab->setText(QStringLiteral("%1 · %2 · 그림 칸 %3 × %4 mm%5 · 바깥 도곽은 평면도와 같다")
                            .arg(paperLabel(st->spec), denomText(st->spec.denom)).arg(G.L.plotW, 0, 'f', 0).arg(G.L.plotH, 0, 'f', 0).arg(pages > 1 ? QStringLiteral(" · %1장").arg(pages) : QString()));
        // 도면 점검
        std::vector<CheckItem> items;
        if (a.overflow) {
            QString what = G.L.cols > 1 ? QStringLiteral("단면 길이 %1 m(%2 mm)").arg(G.lenM, 0, 'f', 2).arg(G.L.contentW, 0, 'f', 0)
                                        : QStringLiteral("단면 높이 %1 m(%2 mm)").arg(G.heightM, 0, 'f', 2).arg(G.L.contentH, 0, 'f', 0);
            CheckItem c; c.ok = false;
            c.text = QStringLiteral("%1에서 %2가 %3 한 장에 안 들어갑니다%4").arg(denomText(st->spec.denom), what, paperLabel(st->spec), G.L.cols > 1 ? QStringLiteral(" — 지금은 A 쪽 %1 m만").arg(a.fitLenM, 0, 'f', 1) : QString());
            if (a.smallerDenom > 0) { c.link = QStringLiteral("%1으로").arg(denomText(a.smallerDenom)); c.fn = [=] { st->spec.denom = a.smallerDenom; (*refresh)(); }; }
            else if (a.a3Fits) { c.link = QStringLiteral("A3로"); c.fn = [=] { st->spec.paper = Paper::A3; (*refresh)(); }; }
            else if (a.rotateFits) { c.link = QStringLiteral("방향 바꾸기"); c.fn = [=] { st->spec.landscape = !st->spec.landscape; (*refresh)(); }; }
            else { c.link = st->split ? QStringLiteral("한 장으로") : QStringLiteral("나눠서 %1장").arg(a.splitSheets); c.fn = [=] { st->split = !st->split; (*refresh)(); }; }
            items.push_back(c);
        } else items.push_back({true, QStringLiteral("%1 한 장에 들어갑니다 — 단면 %2 × %3 m").arg(paperLabel(st->spec)).arg(G.lenM, 0, 'f', 2).arg(G.heightM, 0, 'f', 2), QString(), nullptr});
        { QString hs; heightBadgeText(&hs, nullptr);
          if (hs == "ok") items.push_back({true, QStringLiteral("%1 — 표제란에 들어갑니다").arg(st->heightLabel), QString(), nullptr});
          else items.push_back({false, QStringLiteral("높이 기준 확인 전 — 표제란에 ▲ 로 표시됩니다"), QStringLiteral("지정…"), [this] { dlgHeightDatum(); }}); }
        { const std::vector<SGap> gaps = profileGaps(doc.r.profile, SectionFrame(doc.r.line).L);
          double gl = 0; for (const SGap& gp : gaps) gl += gp.s1 - gp.s0;
          if (gaps.empty()) items.push_back({true, QStringLiteral("빈 구간 없음 · 잘린 돌은 H 도구에서(F 단계)"), QString(), nullptr});
          else items.push_back({false, QStringLiteral("빈 구간 %1곳 %2 m — 단면 화면에서 확인하세요").arg(gaps.size()).arg(gl, 0, 'f', 2), QString(), nullptr}); }
        { const QString ext = st->format == 4 ? QStringLiteral("svg") : QString::fromLatin1(kFormatExt[std::clamp(st->format, 0, 3)]);
          const QString fname = QFileInfo(qs8(sheetFileName(modelName.toStdString(), sectionName().toStdString(), st->spec.denom))).completeBaseName() + QLatin1Char('.') + ext;
          items.push_back({true, QStringLiteral("파일 이름 %1 (같은 이름이면 _2)").arg(fname), QString(), nullptr}); }
        check->setItems(items);
        const QString fmtName = st->format == 4 ? QStringLiteral("SVG") : QString::fromLatin1(kFormatName[std::clamp(st->format, 0, 3)]);
        bSave->setText(QStringLiteral("%1장 %2 저장").arg(std::max(1, checkedSheetCount())).arg(fmtName));
        bPdf->setVisible(st->format != 0);
        QString hn = st->format == 4 ? QStringLiteral("그리기용 빈 층 DRAW_SOIL · DRAW_OUTLINE 2개 · 밑그림은 잠금. ")
                     : st->format == 1 ? QStringLiteral("DXF 는 모델 공간 %1 (용지 · 표제란 없이, 레벨선 · A/A′ 포함). 빈 층 2개. ").arg(denomText(st->spec.denom)) : QString();
        fmtRow->setToolTip(hn + QStringLiteral("높이 기준 %1이 표제란(PDF 는 문서 제목에도)에 들어갑니다.").arg(st->heightLabel.mid(3)));
        baseEl->setEnabled(st->showBaseline);
        pv->update();
    };
    onSheetChecksChanged_ = [refresh] { (*refresh)(); };
    pv->onSettled = [=] {
        if (std::abs(pv->zoom - 1.0) > 1e-6) {   // 조판편집 중 휠로 바꾼 크기 = 새 축척
            const int vv = int(std::lround(snapScaleDenom10(st->spec.denom / pv->zoom)));
            pv->zoom = 1;
            if (scale->value() != vv) scale->setValue(vv);
        }
        pv->update();
    };
    pv->paintFn = [=](QPainter& p, const QRect& rc) {
        p.fillRect(rc, theme::Desk);
        SheetGeom G = sheetGeom(doc, st->spec);
        double sc = std::min((rc.width() - 48.0) / G.L.paperW, (rc.height() - 110.0) / G.L.paperH);   // 위 안내 칩 · 아래 도구 줄 자리
        if (sc <= 0.05) return;
        sc *= pv->pageZoom;
        QSizeF pz(G.L.paperW * sc, G.L.paperH * sc);
        QPointF o(rc.left() + (rc.width() - pz.width()) / 2 + pv->viewPan.x(), rc.top() + (rc.height() - pz.height()) / 2 + pv->viewPan.y());
        pv->lastPaperO = o; pv->lastPaperSize = QPointF(pz.width(), pz.height());
        p.save(); p.translate(o);
        p.setClipRect(QRectF(QPointF(0, 0), pz));
        pv->fitPxPerMm = sc;
        pv->zoomCenter = o + QPointF((G.L.plotX + G.L.plotW / 2) * sc, (G.L.plotY + G.L.plotH / 2) * sc);
        paintSheet(p, doc, *st, sc, 0, screenImg, screenGeo, pv->zoom, 0, 0);
        p.restore();
        paintPaperFrame(p, o, pz);
    };
    auto* editAct = new QAction(QStringLiteral("조판편집"), d); editAct->setCheckable(true);
    QObject::connect(editAct, &QAction::toggled, d, [pv](bool on) { pv->editing = on; pv->setCursor(on ? Qt::SizeAllCursor : Qt::ArrowCursor); });
    addDeskFloats(pv, editAct,
                  [=] { int idx = (st->spec.paper == Paper::A3 ? 2 : 0) + (st->spec.landscape ? 0 : 1); idx = (idx + 1) % 4;
                        st->spec.paper = idx >= 2 ? Paper::A3 : Paper::A4; st->spec.landscape = (idx % 2) == 0; (*refresh)(); },
                  [this] { if (sheetSide_) sheetSide_->setVisible(!sheetSide_->isVisible()); });
    QObject::connect(name, &QLineEdit::textChanged, d, [=](const QString& tx) { st->title = tx; nameLab->setText(tx); pv->update(); });
    QObject::connect(pc.size, &QButtonGroup::idClicked, d, [=](int id) { st->spec.paper = id == 1 ? Paper::A3 : Paper::A4; (*refresh)(); });
    QObject::connect(pc.orient, &QButtonGroup::idClicked, d, [=](int id) { st->spec.landscape = id == 0; (*refresh)(); });
    QObject::connect(scale, &QSpinBox::valueChanged, d, [=](int) { st->spec.denom = snapScaleDenom10(scale->value()); (*refresh)(); });
    QObject::connect(fitLink, &QPushButton::clicked, d, [=] { SheetGeom G = sheetGeom(doc, st->spec); if (G.lenM > 0 && G.heightM > 0) scale->setValue(int(std::lround(fitDenomStep10(st->spec, G.lenM, G.heightM)))); });
    QObject::connect(tImg, &QToolButton::toggled, d, [=](bool on) { st->withImage = on; pv->update(); });
    QObject::connect(tLine, &QToolButton::toggled, d, [=](bool on) { st->withLine = on; pv->update(); });
    QObject::connect(tLev, &QToolButton::toggled, d, [=](bool on) { st->withLevels = on; pv->update(); });
    QObject::connect(tTitle, &QToolButton::toggled, d, [=](bool on) { st->withTitle = on; pv->update(); });
    QObject::connect(ptMinor, &QDoubleSpinBox::valueChanged, d, [=](double vv) { st->levelMinorPt = clampLineWeightPt(vv); pv->update(); });
    QObject::connect(ptMajor, &QDoubleSpinBox::valueChanged, d, [=](double vv) { st->levelMajorPt = clampLineWeightPt(vv); pv->update(); });
    QObject::connect(hatchG, &QButtonGroup::idClicked, d, [=](int id) { st->hatchKind = id; });
    QObject::connect(hatchMm, &QDoubleSpinBox::valueChanged, d, [=](double vv) { st->hatchMm = vv; });
    QObject::connect(fmtG, &QButtonGroup::idClicked, d, [=](int id) { st->format = id; (*refresh)(); });
    QObject::connect(cbBase, &QCheckBox::toggled, d, [=](bool on) { st->showBaseline = on; (*refresh)(); });
    QObject::connect(baseEl, &QDoubleSpinBox::valueChanged, d, [=](double vv) { st->baselineEl = vv; pv->update(); });
    QObject::connect(bDefault, &QPushButton::clicked, d, [=] {
        QSettings s;
        s.setValue("sheet/paper", st->spec.paper == Paper::A3 ? 1 : 0); s.setValue("sheet/landscape", st->spec.landscape);
        s.setValue("sheet/denom", st->spec.denom); s.setValue("sheet/format", st->format); s.setValue("sheet/dpi", st->dpi);
        s.setValue("sheet/withImage", st->withImage); s.setValue("sheet/withLine", st->withLine);
        s.setValue("sheet/withLevels", st->withLevels); s.setValue("sheet/withTitle", st->withTitle);
        s.setValue("sheet/levelMinorPt", st->levelMinorPt); s.setValue("sheet/levelMajorPt", st->levelMajorPt);
        s.setValue("sheet/hatchKind", st->hatchKind); s.setValue("sheet/hatchMm", st->hatchMm);
        bDefault->setText(QStringLiteral("★ 저장함"));
    });
    auto doSave = [=, &io, &accepted] {
        io = *st;                                                                   // io = *params(lastSheet_ 가 소유) — 살아 있음
        if (d->property("embed").toBool()) { saveEmbeddedSheet(); return; }       // 끼운 조판: accepted 는 이미 끝난 함수의 지역 변수(검토 C2)
        accepted = true; d->accept();
    };
    QObject::connect(bSave, &QPushButton::clicked, d, doSave);
    QObject::connect(bPdf, &QPushButton::clicked, d, [=] { st->format = 0; (*refresh)(); doSave(); });
    (*refresh)();
    d->resize(1000, 690);
    return d;
}

void MainWindow::showWorkTab() {
    if (!body_) return;
    if (!src_) { showStart(true); return; }
    body_->setCurrentIndex(1);
    if (viewTabs_ && viewTabs_->currentIndex() != 1) {
        QSignalBlocker b(viewTabs_);
        viewTabs_->setCurrentIndex(1);
    }
    setRibbonContext(1);
}

void MainWindow::saveEmbeddedSheet() {
    if (!embeddedSave_) return;
    const std::function<void()> save = embeddedSave_;   // 실행 중 discardSheetTab 이 멤버를 비워도 캡처(doc · src · params)가 살아 있게(검토 C1)
    save();
}

void MainWindow::showSheetTab(QWidget* page) {
    if (!body_ || !page) return;
    page->setParent(nullptr);
    page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    page->show();
    if (sheetHost_ && sheetHost_ != page) {
        body_->removeWidget(sheetHost_);
        sheetHost_->deleteLater();
    }
    sheetHost_ = page;
    if (body_->indexOf(page) < 0) body_->addWidget(page);
    if (viewTabs_) {   // v5 C2: 「도면 ×」 탭은 도면을 열 때 생기고 × 로 닫힌다
        if (viewTabs_->count() < 3) viewTabs_->addTab(kerf::icon(QStringLiteral("sheet"), 16, theme::Hand), QStringLiteral("도면"));
        QSignalBlocker b(viewTabs_);
        viewTabs_->setCurrentIndex(2);
    }
    body_->setCurrentWidget(page);
    setRibbonContext(2);
}

void MainWindow::showSheetTab(QDialog* d, std::function<void()> save) {
    embeddedSave_ = std::move(save);
    d->setProperty("embed", true);
    d->setWindowFlags(Qt::Widget);
    for (auto* b : d->findChildren<QPushButton*>()) {
        if (b->text() == QStringLiteral("전체화면") || b->text() == QStringLiteral("원래 크기")) {
            b->disconnect();
            b->hide();   // v4: 「작업으로」 단추는 없앰 — 문서 탭 「작업」 · Esc 와 같은 일
        }
    }
    for (auto* sc : d->findChildren<QShortcut*>()) sc->deleteLater();
    auto* page = new QWidget; page->setObjectName("sheetPage");
    auto* h = new QHBoxLayout(page); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(0);
    h->addWidget(buildSheetSide());
    h->addWidget(d, 1);
    showSheetTab(page);
    showStatus(QStringLiteral("도면 — 고른 %1장 · 다 되면 「SVG 저장」 · 돌아가기는 「단면」 탭 또는 Esc").arg(std::max(1, checkedSheetCount())));
}

// 조판 탭 왼쪽: 평면도 1줄 + 단면 목록. 누르면 그 도면 조판으로 바뀜(작업 화면 목록과 같은 선택)
QWidget* MainWindow::buildSheetSide() {
    auto* w = new QWidget; w->setObjectName("sidePanel"); w->setAttribute(Qt::WA_StyledBackground);
    w->setFixedWidth(348);   // 스펙 §8
    auto* v = new QVBoxLayout(w); v->setContentsMargins(10, 8, 10, 8); v->setSpacing(6);
    auto* head = new QHBoxLayout; head->setSpacing(6);
    head->addWidget(lbl(QStringLiteral("단면 목록"), "sectionHead"));
    sheetCount_ = lbl(QString::number(sections_.size()), "cap"); head->addWidget(sheetCount_); head->addStretch();
    auto* all = new QPushButton(QStringLiteral("모두")); all->setObjectName("quiet"); all->setFocusPolicy(Qt::NoFocus);
    all->setToolTip(QStringLiteral("모든 도면을 고르기 — 이미 모두 골랐으면 지금 도면만"));
    QObject::connect(all, &QPushButton::clicked, this, [this] {
        const bool allOn = !sheetChecked_.empty() && std::all_of(sheetChecked_.begin(), sheetChecked_.end(), [](bool b) { return b; });
        for (size_t i = 0; i < sheetChecked_.size(); ++i) sheetChecked_[i] = !allOn;
        if (allOn) { const int cur = sheetIsPlan_ ? 0 : current_ + 1; if (cur >= 0 && cur < int(sheetChecked_.size())) sheetChecked_[size_t(cur)] = true; }
        refreshSheetList();
        if (onSheetChecksChanged_) onSheetChecksChanged_();
    });
    auto* exp = new QPushButton(QStringLiteral("내보내기")); exp->setObjectName("quiet"); exp->setFocusPolicy(Qt::NoFocus);
    exp->setToolTip(QStringLiteral("고른 도면을 저장 — 오른쪽 판 「n장 저장」과 같음"));
    QObject::connect(exp, &QPushButton::clicked, this, [this] { saveEmbeddedSheet(); });
    head->addWidget(all); head->addWidget(exp);
    v->addLayout(head);
    auto* find = new QLineEdit; find->setObjectName("sheetFind"); find->setPlaceholderText(QStringLiteral("이름으로 찾기")); find->setClearButtonEnabled(true);
    find->addAction(kerf::icon(QStringLiteral("search"), 14, theme::Faint), QLineEdit::LeadingPosition);
    v->addWidget(find);
    auto* list = new QListWidget; list->setObjectName("sectionList"); list->setFocusPolicy(Qt::NoFocus); list->setSpacing(0);
    QObject::connect(list, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
        const int row = it->data(Qt::UserRole).toInt();
        if (row < 0) return;
        QTimer::singleShot(0, this, [this, row] { if (sheetHost_) openSheetFromList(row); });   // 이 목록은 새 조판으로 바뀌며 지워지므로 다음 차례에. 그사이 탭이 버려졌으면 안 함(검토 M6)
    });
    QObject::connect(find, &QLineEdit::textChanged, list, [this, list](const QString& tx) {
        const QString q = tx.trimmed();
        for (int i = 0; i < list->count(); ++i) {
            auto* it = list->item(i); const int row = it->data(Qt::UserRole).toInt();
            if (row <= 0) continue;
            const QString nm = row - 1 < int(sections_.size()) ? qs8(sections_[size_t(row - 1)].name) : QString();
            it->setHidden(!q.isEmpty() && !nm.contains(q, Qt::CaseInsensitive));
        }
    });
    v->addWidget(list, 1);
    auto* foot = lbl(QStringLiteral("칸 = 저장할 도면 · 줄을 누르면 미리 봄\n축척 · 용지는 도면마다 자기 값"), "faint");
    foot->setWordWrap(true);
    v->addWidget(foot);
    sheetSide_ = w;
    sheetList_ = list;
    w->setVisible(QSettings().value("ui/sectionList", true).toBool());
    refreshSheetList();
    return w;
}

// 목록 한 줄: 고르기 칸 14 + 썸네일 + 이름 + 「1:20 · A4 가로 · 저장 안 됨 / 10/09 15:10」(스펙 §8)
QWidget* MainWindow::sheetRowWidget(int row) {
    auto* w = new QWidget; w->setObjectName("secRow");
    auto* h = new QHBoxLayout(w); h->setContentsMargins(6, 5, 6, 5); h->setSpacing(8);
    auto* cb = new QCheckBox; cb->setFocusPolicy(Qt::NoFocus); cb->setToolTip(QStringLiteral("칸 = 「n장 저장」에 넣을 도면"));
    cb->setChecked(row >= 0 && row < int(sheetChecked_.size()) && sheetChecked_[size_t(row)]);
    QObject::connect(cb, &QCheckBox::toggled, this, [this, row](bool on) {
        if (row >= 0 && row < int(sheetChecked_.size())) sheetChecked_[size_t(row)] = on;
        if (onSheetChecksChanged_) onSheetChecksChanged_();
    });
    h->addWidget(cb);
    auto* th = new QLabel; th->setFixedSize(64, 30); th->setAlignment(Qt::AlignCenter);
    if (row == 0) { th->setText(QStringLiteral("평면")); th->setStyleSheet("QLabel{background:#F5F4ED;border:1px solid #DEDCD1;border-radius:6px;color:#5E5D59;font-size:11px;}"); }
    else {
        th->setStyleSheet("QLabel{background:#FFFFFF;border:1px solid #DEDCD1;border-radius:6px;}");
        if (auto f = thumbs_.find(row - 1); f != thumbs_.end() && !f->second.isNull())
            th->setPixmap(QPixmap::fromImage(f->second.scaled(62, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    }
    h->addWidget(th);
    const QString nm = row == 0 ? QStringLiteral("평면도") : (row - 1 < int(sections_.size()) ? qs8(sections_[size_t(row - 1)].name) : QString());
    auto* txt = new QWidget; auto* tv = new QVBoxLayout(txt); tv->setContentsMargins(0, 0, 0, 0); tv->setSpacing(0);
    tv->addWidget(lbl(nm, "secName"));
    tv->addWidget(lbl(sheetStatusText(row), "monoFaint"));
    if (row > 0 && row - 1 < int(sections_.size()) && !sections_[size_t(row - 1)].note.empty()) tv->addWidget(lbl(qs8(sections_[size_t(row - 1)].note), "faint"));
    h->addWidget(txt, 1);
    return w;
}

QString MainWindow::sheetStatusText(int row) const {
    QSettings st;
    const QString nm = row > 0 && row - 1 < int(sections_.size()) ? qs8(sections_[size_t(row - 1)].name) : QString();
    const QString k = modelKey(path_) + (row == 0 ? QStringLiteral("sheet/plan/") : QStringLiteral("sheet/%1/").arg(nm));
    double denom = st.value(k + "denom", 0.0).toDouble(); QString paper = st.value(k + "paper").toString();
    if (row == 0 && lastPlanSheet_) { denom = lastPlanSheet_->spec.denom; paper = paperLabel(lastPlanSheet_->spec); }
    else if (row > 0 && !sheetIsPlan_ && row - 1 == current_ && lastSheet_) { denom = lastSheet_->spec.denom; paper = paperLabel(lastSheet_->spec); }
    const QDateTime t = QDateTime::fromString(st.value(k + "saved").toString(), Qt::ISODate);
    const QString when = t.isValid() ? t.toString(QStringLiteral("MM/dd HH:mm")) : QStringLiteral("저장 안 됨");
    return (denom > 0 ? QStringLiteral("%1 · %2").arg(denomText(denom), paper) : QStringLiteral("—")) + QStringLiteral(" · ") + when;
}

void MainWindow::noteSheetSaved(const QString& prefix, const SheetSpec& spec) {
    QSettings st; const QString k = modelKey(path_) + prefix;
    st.setValue(k + "denom", spec.denom); st.setValue(k + "paper", paperLabel(spec)); st.setValue(k + "saved", QDateTime::currentDateTime().toString(Qt::ISODate));
    refreshSheetList();
}

int MainWindow::checkedSheetCount() const { return int(std::count(sheetChecked_.begin(), sheetChecked_.end(), true)); }

void MainWindow::setAllSheetsChecked(bool on) { sheetChecked_.assign(1 + sections_.size(), on); refreshSheetList(); }

// 고른 도면 전부를 dir 에(E4): 평면도는 마지막 평면 조판 설정(없으면 기본), 단면은 base(용지 · 항목 · 형식 · pt)에 도면마다 자기 축척(저장된 값 → 없으면 맞춤, 지금 연 도면은 판의 값).
// 동기 — 단면마다 selectSection + computeNow(최종). 끝나면 고른 단면으로 되돌린다
int MainWindow::saveCheckedSheets(const QString& dir, const SheetParams& base, QStringList* log) {
    if (!src_ || dir.isEmpty()) return 0;
    QDir().mkpath(dir);
    const QString model = QFileInfo(path_).completeBaseName();
    std::vector<std::string> existing;
    for (const QString& n : QDir(dir).entryList(QDir::Files)) existing.push_back(n.toStdString());
    const QString ext = base.format == 4 ? QStringLiteral("svg") : QString::fromLatin1(kFormatExt[std::clamp(base.format, 0, 3)]);
    auto nameFor = [&](const QString& sheet, double denom) {
        QString n = QFileInfo(qs8(sheetFileName(model.toStdString(), sheet.toStdString(), denom))).completeBaseName() + QLatin1Char('.') + ext;
        n = qs8(uniqueSheetFileName(n.toStdString(), existing));
        existing.push_back(n.toStdString());
        return QDir(dir).filePath(n);
    };
    if (sheetChecked_.size() != 1 + sections_.size()) sheetChecked_.resize(1 + sections_.size(), false);
    const int keepCur = current_; const bool keepPlan = sheetIsPlan_;
    QSettings st; const QString mk = modelKey(path_);
    int saved = 0;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    for (size_t row = 0; row < sheetChecked_.size(); ++row) {
        if (!sheetChecked_[row]) continue;
        QString msg, f; bool ok = false;
        if (row == 0) {
            PlanSheetParams pp = lastPlanSheet_ ? *lastPlanSheet_ : defaultPlanSheetParams();
            pp.format = base.format;
            f = nameFor(QStringLiteral("평면도"), pp.spec.denom);
            ok = exportPlanSheet(*src_, pp, f, &msg, nullptr, {});
            if (ok) { st.setValue(mk + "sheet/plan/denom", pp.spec.denom); st.setValue(mk + "sheet/plan/paper", paperLabel(pp.spec)); st.setValue(mk + "sheet/plan/saved", QDateTime::currentDateTime().toString(Qt::ISODate)); }
        } else {
            const int i = int(row) - 1;
            if (i >= int(sections_.size())) continue;
            selectSection(i);
            QString e;
            if (!plan_->hasLine() || !computeNow(plan_->line(), &e)) { if (log) *log << QStringLiteral("sheet-batch %1: 단면 계산 실패 %2").arg(qs8(sections_[size_t(i)].name), e); continue; }
            SheetParams sp = base;
            const QString nm = sectionName();
            sp.title = QStringLiteral("%1 단면도").arg(nm);
            if (!sections_[size_t(i)].note.empty()) sp.title += QStringLiteral(" · ") + qs8(sections_[size_t(i)].note);
            const SectionDoc doc = section_->doc();
            if (!(i == keepCur && !keepPlan)) {   // 지금 연 도면은 판의 축척 그대로, 나머지는 저장된 값 → 없으면 맞춤(스펙 §8 「축척은 도면마다 자기 값」)
                const double saved0 = st.value(mk + QStringLiteral("sheet/%1/denom").arg(nm), 0.0).toDouble();
                const SheetGeom G = sheetGeom(doc, sp.spec);
                sp.spec.denom = saved0 > 0 ? saved0 : (G.lenM > 0 && G.heightM > 0 ? fitDenomStep10(sp.spec, G.lenM, G.heightM) : sp.spec.denom);
            }
            f = nameFor(nm, sp.spec.denom);
            ok = base.format == 4 ? exportSectionSvg(doc, *src_, sp, f, &msg) : exportSheet(doc, *src_, sp, f, &msg, nullptr, {});
            if (ok) { const QString k = mk + QStringLiteral("sheet/%1/").arg(nm); st.setValue(k + "denom", sp.spec.denom); st.setValue(k + "paper", paperLabel(sp.spec)); st.setValue(k + "saved", QDateTime::currentDateTime().toString(Qt::ISODate)); }
        }
        if (log) *log << QStringLiteral("sheet-batch %1: %2").arg(ok ? QStringLiteral("ok") : QStringLiteral("FAIL"), ok ? f : msg);
        if (ok) ++saved;
    }
    if (keepCur >= 0 && keepCur < int(sections_.size()) && keepCur != current_) { selectSection(keepCur); QString e; if (plan_->hasLine()) computeNow(plan_->line(), &e); }
    sheetIsPlan_ = keepPlan;
    QApplication::restoreOverrideCursor();
    refreshSheetList();
    return saved;
}

void MainWindow::noteBatchDone(int n, const QString& dir) {
    if (!sheetDoneLabel_) return;
    sheetDoneLabel_->setText(QStringLiteral("%1장 저장됨 · <a href=\"%2\" style=\"color:#9C4A2F\">폴더 열기</a>").arg(n).arg(QUrl::fromLocalFile(dir).toString()));
    sheetDoneLabel_->show();
}

void MainWindow::refreshSheetList() {
    if (!sheetList_) return;
    const size_t rows = 1 + sections_.size();
    if (sheetChecked_.size() != rows) sheetChecked_.resize(rows, false);
    if (std::none_of(sheetChecked_.begin(), sheetChecked_.end(), [](bool b) { return b; })) {
        const int cur = sheetIsPlan_ ? 0 : (current_ >= 0 ? current_ + 1 : 0);
        if (cur < int(rows)) sheetChecked_[size_t(cur)] = true;
    }
    QSignalBlocker b(sheetList_);
    sheetList_->clear();
    auto caption = [this](const QString& tx) {
        auto* it = new QListWidgetItem(sheetList_); it->setFlags(Qt::NoItemFlags); it->setData(Qt::UserRole, -1); it->setSizeHint(QSize(10, 22));
        sheetList_->setItemWidget(it, lbl(tx, "listCaption"));
    };
    auto rowItem = [this](int row, int hh) {
        auto* it = new QListWidgetItem(sheetList_); it->setData(Qt::UserRole, row); it->setSizeHint(QSize(10, hh));
        sheetList_->setItemWidget(it, sheetRowWidget(row));
        return it;
    };
    caption(QStringLiteral("평면"));
    rowItem(0, 46)->setSelected(sheetIsPlan_);
    caption(QStringLiteral("단면"));
    for (size_t i = 0; i < sections_.size(); ++i) rowItem(int(i) + 1, sections_[i].note.empty() ? 46 : 60)->setSelected(!sheetIsPlan_ && int(i) == current_);
    if (sheetCount_) sheetCount_->setText(QString::number(sections_.size()));
}

void MainWindow::openSheetFromList(int row) {
    if (!src_) return;
    if (row <= 0) {
        if (sheetIsPlan_) return;
        reusePlanSheet_ = true;
        dlgPlanSheet();
        return;
    }
    const int i = row - 1;
    if (i >= int(sections_.size())) return;
    if (!sheetIsPlan_ && i == current_) return;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    selectSection(i);
    QString err;
    const bool ok = plan_->hasLine() && computeNow(plan_->line(), &err);   // 조판은 최종 단면으로 바로 계산
    QApplication::restoreOverrideCursor();
    if (!ok) { report(false, QStringLiteral("단면 계산 실패: ") + err); refreshSheetList(); return; }
    dlgSheet();
}

void MainWindow::dlgSheet() {
    if (!src_) { showStatus(QStringLiteral("모델을 연 뒤에 도면을 만드세요")); return; }
    if (!section_->hasResult()) {
        if (!plan_->hasLine()) { showStatus(QStringLiteral("단면이 없습니다. 평면에서 단면선을 그린 뒤 단면도 조판을 여세요")); return; }
        QString err;
        if (!computeNow(plan_->line(), &err)) { QMessageBox::warning(this, windowTitle(), err); return; }
    }
    if (!ensureFinalSection()) return;
    syncCurrentSection();
    SheetParams sp = defaultSheetParams();
    if (lastSheet_) {   // 조판 목록에서 다른 단면으로: 용지·넣을 것·형식은 그대로, 축척만 새 단면에 맞춤
        const SheetParams& o = *lastSheet_;
        sp.spec.paper = o.spec.paper; sp.spec.landscape = o.spec.landscape;
        sp.format = o.format; sp.dpi = o.dpi; sp.split = o.split;
        sp.withImage = o.withImage; sp.withLine = o.withLine; sp.withLevels = o.withLevels; sp.withTitle = o.withTitle;
        sp.showBaseline = o.showBaseline;
        sp.levelMinorPt = o.levelMinorPt; sp.levelMajorPt = o.levelMajorPt; sp.hatchKind = o.hatchKind; sp.hatchMm = o.hatchMm;
        if (section_->hasResult()) {
            const SheetGeom G0 = sheetGeom(section_->doc(), sp.spec);
            if (G0.lenM > 0 && G0.heightM > 0) sp.spec.denom = fitDenomStep10(sp.spec, G0.lenM, G0.heightM);
        }
    }
    auto params = std::make_shared<SheetParams>(sp);
    lastSheet_ = params;
    sheetIsPlan_ = false;
    bool acc = false;
    QDialog* d = buildSheetDialog(*params, acc);
    SectionDoc doc = section_->doc();
    auto src = src_;
    showSheetTab(d, [this, params, doc, src] {
        SheetParams sp = *params;
        if (checkedSheetCount() > 1) {   // v5 E4: 고른 도면 전부를 한 폴더에(이름 sheetFileName · 겹치면 _2)
            const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("고른 도면 %1장을 저장할 폴더").arg(checkedSheetCount()), QSettings().value("sheet/batchDir").toString());
            if (dir.isEmpty() || src_ != src) return;
            QSettings().setValue("sheet/batchDir", dir);
            QStringList lg; const int n = saveCheckedSheets(dir, sp, &lg);
            report(n > 0, QStringLiteral("%1장 저장됨 — %2").arg(n).arg(QDir::toNativeSeparators(dir)));
            noteBatchDone(n, dir);
            return;
        }
        const bool svg = sp.format == 4;
        QString base = QFileInfo(path_).completeBaseName() + "_" + sectionName().replace(QStringLiteral("–"), "-").replace(QStringLiteral("′"), "'").remove('\'');
        QString ext = svg ? QStringLiteral("svg") : QString::fromLatin1(kFormatExt[std::clamp(sp.format, 0, 3)]);
        QString name = svg ? QStringLiteral("SVG") : QString::fromLatin1(kFormatName[std::clamp(sp.format, 0, 3)]);
        QString filter = QStringLiteral("%1 (*.%2)").arg(name, ext);
        QString f = askSavePath("sheet", base + QStringLiteral("_1-%1.").arg(sp.spec.denom, 0, 'f', 0) + ext, filter);
        if (f.isEmpty() || src_ != src) return;   // 저장 창을 띄운 사이 모델이 바뀌었으면 옛 도면을 저장하지 않음(검토 C1)
        if (QFileInfo(f).suffix().isEmpty()) f += "." + ext;
        runTask(QStringLiteral("도면 %1 만드는 중").arg(name),
            [this, doc, src, sp, f, svg](QString* m) {
                if (svg) return exportSectionSvg(doc, *src, sp, f, m);
                return exportSheet(doc, *src, sp, f, m, &cancelTask_, [this](double x) { QMetaObject::invokeMethod(this, [this, x] { setProgress(x); }, Qt::QueuedConnection); });
            },
            [this, sp, nm = sectionName()](bool ok, const QString& m) { report(ok, m); if (ok) noteSheetSaved(QStringLiteral("sheet/%1/").arg(nm), sp.spec); });
    });
}

PlanSheetParams MainWindow::defaultPlanSheetParams() const {
    PlanSheetParams sp;
    QSettings st;
    sp.spec.paper = st.value("plansheet/paper", st.value("sheet/paper", 0).toInt()).toInt() == 1 ? Paper::A3 : Paper::A4;
    sp.spec.landscape = st.value("plansheet/landscape", true).toBool();
    sp.spec.denom = snapScaleDenom10(st.value("plansheet/denom", 100.0).toDouble());
    sp.format = std::clamp(st.value("plansheet/format", 0).toInt(), 0, 3);
    sp.dpi = std::max(600.0, st.value("plansheet/dpi", 600.0).toDouble());
    sp.wholeModel = false;
    if (!plan_->viewRectLocal(sp.area)) { sp.wholeModel = true; if (src_) sp.area = src_->bounds; }
    SheetParams sec = defaultSheetParams();
    sp.srsLabel = sec.srsLabel;
    sp.heightLabel = sec.heightLabel;
    sp.date = sec.date;
    sp.title = QFileInfo(path_).completeBaseName();
    if (sp.title.isEmpty()) sp.title = QStringLiteral("평면도");
    else sp.title += QStringLiteral(" 평면도");
    if (src_ && src_->bounds.valid()) {   // 3D 로 기울인 왼쪽 화면은 모델보다 큰 상자를 낼 수 있어 먼저 모델로 자름
        sp.area.mn.x = std::max(sp.area.mn.x, src_->bounds.mn.x); sp.area.mn.y = std::max(sp.area.mn.y, src_->bounds.mn.y);
        sp.area.mx.x = std::min(sp.area.mx.x, src_->bounds.mx.x); sp.area.mx.y = std::min(sp.area.mx.y, src_->bounds.mx.y);
        if (!(sp.area.mx.x > sp.area.mn.x && sp.area.mx.y > sp.area.mn.y)) sp.area = src_->bounds;
    }
    double rw = std::max(0.0, sp.area.mx.x - sp.area.mn.x), rh = std::max(0.0, sp.area.mx.y - sp.area.mn.y);
    if (rw > 0 && rh > 0) sp.spec.denom = fitDenomStep10(sp.spec, rw, rh);
    return sp;
}

QDialog* MainWindow::buildPlanSheetDialog(PlanSheetParams& io, const QImage& preview0, double prevX0, double prevY1, double prevRes, bool& accepted) {
    accepted = false;
    auto* d = new QDialog(this);
    d->setWindowTitle(QStringLiteral("평면도"));
    d->setObjectName("sheetDialog");
    auto st = std::make_shared<PlanSheetParams>(io);
    auto img = std::make_shared<QImage>(preview0);
    auto gx0 = std::make_shared<double>(prevX0);
    auto gy1 = std::make_shared<double>(prevY1);
    auto gres = std::make_shared<double>(prevRes);
    auto* outer = new QHBoxLayout(d); outer->setContentsMargins(0, 0, 0, 0); outer->setSpacing(0);
    auto* pv = new SheetPreview; pv->setMinimumSize(470, 330);
    bindImageDrag(pv, &st->imgDxMm, &st->imgDyMm);
    outer->addWidget(pv, 1);
    auto* side = new QWidget; side->setObjectName("dialogSide"); side->setAttribute(Qt::WA_StyledBackground); side->setFixedWidth(320);
    auto* sv = new QVBoxLayout(side); sv->setContentsMargins(0, 0, 0, 0); sv->setSpacing(0);
    auto* scroll = new QScrollArea; scroll->setObjectName("sideScroll"); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto* body = new QWidget; body->setObjectName("sideBody"); body->setAttribute(Qt::WA_StyledBackground);
    auto* v = new QVBoxLayout(body); v->setContentsMargins(12, 10, 12, 10); v->setSpacing(6);
    v->addWidget(lbl(QStringLiteral("고른 도면 · 평면도"), "cap"));
    auto* nameLab = lbl(st->title, "sheetName"); nameLab->setWordWrap(true); v->addWidget(nameLab);
    auto* subLab = lbl(QString(), "hint"); subLab->setWordWrap(true); v->addWidget(subLab);
    // 도면 항목(평면도는 모두 그린다 — FINAL_PLAN §4: 그림 칸에 단면선 없음 · 진북 필수)
    v->addWidget(sideCaption(QStringLiteral("도면 항목")));
    { auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(0);
        for (auto* tb : {sheetTile(QStringLiteral("image-layer"), QStringLiteral("정사영상"), QStringLiteral("그림 칸에는 위에서 본 정사영상만 — 단면선은 그리지 않습니다")),
                         sheetTile(QStringLiteral("title-block"), QStringLiteral("표제란"), QStringLiteral("표제란 · 축척 막대 — 단면도와 같은 자리")),
                         sheetTile(QStringLiteral("scalebar"), QStringLiteral("축척 막대"), QStringLiteral("축척 막대")),
                         sheetTile(QStringLiteral("north"), QStringLiteral("진북"), QStringLiteral("진북 28 mm — 글자는 「진북」만"))}) {
            tb->setChecked(true); tb->setEnabled(false); h->addWidget(tb); }
        h->setSpacing(8);
        h->addStretch(); v->addWidget(row); }
    v->addWidget(sideCaption(QStringLiteral("도면 정보")));
    auto* g = new QGridLayout; g->setContentsMargins(0, 0, 0, 0); g->setHorizontalSpacing(10); g->setVerticalSpacing(8); g->setColumnMinimumWidth(0, 40); g->setColumnStretch(1, 1);
    int r = 0;
    auto* name = new QLineEdit(st->title);
    g->addWidget(lbl(QStringLiteral("도면명"), "hint"), r, 0); g->addWidget(name, r++, 1);
    PaperChips pc = paperChips(st->spec);
    g->addWidget(lbl(QStringLiteral("용지"), "hint"), r, 0); g->addWidget(pc.w, r++, 1);
    auto* scale = scaleSpin(st->spec.denom);
    auto* fitLink = new QPushButton; fitLink->setObjectName("quietLink"); fitLink->setFlat(true); fitLink->setFocusPolicy(Qt::NoFocus); fitLink->setCursor(Qt::PointingHandCursor);
    fitLink->setToolTip(QStringLiteral("범위가 그림 칸에 들어가는 가장 작은 10 단위"));
    { auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(6);
        h->addWidget(scaleWithArrows(scale), 1); h->addWidget(fitLink);
        g->addWidget(lbl(QStringLiteral("축척"), "hint"), r, 0); g->addWidget(row, r++, 1); }
    g->addWidget(scaleGridWidget(scale), r++, 1);
    auto* area = new QComboBox;
    area->addItem(QStringLiteral("지금 왼쪽 화면")); area->addItem(QStringLiteral("모델 전체"));
    Box3 view; bool hasView = plan_->viewRectLocal(view);
    if (!hasView) area->setEnabled(false);
    area->setCurrentIndex(st->wholeModel || !hasView ? 1 : 0);
    g->addWidget(lbl(QStringLiteral("범위"), "hint"), r, 0); g->addWidget(area, r++, 1);
    v->addLayout(g);
    v->addWidget(sideCaption(QStringLiteral("선 · 글자")));
    { auto* g2 = new QGridLayout; g2->setContentsMargins(0, 0, 0, 0); g2->setHorizontalSpacing(10); g2->setVerticalSpacing(6); g2->setColumnMinimumWidth(0, 40); g2->setColumnStretch(1, 1);
        g2->addWidget(lbl(QStringLiteral("좌표 숫자"), "hint"), 0, 0); g2->addWidget(lbl(QStringLiteral("그림 칸 밖 · 4 m마다 · 모노"), "faint"), 0, 1);
        g2->addWidget(lbl(QStringLiteral("진북"), "hint"), 1, 0); g2->addWidget(lbl(QStringLiteral("28 mm · 글자는 「진북」만"), "faint"), 1, 1);
        v->addLayout(g2); }
    v->addWidget(sideCaption(QStringLiteral("형식")));
    auto* fmtG = new QButtonGroup(d); fmtG->setExclusive(true);
    { auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(2);
        h->addWidget(chipBtn(QStringLiteral("SVG"), fmtG, 4, "first", 48)); h->addWidget(chipBtn(QStringLiteral("DXF"), fmtG, 1, nullptr, 48)); h->addWidget(chipBtn(QStringLiteral("PDF"), fmtG, 0, nullptr, 48));
        h->addWidget(chipBtn(QStringLiteral("PNG"), fmtG, 2, nullptr, 48)); h->addWidget(chipBtn(QStringLiteral("TIFF"), fmtG, 3, "last", 48)); h->addStretch();
        v->addWidget(row); }
    v->addWidget(sideCaption(QStringLiteral("도면 점검 · 저장을 막지 않음")));
    auto* check = new SheetCheckBox; v->addWidget(check);
    v->addStretch(1);
    scroll->setWidget(body);
    sv->addWidget(scroll, 1);
    auto* bDefault = new QPushButton(QStringLiteral("☆ 기본으로")); bDefault->setObjectName("quiet"); bDefault->setFocusPolicy(Qt::NoFocus); bDefault->setToolTip(QStringLiteral("이 설정(용지 · 축척 · 항목 · 형식 · 선 굵기)을 다음 도면의 기본으로"));
    auto* bSave = new QPushButton; bSave->setObjectName("primary"); bSave->setDefault(true);
    auto* bPdf = new QPushButton(QStringLiteral("PDF")); bPdf->setFocusPolicy(Qt::NoFocus);
    { QLabel* done = nullptr; sv->addWidget(sideBottom(bDefault, bSave, bPdf, &done)); sheetDoneLabel_ = done; }
    outer->addWidget(side);

    auto reloadImage = [=] {
        if (!src_) return;
        QImage q; double x0 = 0, y1 = 0, res = 0;
        if (renderPlanPreview(*src_, st->spec, st->area, pv->zoom, st->imgDxMm, st->imgDyMm, q, x0, y1, res)) { *img = q; *gx0 = x0; *gy1 = y1; *gres = res; }
    };
    const QString modelName = QFileInfo(path_).completeBaseName();
    auto refresh = std::make_shared<std::function<void()>>();
    *refresh = [=] {
        st->spec.denom = snapScaleDenom10(scale->value());
        if (int(st->spec.denom) != scale->value()) { QSignalBlocker bl(scale); scale->setValue(int(st->spec.denom)); }
        syncPaperChips(pc, st->spec);
        st->wholeModel = area->currentIndex() == 1 || !hasView;
        Box3 raw = st->wholeModel || !hasView ? (src_ ? src_->bounds : st->area) : view;
        if (src_ && src_->bounds.valid()) {
            raw.mn.x = std::max(raw.mn.x, src_->bounds.mn.x); raw.mn.y = std::max(raw.mn.y, src_->bounds.mn.y);
            raw.mx.x = std::min(raw.mx.x, src_->bounds.mx.x); raw.mx.y = std::min(raw.mx.y, src_->bounds.mx.y);
        }
        st->area = raw;
        if (auto* b = fmtG->button(st->format)) { QSignalBlocker bl(b); b->setChecked(true); }
        PlanWindow W = planWindow(*st, 0);
        double rw = std::max(0.0, st->area.mx.x - st->area.mn.x), rh = std::max(0.0, st->area.mx.y - st->area.mn.y);
        double fitD = fitDenomStep10(st->spec, rw, rh);
        fitLink->setText(QStringLiteral("맞춤 %1").arg(denomText(fitD)));
        nameLab->setText(st->title);
        subLab->setText(QStringLiteral("%1 · %2 · 그림 칸 %3 × %4 mm · 단면선 없음 · 진북 있음").arg(paperLabel(st->spec), denomText(st->spec.denom)).arg(W.L.plotW, 0, 'f', 0).arg(W.L.plotH, 0, 'f', 0));
        std::vector<CheckItem> items;
        if (st->spec.denom + 0.1 < fitD)
            items.push_back({false, QStringLiteral("범위 %1 × %2 m 가 %3 칸보다 큽니다 — 가운데만 보입니다").arg(rw, 0, 'f', 1).arg(rh, 0, 'f', 1).arg(denomText(st->spec.denom)),
                             QStringLiteral("맞춤 %1").arg(denomText(fitD)), [=] { scale->setValue(int(std::lround(fitD))); }});
        else items.push_back({true, QStringLiteral("범위 %1 × %2 m 가 %3에서 그림 칸을 채웁니다").arg(rw, 0, 'f', 1).arg(rh, 0, 'f', 1).arg(denomText(st->spec.denom)), QString(), nullptr});
        items.push_back({true, QStringLiteral("그림 칸에 단면선 없음 · 정사영상만"), QString(), nullptr});
        items.push_back({true, QStringLiteral("진북 있음 · 바깥 좌표 숫자 · 범례 · 자는 단면도와 같은 자리"), QString(), nullptr});
        { const QString ext = st->format == 4 ? QStringLiteral("svg") : QString::fromLatin1(kFormatExt[std::clamp(st->format, 0, 3)]);
          const QString fname = QFileInfo(qs8(sheetFileName(modelName.toStdString(), std::string("평면도"), st->spec.denom))).completeBaseName() + QLatin1Char('.') + ext;
          items.push_back({true, QStringLiteral("파일 이름 %1 (같은 이름이면 _2)").arg(fname), QString(), nullptr}); }
        check->setItems(items);
        const QString fmtName = st->format == 4 ? QStringLiteral("SVG") : QString::fromLatin1(kFormatName[std::clamp(st->format, 0, 3)]);
        bSave->setText(QStringLiteral("%1장 %2 저장").arg(std::max(1, checkedSheetCount())).arg(fmtName));
        bPdf->setVisible(st->format != 0);
        pv->update();
    };
    onSheetChecksChanged_ = [refresh] { (*refresh)(); };
    pv->onSettled = [=] {
        if (std::abs(pv->zoom - 1.0) > 1e-6) {   // 조판편집 중 휠로 바꾼 크기 = 새 축척(보이는 그림과 축척 표기가 같게)
            const int vv = int(std::lround(snapScaleDenom10(st->spec.denom / pv->zoom)));
            pv->zoom = 1;
            if (scale->value() != vv) { scale->setValue(vv); return; }   // valueChanged → 새로 그림
            (*refresh)();
        }
        reloadImage(); pv->update();
    };
    pv->paintFn = [=](QPainter& p, const QRect& rc) {
        p.fillRect(rc, theme::Desk);
        PlanWindow W = planWindow(*st, 0);
        double sc = std::min((rc.width() - 48.0) / W.L.paperW, (rc.height() - 110.0) / W.L.paperH);
        if (sc <= 0.05) return;
        sc *= pv->pageZoom;
        QSizeF pz(W.L.paperW * sc, W.L.paperH * sc);
        QPointF o(rc.left() + (rc.width() - pz.width()) / 2 + pv->viewPan.x(), rc.top() + (rc.height() - pz.height()) / 2 + pv->viewPan.y());
        pv->lastPaperO = o; pv->lastPaperSize = QPointF(pz.width(), pz.height());
        p.save(); p.translate(o);
        p.setClipRect(QRectF(QPointF(0, 0), pz));
        Vec3 origin = src_ ? src_->srs.origin : Vec3();
        pv->fitPxPerMm = sc;
        pv->zoomCenter = o + QPointF((W.L.plotX + W.L.plotW / 2) * sc, (W.L.plotY + W.L.plotH / 2) * sc);
        paintPlanSheet(p, *st, sc, 0, *img, *gx0, *gy1, *gres, origin, pv->zoom, 0, 0);
        p.restore();
        paintPaperFrame(p, o, pz);
    };
    auto* editAct = new QAction(QStringLiteral("조판편집"), d); editAct->setCheckable(true);
    QObject::connect(editAct, &QAction::toggled, d, [pv](bool on) { pv->editing = on; pv->setCursor(on ? Qt::SizeAllCursor : Qt::ArrowCursor); });
    addDeskFloats(pv, editAct,
                  [=] { int idx = (st->spec.paper == Paper::A3 ? 2 : 0) + (st->spec.landscape ? 0 : 1); idx = (idx + 1) % 4;
                        st->spec.paper = idx >= 2 ? Paper::A3 : Paper::A4; st->spec.landscape = (idx % 2) == 0; (*refresh)(); reloadImage(); pv->update(); },
                  [this] { if (sheetSide_) sheetSide_->setVisible(!sheetSide_->isVisible()); });
    QObject::connect(name, &QLineEdit::textChanged, d, [=](const QString& tx) { st->title = tx; nameLab->setText(tx); pv->update(); });
    QObject::connect(pc.size, &QButtonGroup::idClicked, d, [=](int id) { st->spec.paper = id == 1 ? Paper::A3 : Paper::A4; (*refresh)(); reloadImage(); pv->update(); });
    QObject::connect(pc.orient, &QButtonGroup::idClicked, d, [=](int id) { st->spec.landscape = id == 0; (*refresh)(); reloadImage(); pv->update(); });
    QObject::connect(scale, &QSpinBox::valueChanged, d, [=](int) { (*refresh)(); reloadImage(); pv->update(); });
    QObject::connect(fitLink, &QPushButton::clicked, d, [=] {
        double rw = std::max(0.0, st->area.mx.x - st->area.mn.x), rh = std::max(0.0, st->area.mx.y - st->area.mn.y);
        st->spec.denom = fitDenomStep10(st->spec, rw, rh);
        QSignalBlocker bl(scale); scale->setValue(int(std::lround(st->spec.denom)));
        (*refresh)(); reloadImage(); pv->update();
    });
    QObject::connect(area, &QComboBox::currentIndexChanged, d, [=](int) { (*refresh)(); reloadImage(); pv->update(); });
    QObject::connect(fmtG, &QButtonGroup::idClicked, d, [=](int id) { st->format = id; (*refresh)(); });
    QObject::connect(bDefault, &QPushButton::clicked, d, [=] {
        QSettings s;
        s.setValue("plansheet/paper", st->spec.paper == Paper::A3 ? 1 : 0); s.setValue("plansheet/landscape", st->spec.landscape);
        s.setValue("plansheet/denom", st->spec.denom); s.setValue("plansheet/format", st->format);
        bDefault->setText(QStringLiteral("★ 저장함"));
    });
    auto doSave = [=, &io, &accepted] {
        QSettings s;
        s.setValue("plansheet/paper", st->spec.paper == Paper::A3 ? 1 : 0);
        s.setValue("plansheet/landscape", st->spec.landscape);
        s.setValue("plansheet/denom", st->spec.denom);
        s.setValue("plansheet/format", st->format);
        io = *st;
        if (d->property("embed").toBool()) { saveEmbeddedSheet(); return; }       // 검토 C2
        accepted = true; d->accept();
    };
    QObject::connect(bSave, &QPushButton::clicked, d, doSave);
    QObject::connect(bPdf, &QPushButton::clicked, d, [=] { st->format = 0; (*refresh)(); doSave(); });
    (*refresh)();
    reloadImage();
    pv->update();
    d->resize(1000, 690);
    return d;
}

void MainWindow::dlgPlanSheet() {
    if (!src_) { showStatus(QStringLiteral("모델을 연 뒤에 평면도를 만드세요")); return; }
    PlanSheetParams sp = (reusePlanSheet_ && lastPlanSheet_) ? *lastPlanSheet_ : defaultPlanSheetParams();
    reusePlanSheet_ = false;
    Box3 box = sp.wholeModel ? src_->bounds : sp.area;
    double w = std::max(0.0, box.mx.x - box.mn.x), h = std::max(0.0, box.mx.y - box.mn.y);
    double res = std::max({1e-4, groundResolution(sp.spec.denom, 400.0), std::max(w, h) / 8000.0});
    QImage prev; double x0 = 0, y1 = 0; QString em;
    renderPlanOrtho(*src_, box, res, prev, x0, y1, &em, nullptr);
    auto params = std::make_shared<PlanSheetParams>(sp);
    lastPlanSheet_ = params;
    sheetIsPlan_ = true;
    bool acc = false;
    QDialog* d = buildPlanSheetDialog(*params, prev, x0, y1, res, acc);
    auto src = src_;
    showSheetTab(d, [this, params, src] {
        PlanSheetParams sp = *params;
        if (checkedSheetCount() > 1) {   // v5 E4
            const QString dir = QFileDialog::getExistingDirectory(this, QStringLiteral("고른 도면 %1장을 저장할 폴더").arg(checkedSheetCount()), QSettings().value("sheet/batchDir").toString());
            if (dir.isEmpty() || src_ != src) return;
            QSettings().setValue("sheet/batchDir", dir);
            SheetParams base = lastSheet_ ? *lastSheet_ : defaultSheetParams(); base.format = sp.format;
            QStringList lg; const int n = saveCheckedSheets(dir, base, &lg);
            report(n > 0, QStringLiteral("%1장 저장됨 — %2").arg(n).arg(QDir::toNativeSeparators(dir)));
            noteBatchDone(n, dir);
            return;
        }
        const bool svg = sp.format == 4;
        QString ext = svg ? QStringLiteral("svg") : QString::fromLatin1(kFormatExt[std::clamp(sp.format, 0, 3)]);
        QString name = svg ? QStringLiteral("SVG") : QString::fromLatin1(kFormatName[std::clamp(sp.format, 0, 3)]);
        QString filter = QStringLiteral("%1 (*.%2)").arg(name, ext);
        QString f = askSavePath("plansheet", QFileInfo(path_).completeBaseName() + QStringLiteral("_평면도_1-%1.").arg(sp.spec.denom, 0, 'f', 0) + ext, filter);
        if (f.isEmpty() || src_ != src) return;   // 저장 창을 띄운 사이 모델이 바뀌었으면 옛 도면을 저장하지 않음(검토 C1)
        if (QFileInfo(f).suffix().isEmpty()) f += "." + ext;
        runTask(QStringLiteral("평면도 %1 만드는 중").arg(name),
            [this, src, sp, f](QString* m) {
                return exportPlanSheet(*src, sp, f, m, &cancelTask_, [this](double x) { QMetaObject::invokeMethod(this, [this, x] { setProgress(x); }, Qt::QueuedConnection); });
            },
            [this, sp](bool ok, const QString& m) { report(ok, m); if (ok) noteSheetSaved(QStringLiteral("sheet/plan/"), sp.spec); });
    });
}

void MainWindow::dlgChooseSheet() {
    if (!src_) {
        QMessageBox::information(this, windowTitle(), QStringLiteral("모델을 연 뒤에 도면을 만드세요"));
        return;
    }
    auto* hold = new QLabel(QStringLiteral("조판 탭을 여는 중…"));
    hold->setAlignment(Qt::AlignCenter);
    hold->setObjectName("dialogMain");
    showSheetTab(hold);
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
    if (section_->hasResult() || plan_->hasLine()) dlgSheet();
    else dlgPlanSheet();
}

bool MainWindow::runSheetCheck(const QString& outDir, QString* log) {
    QDir().mkpath(outDir);
    QStringList lines;
    bool ok = true;
    auto flag = [&](bool v) { if (!v) ok = false; return v ? 1 : 0; };
    QSpinBox* planStep = scaleSpin(110);
    QSpinBox* secStep = scaleSpin(40);
    const int ps = planStep->singleStep(), ss = secStep->singleStep();
    delete planStep;
    delete secStep;
    if (!src_ || !src_->bounds.valid()) {
        lines << QStringLiteral("sheet-check RESULT fail");
        if (log) *log = lines.join('\n');
        return false;
    }
    const Box3 model = src_->bounds;
    const double mw = std::max(0.0, model.mx.x - model.mn.x), mh = std::max(0.0, model.mx.y - model.mn.y);
    Box3 area;
    area.mn.x = model.mn.x + mw * 0.25; area.mx.x = model.mx.x - mw * 0.25;
    area.mn.y = model.mn.y + mh * 0.25; area.mx.y = model.mx.y - mh * 0.25;
    area.mn.z = model.mn.z; area.mx.z = model.mx.z;
    if (!(area.mx.x > area.mn.x && area.mx.y > area.mn.y)) area = model;
    PlanSheetParams sp = defaultPlanSheetParams();
    sp.area = area;
    sp.wholeModel = false;
    sp.spec.denom = fitDenomStep10(sp.spec, area.mx.x - area.mn.x, area.mx.y - area.mn.y);
    QImage base; double bx0 = 0, by1 = 0, bres = 0;
    {
        double res = std::max(1e-4, std::max(mw, mh) / 800.0);
        QString em;
        if (!renderPlanOrtho(*src_, model, res, base, bx0, by1, &em, nullptr) || base.isNull()) {
            int W = 64, H = 64;
            base = QImage(W, H, QImage::Format_RGB32);
            base.fill(QColor(0xC4, 0xA5, 0x74));
            bx0 = model.mn.x; by1 = model.mx.y; bres = std::max(mw, mh) / W;
        } else bres = res;
    }
    const double k = 8;
    const Vec3 origin = src_->srs.origin;
    auto paintPlan = [&](double zoom, double dx, QImage& canvas, SheetPaintProbe& probe) {
        PlanSheetParams draw = sp;
        draw.imgDxMm = dx;
        PlanWindow W = planWindow(draw, 0, zoom);
        canvas = QImage(std::max(1, int(std::ceil(W.L.paperW * k))), std::max(1, int(std::ceil(W.L.paperH * k))), QImage::Format_RGB32);
        canvas.fill(Qt::white);
        QPainter p(&canvas);
        QImage pv; double px0 = bx0, py1 = by1, pres = bres;
        if (!renderPlanPreview(*src_, draw.spec, draw.area, zoom, draw.imgDxMm, draw.imgDyMm, pv, px0, py1, pres)) { pv = base; px0 = bx0; py1 = by1; pres = bres; }
        paintPlanSheet(p, draw, k, 0, pv, px0, py1, pres, origin, zoom, 0, 0, &probe);
    };
    auto outside = [](const SheetPaintProbe& pr) {
        if (pr.labels.isEmpty() || pr.plot.isEmpty()) return false;
        for (const QRectF& r : pr.labels) {
            QRectF i = r.intersected(pr.plot);
            if (i.width() > 0.8 && i.height() > 0.8) return false;
        }
        return true;
    };
    auto contentRatio = [](const QImage& im, const QRectF& plot) {
        QRect r = plot.toAlignedRect().intersected(im.rect()).adjusted(2, 2, -2, -2);
        if (r.width() < 4 || r.height() < 4) return 0.0;
        int n = 0, c = 0;
        for (int y = r.top(); y < r.bottom(); y += 2)
            for (int x = r.left(); x < r.right(); x += 2) {
                QRgb px = im.pixel(x, y);
                int R = qRed(px), G = qGreen(px), B = qBlue(px);
                auto near = [&](int r0, int g0, int b0) { return std::abs(R - r0) < 14 && std::abs(G - g0) < 14 && std::abs(B - b0) < 14; };
                ++n;
                if (!near(255, 255, 255) && !near(0xF7, 0xF5, 0xEE) && !near(0xF4, 0xF1, 0xEA)) ++c;
            }
        return n ? double(c) / n : 0.0;
    };
    auto redCount = [](const QImage& im, const QRectF& plot) {
        QRect r = plot.toAlignedRect().intersected(im.rect());
        int c = 0;
        for (int y = r.top(); y < r.bottom(); ++y)
            for (int x = r.left(); x < r.right(); ++x) {
                QRgb px = im.pixel(x, y);
                if (qRed(px) > 200 && qGreen(px) < 50 && qBlue(px) < 50) ++c;
            }
        return c;
    };
    auto imageCovers = [](const QRectF& img, const QRectF& plot) {
        return !img.isEmpty() && img.left() <= plot.left() + 3 && img.top() <= plot.top() + 3 && img.right() >= plot.right() - 3 && img.bottom() >= plot.bottom() - 3;
    };
    QImage c1; SheetPaintProbe p1;
    paintPlan(1, 0, c1, p1);
    const QString shot1 = QDir(outDir).filePath(QStringLiteral("sheet_plan_z1.png"));
    c1.save(shot1);
    const PlanPlace geo1 = placePlan(sp.spec, sp.area.mn.x, sp.area.mx.x, sp.area.mn.y, sp.area.mx.y, 1, 0, 0);
    const int fills1 = flag(geo1.imageFillsPlot && imageCovers(p1.image, p1.plot) && contentRatio(c1, p1.plot) > 0.5);
    const int whole1 = flag(geo1.rangeInsideImage);
    const int lineOn = p1.drewCutLine ? 1 : 0;
    if (lineOn != 0) ok = false;
    const int out1 = flag(outside(p1));
    const int uni1 = flag(p1.uniform);
    lines << QStringLiteral("sheet-check plan zoom=1.00 image-fills-plot=%1 image-whole=%2 line-on-plan=%3").arg(fills1).arg(whole1).arg(lineOn);
    lines << QStringLiteral("            labels=%1 labels-outside-plot=%2 text-uniform=%3 shot=%4").arg(p1.labels.size()).arg(out1 ? p1.labels.size() : 0).arg(uni1).arg(QDir::toNativeSeparators(shot1));
    QImage c2; SheetPaintProbe p2;
    paintPlan(2.07, 0, c2, p2);
    c2.save(QDir(outDir).filePath(QStringLiteral("sheet_plan_z2.png")));
    const double per1 = p1.worldStep > 0 ? p1.neighborPx / p1.worldStep : 0;
    const double per2 = p2.worldStep > 0 ? p2.neighborPx / p2.worldStep : 0;
    const int grew = flag(per2 > per1 * 1.5 && p2.neighborPx > 0);
    const int fontSame = flag(p1.fontPx > 0 && std::abs(p1.fontPx - p2.fontPx) < 0.5);
    const int out2 = flag(outside(p2));
    lines << QStringLiteral("sheet-check plan zoom=2.07 spacing-grew=%1 font-px-same=%2 labels-outside-plot=%3").arg(grew).arg(fontSame).arg(out2);
    if (section_ && section_->hasResult()) {
        SheetParams sec = defaultSheetParams(); sec.withLevels = true;   // 판정은 레벨선이 있어야 함(검토 M2)
        SectionDoc doc = section_->doc();
        auto paintSec = [&](double zoom, QImage& canvas, SheetPaintProbe& probe) {
            SheetGeom G = sheetGeom(doc, sec.spec);
            canvas = QImage(std::max(1, int(std::ceil(G.L.paperW * k))), std::max(1, int(std::ceil(G.L.paperH * k))), QImage::Format_RGB32);
            canvas.fill(Qt::white);
            QPainter p(&canvas);
            SectionImgGeo geo{doc.imgS0, doc.imgZ1, doc.imgRes};
            paintSheet(p, doc, sec, k, 0, doc.img, geo, zoom, 0, 0, &probe);
        };
        QImage s1; SheetPaintProbe e1;
        paintSec(1, s1, e1);
        s1.save(QDir(outDir).filePath(QStringLiteral("sheet_section_z1.png")));
        const int vis = flag(contentRatio(s1, e1.plot) > 0.02 && !e1.image.isEmpty());
        const int red = flag(redCount(s1, e1.plot) > 0);
        const int so = flag(outside(e1));
        lines << QStringLiteral("sheet-check section zoom=1.00 image-visible=%1 cut-red-visible=%2 labels-outside-plot=%3").arg(vis).arg(red).arg(so);
        {   // 디자인 v5 결정 L: 레벨선 10 cm 0.2 pt · 50 cm 0.5 pt — 종이 위 mm × (px/mm) 가 실제 펜 굵기와 같아야 한다
            const double expMinor = ptToMm(sec.levelMinorPt) * k, expMajor = ptToMm(sec.levelMajorPt) * k;
            const int lvOk = flag(std::abs(e1.levelMinorPx - expMinor) < 0.05 && std::abs(e1.levelMajorPx - expMajor) < 0.05 && e1.levelMajorPx > e1.levelMinorPx);
            lines << QStringLiteral("sheet-check level-pt minor=%1 expect=%2 major=%3 expect=%4 ok=%5").arg(e1.levelMinorPx, 0, 'f', 3).arg(expMinor, 0, 'f', 3).arg(e1.levelMajorPx, 0, 'f', 3).arg(expMajor, 0, 'f', 3).arg(lvOk);
        }
        QImage s2; SheetPaintProbe e2;
        paintSec(2.07, s2, e2);
        s2.save(QDir(outDir).filePath(QStringLiteral("sheet_section_z2.png")));
        const double se1 = e1.worldStep > 0 ? e1.neighborPx / e1.worldStep : 0;
        const double se2 = e2.worldStep > 0 ? e2.neighborPx / e2.worldStep : 0;
        const int sgrew = flag(se2 > se1 * 1.5 && e2.neighborPx > 0);
        const int suni = flag(e2.uniform && e1.fontPx > 0 && std::abs(e1.fontPx - e2.fontPx) < 0.5);
        const int svis = flag(contentRatio(s2, e2.plot) > 0.02);
        lines << QStringLiteral("sheet-check section zoom=2.07 spacing-grew=%1 text-uniform=%2 image-visible=%3").arg(sgrew).arg(suni).arg(svis);
    } else {
        lines << QStringLiteral("sheet-check section zoom=1.00 image-visible=0 cut-red-visible=0 labels-outside-plot=0");
        lines << QStringLiteral("sheet-check section zoom=2.07 spacing-grew=0 text-uniform=0 image-visible=0");
        ok = false;
    }
    QImage c3; SheetPaintProbe p3;
    paintPlan(1, 12, c3, p3);
    c3.save(QDir(outDir).filePath(QStringLiteral("sheet_plan_move.png")));
    const bool slid = !p1.labels.isEmpty() && !p3.labels.isEmpty() && std::abs(p1.labels[0].center().x() - p3.labels[0].center().x()) > 2;
    const int chrome = flag(std::abs(p1.compass.x() - p3.compass.x()) < 0.5 && std::abs(p1.legend.y() - p3.legend.y()) < 0.5 && std::abs(p1.frame.x() - p3.frame.x()) < 0.5);
    const int changed = flag(p1.labelText != p3.labelText || slid);
    lines << QStringLiteral("sheet-check plan move=12mm chrome-fixed=%1 label-values-changed=%2").arg(chrome).arg(changed);
    lines << QStringLiteral("sheet-check scale-step plan=%1 section=%2").arg(ps).arg(ss);
    flag(ps == 10 && ss == 10);
    lines << QStringLiteral("sheet-check RESULT %1").arg(ok ? "ok" : "fail");
    QFile out(QDir(outDir).filePath(QStringLiteral("sheet-check.txt")));
    if (out.open(QIODevice::WriteOnly | QIODevice::Text)) out.write((lines.join('\n') + '\n').toUtf8());
    if (log) *log = lines.join('\n');
    return ok;
}
