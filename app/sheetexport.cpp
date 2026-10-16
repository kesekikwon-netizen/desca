// 도면(용지) 내보내기: 축척·용지 미리보기, 넘침 경고·추천, 표제란(높이 기준
// 포함), PDF/DXF/PNG/TIFF 한 창(Ctrl+P). PDF 는 QPdfWriter(QtGui), 인쇄는
// QtPrintSupport(QPrinter · QPrintPreviewDialog — 2026-10-12 사용자 요청)
#include "mainwindow.hpp"

#include "asec/dxf.hpp"
#include "asec/raster.hpp"
#include "guideband.hpp"
#include "icons.hpp"
#include <QAbstractSpinBox>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPainterPath>
#include <QPdfWriter>
#include <QPointer>
#include <QPrintPreviewDialog>
#include <QPrinter>
#include <QPrinterInfo>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QShortcut>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QSvgGenerator>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <thread>

using namespace asec;
namespace fs = std::filesystem;
extern const char *const kVersion;

namespace {
QString qs8(const std::string &s) { return QString::fromUtf8(s.c_str()); }
fs::path toFsS(const QString &q) {
#ifdef _WIN32
  return fs::path(q.toStdWString());
#else
  return fs::u8path(q.toStdString());
#endif
}
QImage toQImageS(const RgbaImage &im) {
  if (im.empty())
    return {};
  return QImage(im.px.data(), im.w, im.h, im.w * 4, QImage::Format_RGBA8888)
      .copy();
}
RgbaImage fromQImageS(const QImage &q0) {
  QImage q = q0.convertToFormat(QImage::Format_RGBA8888);
  RgbaImage r;
  r.w = q.width();
  r.h = q.height();
  r.px.resize(size_t(r.w) * r.h * 4);
  for (int y = 0; y < r.h; ++y)
    std::memcpy(&r.px[size_t(y) * r.w * 4], q.constScanLine(y),
                size_t(r.w) * 4);
  return r;
}

/// 단면 → 용지 배치(높이는 위아래 4% 여유)
/// 단면도 조판 배치. 위치도(strip)가 있으면 그림 칸 = [평면 띠 stripM][틈 gapMm
/// = A/A′ 글자 띠][단면 heightM] 세로로.
struct SheetGeom {
  SheetLayout L;
  SheetSpec spec;
  double lenM = 0, heightM = 0, zTopAbs = 0;
  bool strip = false;
  double halfM = 0, stripM = 0, gapMm = 0;
  double contentM() const {
    return heightM + stripM;
  } // 맞춤·넘침 판정에 쓰는 세로 길이(m, 틈 제외)
  SheetSpec fitSpec() const {
    SheetSpec t = spec;
    t.gutterTopMm += gapMm;
    return t;
  } // 틈은 위 여백에 더해 계산
  /// 맞춤(축척 고르기)용: 띠가 그림 칸의 1/3 까지 올 수 있으므로 그만큼을 위
  /// 여백으로 잡고 단면만 맞춘다
  SheetSpec fitSpecMax() const {
    if (!strip)
      return spec;
    SheetSpec t = spec;
    SheetLayout L0 = layoutSheet(spec, lenM, heightM);
    t.gutterTopMm += gapMm + std::max(10.0, (L0.plotH - gapMm) / 3.0);
    return t;
  }
  double stripMm() const {
    return stripM * 1000.0 / std::max(1e-9, spec.denom);
  }
};
SheetGeom sheetGeom(const SectionDoc &doc, const SheetParams &sp) {
  SheetGeom g;
  g.spec = sp.spec;
  const double oz = doc.r.srs.origin.z;
  double zr = std::max(0.2, doc.r.zMax - doc.r.zMin);
  const double pad = 0.0; // 표고 범위가 이미 잘린 선 ±10 cm 선이라 더 띄우지
                          // 않음(사용자 2026-10-16 「표고 여백을 너무 두지
                          // 마라」). 옛 값: max(5 cm, 4 %)
  g.lenM = SectionFrame(doc.r.line).L;
  g.heightM = zr + 2 * pad;
  g.zTopAbs = doc.r.zMax + oz + pad;
  g.strip = sp.withPlan && sp.format != 1 &&
            g.lenM > 0; // DXF(모델 공간)에는 위치도 없음
  if (g.strip) {
    // 띠 높이 = 앞·뒤 각 max(3 m, 앞, 뒤) 를 같은 축척으로 — 단, 그림 칸의 1/3
    // 까지만(남은 2/3 는 단면). 넘치면 반폭을 줄인다
    g.gapMm = sp.spec.gutterTopMm;
    const SheetLayout L0 = layoutSheet(sp.spec, g.lenM, g.heightM);
    const double capMm = std::max(10.0, (L0.plotH - g.gapMm) / 3.0);
    const double wantMm =
        2 * planStripHalfWidth(doc.r.line.front, doc.r.line.back) * 1000.0 /
        std::max(1e-9, sp.spec.denom);
    const double useMm = std::min(wantMm, capMm);
    g.stripM = useMm * sp.spec.denom / 1000.0;
    g.halfM = g.stripM * 0.5;
  }
  g.L = layoutSheet(g.fitSpec(), g.lenM, g.contentM());
  if (g.strip) {
    g.L.plotY -= g.gapMm;
    g.L.plotH += g.gapMm;
  } // 그림 칸은 띠 위부터(틈 포함) — 맞춤 계산만 틈을 뺐다
  return g;
}

double niceLen(double target) {
  double p = std::pow(10.0, std::floor(std::log10(std::max(1e-9, target))));
  for (double m : {1.0, 2.0, 5.0, 10.0})
    if (m * p >= target * 0.999)
      return m * p;
  return 10 * p;
}

QString paperLabel(const SheetSpec &s) {
  return QStringLiteral("%1 %2").arg(QString::fromLatin1(paperName(s.paper)),
                                     s.landscape ? QStringLiteral("가로")
                                                 : QStringLiteral("세로"));
}
QString denomText(double d) { return QStringLiteral("1:%1").arg(d, 0, 'f', 0); }
const char *kFormatExt[4] = {"pdf", "dxf", "png", "tif"};
const char *kFormatName[4] = {"PDF", "DXF", "PNG", "TIFF"};

/// 평면·단면 조판이 같이 쓰는 바깥 자리(mm, 종이 왼쪽 위가 원점, y 아래).
struct SheetMarks {
  double compassX = 0, compassY = 0, compassR = 3.2;
  double legendX = 0, legendY = 0, legendW = 40, legendH = 18;
  double titleX = 0, titleY = 0, titleW = 120, titleH = 21;
  double scaleX = 0, scaleY = 0;
};
SheetMarks sheetMarks(const SheetLayout &L) {
  SheetMarks m;
  m.compassR = 3.2;
  m.compassX = L.frameX + L.frameW - 8;
  m.compassY = L.frameY + 5;
  m.legendX = L.frameX;
  m.legendY = L.titleY;
  m.legendW = L.frameW;
  m.legendH = L.frameY + L.frameH - L.titleY;
  m.titleX = m.legendX;
  m.titleY = m.legendY;
  m.titleW = m.legendW;
  m.titleH = m.legendH;
  m.scaleX = L.frameX + 3.0;
  m.scaleY = L.titleY + 1.8;
  return m;
}

QString coordText(double v, double step) {
  int dec = step >= 1 ? 0 : step >= 0.1 ? 1 : 2;
  return QString::number(v, 'f', dec);
}

struct SheetChrome {
  QString title, srs, date, facing, kind, paper, pageTag;
  double denom = 40;
  bool showTitle = true;
};

void paintCompass(QPainter &p, double cx, double cy, double r, double k) {
  QPointF c(cx * k, cy * k);
  const double R = r * k;
  p.setPen(QPen(theme::Ink, std::max(0.6, 0.25 * k)));
  p.setBrush(Qt::white);
  p.drawEllipse(c, R, R);
  QPolygonF arrow;
  arrow << QPointF(c.x(), c.y() - R * 0.78)
        << QPointF(c.x() - R * 0.28, c.y() + R * 0.15)
        << QPointF(c.x(), c.y() - R * 0.05)
        << QPointF(c.x() + R * 0.28, c.y() + R * 0.15);
  p.setBrush(theme::Ink);
  p.drawPolygon(arrow);
  p.setPen(theme::Ink);
  QFont f = theme::uiFont(std::max(6, int(std::lround(2.2 * k))), true);
  p.setFont(f);
  p.drawText(QRectF(c.x() - R, c.y() - R - 3.2 * k, R * 2, 3.0 * k),
             Qt::AlignHCenter | Qt::AlignBottom, QStringLiteral("진북"));
}

void paintSheetChrome(QPainter &p, const SheetLayout &L, double k,
                      const SheetChrome &c, bool compass = true) {
  const SheetMarks m = sheetMarks(L);
  const QRectF frame(L.frameX * k, L.frameY * k, L.frameW * k, L.frameH * k);
  p.setPen(QPen(theme::Ink, 0.35 * k));
  p.setBrush(Qt::NoBrush);
  p.drawRect(frame);
  if (compass) // 평·단면도는 돌린 평면 옆에 돌린 진북을 따로 그린다
    paintCompass(p, m.compassX, m.compassY, m.compassR, k);
  if (!c.showTitle)
    return;

  const double ui = k * 25.4 / 96.0;
  QFont fText = theme::uiFont(std::max(6, int(std::lround(10.5 * ui))));
  QFont fMono = theme::monoFont(std::max(5, int(std::lround(9.5 * ui))));

  // 막대는 종이 위에 들어가는 가장 긴 1·2·5×10ⁿ m (폭 약 20–35 mm)
  double lenM = 0;
  {
    const double maxM = 35.0 * c.denom / 1000.0,
                 pw = std::pow(10.0,
                               std::floor(std::log10(std::max(1e-9, maxM))));
    for (double mlt : {1.0, 2.0, 5.0, 10.0})
      if (mlt * pw <= maxM * 1.0001)
        lenM = mlt * pw;
    if (lenM <= 0)
      lenM = pw;
  }
  const double barMm = lenM * 1000.0 / c.denom;
  const double sbLeft = (L.frameX + 3.0) * k;
  const double sbTop = (L.titleY + 1.8) * k;
  const double sbH = 1.4 * k;
  const QRectF sb(sbLeft, sbTop, barMm * k, sbH);

  p.setPen(QPen(theme::Ink, 0.2 * k));
  p.setBrush(Qt::white);
  p.drawRect(sb);
  p.setBrush(theme::Ink);
  for (int i = 0; i < 4; i += 2) {
    p.drawRect(QRectF(sb.x() + sb.width() * i / 4.0, sb.y(), sb.width() / 4.0,
                      sb.height()));
  }

  p.setFont(fMono);
  p.setPen(theme::Ink);
  auto ml = [](double meters) {
    return meters >= 1 ? QString::number(meters, 'g', 4) + " m"
                       : QString::number(meters * 100, 'g', 4) + " cm";
  };
  p.drawText(QRectF(sb.left() - 5 * k, sb.bottom() + 0.4 * k, 10 * k, 3.2 * k),
             Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("0"));
  p.drawText(
      QRectF(sb.right() - 12 * k, sb.bottom() + 0.4 * k, 24 * k, 3.2 * k),
      Qt::AlignHCenter | Qt::AlignTop, ml(lenM));

  // 제일 하단 한 줄: 도면명 · 축척 1:<denom> · <srs> · <date> (· <pageTag>)
  QStringList parts;
  if (!c.title.isEmpty())
    parts << c.title;
  parts << QStringLiteral("축척 %1").arg(denomText(c.denom));
  if (!c.srs.isEmpty())
    parts << c.srs;
  if (!c.date.isEmpty())
    parts << c.date;
  if (!c.pageTag.isEmpty())
    parts << c.pageTag;
  const QString meta = parts.join(QStringLiteral("  ·  "));

  const double textLeft = sb.right() + 8.0 * k;
  const double textRight = (L.frameX + L.frameW - 4.0) * k;
  const double bandTop = L.titleY * k;
  const double bandH = (L.frameY + L.frameH - L.titleY) * k;
  if (textRight > textLeft) {
    QRectF metaRect(textLeft, bandTop, textRight - textLeft, bandH);
    p.setFont(fText);
    p.setPen(theme::Ink);
    p.drawText(metaRect, Qt::AlignRight | Qt::AlignVCenter, meta);
  }
}

/// 평면 정사영상에 쓸 메시: 3MX · 3SM 은 해상도 res 에 맞는 단계(LOD)의 타일,
/// OBJ 는 범위(로컬 XY)에 걸치는 메시 전부
bool collectPlanMeshes(MeshSource &src, const Box3 &area, double res,
                       std::vector<MeshPtr> &meshes, QString *msg,
                       const std::atomic<bool> *cancel) {
  std::string e;
  if (auto *t = dynamic_cast<TmxSource *>(&src)) {
    LeafStats st;
    if (!t->areaMeshes(area, res, meshes, &st, &e, cancel)) {
      if (msg)
        *msg = qs8(e);
      return false;
    }
  } else if (auto *s = dynamic_cast<StaticSource *>(&src)) {
    for (auto &m : s->meshes)
      if (!m->bbox.valid() ||
          !(m->bbox.mx.x < area.mn.x || m->bbox.mn.x > area.mx.x ||
            m->bbox.mx.y < area.mn.y || m->bbox.mn.y > area.mx.y))
        meshes.push_back(m);
  }
  return true;
}

bool renderPlanOrtho(MeshSource &src, const Box3 &areaIn, double res,
                     QImage &out, double &x0, double &y1, QString *msg,
                     const std::atomic<bool> *cancel) {
  Box3 area = areaIn;
  Box3 b = src.bounds;
  area.mn.x = std::max(area.mn.x, b.mn.x);
  area.mn.y = std::max(area.mn.y, b.mn.y);
  area.mx.x = std::min(area.mx.x, b.mx.x);
  area.mx.y = std::min(area.mx.y, b.mx.y);
  if (!area.valid() || area.mx.x <= area.mn.x || area.mx.y <= area.mn.y) {
    if (msg)
      *msg = QStringLiteral("내보낼 범위가 비어 있습니다");
    return false;
  }
  if (!(res > 0)) {
    if (msg)
      *msg = QStringLiteral("해상도가 없습니다");
    return false;
  }
  x0 = std::floor(area.mn.x / res) * res;
  y1 = std::ceil(area.mx.y / res) * res;
  int W = int(std::ceil((area.mx.x - x0) / res)),
      H = int(std::ceil((y1 - area.mn.y) / res));
  if (W < 1 || H < 1) {
    if (msg)
      *msg = QStringLiteral("내보낼 범위가 비어 있습니다");
    return false;
  }
  if (double(W) * H > 200e6) {
    if (msg)
      *msg = QStringLiteral("출력 영상이 너무 큽니다(%1 × %2 px). 축척 분모를 "
                            "키우거나 범위를 줄이세요.")
                 .arg(W)
                 .arg(H);
    return false;
  }
  std::vector<MeshPtr> meshes;
  if (!collectPlanMeshes(src, area, res, meshes, msg, cancel))
    return false;
  RgbaImage img;
  if (!renderPlan(meshes, x0, y1, res, W, H, img, cancel)) {
    if (msg)
      *msg = QStringLiteral("평면 영상 생성 실패(취소 또는 메모리)");
    return false;
  }
  out = toQImageS(img);
  return true;
}
/// 위치도 영상: 단면선 주변 띠(반폭 halfM, 양끝 여유 1 m)의 평면 정사영상. res
/// 는 요청값과 maxPixels 상한 중 거친 쪽
bool renderPlanStrip(MeshSource &src, const SectionLine &ln, double halfM,
                     double res, double maxPixels, PlanStrip &out,
                     const std::atomic<bool> *cancel) {
  const PlanStripArea a =
      planStripArea(ln.a.x, ln.a.y, ln.b.x, ln.b.y, halfM, 1.0);
  Box3 box;
  box.mn = Vec3(a.x0, a.y0, src.bounds.mn.z);
  box.mx = Vec3(a.x1, a.y1, src.bounds.mx.z);
  const double w = std::max(1e-6, a.x1 - a.x0), h = std::max(1e-6, a.y1 - a.y0);
  res = std::max({1e-4, res, std::sqrt(w * h / std::max(1.0, maxPixels))});
  QString em;
  if (!renderPlanOrtho(src, box, res, out.img, out.x0, out.y1, &em, cancel)) {
    out = PlanStrip();
    return false;
  }
  out.res = res;
  return true;
}

/// 위치도 띠 그리기: 단면선이 가로(A 왼쪽), 단면의 '뒤' 쪽이 위. 영상 → 두께
/// 띠(앞/뒤) → 단면선(빨강) → 테두리·설명
void paintPlanStrip(QPainter &p, const SectionDoc &doc, const SheetGeom &G,
                    const SectionPaperWindow &W, const SectionXf &xf, double k,
                    double ui, const PlanStrip *plan,
                    SheetPaintProbe *probe = nullptr) {
  const SectionLine &ln = doc.r.line;
  const SheetLayout &L = G.L;
  const double mmPerM = 1.0 / W.mPerMm; // 확대 포함
  const double stripMm0 = G.stripMm();  // 예약한 띠 높이(확대 없음)
  const double yT0 =
      L.plotY + stripMm0 * 0.5; // t = 0(단면선)의 종이 y(mm): 띠 가운데
  const double lenM = SectionFrame(ln).L;
  auto X = [&](double sM) { return xf.plot.left() + (sM - xf.s0) * xf.ppm; };
  auto Y = [&](double tM) { return (yT0 - tM * mmPerM) * k; };
  const QRectF strip(L.plotX * k, Y(G.halfM), L.plotW * k,
                     Y(-G.halfM) - Y(G.halfM));
  const QRectF room(L.plotX * k, L.frameY * k, L.plotW * k,
                    (L.plotY + stripMm0 - L.frameY) *
                        k); // 확대해도 틈·단면 쪽으로 넘치지 않게
  const QRectF clip = strip.intersected(room);
  if (clip.isEmpty())
    return;
  if (probe) {
    probe->strip = clip;
    probe->stripLineY = Y(0);
  }
  p.save();
  p.setClipRect(clip);
  p.fillRect(strip, Qt::white);
  if (plan && !plan->img.isNull() && plan->res > 0) {
    const PlanStripXf T =
        planStripTransform(ln.a.x, ln.a.y, ln.b.x, ln.b.y, plan->x0, plan->y1,
                           plan->res, X(0) / k, yT0, mmPerM);
    p.save();
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.setTransform(QTransform(T.m11 * k, T.m12 * k, T.m21 * k, T.m22 * k,
                              T.dx * k, T.dy * k),
                   true);
    p.drawImage(QPointF(0, 0), plan->img);
    p.restore();
  }
  // 두께 띠: 앞(front, 아래)·뒤(back, 위) — 평면 보기의 분홍 띠와 같은 뜻
  QColor wash = theme::SectionRed;
  wash.setAlpha(30);
  p.setPen(Qt::NoPen);
  p.setBrush(wash);
  p.drawRect(
      QRectF(X(0), Y(ln.back), X(lenM) - X(0), Y(-ln.front) - Y(ln.back)));
  // 단면선(빨강 0.35 mm) + 양끝 동그라미
  p.setPen(QPen(theme::SectionRed, exportLineMm(doc.st.lineWidthPx) * k,
                Qt::SolidLine, Qt::RoundCap));
  p.setBrush(Qt::NoBrush);
  p.drawLine(QPointF(X(0), Y(0)), QPointF(X(lenM), Y(0)));
  p.setBrush(Qt::white);
  for (double sM : {0.0, lenM})
    p.drawEllipse(QPointF(X(sM), Y(0)), 1.2 * k, 1.2 * k);
  // 테두리 · 설명
  p.setPen(QPen(theme::Ink, 0.25 * k));
  p.setBrush(Qt::NoBrush);
  p.drawRect(clip);
  QFont fSmall = theme::uiFont(std::max(5, int(std::lround(9 * ui))));
  p.setFont(fSmall);
  QString cap =
      QStringLiteral(
          "위치도 · 평면 %1 · 위 = 단면의 뒤쪽(%2) · 앞 %3 m / 뒤 %4 m")
          .arg(denomText(G.spec.denom),
               QString::fromUtf8(facingKo(sectionAzimuthDeg(ln)).c_str()))
          .arg(G.halfM, 0, 'g', 3)
          .arg(G.halfM, 0, 'g', 3);
  if (!plan || plan->img.isNull())
    cap += QStringLiteral(" · 평면 영상 준비 중…");
  QFontMetricsF fm(fSmall);
  QRectF cr(clip.left() + 1.5 * k, clip.top() + 1.0 * k,
            std::min(clip.width() - 3 * k, fm.horizontalAdvance(cap) + 3 * k),
            fm.height() + 1.0 * k);
  p.setPen(Qt::NoPen);
  p.setBrush(QColor(255, 255, 255, 220));
  p.drawRect(cr);
  p.setPen(theme::Muted);
  p.drawText(cr.adjusted(1.5 * k, 0, -1.5 * k, 0),
             Qt::AlignLeft | Qt::AlignVCenter, cap);
  p.restore();
}

/// 조판 미리보기 영상: 지금 보이는 범위(주변 30 % 포함)만 화면 미리보기에
/// 충분한 해상도(긴 변 2000 px)로 빠르게 생성
bool renderPlanPreview(MeshSource &src, const SheetSpec &spec, const Box3 &area,
                       double zoom, double dxMm, double dyMm, QImage &out,
                       double &x0, double &y1, double &res,
                       const std::atomic<bool> *cancel = nullptr) {
  PlanPlace v = placePlan(spec, area.mn.x, area.mx.x, area.mn.y, area.mx.y,
                          zoom, dxMm, dyMm);
  const double mx = (v.visX1 - v.visX0) * 0.3,
               my = (v.visYTop - v.visYBot) * 0.3;
  Box3 box;
  box.mn.x = v.visX0 - mx;
  box.mx.x = v.visX1 + mx;
  box.mn.y = v.visYBot - my;
  box.mx.y = v.visYTop + my;
  box.mn.z = src.bounds.mn.z;
  box.mx.z = src.bounds.mx.z;
  if (src.bounds.valid()) {
    box.mn.x = std::max(box.mn.x, src.bounds.mn.x);
    box.mn.y = std::max(box.mn.y, src.bounds.mn.y);
    box.mx.x = std::min(box.mx.x, src.bounds.mx.x);
    box.mx.y = std::min(box.mx.y, src.bounds.mx.y);
  }
  const double w = std::max(0.0, box.mx.x - box.mn.x),
               h = std::max(0.0, box.mx.y - box.mn.y);
  res = std::max({1e-4, 0.002, std::max(w, h) / 2000.0});
  QString em;
  return renderPlanOrtho(src, box, res, out, x0, y1, &em, cancel);
}
} // namespace

void MainWindow::paintSheet(QPainter &p, const SectionDoc &doc0,
                            const SheetParams &sp, double pxPerMm, int page,
                            const QImage &img0, const SectionImgGeo &geo0,
                            double viewZoom, double viewPanX, double viewPanY,
                            SheetPaintProbe *probe, const PlanStrip *plan) {
  const SheetGeom G = sheetGeom(doc0, sp);
  const SheetLayout &L = G.L;
  // 위치도가 있으면 그림 칸 위쪽을 띠(stripMm)+틈(gapMm = A/A′ 글자 띠) 만큼
  // 비우고 그 아래가 단면 칸
  const double secY = L.plotY + (G.strip ? G.stripMm() + G.gapMm : 0.0);
  const double secH = std::max(1.0, L.plotH - (secY - L.plotY));
  const double k = pxPerMm; // mm → 장치 픽셀
  const double ui =
      pxPerMm * 25.4 / 96.0; // 96 dpi 기준 글자·선 배율 → 실제 mm. 휠과 무관
  (void)viewPanX;
  (void)viewPanY; // 왼쪽 끌기는 종이 전체를 옮긴다. 그림 좌표는 imgDx/imgDy
  p.save();
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.fillRect(QRectF(0, 0, L.paperW * k, L.paperH * k), Qt::white);
  const int cols = sp.split ? L.cols : 1;
  const int col = sp.split ? page % cols : 0, row = sp.split ? page / cols : 0;
  const SectionPaperWindow W = sectionPaperWindow(
      G.lenM, G.heightM, G.zTopAbs, L.plotW, secH, sp.spec.denom, viewZoom,
      sp.imgDxMm, sp.imgDyMm, col, row, sp.split);
  SectionXf xf;
  xf.ppm = k / W.mPerMm;
  xf.plot = QRectF(L.plotX * k, secY * k, L.plotW * k, secH * k);
  xf.s0 = W.s0;
  xf.zTop = W.zTop;
  // 문서 스타일: 넣을 것 선택 그대로(그리는 순서 레벨선 → 영상 → 단면선은
  // paintSectionDoc 이 지킴)
  SectionDoc doc = doc0;
  doc.st.showImage = sp.withImage;
  doc.st.showLine = sp.withLine;
  doc.st.showLevels = sp.withLevels;
  doc.st.showTicks = sp.withTicks;
  doc.st.imageOpacity = doc0.st.imageOpacity;
  doc.st.showBaseline = sp.showBaseline;
  doc.st.baselineEl = sp.baselineEl; // 불투명도·선 굵기(st.lineWidthPx)는 화면
                                     // 설정 그대로(2026-10-11)
  doc.st.plotScaleBar = !sp.withTitle;
  doc.st.levelMinorPt = clampLineWeightPt(sp.levelMinorPt);
  doc.st.levelMajorPt = clampLineWeightPt(sp.levelMajorPt);
  // 영상: 여러 쪽 분할 인쇄/PDF 일 때만 쪽별 거리 범위로 자른다(화면
  // 미리보기에서 매 프레임 대용량 QImage::copy 방지)
  QImage img = img0;
  SectionImgGeo geo = geo0;
  if (sp.split && (L.cols * L.rows > 1) && !img.isNull() && geo.res > 0) {
    double sa = xf.s0, sb = xf.s0 + W.visLen;
    int x0 = std::max(0, int(std::floor((sa - geo.s0) / geo.res)) - 2),
        x1 = std::min(img.width(), int(std::ceil((sb - geo.s0) / geo.res)) + 2);
    if (x1 > x0 && (x0 > 0 || x1 < img.width())) {
      img = img.copy(x0, 0, x1 - x0, img.height());
      geo.s0 += x0 * geo.res;
    } else if (x1 <= x0)
      img = QImage();
  }
  const QRectF frame(L.frameX * k, L.frameY * k, L.frameW * k, L.frameH * k);
  const double areaTop = G.strip
                             ? (secY - G.gapMm) * k
                             : frame.top(); // 위치도가 있으면 A/A′ 글자 띠부터
  const QRectF area(frame.left(), areaTop, frame.width(),
                    L.titleY * k - areaTop);
  paintSectionDoc(p, doc, area, xf, ui, img, true, QString(), &geo, false,
                  false, 0, 0, 1, 0, 0, probe);
  if (probe) {
    probe->sectionS0 = xf.s0;
    probe->sectionPpm = xf.ppm;
    probe->sectionLenM = G.lenM;
    probe->strip = QRectF();
  }
  if (G.strip)
    paintPlanStrip(p, doc0, G, W, xf, k, ui, plan, probe);
  SheetChrome ch;
  ch.title = sp.title;
  ch.srs = sp.srsLabel;
  ch.date = sp.date;
  ch.facing = sp.facing;
  ch.kind = QStringLiteral("단면도");
  ch.paper = paperLabel(sp.spec);
  ch.denom = sp.spec.denom;
  ch.showTitle = sp.withTitle;
  if (sp.split && G.L.cols * G.L.rows > 1)
    ch.pageTag =
        QStringLiteral("%1 / %2").arg(page + 1).arg(G.L.cols * G.L.rows);
  paintSheetChrome(p, L, k, ch);
  if (probe) {
    SheetMarks m = sheetMarks(L);
    probe->compass =
        QRectF((m.compassX - m.compassR) * k, (m.compassY - m.compassR) * k,
               m.compassR * 2 * k, m.compassR * 2 * k);
    probe->legend =
        QRectF(m.legendX * k, m.legendY * k, m.legendW * k, m.legendH * k);
    probe->plot = xf.plot;
  }
  p.restore();
}

SheetParams MainWindow::defaultSheetParams() const {
  SheetParams sp;
  QSettings st;
  sp.spec.paper =
      st.value("sheet/paper", 0).toInt() == 1 ? Paper::A3 : Paper::A4;
  sp.spec.landscape = st.value("sheet/landscape", true).toBool();
  sp.spec.denom = st.value("sheet/denom", 40.0).toDouble();
  sp.format = std::clamp(st.value("sheet/format", 4).toInt(), 0,
                         4); // 기본 SVG(4) — 일러스트레이터로(스펙 §8)
  sp.dpi = std::max(600.0, st.value("sheet/dpi", 600.0).toDouble());
  sp.withImage = st.value("sheet/withImage", true).toBool();
  sp.withLine = st.value("sheet/withLine", true).toBool();
  sp.withLevels = st.value("sheet/withLevels", true).toBool();
  { // 거리 눈금(T10)은 단면 화면의 켜기 · 끄기(D)를 따름 — 기본 끔(사용자
    // 2026-10-16). 도면 판에 따로 끄는 칸이 생기기 전까지(T16)
    const auto ti = act_.find(QStringLiteral("ticks"));
    sp.withTicks = ti != act_.end() && ti->second->isChecked();
  }
  sp.withTitle = st.value("sheet/withTitle", true).toBool();
  sp.withPlan = st.value("sheet/withPlan", true)
                    .toBool(); // 위치도(평면 띠) — 사용자 결정 2026-10-11
  sp.levelMinorPt =
      clampLineWeightPt(st.value("sheet/levelMinorPt", 0.2).toDouble());
  sp.levelMajorPt =
      clampLineWeightPt(st.value("sheet/levelMajorPt", 0.5).toDouble());
  sp.hatchKind = std::clamp(st.value("sheet/hatchKind", 0).toInt(), 0, 2);
  sp.hatchMm = std::clamp(st.value("sheet/hatchMm", 1.0).toDouble(), 0.2, 5.0);
  sp.title = QStringLiteral("%1 단면도").arg(sectionName());
  if (current_ >= 0 && current_ < int(sections_.size()) &&
      !sections_[size_t(current_)].note.empty())
    sp.title += QStringLiteral(" · ") + qs8(sections_[size_t(current_)].note);
  const SrsDesc &d = srsReport_.desc;
  sp.srsLabel =
      d.horizontalEpsg
          ? QStringLiteral("수평 ") + qs8(d.horizontalShortKo())
          : (d.known()
                 ? qs8(d.shortAscii())
                 : QStringLiteral(
                       "좌표계 없음")); // 높이 글은 도면 어디에도 안 넣는다(T8)
  if (section_->hasResult()) {
    sp.facing = qs8(facingKo(sectionAzimuthDeg(section_->doc().r.line)));
    SectionDoc doc = section_->doc();
    sp.baselineEl =
        std::round((0.5 * (doc.r.zMin + doc.r.zMax) + doc.r.srs.origin.z) *
                   2.0) /
        2.0; // 가운데쯤 0.5 m 단위
    const SheetGeom G0 = sheetGeom(
        doc,
        sp); // 열 때는 단면이 그림 칸에 다 들어가는 가장 작은 10 단위(맞춤)
    if (G0.lenM > 0 && G0.heightM > 0)
      sp.spec.denom = fitDenomStep10(G0.fitSpecMax(), G0.lenM, G0.heightM);
  }
  sp.date = QDate::currentDate().toString("yyyy-MM-dd");
  return sp;
}

namespace {
struct PlanWindow {
  SheetLayout L;
  PlanPlace place;
};
PlanWindow planWindow(const PlanSheetParams &sp, int page,
                      double viewZoom = 1) {
  PlanWindow o;
  double w = std::max(0.0, sp.area.mx.x - sp.area.mn.x),
         h = std::max(0.0, sp.area.mx.y - sp.area.mn.y);
  o.L = layoutSheet(sp.spec, w, h);
  const int cols = sp.split ? std::max(1, o.L.cols) : 1;
  const int col = sp.split ? page % cols : 0;
  const int row = sp.split ? page / cols : 0;
  o.place =
      placePlan(sp.spec, sp.area.mn.x, sp.area.mx.x, sp.area.mn.y, sp.area.mx.y,
                viewZoom, sp.imgDxMm, sp.imgDyMm, col, row, sp.split);
  return o;
}
QString groupedCoord(double v, double step) {
  QLocale loc(QLocale::English);
  if (step >= 1)
    return loc.toString(qint64(std::llround(v)));
  return loc.toString(v, 'f', 1);
}
void noteProbeTransform(QPainter &p, SheetPaintProbe *probe) {
  if (!probe)
    return;
  const QTransform tr = p.worldTransform();
  probe->uniform = probe->uniform && std::abs(tr.m11() - tr.m22()) < 1e-4 &&
                   std::abs(tr.m12()) < 1e-4 && std::abs(tr.m21()) < 1e-4;
}
} // namespace

void MainWindow::paintPlanSheet(QPainter &p, const PlanSheetParams &sp,
                                double pxPerMm, int page, const QImage &img,
                                double imgX0, double imgY1, double imgRes,
                                const Vec3 &origin, double viewZoom,
                                double viewPanX, double viewPanY,
                                SheetPaintProbe *probe) {
  (void)viewPanX;
  (void)viewPanY;
  const PlanWindow W = planWindow(sp, page, viewZoom);
  const SheetLayout &L = W.L;
  const PlanPlace &pl = W.place;
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
    const QRectF dest(pl.xMm(imgX0) * k, pl.yMm(imgY1) * k,
                      worldW / pl.mPerMm * k, worldH / pl.mPerMm * k);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(dest, img);
    if (probe)
      probe->image = dest;
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
    probe->frame =
        QRectF(L.frameX * k, L.frameY * k, L.frameW * k, L.frameH * k);
  }
  const QRectF gutterL(L.frameX * k, plot.top(),
                       std::max(0.0, plot.left() - L.frameX * k),
                       plot.height());
  const QRectF gutterR(plot.right(), plot.top(),
                       std::max(0.0, (L.frameX + L.frameW) * k - plot.right()),
                       plot.height());
  const QRectF gutterB(plot.left(), plot.bottom(), plot.width(),
                       std::max(0.0, L.titleY * k - plot.bottom()));
  auto markLabel = [&](const QRectF &r, const QString &t) {
    if (!probe)
      return;
    probe->labels.push_back(r);
    probe->labelText << t;
  };
  p.setFont(fMono);
  p.setPen(theme::Ink2);
  QVector<double> xs;
  p.save();
  p.setClipRect(gutterB);
  p.fillRect(gutterB, Qt::white);
  for (double X = std::ceil(pl.visX0 / pl.tickStep - 1e-9) * pl.tickStep;
       X <= pl.visX1 + pl.tickStep * 1e-8; X += pl.tickStep) {
    double x = pl.xMm(X) * k;
    if (x < plot.left() - 0.2 || x > plot.right() + 0.2)
      continue;
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
  p.drawText(
      QRectF(plot.right() - 70 * k, plot.bottom() + 6.2 * k, 70 * k, 2.6 * k),
      Qt::AlignRight | Qt::AlignTop,
      QStringLiteral("X (동) · 1 칸 %1 m").arg(pl.tickStep, 0, 'f', 0));
  p.restore();
  auto drawY = [&](const QRectF &gutter, bool left) {
    p.save();
    p.setClipRect(gutter);
    p.fillRect(gutter, Qt::white);
    p.setFont(fMono);
    for (double Y = std::ceil(pl.visYBot / pl.tickStep - 1e-9) * pl.tickStep;
         Y <= pl.visYTop + pl.tickStep * 1e-8; Y += pl.tickStep) {
      double y = pl.yMm(Y) * k;
      if (y < plot.top() - 0.2 || y > plot.bottom() + 0.2)
        continue;
      p.setPen(QPen(theme::Ink, std::max(0.4, 0.2 * k)));
      if (left)
        p.drawLine(QPointF(plot.left() - 1.6 * k, y), QPointF(plot.left(), y));
      else
        p.drawLine(QPointF(plot.right(), y),
                   QPointF(plot.right() + 1.6 * k, y));
      QString t = groupedCoord(Y + origin.y, pl.tickStep);
      QRectF tr =
          left ? QRectF(gutter.left(), y - 1.4 * k,
                        std::max(0.0, gutter.width() - 0.8 * k), 2.8 * k)
               : QRectF(gutter.left() + 0.8 * k, y - 1.4 * k,
                        std::max(0.0, gutter.width() - 0.8 * k), 2.8 * k);
      p.setPen(theme::Ink2);
      p.setFont(fMono);
      p.drawText(tr, (left ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter,
                 t);
      markLabel(tr, t);
    }
    p.restore();
  };
  drawY(gutterL, true);
  drawY(gutterR, false);
  p.save();
  p.setClipRect(QRectF(L.frameX * k, L.frameY * k,
                       std::max(0.0, plot.left() - L.frameX * k),
                       std::max(0.0, plot.top() - L.frameY * k)));
  p.setFont(fCap);
  p.setPen(theme::Muted);
  p.drawText(QRectF(L.frameX * k, plot.top() - 4.2 * k,
                    std::max(0.0, plot.left() - L.frameX * k), 3.2 * k),
             Qt::AlignRight | Qt::AlignBottom, QStringLiteral("Y (북)"));
  p.restore();
  if (probe && xs.size() >= 2)
    probe->neighborPx = xs[1] - xs[0];
  p.setPen(QPen(theme::Ink, std::max(0.6, 0.35 * k)));
  p.setBrush(Qt::NoBrush);
  p.drawRect(plot);
  SheetChrome ch;
  ch.title = sp.title;
  ch.srs = sp.srsLabel;
  ch.date = sp.date;
  ch.facing = QStringLiteral("북이 위");
  ch.kind = QStringLiteral("평면도");
  ch.paper = paperLabel(sp.spec);
  ch.denom = sp.spec.denom;
  ch.showTitle = true;
  if (sp.split && L.cols * L.rows > 1)
    ch.pageTag = QStringLiteral("%1 / %2").arg(page + 1).arg(L.cols * L.rows);
  paintSheetChrome(p, L, k, ch);
  if (probe) {
    SheetMarks m = sheetMarks(L);
    probe->compass =
        QRectF((m.compassX - m.compassR) * k, (m.compassY - m.compassR) * k,
               m.compassR * 2 * k, m.compassR * 2 * k);
    probe->legend =
        QRectF(m.legendX * k, m.legendY * k, m.legendW * k, m.legendH * k);
    noteProbeTransform(p, probe);
  }
  p.restore();
}

namespace {
bool writePlanSheetDxf(const PlanSheetParams &sp, const QImage &img,
                       double imgX0, double imgY1, double imgRes,
                       const Vec3 &origin, const QString &path, QString *msg) {
  const PlanWindow W = planWindow(sp, 0);
  const SheetLayout &L = W.L;
  QString png = QFileInfo(path).completeBaseName() + "_image.png";
  QString pngPath = QFileInfo(path).absolutePath() + "/" + png;
  const PlanPlace &pl = W.place;
  QImage crop = img;
  DxfWriter d;
  d.addLayer("DRAW_SOIL", 7, 25);
  d.addLayer("DRAW_OUTLINE", 7, 35);
  d.addLayer("FRAME", 7);
  d.addLayer("COMPASS", 7);
  d.addLayer("COORD", 7);
  d.addLayer("LEGEND", 7);
  d.addLayer("IMAGE", 7);
  auto P = [&](double xMm, double yTopMm) {
    return Vec3(xMm, L.paperH - yTopMm, 0);
  };
  d.polyline2d("FRAME",
               {Vec2(L.frameX, L.paperH - L.frameY),
                Vec2(L.frameX + L.frameW, L.paperH - L.frameY),
                Vec2(L.frameX + L.frameW, L.paperH - (L.frameY + L.frameH)),
                Vec2(L.frameX, L.paperH - (L.frameY + L.frameH))},
               true);
  SheetMarks m = sheetMarks(L);
  Vec3 c = P(m.compassX, m.compassY);
  d.line("COMPASS", Vec3(c.x, c.y - m.compassR, 0),
         Vec3(c.x, c.y + m.compassR, 0));
  d.line("COMPASS", Vec3(c.x, c.y + m.compassR, 0),
         Vec3(c.x - m.compassR * 0.35, c.y + m.compassR * 0.2, 0));
  d.line("COMPASS", Vec3(c.x, c.y + m.compassR, 0),
         Vec3(c.x + m.compassR * 0.35, c.y + m.compassR * 0.2, 0));
  d.text("COMPASS", Vec3(c.x, c.y + m.compassR + 1.2, 0), 2.2, "N", 1, 0);
  d.text("LEGEND", P(m.legendX + 1.5, m.legendY + 4), 2.4, "PLAN", 0, 0);
  d.text("LEGEND", P(m.legendX + 1.5, m.legendY + 8), 2.2,
         ("1:" + std::to_string(int(std::lround(sp.spec.denom)))), 0, 0);
  for (double X = std::ceil(pl.visX0 / pl.tickStep - 1e-9) * pl.tickStep;
       X <= pl.visX1 + pl.tickStep * 1e-8; X += pl.tickStep) {
    double xmm = pl.xMm(X);
    if (xmm < L.plotX || xmm > L.plotX + L.plotW)
      continue;
    d.text("COORD", P(xmm, L.plotY + L.plotH + 3.2), 1.8,
           groupedCoord(X + origin.x, pl.tickStep).toStdString(), 1, 0);
  }
  for (double Y = std::ceil(pl.visYBot / pl.tickStep - 1e-9) * pl.tickStep;
       Y <= pl.visYTop + pl.tickStep * 1e-8; Y += pl.tickStep) {
    double ymm = pl.yMm(Y);
    if (ymm < L.plotY || ymm > L.plotY + L.plotH)
      continue;
    d.text("COORD", P(L.plotX - 1.2, ymm), 1.8,
           groupedCoord(Y + origin.y, pl.tickStep).toStdString(), 2, 2);
  }
  if (!crop.isNull() && imgRes > 0) {
    crop = crop.mirrored(false,
                         true); // DXF 는 삽입점(왼쪽 아래)에서 +Y 가 다음 행
    if (!crop.save(pngPath, "PNG")) {
      if (msg)
        *msg = QStringLiteral("PNG 저장 실패: %1").arg(pngPath);
      return false;
    }
    double destW = crop.width() * imgRes / pl.mPerMm,
           destH = crop.height() * imgRes / pl.mPerMm;
    double top = pl.yMm(imgY1);
    Vec3 ins = P(pl.xMm(imgX0), top + destH);
    Vec3 u(destW / crop.width(), 0, 0);
    Vec3 v(0, destH / crop.height(), 0);
    d.image("IMAGE", png.toStdString(), crop.width(), crop.height(), ins, u, v);
  }
  std::string err;
  if (!d.save(toFsS(path), &err)) {
    if (msg)
      *msg = qs8(err);
    return false;
  }
  if (msg)
    *msg = QStringLiteral(
               "%1%2\n\n평면도 DXF (용지 mm, 단면선 없음, 나침반·좌표·범례)")
               .arg(QFileInfo(path).fileName(),
                    crop.isNull() ? QString() : ", " + png);
  return true;
}
} // namespace

bool MainWindow::preparePlanAssets(
    MeshSource &src, const PlanSheetParams &p, PlanSheetParams &sp, QImage &img,
    double &x0, double &y1, double &res, QString *msg,
    const std::atomic<bool> *cancel,
    const std::function<void(double)> &progress) {
  sp = p;
  sp.spec.denom = std::max(10.0, std::round(sp.spec.denom));
  Box3 area = sp.wholeModel ? src.bounds : sp.area;
  if (src.bounds.valid()) {
    area.mn.x = std::max(area.mn.x, src.bounds.mn.x);
    area.mn.y = std::max(area.mn.y, src.bounds.mn.y);
    area.mx.x = std::min(area.mx.x, src.bounds.mx.x);
    area.mx.y = std::min(area.mx.y, src.bounds.mx.y);
    area.mn.z = src.bounds.mn.z;
    area.mx.z = src.bounds.mx.z;
    if (!(area.mx.x > area.mn.x && area.mx.y > area.mn.y))
      area = src.bounds;
  }
  sp.area = area;
  const double imgDpi = sp.dpi; // PDF 도 제한 없이 지정한 dpi 그대로(가장 선명)
  const PlanPlace cover = placePlan(sp.spec, area.mn.x, area.mx.x, area.mn.y,
                                    area.mx.y, 1, sp.imgDxMm, sp.imgDyMm);
  Box3 render;
  render.mn.x = cover.worldX0;
  render.mx.x = cover.worldX1;
  render.mn.y = cover.worldYBot;
  render.mx.y = cover.worldYTop;
  render.mn.z = src.bounds.mn.z;
  render.mx.z = src.bounds.mx.z;
  res = groundResolution(sp.spec.denom, imgDpi);
  double pix =
      (render.mx.x - render.mn.x) / res * (render.mx.y - render.mn.y) / res;
  if (pix > 40e6)
    res *= std::sqrt(pix / 40e6);
  x0 = 0;
  y1 = 0;
  if (progress)
    progress(0.05);
  if (!renderPlanOrtho(src, render, res, img, x0, y1, msg, cancel))
    return false;
  if (cancel && cancel->load()) {
    if (msg)
      *msg = QStringLiteral("취소됨");
    return false;
  }
  return true;
}

bool MainWindow::exportPlanSheet(MeshSource &src, const PlanSheetParams &p,
                                 const QString &path, QString *msg,
                                 const std::atomic<bool> *cancel,
                                 const std::function<void(double)> &progress) {
  PlanSheetParams sp;
  QImage img;
  double x0 = 0, y1 = 0, res = 0;
  if (!preparePlanAssets(src, p, sp, img, x0, y1, res, msg, cancel, progress))
    return false;
  if (sp.format == 1)
    return writePlanSheetDxf(sp, img, x0, y1, res, src.srs.origin, path, msg);
  if (sp.format == 4) {
    const PlanWindow Wsvg = planWindow(sp, 0);
    const double dpi = 96.0, k = dpi / 25.4;
    QSvgGenerator gen(QSvgGenerator::SvgVersion::Svg11);
    gen.setFileName(path);
    gen.setResolution(int(dpi));
    gen.setTitle(sp.title);
    gen.setDescription(
        QStringLiteral("DRAW_SOIL and DRAW_OUTLINE are empty drawing layers"));
    gen.setSize(QSize(std::max(1, int(std::lround(Wsvg.L.paperW * k))),
                      std::max(1, int(std::lround(Wsvg.L.paperH * k)))));
    gen.setViewBox(QRectF(0, 0, Wsvg.L.paperW * k, Wsvg.L.paperH * k));
    QPainter pt;
    if (!pt.begin(&gen)) {
      if (msg)
        *msg = QStringLiteral("SVG 를 만들 수 없습니다: %1").arg(path);
      return false;
    }
    paintPlanSheet(pt, sp, k, 0, img, x0, y1, res, src.srs.origin);
    pt.end();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
      if (msg)
        *msg = QStringLiteral("SVG 를 읽을 수 없습니다");
      return false;
    }
    QString svg = QString::fromUtf8(f.readAll());
    f.close();
    int at = svg.indexOf(QStringLiteral("<svg"));
    int gt = at < 0 ? -1 : svg.indexOf('>', at);
    if (gt > 0)
      svg.insert(gt + 1,
                 QStringLiteral(
                     "\n<g id=\"DRAW_SOIL\"/>\n<g id=\"DRAW_OUTLINE\"/>\n"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      if (msg)
        *msg = QStringLiteral("SVG 를 쓸 수 없습니다");
      return false;
    }
    f.write(svg.toUtf8());
    if (msg)
      *msg =
          QStringLiteral(
              "%1\n평면도 SVG · 빈 층 DRAW_SOIL · DRAW_OUTLINE · 단면선 없음")
              .arg(QFileInfo(path).fileName());
    return true;
  }
  const PlanWindow W0 = planWindow(sp, 0);
  const int pages = sp.split ? W0.L.cols * W0.L.rows : 1;
  const Vec3 origin = src.srs.origin;
  QStringList files;
  if (sp.format == 0) {
    QPdfWriter w(path);
    w.setResolution(int(std::lround(sp.dpi)));
    w.setTitle(QStringLiteral("%1 — %2").arg(sp.title, sp.srsLabel));
    w.setCreator(QStringLiteral("Trench %1").arg(QString::fromUtf8(kVersion)));
    QPageSize ps(sp.spec.paper == Paper::A3 ? QPageSize::A3 : QPageSize::A4);
    w.setPageLayout(QPageLayout(
        ps, sp.spec.landscape ? QPageLayout::Landscape : QPageLayout::Portrait,
        QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter));
    QPainter pt;
    if (!pt.begin(&w)) {
      if (msg)
        *msg = QStringLiteral("PDF 를 만들 수 없습니다: %1").arg(path);
      return false;
    }
    const double pxPerMm = w.resolution() / 25.4;
    for (int i = 0; i < pages; ++i) {
      if (i > 0)
        w.newPage();
      paintPlanSheet(pt, sp, pxPerMm, i, img, x0, y1, res, origin);
      if (progress)
        progress(0.3 + 0.7 * (i + 1) / pages);
      if (cancel && cancel->load())
        break;
    }
    pt.end();
    files << QFileInfo(path).fileName();
  } else {
    const double pxPerMm = sp.dpi / 25.4;
    const QSize sz(int(std::lround(W0.L.paperW * pxPerMm)),
                   int(std::lround(W0.L.paperH * pxPerMm)));
    QFileInfo fi(path);
    for (int i = 0; i < pages; ++i) {
      QImage canvas(sz, QImage::Format_RGB32);
      canvas.fill(Qt::white);
      {
        QPainter pt(&canvas);
        paintPlanSheet(pt, sp, pxPerMm, i, img, x0, y1, res, origin);
      }
      const int dpm = int(std::lround(sp.dpi / 0.0254));
      canvas.setDotsPerMeterX(dpm);
      canvas.setDotsPerMeterY(dpm);
      QString f = pages > 1
                      ? fi.absolutePath() + "/" + fi.completeBaseName() +
                            QStringLiteral("_%1.").arg(i + 1) + fi.suffix()
                      : path;
      if (sp.format == 2) {
        if (!canvas.save(f, "PNG")) {
          if (msg)
            *msg = QStringLiteral("PNG 저장 실패: %1").arg(f);
          return false;
        }
      } else {
        TiffOptions o;
        o.dpi = sp.dpi;
        o.alpha = false;
        o.description = "Plan sheet " + src.srs.shortLabel();
        std::string err;
        if (!writeTiff(toFsS(f), fromQImageS(canvas), o, &err)) {
          if (msg)
            *msg = qs8(err);
          return false;
        }
      }
      files << QFileInfo(f).fileName();
      if (progress)
        progress(0.3 + 0.7 * (i + 1) / pages);
      if (cancel && cancel->load()) {
        if (msg)
          *msg = QStringLiteral("취소됨");
        return false;
      }
    }
  }
  if (msg) {
    *msg = QStringLiteral("%1\n\n평면도 · %2 · %3 · %4쪽\n단면선 없음 · 나침반 "
                          "· 바깥 좌표 · 범례 · 자\n%5")
               .arg(files.join(", "), paperLabel(sp.spec),
                    denomText(sp.spec.denom))
               .arg(pages)
               .arg(sp.srsLabel);
  }
  return true;
}

bool MainWindow::exportSectionSvg(const SectionDoc &doc0, MeshSource &src,
                                  const SheetParams &p, const QString &path,
                                  QString *msg) {
  SheetParams sp = p;
  sp.withImage = true;
  const SheetGeom G = sheetGeom(doc0, sp);
  QImage img;
  SectionImgGeo geo{doc0.imgS0, doc0.imgZ1, doc0.imgRes};
  if (sp.withImage && (img.isNull() || geo.res <= 0)) {
    SectionRequest rq;
    rq.line = doc0.r.line;
    rq.zRangeFromProfile = true;
    rq.depthFade = doc0.st.depthFade ? kDepthFadeStrength : 0.0;
    rq.imageRes = groundResolution(sp.spec.denom, 300);
    rq.maxImagePixels = size_t(60) << 20;
    SectionOutput out;
    std::string e;
    if (!computeSection(src, rq, out, &e, nullptr)) {
      if (msg)
        *msg = qs8(e);
      return false;
    }
    img = toQImageS(out.image.img);
    geo = SectionImgGeo{out.image.s0, out.image.z1, out.image.res};
  }
  PlanStrip strip;
  const PlanStrip *stripPtr = nullptr;
  if (G.strip && renderPlanStrip(src, doc0.r.line, G.halfM,
                                 groundResolution(sp.spec.denom, 300), 30e6,
                                 strip, nullptr))
    stripPtr = &strip;
  const double dpi = 96.0;
  const double k = dpi / 25.4;
  QSvgGenerator gen(QSvgGenerator::SvgVersion::Svg11);
  gen.setFileName(path);
  gen.setResolution(int(dpi));
  gen.setTitle(sp.title);
  gen.setDescription(
      QStringLiteral("DRAW_SOIL and DRAW_OUTLINE are empty drawing layers"));
  gen.setSize(QSize(std::max(1, int(std::lround(G.L.paperW * k))),
                    std::max(1, int(std::lround(G.L.paperH * k)))));
  gen.setViewBox(QRectF(0, 0, G.L.paperW * k, G.L.paperH * k));
  QPainter pt;
  if (!pt.begin(&gen)) {
    if (msg)
      *msg = QStringLiteral("SVG 를 만들 수 없습니다: %1").arg(path);
    return false;
  }
  paintSheet(pt, doc0, sp, k, 0, img, geo, 1, 0, 0, nullptr, stripPtr);
  pt.end();
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    if (msg)
      *msg = QStringLiteral("SVG 를 읽을 수 없습니다");
    return false;
  }
  QString svg = QString::fromUtf8(f.readAll());
  f.close();
  int at = svg.indexOf(QStringLiteral("<svg"));
  int gt = at < 0 ? -1 : svg.indexOf('>', at);
  if (gt > 0)
    svg.insert(
        gt + 1,
        QStringLiteral("\n<g id=\"DRAW_SOIL\"/>\n<g id=\"DRAW_OUTLINE\"/>\n"));
  if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    if (msg)
      *msg = QStringLiteral("SVG 를 쓸 수 없습니다");
    return false;
  }
  f.write(svg.toUtf8());
  f.close();
  if (msg)
    *msg = QStringLiteral("%1\n빈 층 DRAW_SOIL · DRAW_OUTLINE")
               .arg(QFileInfo(path).fileName());
  return true;
}

bool MainWindow::prepareSheetAssets(
    const SectionDoc &doc0, MeshSource &src, const SheetParams &p,
    SheetAssets &assets, QString *msg, const std::atomic<bool> *cancel,
    const std::function<void(double)> &progress) {
  const SheetGeom G = sheetGeom(doc0, p);
  // 입면 영상: 출력 해상도로 다시 계산(같은 단면선 → 같은 윤곽은 스냅숏 것을
  // 유지)
  QImage &img = assets.img;
  SectionImgGeo &geo = assets.geo;
  geo = SectionImgGeo{doc0.imgS0, doc0.imgZ1, doc0.imgRes};
  double &usedRes = assets.usedRes;
  usedRes = 0;
  assets.hasStrip = false;
  if (p.withImage) {
    const double imgDpi = p.dpi;
    SectionRequest rq;
    rq.line = doc0.r.line;
    rq.zRangeFromProfile = true;
    rq.depthFade = doc0.st.depthFade ? kDepthFadeStrength : 0.0;
    rq.imageRes = groundResolution(p.spec.denom, imgDpi);
    rq.maxImagePixels = size_t(120) << 20;
    SectionOutput out;
    std::string e;
    if (progress)
      progress(0.05);
    if (!computeSection(src, rq, out, &e, cancel)) {
      if (msg)
        *msg = qs8(e);
      return false;
    }
    img = toQImageS(out.image.img);
    geo = SectionImgGeo{out.image.s0, out.image.z1, out.image.res};
    usedRes = out.image.res;
  }
  if (cancel && cancel->load()) {
    if (msg)
      *msg = QStringLiteral("취소됨");
    return false;
  }
  if (G.strip) {
    if (progress)
      progress(0.2);
    if (renderPlanStrip(src, doc0.r.line, G.halfM,
                        groundResolution(p.spec.denom, p.dpi), 40e6,
                        assets.strip, cancel))
      assets.hasStrip = true;
    if (cancel && cancel->load()) {
      if (msg)
        *msg = QStringLiteral("취소됨");
      return false;
    }
  }
  return true;
}

bool MainWindow::exportSheet(const SectionDoc &doc0, MeshSource &src,
                             const SheetParams &p, const QString &path,
                             QString *msg, const std::atomic<bool> *cancel,
                             const std::function<void(double)> &progress) {
  const SheetGeom G = sheetGeom(doc0, p);
  const int pages = p.split ? G.L.cols * G.L.rows : 1;
  if (p.format == 1) { // DXF: 모델 공간 1:N(용지 배치 없음)
    DxfParams dp;
    dp.world3d = false;
    dp.denom = p.spec.denom;
    dp.image = p.withImage;
    dp.imageDpi = std::min(p.dpi, 200.0);
    dp.levelMinorPt = p.levelMinorPt;
    dp.levelMajorPt = p.levelMajorPt;
    SectionDoc doc = doc0;
    doc.st.showLevels = p.withLevels;
    doc.st.showTicks = p.withTicks;
    return exportSectionDxf(doc, src, dp, path, msg, cancel);
  }
  SheetAssets A;
  if (!prepareSheetAssets(doc0, src, p, A, msg, cancel, progress))
    return false;
  const QImage &img = A.img;
  const SectionImgGeo &geo = A.geo;
  const double usedRes = A.usedRes;
  const PlanStrip &strip = A.strip;
  const PlanStrip *stripPtr = A.hasStrip ? &A.strip : nullptr;
  QStringList files;
  if (p.format == 0) {
    QPdfWriter w(path);
    w.setResolution(int(std::lround(p.dpi)));
    w.setTitle(QStringLiteral("%1 — %2").arg(p.title, p.srsLabel));
    w.setCreator(QStringLiteral("Trench %1").arg(QString::fromUtf8(kVersion)));
    QPageSize ps(p.spec.paper == Paper::A3 ? QPageSize::A3 : QPageSize::A4);
    w.setPageLayout(QPageLayout(
        ps, p.spec.landscape ? QPageLayout::Landscape : QPageLayout::Portrait,
        QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter));
    QPainter pt;
    if (!pt.begin(&w)) {
      if (msg)
        *msg = QStringLiteral("PDF 를 만들 수 없습니다: %1").arg(path);
      return false;
    }
    const double pxPerMm = w.resolution() / 25.4;
    for (int i = 0; i < pages; ++i) {
      if (i > 0)
        w.newPage();
      paintSheet(pt, doc0, p, pxPerMm, i, img, geo, 1, 0, 0, nullptr, stripPtr);
      if (progress)
        progress(0.3 + 0.7 * (i + 1) / pages);
      if (cancel && cancel->load())
        break;
    }
    pt.end();
    files << QFileInfo(path).fileName();
  } else {
    const double pxPerMm = p.dpi / 25.4;
    const QSize sz(int(std::lround(G.L.paperW * pxPerMm)),
                   int(std::lround(G.L.paperH * pxPerMm)));
    QFileInfo fi(path);
    for (int i = 0; i < pages; ++i) {
      QImage canvas(sz, QImage::Format_RGB32);
      canvas.fill(Qt::white);
      {
        QPainter pt(&canvas);
        paintSheet(pt, doc0, p, pxPerMm, i, img, geo, 1, 0, 0, nullptr,
                   stripPtr);
      }
      const int dpm = int(std::lround(p.dpi / 0.0254));
      canvas.setDotsPerMeterX(dpm);
      canvas.setDotsPerMeterY(dpm);
      QString f = pages > 1
                      ? fi.absolutePath() + "/" + fi.completeBaseName() +
                            QStringLiteral("_%1.").arg(i + 1) + fi.suffix()
                      : path;
      if (p.format == 2) {
        if (!canvas.save(f, "PNG")) {
          if (msg)
            *msg = QStringLiteral("PNG 저장 실패: %1").arg(f);
          return false;
        }
      } else {
        TiffOptions o;
        o.dpi = p.dpi;
        o.alpha = false;
        o.description = "Section sheet " + doc0.r.srs.shortLabel();
        std::string err;
        if (!writeTiff(toFsS(f), fromQImageS(canvas), o, &err)) {
          if (msg)
            *msg = qs8(err);
          return false;
        }
      }
      files << QFileInfo(f).fileName();
      if (progress)
        progress(0.3 + 0.7 * (i + 1) / pages);
      if (cancel && cancel->load()) {
        if (msg)
          *msg = QStringLiteral("취소됨");
        return false;
      }
    }
  }
  if (msg) {
    *msg =
        QStringLiteral("%1\n\n%2 · %3 · %4쪽%5\n%6")
            .arg(files.join(", "), paperLabel(p.spec), denomText(p.spec.denom))
            .arg(pages)
            .arg(!G.L.fits && !p.split ? QStringLiteral(" (넘친 부분은 잘림)")
                                       : QString())
            .arg(p.srsLabel);
    if (usedRes > 0)
      *msg +=
          QStringLiteral("\n입면 영상 %1 mm/px").arg(usedRes * 1000, 0, 'f', 2);
    if (G.strip)
      *msg +=
          stripPtr
              ? QStringLiteral("\n위치도(평면 띠, 앞·뒤 %1 m) %2 mm/px")
                    .arg(G.halfM, 0, 'g', 3)
                    .arg(strip.res * 1000, 0, 'f', 2)
              : QStringLiteral("\n위치도 영상을 만들지 못해 띠만 그렸습니다");
  }
  return true;
}

// ---------------------------------------------------------------- 도면 창
namespace {
class SheetPreview : public QWidget {
public:
  std::function<void(QPainter &, const QRect &)> paintFn;
  std::function<void(QPointF)> onImageDrag;
  std::function<void()> onSettled; // 확대·그림 이동이 멈춘 0.25 초 뒤(보이는
                                   // 범위를 더 촘촘히 다시 그리는 데 씀)
  bool editing = false;
  double zoom = 1;     // 조판편집 중 휠로 바꾸는 칸 안 배율(멈추면 축척 값으로
                       // 바뀌고 1 로 돌아감)
  double pageZoom = 1; // 편집 아닐 때 휠: 종이 전체를 크게·작게 보기만(도면
                       // 내용·축척은 그대로)
  double fitPxPerMm = 1;
  QPointF lastPaperO,
      lastPaperSize; // 마지막으로 그린 종이 자리·크기(px) — 커서 기준 확대용
  QPointF viewPan;
  QPointF zoomCenter;
  // 위치도 선 ↔ 단면 연동(사용자 결정 2026-10-11): 위 선 근처(자석)·아래 단면
  // 칸 위에서 마우스가 가리키는 A 로부터의 거리 s(m). paintFn 이 geom 을 채운다
  SheetPaintProbe geom;
  bool geomValid = false;
  bool hasHover = false;
  double hoverS = 0;
  bool hoverOnStrip = false;
  QString letterA =
      QStringLiteral("A"); // 거리 글자의 기준 글자(단면 이름 앞 글자)
  static constexpr double kSnapPx =
      14; // 위치도 선에서 이만큼 안이면 선에 붙는다
  SheetPreview() {
    setMouseTracking(true);
    setFocusPolicy(Qt::WheelFocus);
    setObjectName(QStringLiteral("sheetPreview"));
  }
  /// 위젯 좌표 → 거리 s. 띠 선 근처면 선에 붙고, 단면 칸 위면 그 x 의 거리. 둘
  /// 다 아니면 false
  bool hoverAt(const QPointF &wpos, double *s, bool *onStrip) const {
    if (!geomValid || geom.sectionPpm <= 0)
      return false;
    const double sc =
        std::max(1e-9, fitPxPerMm); // paintFn 은 종이 원점 o 로 옮긴 뒤 pxPerMm
                                    // = sc 로 그린다 → geom 은 그 픽셀 단위
    (void)sc;
    const QPointF q = wpos - lastPaperO; // 종이 원점 기준 픽셀
    auto sAtX = [&](double x) {
      return std::clamp(geom.sectionS0 +
                            (x - geom.plot.left()) / geom.sectionPpm,
                        0.0, geom.sectionLenM);
    };
    if (!geom.strip.isEmpty() &&
        geom.strip.adjusted(0, -kSnapPx, 0, kSnapPx).contains(q) &&
        std::fabs(q.y() - geom.stripLineY) <= kSnapPx) {
      *s = sAtX(q.x());
      *onStrip = true;
      return true;
    }
    if (geom.plot.contains(q)) {
      *s = sAtX(q.x());
      *onStrip = false;
      return true;
    }
    return false;
  }
  /// 연동 표시: 단면 칸 세로선 + 위치도 선 위 점 + 거리 글자(화면 전용,
  /// 저장에는 안 들어감). 종이 원점 기준 painter 에서 부른다
  void paintHover(QPainter &p) const {
    if (!hasHover || !geomValid || geom.sectionPpm <= 0)
      return;
    const double x =
        geom.plot.left() + (hoverS - geom.sectionS0) * geom.sectionPpm;
    if (x < geom.plot.left() - 1 || x > geom.plot.right() + 1)
      return;
    p.save();
    QPen pen(theme::SectionRed, 1.2, Qt::DashLine, Qt::FlatCap);
    p.setPen(pen);
    p.drawLine(QPointF(x, geom.plot.top()), QPointF(x, geom.plot.bottom()));
    if (!geom.strip.isEmpty()) {
      p.drawLine(QPointF(x, geom.strip.top()), QPointF(x, geom.strip.bottom()));
      p.setPen(QPen(theme::SectionRed, 1.5));
      p.setBrush(Qt::white);
      p.drawEllipse(QPointF(x, geom.stripLineY), 4.5, 4.5);
    }
    const QString t =
        QStringLiteral("%1 에서 %2 m").arg(letterA).arg(hoverS, 0, 'f', 2);
    QFont f = theme::monoFont(11);
    p.setFont(f);
    const QRectF tb = QFontMetricsF(f).boundingRect(t).adjusted(-5, -2, 5, 2);
    QRectF box(QPointF(x + 8, geom.plot.top() + 6), tb.size());
    if (box.right() > geom.plot.right())
      box.moveRight(x - 8);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255, 230));
    p.drawRect(box);
    p.setPen(theme::SectionRed);
    p.drawText(box, Qt::AlignCenter, t);
    p.restore();
  }
  void leaveEvent(QEvent *e) override {
    if (hasHover) {
      hasHover = false;
      update();
    }
    QWidget::leaveEvent(e);
  }
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.fillRect(rect(), theme::Wash);
    if (paintFn)
      paintFn(p, rect());
  }
  void wheelEvent(QWheelEvent *e) override {
    double steps = e->angleDelta().y() / 120.0;
    if (steps == 0 && !e->pixelDelta().isNull())
      steps = e->pixelDelta().y() / 60.0;
    if (steps == 0) {
      e->ignore();
      return;
    }
    const double factor = std::exp(-std::log(0.85) * steps);
    if (editing) { // 조판편집: 칸 안 그림 배율 → 멈추면 축척 값으로
      zoom = std::clamp(zoom * factor, 0.35, 8.0);
      update();
      scheduleSettled();
    } else { // 보기만: 커서 아래 종이 점을 고정하고 종이 전체를 확대·축소
      const double nz = std::clamp(pageZoom * factor, 0.5, 8.0),
                   f = nz / pageZoom;
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
  void mouseDoubleClickEvent(QMouseEvent *e) override {
    pageZoom = 1;
    viewPan = QPointF();
    update();
    e->accept();
  } // 종이 보기 처음 크기로
  void mousePressEvent(QMouseEvent *e) override {
    last_ = e->position();
    const bool image = editing && e->button() == Qt::MiddleButton;
    const bool view = e->button() == Qt::LeftButton;
    if (image || view) {
      drag_ = e->button();
      grabMouse();
      e->accept();
    } else
      e->ignore();
  }
  void mouseMoveEvent(QMouseEvent *e) override {
    const QPointF d = e->position() - last_;
    last_ = e->position();
    if (editing && (e->buttons() & Qt::MiddleButton) && onImageDrag) {
      onImageDrag(d);
      e->accept();
      return;
    }
    if (e->buttons() & Qt::LeftButton) {
      viewPan += d;
      update();
      e->accept();
      return;
    }
    double sv = 0;
    bool onStrip = false;
    const bool h = hoverAt(e->position(), &sv, &onStrip);
    if (h != hasHover ||
        (h && (std::fabs(sv - hoverS) > 1e-9 || onStrip != hoverOnStrip))) {
      hasHover = h;
      hoverS = sv;
      hoverOnStrip = onStrip;
      update();
    }
    setCursor(h && onStrip ? Qt::CrossCursor
                           : (editing ? Qt::SizeAllCursor : Qt::ArrowCursor));
  }
  void mouseReleaseEvent(QMouseEvent *e) override {
    if (e->button() == drag_) {
      const bool img = drag_ == Qt::MiddleButton;
      releaseMouse();
      drag_ = Qt::NoButton;
      e->accept();
      if (img)
        scheduleSettled();
    }
  }
  void scheduleSettled() {
    if (!onSettled)
      return;
    if (!settle_) {
      settle_ = new QTimer(this);
      settle_->setSingleShot(true);
      settle_->setInterval(250);
      QObject::connect(settle_, &QTimer::timeout, this, [this] {
        if (onSettled)
          onSettled();
      });
    }
    settle_->start();
  }

private:
  QTimer *settle_ = nullptr;
  QPointF last_;
  Qt::MouseButton drag_ = Qt::NoButton;
};
QSpinBox *scaleSpin(double denom) {
  auto *scale = new QSpinBox;
  scale->setRange(10, 2000000000);
  scale->setSingleStep(10);
  scale->setKeyboardTracking(false);
  scale->setPrefix(QStringLiteral("1:"));
  scale->setButtonSymbols(QAbstractSpinBox::UpDownArrows);
  scale->setValue(std::max(10, int(std::lround(denom))));
  scale->setToolTip(
      QStringLiteral("위·아래 화살표는 10 단위, 키보드로 직접 입력 가능"));
  return scale;
}
void bindScaleTyping(QSpinBox *scale, std::function<void(int)> onTyped) {
  auto *le = scale->findChild<QLineEdit *>();
  if (!le)
    return;
  auto *tm = new QTimer(scale);
  tm->setSingleShot(true);
  tm->setInterval(350);
  QObject::connect(le, &QLineEdit::textEdited, tm, [tm] { tm->start(); });
  QObject::connect(tm, &QTimer::timeout, scale, [scale, onTyped] {
    bool ok = false;
    const int v = scale->cleanText().trimmed().toInt(&ok);
    if (ok && v >= scale->minimum() && v <= scale->maximum())
      onTyped(v);
  });
}
QWidget *scaleWithArrows(QSpinBox *spin) {
  auto *w = new QWidget;
  auto *h = new QHBoxLayout(w);
  h->setContentsMargins(0, 0, 0, 0);
  h->setSpacing(2);
  h->addWidget(spin, 1);
  auto *col = new QVBoxLayout;
  col->setContentsMargins(0, 0, 0, 0);
  col->setSpacing(1);
  for (int up = 1; up >= 0; --up) {
    auto *b = new QToolButton;
    b->setObjectName("scaleArrow");
    b->setText(up ? QString::fromUtf8("▲") : QString::fromUtf8("▼"));
    b->setToolTip(up ? QStringLiteral("축척 분모 +10")
                     : QStringLiteral("축척 분모 −10"));
    b->setFocusPolicy(Qt::NoFocus);
    b->setAutoRepeat(true);
    b->setFixedSize(22, 14);
    b->setStyleSheet(
        QStringLiteral("QToolButton{font-size:8px;padding:0;border:1px solid "
                       "#C2C0B6;border-radius:4px;background:#FFFFFF;color:#"
                       "3D3D3A}QToolButton:hover{background:#F5F4ED}"
                       "QToolButton:pressed{background:#E8E6DC}"));
    QObject::connect(b, &QToolButton::clicked, spin, [spin, up] {
      spin->interpretText();
      const int cur = spin->value();
      const int next = up ? ((cur / 10) * 10 + 10)
                          : std::max(10, ((cur + 9) / 10) * 10 - 10);
      spin->setValue(next);
    });
    col->addWidget(b);
  }
  h->addLayout(col);
  return w;
}
void bindImageDrag(SheetPreview *pv, double *dx, double *dy) {
  pv->onImageDrag = [pv, dx, dy](QPointF d) {
    const double px = std::max(1e-6, pv->fitPxPerMm);
    *dx += d.x() / px;
    *dy += d.y() / px;
    pv->update();
  };
}
QToolButton *chipBtn(const QString &t, QButtonGroup *g, int id,
                     const char *pos = nullptr, int minW = 56) {
  auto *b = new QToolButton;
  b->setObjectName("chip");
  b->setCheckable(true);
  b->setText(t);
  b->setFocusPolicy(Qt::NoFocus);
  b->setMinimumWidth(minW);
  if (pos)
    b->setProperty("pos", pos);
  g->addButton(b, id);
  return b;
}
QLabel *sideHead(const QString &t) {
  auto *l = new QLabel(t);
  l->setObjectName("sideHead");
  return l;
}
QLabel *lbl(const QString &t, const char *obj = nullptr) {
  auto *l = new QLabel(t);
  if (obj)
    l->setObjectName(obj);
  return l;
}

// ---- 디자인 v5 §8 조판 판 부품(Strata 「항목 타일 · 축척 격자 · 도면 점검」과
// 같은 모양). Q_OBJECT 없음
QWidget *
sideCaption(const QString &t) { // 절 이름 11/700 Muted + 1 px Line 리더
  auto *w = new QWidget;
  auto *h = new QHBoxLayout(w);
  h->setContentsMargins(0, 6, 0, 0);
  h->setSpacing(8);
  h->addWidget(sideHead(t));
  auto *rule = new QFrame;
  rule->setObjectName("sideRule");
  rule->setFixedHeight(1);
  h->addWidget(rule, 1);
  return w;
}
QToolButton *sheetTile(const QString &icon, const QString &text,
                       const QString &tip) { // 72×56 켜짐 = Oat + Ring
  auto *b = new QToolButton;
  b->setObjectName("sheetTile");
  b->setCheckable(true);
  b->setFocusPolicy(Qt::NoFocus);
  b->setIcon(kerf::icon(icon, 20, theme::Hand));
  b->setIconSize(QSize(20, 20));
  b->setText(text);
  b->setToolTip(tip);
  b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
  return b;
}
QDoubleSpinBox *
ptSpin(double v, const QString &tip) { // 레벨선 굵기 pt 칸(모노 72, 0.05–3.0)
  auto *sp = new QDoubleSpinBox;
  sp->setRange(0.05, 3.0);
  sp->setDecimals(2);
  sp->setSingleStep(0.05);
  sp->setSuffix(QStringLiteral(" pt"));
  sp->setValue(v);
  sp->setFixedWidth(72);
  sp->setAlignment(Qt::AlignRight);
  sp->setToolTip(tip);
  return sp;
}
QWidget *scaleGridWidget(QSpinBox *spin) { // 축척 격자 4열 × 2줄 — 누르면 칸에
                                           // 값, 칸 값과 같은 칸이 켜짐
  auto *w = new QWidget;
  auto *g = new QGridLayout(w);
  g->setContentsMargins(0, 0, 0, 0);
  g->setHorizontalSpacing(4);
  g->setVerticalSpacing(4);
  static const int vals[8] = {10, 20, 30, 40, 50, 100, 200, 500};
  QList<QToolButton *> btns;
  for (int i = 0; i < 8; ++i) {
    auto *b = new QToolButton;
    b->setObjectName("scaleGrid");
    b->setCheckable(true);
    b->setFocusPolicy(Qt::NoFocus);
    b->setText(QString::number(vals[i]));
    b->setToolTip(QStringLiteral("1:%1").arg(vals[i]));
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    b->setProperty("denom", vals[i]);
    QObject::connect(b, &QToolButton::clicked, spin,
                     [spin, v = vals[i]] { spin->setValue(v); });
    g->addWidget(b, i / 4, i % 4);
    btns << b;
  }
  auto sync = [btns](int v) {
    for (auto *b : btns) {
      QSignalBlocker bl(b);
      b->setChecked(b->property("denom").toInt() == v);
    }
  };
  sync(spin->value());
  QObject::connect(spin, &QSpinBox::valueChanged, w, sync);
  return w;
}
struct PaperChips {
  QWidget *w = nullptr;
  QButtonGroup *size = nullptr;
  QButtonGroup *orient = nullptr;
};
PaperChips paperChips(const SheetSpec &s) { // 용지 [A4 | A3] [가로 | 세로]
  PaperChips pc;
  pc.w = new QWidget;
  auto *h = new QHBoxLayout(pc.w);
  h->setContentsMargins(0, 0, 0, 0);
  h->setSpacing(8);
  pc.size = new QButtonGroup(pc.w);
  pc.size->setExclusive(true);
  pc.orient = new QButtonGroup(pc.w);
  pc.orient->setExclusive(true);
  auto *a = new QWidget;
  {
    auto *hh = new QHBoxLayout(a);
    hh->setContentsMargins(0, 0, 0, 0);
    hh->setSpacing(2);
    hh->addWidget(chipBtn(QStringLiteral("A4"), pc.size, 0, "first"));
    hh->addWidget(chipBtn(QStringLiteral("A3"), pc.size, 1, "last"));
  }
  auto *o = new QWidget;
  {
    auto *hh = new QHBoxLayout(o);
    hh->setContentsMargins(0, 0, 0, 0);
    hh->setSpacing(2);
    hh->addWidget(chipBtn(QStringLiteral("가로"), pc.orient, 0, "first"));
    hh->addWidget(chipBtn(QStringLiteral("세로"), pc.orient, 1, "last"));
  }
  h->addWidget(a);
  h->addWidget(o);
  h->addStretch();
  pc.size->button(s.paper == Paper::A3 ? 1 : 0)->setChecked(true);
  pc.orient->button(s.landscape ? 0 : 1)->setChecked(true);
  return pc;
}
void syncPaperChips(const PaperChips &pc, const SheetSpec &s) {
  if (auto *b = pc.size->button(s.paper == Paper::A3 ? 1 : 0)) {
    QSignalBlocker bl(b);
    b->setChecked(true);
  }
  if (auto *b = pc.orient->button(s.landscape ? 0 : 1)) {
    QSignalBlocker bl(b);
    b->setChecked(true);
  }
}
struct CheckItem {
  bool ok = true;
  QString text;
  QString link;
  std::function<void()> fn;
};
class SheetCheckBox
    : public QFrame { // 「도면 점검」 상자: 항목마다 ✓(Done) 또는 ▲(Caution) +
                      // 문장 + 고치는 곳 링크. 저장을 막지 않는다
public:
  SheetCheckBox() {
    setObjectName(QStringLiteral("sheetCheck"));
    setAttribute(Qt::WA_StyledBackground);
    lay_ = new QVBoxLayout(this);
    lay_->setContentsMargins(12, 10, 12, 10);
    lay_->setSpacing(6);
  }
  void setItems(const std::vector<CheckItem> &items) {
    MainWindow::clearLayout(lay_); // 같은 비우기 한 곳(AC14) — 여기는 줄 위젯만
                                   // 담겨 전부터 문제 없었음
    for (const CheckItem &c : items) {
      auto *row = new QWidget(this);
      auto *h = new QHBoxLayout(row);
      h->setContentsMargins(0, 0, 0, 0);
      h->setSpacing(8);
      auto *mark = new QLabel(c.ok ? QStringLiteral("✓") : QStringLiteral("▲"));
      mark->setObjectName(c.ok ? "checkOk" : "checkWarn");
      mark->setFixedWidth(14);
      mark->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
      auto *tx = new QLabel(c.text);
      tx->setObjectName("checkText");
      tx->setWordWrap(true);
      h->addWidget(mark, 0, Qt::AlignTop);
      h->addWidget(tx, 1);
      if (!c.link.isEmpty()) {
        auto *b = new QPushButton(c.link);
        b->setObjectName("quietLink");
        b->setFlat(true);
        b->setFocusPolicy(Qt::NoFocus);
        b->setCursor(Qt::PointingHandCursor);
        if (c.fn)
          QObject::connect(b, &QPushButton::clicked, b, [fn = c.fn] { fn(); });
        h->addWidget(b, 0, Qt::AlignTop);
      }
      lay_->addWidget(row);
      row->show();
    }
  }

private:
  QVBoxLayout *lay_ = nullptr;
};
QWidget *
sideBottom(QPushButton *def, QPushButton *primary, QPushButton *secondary,
           QLabel **done,
           QPushButton *extra =
               nullptr) { // 맨 아래 Line 위: 「n장 저장됨 · 폴더 열기」(저장
                          // 뒤) / 조용한 「☆ 기본으로」 | 보조 | 흙색 주 단추
  auto *w = new QWidget;
  w->setObjectName(QStringLiteral("sideBottom"));
  w->setAttribute(Qt::WA_StyledBackground);
  auto *v = new QVBoxLayout(w);
  v->setContentsMargins(12, 8, 12, 10);
  v->setSpacing(6);
  auto *d = new QLabel;
  d->setObjectName(QStringLiteral("sheetDone"));
  d->setTextFormat(Qt::RichText);
  d->setOpenExternalLinks(false);
  d->hide();
  QObject::connect(d, &QLabel::linkActivated, d, [](const QString &href) {
    QDesktopServices::openUrl(QUrl(href));
  });
  v->addWidget(d);
  if (done)
    *done = d;
  auto *h = new QHBoxLayout;
  h->setContentsMargins(0, 0, 0, 0);
  h->setSpacing(6);
  if (extra) { // 인쇄(2026-10-12): 한 줄에 넷은 320 px 판에서 「☆ 기본으로」가
               // 눌려 잘리므로 윗줄 = 기본으로 · 인쇄, 아랫줄 = 보조 · 주 단추
    auto *h0 = new QHBoxLayout;
    h0->setContentsMargins(0, 0, 0, 0);
    h0->setSpacing(6);
    h0->addWidget(def);
    h0->addStretch();
    h0->addWidget(extra);
    v->addLayout(h0);
    h->addStretch();
    if (secondary)
      h->addWidget(secondary);
    h->addWidget(primary);
  } else {
    h->addWidget(def);
    h->addStretch();
    if (secondary)
      h->addWidget(secondary);
    h->addWidget(primary);
  }
  v->addLayout(h);
  return w;
}
// 책상 위 떠 있는 것(스펙 §8): 왼쪽 위 「도면 보기」 안내 칩 + 아래 가운데 도구
// 줄 6칸 64×52
void addDeskFloats(SheetPreview *pv, QAction *editAct,
                   std::function<void()> paperCycle,
                   std::function<void()> listToggle) {
  auto *g = new GuideBand(pv);
  g->setReserveRight(0);
  g->setTool(QStringLiteral("sheet"), QStringLiteral("도면 보기"));
  g->setHint(QStringLiteral(
      "휠 = 종이 확대 · 왼쪽 끌기 = 옮기기 · 두 번 = 처음 크기"));
  g->show();
  g->place();
  auto *tools = new FloatButtons(pv, Qt::Horizontal,
                                 Qt::AlignHCenter | Qt::AlignBottom, 12);
  auto zoomBy = [pv](double f) {
    pv->pageZoom = std::clamp(pv->pageZoom * f, 0.5, 8.0);
    pv->update();
  };
  QObject::connect(tools->add(QStringLiteral("square-dashed"),
                              QStringLiteral("맞춤 — 종이를 처음 크기로"),
                              nullptr, QStringLiteral("맞춤")),
                   &QToolButton::clicked, pv, [pv] {
                     pv->pageZoom = 1;
                     pv->viewPan = QPointF();
                     pv->update();
                   });
  QObject::connect(tools->add(QStringLiteral("zoom-in"), QStringLiteral("확대"),
                              nullptr, QStringLiteral("확대")),
                   &QToolButton::clicked, pv, [zoomBy] { zoomBy(1.25); });
  QObject::connect(tools->add(QStringLiteral("zoom-out"),
                              QStringLiteral("축소"), nullptr,
                              QStringLiteral("축소")),
                   &QToolButton::clicked, pv, [zoomBy] { zoomBy(0.8); });
  tools->add(QStringLiteral("edit-move"),
             QStringLiteral("조판편집 — 켜면 가운데 버튼으로 칸 안 그림을 "
                            "옮기고, 휠은 축척을 바꿉니다"),
             editAct, QStringLiteral("조판편집"));
  QObject::connect(
      tools->add(
          QStringLiteral("paper"),
          QStringLiteral(
              "용지 · 방향 바꾸기 (A4 가로 → A4 세로 → A3 가로 → A3 세로)"),
          nullptr, QStringLiteral("용지/방향")),
      &QToolButton::clicked, pv, [paperCycle] { paperCycle(); });
  QObject::connect(tools->add(QStringLiteral("list"),
                              QStringLiteral("도면 목록 보이기 · 숨기기"),
                              nullptr, QStringLiteral("도면 목록")),
                   &QToolButton::clicked, pv, [listToggle] { listToggle(); });
  tools->show();
  tools->place();
}
// 종이: Card · Edge 1 px · 둥글기 0 · 그림자 없음(스펙 §8)
void paintPaperFrame(QPainter &p, const QPointF &o, const QSizeF &pz) {
  p.setPen(QPen(theme::Edge, 1));
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(QRectF(o, pz), 4,
                    4); // 화면 테두리만 둥글게(종이 위 도면 · 인쇄물은 그대로)
}
} // namespace

namespace {
std::atomic<int> g_pvStarted{0}, g_pvActive{0}, g_pvPeak{0};
struct PreviewActive { // 조판 미리보기 작업 스레드 하나가 도는 동안(동시에 몇
                       // 개 도는지 셈)
  PreviewActive() {
    const int a = ++g_pvActive;
    int p = g_pvPeak.load();
    while (a > p && !g_pvPeak.compare_exchange_weak(p, a)) {
    }
  }
  ~PreviewActive() { --g_pvActive; }
};
} // namespace

MainWindow::SheetPreviewCounters MainWindow::sheetPreviewCounters() {
  return {g_pvStarted.load(), g_pvActive.load(), g_pvPeak.load()};
}

namespace {
// 조판의 고해상 입면 · 위치도를 다시 계산해야 하는 차이: 단면선 양 끝 · 앞/뒤
// 깊이 · 깊이 음영. 불투명도 · 선 굵기 · 평활은 그리기(또는 단면선)만 바뀌므로
// 다시 계산하지 않는다
bool sameElevation(const SectionDoc &a, const SectionDoc &b) {
  const SectionLine &x = a.r.line;
  const SectionLine &y = b.r.line;
  auto eq = [](double u, double v) { return std::fabs(u - v) < 1e-9; };
  return eq(x.a.x, y.a.x) && eq(x.a.y, y.a.y) && eq(x.b.x, y.b.x) &&
         eq(x.b.y, y.b.y) && eq(x.front, y.front) && eq(x.back, y.back) &&
         a.st.depthFade == b.st.depthFade;
}
} // namespace

QDialog *MainWindow::buildSheetDialog(SheetParams &io, bool &accepted) {
  accepted = false;
  auto *d = new QDialog(this);
  d->setWindowTitle(QStringLiteral("단면도"));
  d->setObjectName("sheetDialog");
  auto st = std::make_shared<SheetParams>(io);
  auto docP = std::make_shared<SectionDoc>(
      section_->doc()); // 도면 탭에서 입면을 바꾸면 sheetRefresh_ 가 새
                        // 단면으로 바꿔 넣는다(2026-10-11)
  // 미리보기 영상은 화면 것((*docP).img)으로 바로 열고, 더 촘촘한 입면 영상(1.5
  // mm/px)과 위치도 영상은 작업 스레드가 만들어 뒤에 바꿔 넣는다 (전환이
  // 느리다는 요청 2026-10-11 — 큰 모델에서 UI 스레드가 수 초 멈추던 곳)
  auto screenImgP = std::make_shared<QImage>((*docP).img);
  auto screenGeoP = std::make_shared<SectionImgGeo>(
      SectionImgGeo{(*docP).imgS0, (*docP).imgZ1, (*docP).imgRes});
  auto screenPlan = std::make_shared<PlanStrip>();
  auto runCancel = std::make_shared<std::shared_ptr<
      std::atomic<bool>>>(); // 지금 도는 미리보기 계산의 취소 표시 — 새 계산이
                             // 시작되거나 조판이 닫히면 멈춘다
  QObject::connect(d, &QObject::destroyed, [runCancel] {
    if (*runCancel)
      (*runCancel)->store(true);
  });
  auto *outer = new QHBoxLayout(d);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(0);
  // ---- 책상(Desk): 종이 가운데, 떠 있는 안내 · 도구 줄은 아래에서
  auto *pv = new SheetPreview;
  pv->setMinimumSize(470, 330);
  // 고해상 입면·위치도 영상을 작업 스레드로(열 때 한 번, 그리고 도면 탭에서
  // 입면을 바꿔 단면이 다시 계산될 때마다)
  auto previewGen = std::make_shared<std::atomic<int>>(
      0); // 늦게 끝난 옛 미리보기 스레드 결과는 버린다
  auto startPreview = std::make_shared<std::function<void()>>();
  *startPreview = [this, docP, st, screenImgP, screenGeoP, screenPlan,
                   runCancel, previewGen,
                   pv] { // CI 는 MSVC C++17: [=, this] 는 C++20(C3791)
    if (!src_)
      return;
    // 앞 계산은 바로 멈춘다 — 전에는 멈추지 않아 불투명도 끌기 · 단면이 바뀔
    // 때마다 고해상 계산이 쌓여 점점 느려졌다(사용자 신고 2026-10-11)
    if (*runCancel)
      (*runCancel)->store(true);
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    *runCancel = cancel;
    const int myGen = ++*previewGen;
    const SectionDoc docSnap = *docP;
    const bool needImg =
        docSnap.img.width() < 2400 &&
        docSnap.imgRes > 0.0025; // 화면 미리보기용: 이미 2400 px 이상이면
                                 // 충분(출력 시에는 600 dpi 로 따로 그림)
    const SheetGeom G0 = sheetGeom(docSnap, *st);
    const double h0 =
        planStripHalfWidth(docSnap.r.line.front, docSnap.r.line.back);
    const double planRes = groundResolution(std::max(10.0, G0.spec.denom), 96);
    const double imgRes = std::max(0.002, G0.lenM / 4000.0);
    const bool needPlan = G0.lenM > 0 && st->withPlan;
    ++g_pvStarted;
    std::thread([src = src_, docSnap, needImg, needPlan, h0, planRes, imgRes,
                 screenImgP, screenGeoP, screenPlan, cancel, previewGen, myGen,
                 pvp = QPointer<SheetPreview>(pv)] {
      PreviewActive active;
      if (needPlan && !cancel->load()) { // 위치도부터(가볍다) → 바로 보이게
        PlanStrip ps;
        if (renderPlanStrip(*src, docSnap.r.line, h0, planRes, 1.5e6, ps,
                            cancel.get()) &&
            !cancel->load())
          QMetaObject::invokeMethod(
              qApp,
              [screenPlan, ps, pvp, previewGen, myGen] {
                if (previewGen->load() != myGen)
                  return;
                *screenPlan = ps;
                if (pvp)
                  pvp->update();
              },
              Qt::QueuedConnection);
      }
      if (needImg && !cancel->load()) {
        SectionRequest rq;
        rq.line = docSnap.r.line;
        rq.zRangeFromProfile = true;
        rq.depthFade = docSnap.st.depthFade ? kDepthFadeStrength : 0.0;
        rq.imageRes = imgRes;
        rq.maxImagePixels = size_t(20) << 20;
        SectionOutput out;
        std::string e;
        if (computeSection(*src, rq, out, &e, cancel.get()) &&
            !cancel->load()) {
          QImage im = toQImageS(out.image.img);
          const SectionImgGeo g{out.image.s0, out.image.z1, out.image.res};
          QMetaObject::invokeMethod(
              qApp,
              [screenImgP, screenGeoP, im, g, pvp, previewGen, myGen] {
                if (previewGen->load() != myGen)
                  return;
                *screenImgP = im;
                *screenGeoP = g;
                if (pvp)
                  pvp->update();
              },
              Qt::QueuedConnection);
        }
      }
    }).detach();
  };
  (*startPreview)();
  // 단면이 바뀌면(뒤·앞 깊이 · 깊이 음영 · 단면선) 0.3 s 모아서 한 번, 도면
  // 탭이 보일 때만 다시 계산 — 안 보이면 표시만 해 두고 보일 때(paintFn)
  auto previewDirty = std::make_shared<bool>(false);
  auto *previewTimer = new QTimer(d);
  previewTimer->setSingleShot(true);
  previewTimer->setInterval(300);
  QObject::connect(previewTimer, &QTimer::timeout, d,
                   [pv, startPreview, previewDirty] {
                     if (pv->isVisible()) {
                       *previewDirty = false;
                       (*startPreview)();
                     } else
                       *previewDirty = true;
                   });
  bindImageDrag(pv, &st->imgDxMm, &st->imgDyMm);
  // ---- 오른쪽 판: 머리 · 도면 항목 · 도면 정보 · 선 글자 · 형식 · 도면 점검 |
  // 바닥
  auto *side = new QWidget;
  side->setObjectName("dialogSide");
  side->setAttribute(Qt::WA_StyledBackground);
  side->setMinimumWidth(200);
  side->setMaximumWidth(600);
  auto *sv = new QVBoxLayout(side);
  sv->setContentsMargins(0, 0, 0, 0);
  sv->setSpacing(0);
  auto *scroll = new QScrollArea;
  scroll->setObjectName("sideScroll");
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto *body = new QWidget;
  body->setObjectName("sideBody");
  body->setAttribute(Qt::WA_StyledBackground);
  auto *v = new QVBoxLayout(body);
  v->setContentsMargins(12, 10, 12, 10);
  v->setSpacing(6);
  const int nSec = int(sections_.size()), kSec = current_ + 1;
  v->addWidget(lbl(
      nSec > 1 ? QStringLiteral("고른 도면 · 단면 %1 / %2").arg(kSec).arg(nSec)
               : QStringLiteral("고른 도면 · 단면"),
      "cap"));
  auto *nameLab = lbl(st->title, "sheetName");
  nameLab->setWordWrap(true);
  v->addWidget(nameLab);
  auto *subLab = lbl(QString(), "hint");
  subLab->setWordWrap(true);
  v->addWidget(subLab);
  // 도면 항목(단면도의 층): 입면 영상 · 단면선 · 레벨선 · 표제란(자 포함) ·
  // 위치도(단면 위 평면 띠). 진북 · 범례는 평면도에만
  v->addWidget(sideCaption(QStringLiteral("도면 항목")));
  auto *tImg =
      sheetTile(QStringLiteral("image-layer"), QStringLiteral("입면 영상"),
                QStringLiteral("입면 영상 (뒤 %1 m) — 레벨선 위, 단면선 아래")
                    .arg((*docP).r.line.back, 0, 'g', 3));
  auto *tLine = sheetTile(QStringLiteral("pen-line"), QStringLiteral("단면선"),
                          QStringLiteral("잘린 선 — 맨 위 · 빨강 0.35 mm"));
  auto *tLev = sheetTile(QStringLiteral("levels"), QStringLiteral("레벨선"),
                         QStringLiteral("레벨선과 표고 숫자 — 맨 뒤"));
  auto *tTitle = sheetTile(
      QStringLiteral("title-block"), QStringLiteral("표제란"),
      QStringLiteral("표제란 · 축척 막대(끄면 그림 칸 안에 작은 축척 막대)"));
  auto *tPlan = sheetTile(
      QStringLiteral("image-layer"), QStringLiteral("위치도"),
      QStringLiteral("단면선 주변 평면(같은 축척, 단면선이 가로) — 단면 위. "
                     "앞·뒤 각 3 m(뒤 깊이가 크면 그 값). DXF 에는 없음"));
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);
    h->setSpacing(8);
    for (auto *tb : {tImg, tLine, tLev, tTitle})
      h->addWidget(tb);
    h->addStretch();
    v->addWidget(row);
  }
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(8); // 둘째 줄: 위치도(옆 칸 너비에 맞춰)
    h->addWidget(tPlan);
    h->addStretch();
    v->addWidget(row);
  }
  // 도면 정보: 도면명 · 용지 · 축척 1 : [칸] 맞춤 · 격자
  v->addWidget(sideCaption(QStringLiteral("도면 정보")));
  auto *g = new QGridLayout;
  g->setContentsMargins(0, 0, 0, 0);
  g->setHorizontalSpacing(10);
  g->setVerticalSpacing(8);
  g->setColumnMinimumWidth(0, 40);
  g->setColumnStretch(1, 1);
  int r = 0;
  auto *name = new QLineEdit(st->title);
  g->addWidget(lbl(QStringLiteral("도면명"), "hint"), r, 0);
  g->addWidget(name, r++, 1);
  PaperChips pc = paperChips(st->spec);
  g->addWidget(lbl(QStringLiteral("용지"), "hint"), r, 0);
  g->addWidget(pc.w, r++, 1);
  auto *scale = scaleSpin(st->spec.denom);
  auto *fitLink = new QPushButton;
  fitLink->setObjectName("quietLink");
  fitLink->setFlat(true);
  fitLink->setFocusPolicy(Qt::NoFocus);
  fitLink->setCursor(Qt::PointingHandCursor);
  fitLink->setToolTip(
      QStringLiteral("단면이 그림 칸에 다 들어가는 가장 작은 10 단위"));
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    h->addWidget(scaleWithArrows(scale), 1);
    h->addWidget(fitLink);
    g->addWidget(lbl(QStringLiteral("축척"), "hint"), r, 0);
    g->addWidget(row, r++, 1);
  }
  g->addWidget(scaleGridWidget(scale), r++, 1);
  v->addLayout(g);
  // 선 · 글자: 레벨선 pt 둘 · 숫자 · 빗금 · 기준선
  v->addWidget(sideCaption(QStringLiteral("선 · 글자")));
  auto *g2 = new QGridLayout;
  g2->setContentsMargins(0, 0, 0, 0);
  g2->setHorizontalSpacing(10);
  g2->setVerticalSpacing(8);
  g2->setColumnMinimumWidth(0, 40);
  g2->setColumnStretch(1, 1);
  r = 0;
  auto *ptMinor = ptSpin(st->levelMinorPt,
                         QStringLiteral("10 cm 레벨선 굵기 — 일러스트레이터 "
                                        "pt. 종이 위 mm = pt × 25.4 / 72"));
  auto *ptMajor = ptSpin(st->levelMajorPt,
                         QStringLiteral("50 cm(1 m 포함) 레벨선 굵기 — pt"));
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    h->addWidget(lbl(QStringLiteral("10 cm"), "faint"));
    h->addWidget(ptMinor);
    h->addSpacing(6);
    h->addWidget(lbl(QStringLiteral("50 cm"), "faint"));
    h->addWidget(ptMajor);
    h->addStretch();
    g2->addWidget(lbl(QStringLiteral("레벨선"), "hint"), r, 0);
    g2->addWidget(row, r++, 1);
  }
  auto *numLab = lbl(QString(), "faint");
  g2->addWidget(lbl(QStringLiteral("숫자"), "hint"), r, 0);
  g2->addWidget(numLab, r++, 1);
  auto *hatchG = new QButtonGroup(d);
  hatchG->setExclusive(true);
  auto *hatchMm = new QDoubleSpinBox;
  hatchMm->setRange(0.2, 5.0);
  hatchMm->setDecimals(1);
  hatchMm->setSingleStep(0.1);
  hatchMm->setSuffix(QStringLiteral(" mm"));
  hatchMm->setValue(st->hatchMm);
  hatchMm->setFixedWidth(64);
  hatchMm->setAlignment(Qt::AlignRight);
  hatchMm->setToolTip(
      QStringLiteral("빗금 간격(종이 위 mm) — 잘린 돌(H)에 씀"));
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    auto *chips = new QWidget;
    {
      auto *hh = new QHBoxLayout(chips);
      hh->setContentsMargins(0, 0, 0, 0);
      hh->setSpacing(2);
      hh->addWidget(chipBtn(QStringLiteral("45°"), hatchG, 0, "first", 40));
      hh->addWidget(chipBtn(QStringLiteral("검은 칠"), hatchG, 1, nullptr, 56));
      hh->addWidget(chipBtn(QStringLiteral("점묘"), hatchG, 2, "last", 40));
    }
    h->addWidget(chips);
    h->addWidget(hatchMm);
    h->addStretch();
    g2->addWidget(lbl(QStringLiteral("빗금"), "hint"), r, 0);
    g2->addWidget(row, r++, 1);
  }
  auto *cbBase = new QCheckBox(QStringLiteral("기준선"));
  cbBase->setChecked(st->showBaseline);
  cbBase->setFocusPolicy(Qt::NoFocus);
  auto *baseEl = new QDoubleSpinBox;
  baseEl->setRange(-1000, 10000);
  baseEl->setDecimals(2);
  baseEl->setSingleStep(0.1);
  baseEl->setPrefix("EL. ");
  baseEl->setSuffix(" m");
  baseEl->setValue(st->baselineEl);
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    h->addWidget(cbBase);
    h->addWidget(baseEl, 1);
    g2->addWidget(row, r++, 0, 1, 2);
  }
  v->addLayout(g2);
  // 형식: SVG 가 먼저(일러스트레이터)
  v->addWidget(sideCaption(QStringLiteral("형식")));
  auto *fmtG = new QButtonGroup(d);
  fmtG->setExclusive(true);
  auto *fmtRow = new QWidget;
  {
    auto *h = new QHBoxLayout(fmtRow);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(2);
    h->addWidget(chipBtn(QStringLiteral("SVG"), fmtG, 4, "first", 48));
    h->addWidget(chipBtn(QStringLiteral("DXF"), fmtG, 1, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("PDF"), fmtG, 0, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("PNG"), fmtG, 2, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("TIFF"), fmtG, 3, "last", 48));
    h->addStretch();
    v->addWidget(fmtRow);
  }
  // 도면 점검
  v->addWidget(sideCaption(QStringLiteral("도면 점검 · 저장을 막지 않음")));
  auto *check = new SheetCheckBox;
  v->addWidget(check);
  v->addStretch(1);
  scroll->setWidget(body);
  sv->addWidget(scroll, 1);
  auto *bDefault = new QPushButton(QStringLiteral("☆ 기본으로"));
  bDefault->setObjectName("quiet");
  bDefault->setFocusPolicy(Qt::NoFocus);
  bDefault->setToolTip(QStringLiteral(
      "이 설정(용지 · 축척 · 항목 · 형식 · 선 굵기)을 다음 도면의 기본으로"));
  auto *bSave = new QPushButton;
  bSave->setObjectName("primary");
  bSave->setDefault(true);
  auto *bPdf = new QPushButton(QStringLiteral("PDF"));
  bPdf->setFocusPolicy(Qt::NoFocus);
  bPdf->setToolTip(QStringLiteral("같은 도면을 PDF 로"));
  auto *bPrint = new QPushButton(QStringLiteral("인쇄…"));
  bPrint->setObjectName("sheetPrint");
  bPrint->setFocusPolicy(Qt::NoFocus);
  bPrint->setToolTip(QStringLiteral(
      "인쇄(Ctrl+P) — 미리보기 창에서 프린터 · 쪽 설정. 도면 용지 그대로 100 "
      "%(줄이지 않음) — 축척이 종이에서도 맞음"));
  {
    QLabel *done = nullptr;
    sv->addWidget(sideBottom(bDefault, bSave, bPdf, &done, bPrint));
    sheetDoneLabel_ = done;
  }
  QObject::connect(bPrint, &QPushButton::clicked, d,
                   [this, docP, st] { printSectionSheet(*docP, *st); });
  auto *dlgSplit = new QSplitter(Qt::Horizontal, d);
  dlgSplit->setObjectName("sheetDialogSplitter");
  dlgSplit->setHandleWidth(4);
  dlgSplit->setChildrenCollapsible(false);
  dlgSplit->addWidget(pv);
  dlgSplit->addWidget(side);
  dlgSplit->setStretchFactor(0, 1);
  dlgSplit->setStretchFactor(1, 0);
  dlgSplit->setSizes({700, 320});
  outer->addWidget(dlgSplit, 1);

  // ---- 갱신
  const QString modelName = modelDisplayName(path_);
  auto refresh = std::make_shared<std::function<void()>>();
  *refresh = [=] {
    st->spec.denom = std::max(10.0, std::round(st->spec.denom));
    if (!scale->hasFocus() &&
        scale->value() != int(std::lround(st->spec.denom))) {
      QSignalBlocker bl(scale);
      scale->setValue(int(std::lround(st->spec.denom)));
    }
    syncPaperChips(pc, st->spec);
    SheetGeom G = sheetGeom((*docP), *st);
    SheetAdvice a = adviseSheet(G.fitSpec(), G.lenM, G.contentM());
    if (auto *b = fmtG->button(st->format)) {
      QSignalBlocker bl(b);
      b->setChecked(true);
    }
    if (auto *b = hatchG->button(st->hatchKind)) {
      QSignalBlocker bl(b);
      b->setChecked(true);
    }
    for (auto &pr : {std::make_pair(tImg, st->withImage),
                     std::make_pair(tLine, st->withLine),
                     std::make_pair(tLev, st->withLevels),
                     std::make_pair(tTitle, st->withTitle),
                     std::make_pair(tPlan, st->withPlan)}) {
      QSignalBlocker bl(pr.first);
      pr.first->setChecked(pr.second);
    }
    const double pxPerMm = 300 / 25.4, ui = pxPerMm * 25.4 / 96.0;
    LevelPlan lp =
        sectionLevelPlan(pxPerMm * 1000.0 / st->spec.denom, ui, true);
    numLab->setText(QStringLiteral("%1 cm마다 · 해발 EL. · 선 %2 cm")
                        .arg(lp.labelCm)
                        .arg(lp.lineCm));
    const double fitD = G.lenM > 0 && G.heightM > 0
                            ? fitDenomStep10(G.fitSpecMax(), G.lenM, G.heightM)
                            : st->spec.denom;
    fitLink->setText(QStringLiteral("맞춤 %1").arg(denomText(fitD)));
    nameLab->setText(st->title);
    const int pages = st->split ? G.L.cols * G.L.rows : 1;
    subLab->setText(
        QStringLiteral(
            "%1 · %2 · 그림 칸 %3 × %4 mm%5 · 바깥 도곽은 평면도와 같다")
            .arg(paperLabel(st->spec), denomText(st->spec.denom))
            .arg(G.L.plotW, 0, 'f', 0)
            .arg(G.L.plotH, 0, 'f', 0)
            .arg(pages > 1 ? QStringLiteral(" · %1장").arg(pages) : QString()));
    // 도면 점검
    std::vector<CheckItem> items;
    if (a.overflow) {
      QString what = G.L.cols > 1 ? QStringLiteral("단면 길이 %1 m(%2 mm)")
                                        .arg(G.lenM, 0, 'f', 2)
                                        .arg(G.L.contentW, 0, 'f', 0)
                                  : QStringLiteral("단면 높이 %1 m(%2 mm)")
                                        .arg(G.heightM, 0, 'f', 2)
                                        .arg(G.L.contentH, 0, 'f', 0);
      CheckItem c;
      c.ok = false;
      c.text = QStringLiteral("%1에서 %2가 %3 한 장에 안 들어갑니다%4")
                   .arg(denomText(st->spec.denom), what, paperLabel(st->spec),
                        G.L.cols > 1 ? QStringLiteral(" — 지금은 A 쪽 %1 m만")
                                           .arg(a.fitLenM, 0, 'f', 1)
                                     : QString());
      if (a.smallerDenom > 0) {
        c.link = QStringLiteral("%1으로").arg(denomText(a.smallerDenom));
        c.fn = [=] {
          st->spec.denom = a.smallerDenom;
          (*refresh)();
        };
      } else if (a.a3Fits) {
        c.link = QStringLiteral("A3로");
        c.fn = [=] {
          st->spec.paper = Paper::A3;
          (*refresh)();
        };
      } else if (a.rotateFits) {
        c.link = QStringLiteral("방향 바꾸기");
        c.fn = [=] {
          st->spec.landscape = !st->spec.landscape;
          (*refresh)();
        };
      } else {
        c.link = st->split ? QStringLiteral("한 장으로")
                           : QStringLiteral("나눠서 %1장").arg(a.splitSheets);
        c.fn = [=] {
          st->split = !st->split;
          (*refresh)();
        };
      }
      items.push_back(c);
    } else
      items.push_back({true,
                       QStringLiteral("%1 한 장에 들어갑니다 — 단면 %2 × %3 m")
                           .arg(paperLabel(st->spec))
                           .arg(G.lenM, 0, 'f', 2)
                           .arg(G.heightM, 0, 'f', 2),
                       QString(), nullptr});
    {
      const std::vector<SGap> gaps =
          profileGaps((*docP).r.profile, SectionFrame((*docP).r.line).L);
      double gl = 0;
      for (const SGap &gp : gaps)
        gl += gp.s1 - gp.s0;
      if (gaps.empty())
        items.push_back(
            {true,
             QStringLiteral("빈 구간 없음 · 잘린 돌은 H 도구에서(F 단계)"),
             QString(), nullptr});
      else
        items.push_back(
            {false,
             QStringLiteral("빈 구간 %1곳 %2 m — 단면 화면에서 확인하세요")
                 .arg(gaps.size())
                 .arg(gl, 0, 'f', 2),
             QString(), nullptr});
    }
    {
      const QString ext =
          st->format == 4
              ? QStringLiteral("svg")
              : QString::fromLatin1(kFormatExt[std::clamp(st->format, 0, 3)]);
      const QString fname =
          QFileInfo(
              qs8(sheetFileName(modelName.toStdString(),
                                sectionName().toStdString(), st->spec.denom)))
              .completeBaseName() +
          QLatin1Char('.') + ext;
      items.push_back(
          {true, QStringLiteral("파일 이름 %1 (같은 이름이면 _2)").arg(fname),
           QString(), nullptr});
    }
    check->setItems(items);
    const QString fmtName =
        st->format == 4
            ? QStringLiteral("SVG")
            : QString::fromLatin1(kFormatName[std::clamp(st->format, 0, 3)]);
    bSave->setText(QStringLiteral("%1장 %2 저장")
                       .arg(std::max(1, checkedSheetCount()))
                       .arg(fmtName));
    bPdf->setVisible(st->format != 0);
    QString hn = st->format == 4
                     ? QStringLiteral("그리기용 빈 층 DRAW_SOIL · DRAW_OUTLINE "
                                      "2개 · 밑그림은 잠금. ")
                 : st->format == 1
                     ? QStringLiteral("DXF 는 모델 공간 %1 (용지 · 표제란 "
                                      "없이, 레벨선 · A/A′ 포함). 빈 층 2개. ")
                           .arg(denomText(st->spec.denom))
                     : QString();
    fmtRow->setToolTip(hn);
    baseEl->setEnabled(st->showBaseline);
    pv->update();
  };
  onSheetChecksChanged_ = [refresh] { (*refresh)(); };
  pv->onSettled = [=] {
    if (std::abs(pv->zoom - 1.0) >
        1e-6) { // 조판편집 중 휠로 바꾼 크기 = 새 축척
      const int vv =
          int(std::lround(snapScaleDenom10(st->spec.denom / pv->zoom)));
      pv->zoom = 1;
      if (scale->value() != vv)
        scale->setValue(vv);
    }
    pv->update();
  };
  // 도면 탭에서 입면(뒤·앞 깊이, 불투명도, 선 굵기, 평활)을 바꾸면 단면이 다시
  // 계산되고, 끝나면 여기로 새 단면이 들어온다(사용자 요청 2026-10-11) 이
  // 조판은 지금 단면(secIdx) 것 — 작업 탭에서 다른 단면을 고르면 건드리지
  // 않는다(그 단면의 조판은 도면을 다시 열 때 새로 만든다)
  const int secIdx = current_;
  const std::string secName =
      (current_ >= 0 && current_ < int(sections_.size()))
          ? sections_[size_t(current_)].name
          : std::string();
  sheetRefresh_ = [this, docP, screenImgP, screenGeoP, previewTimer, refresh,
                   secIdx, secName] {
    if (!section_ || !section_->hasResult())
      return;
    if (current_ != secIdx ||
        (secIdx >= 0 && (secIdx >= int(sections_.size()) ||
                         sections_[size_t(secIdx)].name != secName)))
      return;
    SectionDoc nd = section_->doc();
    const bool imageChanged = !sameElevation(*docP, nd);
    *docP = std::move(nd);
    if (imageChanged) { // 화면 영상으로 바로 바꿔 보여 주고, 고해상 입면은 0.3
                        // s 뒤 한 번(끌기 · 연타는 합쳐짐)
      *screenImgP = docP->img;
      *screenGeoP = SectionImgGeo{docP->imgS0, docP->imgZ1, docP->imgRes};
      previewTimer->start();
    } else if (docP->img.width() > screenImgP->width()) {
      *screenImgP = docP->img;
      *screenGeoP = SectionImgGeo{docP->imgS0, docP->imgZ1, docP->imgRes};
    }
    (*refresh)();
  };
  const quint64 refreshGen = ++sheetRefreshGen_;
  QObject::connect(d, &QObject::destroyed, this, [this, refreshGen] {
    if (sheetRefreshGen_ == refreshGen)
      sheetRefresh_ = nullptr;
  }); // 옛 조판이 늦게 지워져도 새 조판의 훅은 남긴다
  pv->paintFn = [=](QPainter &p, const QRect &rc) {
    if (*previewDirty) {
      *previewDirty = false;
      QTimer::singleShot(0, d, [startPreview] { (*startPreview)(); });
    } // 안 보일 때 미뤄 둔 계산
    p.fillRect(rc, theme::Desk);
    SheetGeom G = sheetGeom((*docP), *st);
    double sc = std::min((rc.width() - 48.0) / G.L.paperW,
                         (rc.height() - 110.0) /
                             G.L.paperH); // 위 안내 칩 · 아래 도구 줄 자리
    if (sc <= 0.05)
      return;
    sc *= pv->pageZoom;
    QSizeF pz(G.L.paperW * sc, G.L.paperH * sc);
    QPointF o(rc.left() + (rc.width() - pz.width()) / 2 + pv->viewPan.x(),
              rc.top() + (rc.height() - pz.height()) / 2 + pv->viewPan.y());
    pv->lastPaperO = o;
    pv->lastPaperSize = QPointF(pz.width(), pz.height());
    p.save();
    p.translate(o);
    p.setClipRect(QRectF(QPointF(0, 0), pz));
    pv->fitPxPerMm = sc;
    pv->zoomCenter = o + QPointF((G.L.plotX + G.L.plotW / 2) * sc,
                                 (G.L.plotY + G.L.plotH / 2) * sc);
    pv->geom = SheetPaintProbe();
    pv->letterA = (*docP).st.nameA;
    paintSheet(p, (*docP), *st, sc, 0, *screenImgP, *screenGeoP, pv->zoom, 0, 0,
               &pv->geom, screenPlan.get());
    pv->geomValid = true;
    pv->paintHover(p);
    p.restore();
    paintPaperFrame(p, o, pz);
  };
  auto *editAct = new QAction(QStringLiteral("조판편집"), d);
  editAct->setCheckable(true);
  QObject::connect(editAct, &QAction::toggled, d, [pv](bool on) {
    pv->editing = on;
    pv->setCursor(on ? Qt::SizeAllCursor : Qt::ArrowCursor);
  });
  addDeskFloats(
      pv, editAct,
      [=] {
        int idx = (st->spec.paper == Paper::A3 ? 2 : 0) +
                  (st->spec.landscape ? 0 : 1);
        idx = (idx + 1) % 4;
        st->spec.paper = idx >= 2 ? Paper::A3 : Paper::A4;
        st->spec.landscape = (idx % 2) == 0;
        (*refresh)();
      },
      [this] {
        if (sheetSide_)
          sheetSide_->setVisible(!sheetSide_->isVisible());
      });
  QObject::connect(name, &QLineEdit::textChanged, d, [=](const QString &tx) {
    st->title = tx;
    nameLab->setText(tx);
    pv->update();
  });
  QObject::connect(pc.size, &QButtonGroup::idClicked, d, [=](int id) {
    st->spec.paper = id == 1 ? Paper::A3 : Paper::A4;
    (*refresh)();
  });
  QObject::connect(pc.orient, &QButtonGroup::idClicked, d, [=](int id) {
    st->spec.landscape = id == 0;
    (*refresh)();
  });
  QObject::connect(scale, &QSpinBox::valueChanged, d, [=](int v) {
    st->spec.denom = std::max(10, v);
    (*refresh)();
  });
  bindScaleTyping(scale, [=](int v) {
    st->spec.denom = std::max(10, v);
    (*refresh)();
  });
  QObject::connect(fitLink, &QPushButton::clicked, d, [=] {
    SheetGeom G = sheetGeom((*docP), *st);
    if (G.lenM > 0 && G.heightM > 0)
      scale->setValue(
          int(std::lround(fitDenomStep10(G.fitSpecMax(), G.lenM, G.heightM))));
  });
  QObject::connect(tImg, &QToolButton::toggled, d, [=](bool on) {
    st->withImage = on;
    pv->update();
  });
  QObject::connect(tLine, &QToolButton::toggled, d, [=](bool on) {
    st->withLine = on;
    pv->update();
  });
  QObject::connect(tLev, &QToolButton::toggled, d, [=](bool on) {
    st->withLevels = on;
    pv->update();
  });
  QObject::connect(tTitle, &QToolButton::toggled, d, [=](bool on) {
    st->withTitle = on;
    pv->update();
  });
  QObject::connect(tPlan, &QToolButton::toggled, d, [=](bool on) {
    st->withPlan = on;
    if (on && screenPlan->img.isNull())
      (*startPreview)();
    (*refresh)();
  }); // 띠 높이만큼 배치가 바뀜
  QObject::connect(ptMinor, &QDoubleSpinBox::valueChanged, d, [=](double vv) {
    st->levelMinorPt = clampLineWeightPt(vv);
    pv->update();
  });
  QObject::connect(ptMajor, &QDoubleSpinBox::valueChanged, d, [=](double vv) {
    st->levelMajorPt = clampLineWeightPt(vv);
    pv->update();
  });
  QObject::connect(hatchG, &QButtonGroup::idClicked, d,
                   [=](int id) { st->hatchKind = id; });
  QObject::connect(hatchMm, &QDoubleSpinBox::valueChanged, d,
                   [=](double vv) { st->hatchMm = vv; });
  QObject::connect(fmtG, &QButtonGroup::idClicked, d, [=](int id) {
    st->format = id;
    (*refresh)();
  });
  QObject::connect(cbBase, &QCheckBox::toggled, d, [=](bool on) {
    st->showBaseline = on;
    (*refresh)();
  });
  QObject::connect(baseEl, &QDoubleSpinBox::valueChanged, d, [=](double vv) {
    st->baselineEl = vv;
    pv->update();
  });
  QObject::connect(bDefault, &QPushButton::clicked, d, [=] {
    QSettings s;
    s.setValue("sheet/paper", st->spec.paper == Paper::A3 ? 1 : 0);
    s.setValue("sheet/landscape", st->spec.landscape);
    s.setValue("sheet/denom", st->spec.denom);
    s.setValue("sheet/format", st->format);
    s.setValue("sheet/dpi", st->dpi);
    s.setValue("sheet/withImage", st->withImage);
    s.setValue("sheet/withLine", st->withLine);
    s.setValue("sheet/withLevels", st->withLevels);
    s.setValue("sheet/withTitle", st->withTitle);
    s.setValue("sheet/withPlan", st->withPlan);
    s.setValue("sheet/levelMinorPt", st->levelMinorPt);
    s.setValue("sheet/levelMajorPt", st->levelMajorPt);
    s.setValue("sheet/hatchKind", st->hatchKind);
    s.setValue("sheet/hatchMm", st->hatchMm);
    bDefault->setText(QStringLiteral("★ 저장함"));
  });
  auto doSave = [=, &io, &accepted] {
    io = *st; // io = *params(lastSheet_ 가 소유) — 살아 있음
    if (d->property("embed").toBool()) {
      saveEmbeddedSheet();
      return;
    } // 끼운 조판: accepted 는 이미 끝난 함수의 지역 변수(검토 C2)
    accepted = true;
    d->accept();
  };
  QObject::connect(bSave, &QPushButton::clicked, d, doSave);
  QObject::connect(bPdf, &QPushButton::clicked, d, [=] {
    st->format = 0;
    (*refresh)();
    doSave();
  });
  (*refresh)();
  d->resize(1000, 690);
  return d;
}

void MainWindow::showWorkTab() {
  if (!body_)
    return;
  if (!src_) {
    showStart(true);
    return;
  }
  body_->setCurrentIndex(1);
  if (viewTabs_ && viewTabs_->currentIndex() != 1) {
    QSignalBlocker b(viewTabs_);
    viewTabs_->setCurrentIndex(1);
  }
  setRibbonContext(1);
}

void MainWindow::saveEmbeddedSheet() {
  if (!embeddedSave_)
    return;
  const std::function<void()> save =
      embeddedSave_; // 실행 중 discardSheetTab 이 멤버를 비워도 캡처(doc · src
                     // · params)가 살아 있게(검토 C1)
  save();
}

void MainWindow::showSheetTab(QWidget *page) {
  if (!body_ || !page)
    return;
  page->setParent(nullptr);
  page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  page->show();
  if (sheetHost_ && sheetHost_ != page) {
    body_->removeWidget(sheetHost_);
    sheetHost_->deleteLater();
  }
  sheetHost_ = page;
  if (body_->indexOf(page) < 0)
    body_->addWidget(page);
  if (viewTabs_) { // v5 C2: 「도면 ×」 탭은 도면을 열 때 생기고 × 로 닫힌다
    if (viewTabs_->count() < 3)
      viewTabs_->addTab(kerf::icon(QStringLiteral("sheet"), 16, theme::Hand),
                        QStringLiteral("도면"));
    QSignalBlocker b(viewTabs_);
    viewTabs_->setCurrentIndex(2);
  }
  body_->setCurrentWidget(page);
  setRibbonContext(2);
}

void MainWindow::showSheetTab(QDialog *d, std::function<void()> save) {
  embeddedSave_ = std::move(save);
  d->setProperty("embed", true);
  d->setWindowFlags(Qt::Widget);
  for (auto *b : d->findChildren<QPushButton *>()) {
    if (b->text() == QStringLiteral("전체화면") ||
        b->text() == QStringLiteral("원래 크기")) {
      b->disconnect();
      b->hide(); // v4: 「작업으로」 단추는 없앰 — 문서 탭 「작업」 · Esc 와
                 // 같은 일
    }
  }
  for (auto *sc : d->findChildren<QShortcut *>())
    sc->deleteLater();
  auto *page = new QWidget;
  page->setObjectName("sheetPage");
  auto *h = new QHBoxLayout(page);
  h->setContentsMargins(0, 0, 0, 0);
  h->setSpacing(0);
  auto *pageSplit = new QSplitter(Qt::Horizontal, page);
  pageSplit->setObjectName("sheetPageSplitter");
  pageSplit->setHandleWidth(4);
  pageSplit->setChildrenCollapsible(false);
  pageSplit->addWidget(buildSheetSide());
  pageSplit->addWidget(d);
  pageSplit->setStretchFactor(0, 0);
  pageSplit->setStretchFactor(1, 1);
  const int totalW = width() > 0 ? width() : 1280;
  pageSplit->setSizes({348, std::max(200, totalW - 348)});
  h->addWidget(pageSplit, 1);
  showSheetTab(page);
  showStatus(QStringLiteral("도면 — 고른 %1장 · 다 되면 「SVG 저장」 · "
                            "돌아가기는 「단면」 탭 또는 Esc")
                 .arg(std::max(1, checkedSheetCount())));
}

// 조판 탭 왼쪽: 평면도 1줄 + 단면 목록. 누르면 그 도면 조판으로 바뀜(작업 화면
// 목록과 같은 선택)
QWidget *MainWindow::buildSheetSide() {
  auto *w = new QWidget;
  w->setObjectName("sidePanel");
  w->setAttribute(Qt::WA_StyledBackground);
  w->setMinimumWidth(160);
  w->setMaximumWidth(600);
  auto *v = new QVBoxLayout(w);
  v->setContentsMargins(10, 8, 10, 8);
  v->setSpacing(6);
  auto *head = new QHBoxLayout;
  head->setSpacing(6);
  head->addWidget(lbl(QStringLiteral("단면 목록"), "sectionHead"));
  sheetCount_ = lbl(QString::number(sections_.size()), "cap");
  head->addWidget(sheetCount_);
  head->addStretch();
  auto *all = new QPushButton(QStringLiteral("모두"));
  all->setObjectName("quiet");
  all->setFocusPolicy(Qt::NoFocus);
  all->setToolTip(
      QStringLiteral("모든 도면을 고르기 — 이미 모두 골랐으면 지금 도면만"));
  QObject::connect(all, &QPushButton::clicked, this, [this] {
    const bool allOn = !sheetChecked_.empty() &&
                       std::all_of(sheetChecked_.begin(), sheetChecked_.end(),
                                   [](bool b) { return b; });
    for (size_t i = 0; i < sheetChecked_.size(); ++i)
      sheetChecked_[i] = !allOn;
    if (allOn) {
      const int cur = sheetIsPlan_ ? 0 : current_ + 1;
      if (cur >= 0 && cur < int(sheetChecked_.size()))
        sheetChecked_[size_t(cur)] = true;
    }
    refreshSheetList();
    if (onSheetChecksChanged_)
      onSheetChecksChanged_();
  });
  auto *exp = new QPushButton(QStringLiteral("내보내기"));
  exp->setObjectName("quiet");
  exp->setFocusPolicy(Qt::NoFocus);
  exp->setToolTip(
      QStringLiteral("고른 도면을 저장 — 오른쪽 판 「n장 저장」과 같음"));
  QObject::connect(exp, &QPushButton::clicked, this,
                   [this] { saveEmbeddedSheet(); });
  head->addWidget(all);
  head->addWidget(exp);
  v->addLayout(head);
  auto *find = new QLineEdit;
  find->setObjectName("sheetFind");
  find->setPlaceholderText(QStringLiteral("이름으로 찾기"));
  find->setClearButtonEnabled(true);
  find->addAction(kerf::icon(QStringLiteral("search"), 14, theme::Faint),
                  QLineEdit::LeadingPosition);
  v->addWidget(find);
  auto *list = new QListWidget;
  list->setObjectName("sectionList");
  list->setFocusPolicy(Qt::NoFocus);
  list->setSpacing(0);
  QObject::connect(list, &QListWidget::itemClicked, this,
                   [this](QListWidgetItem *it) {
                     const int row = it->data(Qt::UserRole).toInt();
                     if (row == -1) // 절 이름 줄(-2 = 평·단면도 한 장)
                       return;
                     QTimer::singleShot(0, this, [this, row] {
                       if (sheetHost_)
                         openSheetFromList(row);
                     }); // 이 목록은 새 조판으로 바뀌며 지워지므로 다음 차례에.
                         // 그사이 탭이 버려졌으면 안 함(검토 M6)
                   });
  QObject::connect(
      find, &QLineEdit::textChanged, list, [this, list](const QString &tx) {
        const QString q = tx.trimmed();
        for (int i = 0; i < list->count(); ++i) {
          auto *it = list->item(i);
          const int row = it->data(Qt::UserRole).toInt();
          if (row <= 0)
            continue;
          const QString nm = row - 1 < int(sections_.size())
                                 ? qs8(sections_[size_t(row - 1)].name)
                                 : QString();
          it->setHidden(!q.isEmpty() && !nm.contains(q, Qt::CaseInsensitive));
        }
      });
  v->addWidget(list, 1);
  auto *foot = lbl(QStringLiteral("칸 = 저장할 도면 · 줄을 누르면 미리 "
                                  "봄\n축척 · 용지는 도면마다 자기 값"),
                   "faint");
  foot->setWordWrap(true);
  v->addWidget(foot);
  sheetSide_ = w;
  sheetList_ = list;
  w->setVisible(QSettings().value("ui/sectionList", true).toBool());
  refreshSheetList();
  return w;
}

// 목록 한 줄: 고르기 칸 14 + 썸네일 + 이름 + 「1:20 · A4 가로 · 저장 안 됨 /
// 10/09 15:10」(스펙 §8)
QWidget *MainWindow::sheetRowWidget(int row) {
  auto *w = new QWidget;
  w->setObjectName("secRow");
  auto *h = new QHBoxLayout(w);
  h->setContentsMargins(6, 5, 6, 5);
  h->setSpacing(8);
  auto *cb = new QCheckBox;
  cb->setFocusPolicy(Qt::NoFocus);
  cb->setToolTip(QStringLiteral("칸 = 「n장 저장」에 넣을 도면"));
  cb->setChecked(row >= 0 && row < int(sheetChecked_.size()) &&
                 sheetChecked_[size_t(row)]);
  QObject::connect(cb, &QCheckBox::toggled, this, [this, row](bool on) {
    if (row >= 0 && row < int(sheetChecked_.size()))
      sheetChecked_[size_t(row)] = on;
    if (onSheetChecksChanged_)
      onSheetChecksChanged_();
  });
  h->addWidget(cb);
  auto *th = new QLabel;
  th->setFixedSize(64, 30);
  th->setAlignment(Qt::AlignCenter);
  if (row == 0) {
    th->setText(QStringLiteral("평면"));
    th->setStyleSheet(
        "QLabel{background:#F5F4ED;border:1px solid "
        "#DEDCD1;border-radius:6px;color:#5E5D59;font-size:11px;}");
  } else {
    th->setStyleSheet("QLabel{background:#FFFFFF;border:1px solid "
                      "#DEDCD1;border-radius:6px;}");
    if (auto f = thumbs_.find(row - 1);
        f != thumbs_.end() && !f->second.isNull())
      th->setPixmap(QPixmap::fromImage(f->second.scaled(
          62, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
  }
  h->addWidget(th);
  const QString nm = row == 0 ? QStringLiteral("평면도")
                              : (row - 1 < int(sections_.size())
                                     ? qs8(sections_[size_t(row - 1)].name)
                                     : QString());
  auto *txt = new QWidget;
  auto *tv = new QVBoxLayout(txt);
  tv->setContentsMargins(0, 0, 0, 0);
  tv->setSpacing(0);
  tv->addWidget(lbl(nm, "secName"));
  tv->addWidget(lbl(sheetStatusText(row), "monoFaint"));
  if (row > 0 && row - 1 < int(sections_.size()) &&
      !sections_[size_t(row - 1)].note.empty())
    tv->addWidget(lbl(qs8(sections_[size_t(row - 1)].note), "faint"));
  h->addWidget(txt, 1);
  return w;
}

QString MainWindow::sheetStatusText(int row) const {
  QSettings st;
  const QString nm = row > 0 && row - 1 < int(sections_.size())
                         ? qs8(sections_[size_t(row - 1)].name)
                         : QString();
  const QString k =
      modelKey(path_) + (row == 0 ? QStringLiteral("sheet/plan/")
                                  : QStringLiteral("sheet/%1/").arg(nm));
  double denom = st.value(k + "denom", 0.0).toDouble();
  QString paper = st.value(k + "paper").toString();
  if (row == 0 && lastPlanSheet_) {
    denom = lastPlanSheet_->spec.denom;
    paper = paperLabel(lastPlanSheet_->spec);
  } else if (row > 0 && !sheetIsPlan_ && row - 1 == current_ && lastSheet_) {
    denom = lastSheet_->spec.denom;
    paper = paperLabel(lastSheet_->spec);
  }
  const QDateTime t =
      QDateTime::fromString(st.value(k + "saved").toString(), Qt::ISODate);
  const QString when = t.isValid() ? t.toString(QStringLiteral("MM/dd HH:mm"))
                                   : QStringLiteral("저장 안 됨");
  return (denom > 0 ? QStringLiteral("%1 · %2").arg(denomText(denom), paper)
                    : QStringLiteral("—")) +
         QStringLiteral(" · ") + when;
}

void MainWindow::noteSheetSaved(const QString &prefix, const SheetSpec &spec) {
  QSettings st;
  const QString k = modelKey(path_) + prefix;
  st.setValue(k + "denom", spec.denom);
  st.setValue(k + "paper", paperLabel(spec));
  st.setValue(k + "saved", QDateTime::currentDateTime().toString(Qt::ISODate));
  refreshSheetList();
}

int MainWindow::checkedSheetCount() const {
  return int(std::count(sheetChecked_.begin(), sheetChecked_.end(), true));
}

void MainWindow::setAllSheetsChecked(bool on) {
  sheetChecked_.assign(1 + sections_.size(), on);
  refreshSheetList();
}

// 고른 도면 전부를 dir 에(E4): 평면도는 마지막 평면 조판 설정(없으면 기본),
// 단면은 base(용지 · 항목 · 형식 · pt)에 도면마다 자기 축척(저장된 값 → 없으면
// 맞춤, 지금 연 도면은 판의 값). 동기 — 단면마다 selectSection +
// computeNow(최종). 끝나면 고른 단면으로 되돌린다
int MainWindow::saveCheckedSheets(const QString &dir, const SheetParams &base,
                                  QStringList *log) {
  if (!src_ || dir.isEmpty())
    return 0;
  QDir().mkpath(dir);
  const QString model = modelDisplayName(path_);
  std::vector<std::string> existing;
  for (const QString &n : QDir(dir).entryList(QDir::Files))
    existing.push_back(n.toStdString());
  const QString ext =
      base.format == 4
          ? QStringLiteral("svg")
          : QString::fromLatin1(kFormatExt[std::clamp(base.format, 0, 3)]);
  auto nameFor = [&](const QString &sheet, double denom) {
    QString n = QFileInfo(qs8(sheetFileName(model.toStdString(),
                                            sheet.toStdString(), denom)))
                    .completeBaseName() +
                QLatin1Char('.') + ext;
    n = qs8(uniqueSheetFileName(n.toStdString(), existing));
    existing.push_back(n.toStdString());
    return QDir(dir).filePath(n);
  };
  if (sheetChecked_.size() != 1 + sections_.size())
    sheetChecked_.resize(1 + sections_.size(), false);
  const int keepCur = current_;
  const bool keepPlan = sheetIsPlan_;
  QSettings st;
  const QString mk = modelKey(path_);
  int saved = 0;
  QApplication::setOverrideCursor(Qt::WaitCursor);
  for (size_t row = 0; row < sheetChecked_.size(); ++row) {
    if (!sheetChecked_[row])
      continue;
    QString msg, f;
    bool ok = false;
    if (row == 0) {
      PlanSheetParams pp =
          lastPlanSheet_ ? *lastPlanSheet_ : defaultPlanSheetParams();
      pp.format = base.format;
      f = nameFor(QStringLiteral("평면도"), pp.spec.denom);
      ok = exportPlanSheet(*src_, pp, f, &msg, nullptr, {});
      if (ok) {
        st.setValue(mk + "sheet/plan/denom", pp.spec.denom);
        st.setValue(mk + "sheet/plan/paper", paperLabel(pp.spec));
        st.setValue(mk + "sheet/plan/saved",
                    QDateTime::currentDateTime().toString(Qt::ISODate));
      }
    } else {
      const int i = int(row) - 1;
      if (i >= int(sections_.size()))
        continue;
      selectSection(i);
      QString e;
      if (!plan_->hasLine() || !computeNow(plan_->line(), &e)) {
        if (log)
          *log << QStringLiteral("sheet-batch %1: 단면 계산 실패 %2")
                      .arg(qs8(sections_[size_t(i)].name), e);
        continue;
      }
      SheetParams sp = base;
      const QString nm = sectionName();
      sp.title = QStringLiteral("%1 단면도").arg(nm);
      if (!sections_[size_t(i)].note.empty())
        sp.title += QStringLiteral(" · ") + qs8(sections_[size_t(i)].note);
      const SectionDoc doc = section_->doc();
      if (!(i == keepCur &&
            !keepPlan)) { // 지금 연 도면은 판의 축척 그대로, 나머지는 저장된 값
                          // → 없으면 맞춤(스펙 §8 「축척은 도면마다 자기 값」)
        const double saved0 =
            st.value(mk + QStringLiteral("sheet/%1/denom").arg(nm), 0.0)
                .toDouble();
        const SheetGeom G = sheetGeom(doc, sp);
        sp.spec.denom =
            saved0 > 0
                ? saved0
                : (G.lenM > 0 && G.heightM > 0
                       ? fitDenomStep10(G.fitSpecMax(), G.lenM, G.heightM)
                       : sp.spec.denom);
      }
      f = nameFor(nm, sp.spec.denom);
      ok = base.format == 4 ? exportSectionSvg(doc, *src_, sp, f, &msg)
                            : exportSheet(doc, *src_, sp, f, &msg, nullptr, {});
      if (ok) {
        const QString k = mk + QStringLiteral("sheet/%1/").arg(nm);
        st.setValue(k + "denom", sp.spec.denom);
        st.setValue(k + "paper", paperLabel(sp.spec));
        st.setValue(k + "saved",
                    QDateTime::currentDateTime().toString(Qt::ISODate));
      }
    }
    if (log)
      *log << QStringLiteral("sheet-batch %1: %2")
                  .arg(ok ? QStringLiteral("ok") : QStringLiteral("FAIL"),
                       ok ? f : msg);
    if (ok)
      ++saved;
  }
  if (keepCur >= 0 && keepCur < int(sections_.size()) && keepCur != current_) {
    selectSection(keepCur);
    QString e;
    if (plan_->hasLine())
      computeNow(plan_->line(), &e);
  }
  sheetIsPlan_ = keepPlan;
  QApplication::restoreOverrideCursor();
  refreshSheetList();
  return saved;
}

void MainWindow::noteBatchDone(int n, const QString &dir) {
  if (!sheetDoneLabel_)
    return;
  sheetDoneLabel_->setText(
      QStringLiteral(
          "%1장 저장됨 · <a href=\"%2\" style=\"color:#9C4A2F\">폴더 열기</a>")
          .arg(n)
          .arg(QUrl::fromLocalFile(dir).toString()));
  sheetDoneLabel_->show();
}

void MainWindow::refreshSheetList() {
  if (!sheetList_)
    return;
  const size_t rows = 1 + sections_.size();
  if (sheetChecked_.size() != rows)
    sheetChecked_.resize(rows, false);
  if (std::none_of(sheetChecked_.begin(), sheetChecked_.end(),
                   [](bool b) { return b; })) {
    const int cur = sheetIsPlan_ ? 0 : (current_ >= 0 ? current_ + 1 : 0);
    if (cur < int(rows))
      sheetChecked_[size_t(cur)] = true;
  }
  QSignalBlocker b(sheetList_);
  sheetList_->clear();
  auto caption = [this](const QString &tx) {
    auto *it = new QListWidgetItem(sheetList_);
    it->setFlags(Qt::NoItemFlags);
    it->setData(Qt::UserRole, -1);
    it->setSizeHint(QSize(10, 22));
    sheetList_->setItemWidget(it, lbl(tx, "listCaption"));
  };
  auto rowItem = [this](int row, int hh) {
    auto *it = new QListWidgetItem(sheetList_);
    it->setData(Qt::UserRole, row);
    it->setSizeHint(QSize(10, hh));
    sheetList_->setItemWidget(it, sheetRowWidget(row));
    return it;
  };
  if (!sections_.empty()) { // 평·단면도 한 장(2026-10-16): 평면 + 모든 단면
    caption(QStringLiteral("한 장"));
    auto *it = new QListWidgetItem(sheetList_);
    it->setData(Qt::UserRole, -2);
    it->setSizeHint(QSize(10, 46));
    auto *w = new QWidget;
    w->setObjectName("secRow");
    auto *h = new QHBoxLayout(w);
    h->setContentsMargins(6, 5, 6, 5);
    h->setSpacing(8);
    h->addSpacing(20); // 고르기 칸 자리(한 장은 「n장 저장」에 넣지 않음)
    auto *th = new QLabel(QStringLiteral("평·단"));
    th->setFixedSize(64, 30);
    th->setAlignment(Qt::AlignCenter);
    th->setStyleSheet(
        "QLabel{background:#F5F4ED;border:1px solid "
        "#DEDCD1;border-radius:6px;color:#5E5D59;font-size:11px;}");
    h->addWidget(th);
    auto *txt = new QWidget;
    auto *tv = new QVBoxLayout(txt);
    tv->setContentsMargins(0, 0, 0, 0);
    tv->setSpacing(0);
    tv->addWidget(lbl(QStringLiteral("평·단면도"), "secName"));
    tv->addWidget(lbl(QStringLiteral("평면 + 단면 %1개 · 같은 축척 · 한 장")
                          .arg(sections_.size()),
                      "monoFaint"));
    h->addWidget(txt, 1);
    w->setToolTip(QStringLiteral(
        "첫 단면이 가로가 되게 평면을 돌리고, 단면마다 평면 화살표 쪽에 "
        "펼쳐 놓습니다(종이 = 내용 크기)"));
    sheetList_->setItemWidget(it, w);
    it->setSelected(sheetIsCombo_);
  }
  caption(QStringLiteral("평면"));
  rowItem(0, 46)->setSelected(sheetIsPlan_ && !sheetIsCombo_);
  caption(QStringLiteral("단면"));
  for (size_t i = 0; i < sections_.size(); ++i)
    rowItem(int(i) + 1, sections_[i].note.empty() ? 46 : 60)
        ->setSelected(!sheetIsPlan_ && !sheetIsCombo_ && int(i) == current_);
  if (sheetCount_)
    sheetCount_->setText(QString::number(sections_.size()));
}

void MainWindow::openSheetFromList(int row) {
  if (!src_)
    return;
  if (row == -2) { // 평·단면도 한 장
    if (!sheetIsCombo_)
      dlgComboSheet();
    return;
  }
  if (row <= 0) {
    if (sheetIsPlan_ && !sheetIsCombo_)
      return;
    reusePlanSheet_ = true;
    dlgPlanSheet();
    return;
  }
  const int i = row - 1;
  if (i >= int(sections_.size()))
    return;
  if (!sheetIsPlan_ && !sheetIsCombo_ && i == current_)
    return;
  QApplication::setOverrideCursor(Qt::WaitCursor);
  selectSection(i);
  QString err;
  const bool ok =
      plan_->hasLine() &&
      computeNow(plan_->line(), &err); // 조판은 최종 단면으로 바로 계산
  QApplication::restoreOverrideCursor();
  if (!ok) {
    report(false, QStringLiteral("단면 계산 실패: ") + err);
    refreshSheetList();
    return;
  }
  dlgSheet();
}

void MainWindow::dlgSheet() {
  if (!src_) {
    showStatus(QStringLiteral("모델을 연 뒤에 도면을 만드세요"));
    return;
  }
  if (!section_->hasResult()) {
    if (!plan_->hasLine()) {
      showStatus(QStringLiteral(
          "단면이 없습니다. 평면에서 단면선을 그린 뒤 단면도 조판을 여세요"));
      return;
    }
    QString err;
    if (!computeNow(plan_->line(), &err)) {
      QMessageBox::warning(this, windowTitle(), err);
      return;
    }
  }
  syncCurrentSection();
  SheetParams sp = defaultSheetParams();
  if (lastSheet_) { // 조판 목록에서 다른 단면으로: 용지·넣을 것·형식은 그대로,
                    // 축척만 새 단면에 맞춤
    const SheetParams &o = *lastSheet_;
    sp.spec.paper = o.spec.paper;
    sp.spec.landscape = o.spec.landscape;
    sp.format = o.format;
    sp.dpi = o.dpi;
    sp.split = o.split;
    sp.withImage = o.withImage;
    sp.withLine = o.withLine;
    sp.withLevels = o.withLevels;
    sp.withTitle = o.withTitle;
    sp.showBaseline = o.showBaseline;
    sp.levelMinorPt = o.levelMinorPt;
    sp.levelMajorPt = o.levelMajorPt;
    sp.hatchKind = o.hatchKind;
    sp.hatchMm = o.hatchMm;
    if (section_->hasResult()) {
      const SheetGeom G0 = sheetGeom(section_->doc(), sp);
      if (G0.lenM > 0 && G0.heightM > 0)
        sp.spec.denom = fitDenomStep10(G0.fitSpecMax(), G0.lenM, G0.heightM);
    }
  }
  auto params = std::make_shared<SheetParams>(sp);
  lastSheet_ = params;
  sheetIsPlan_ = false;
  sheetIsCombo_ = false;
  bool acc = false;
  QDialog *d = buildSheetDialog(*params, acc);
  auto src = src_;
  showSheetTab(d, [this, params, src] {
    if (!ensureFinalSection())
      return;
    const SectionDoc doc =
        section_
            ->doc(); // 저장 시점의 단면(도면 탭에서 입면을 바꿨으면 그 결과)
    SheetParams sp = *params;
    if (checkedSheetCount() > 1) { // v5 E4: 고른 도면 전부를 한 폴더에(이름
                                   // sheetFileName · 겹치면 _2)
      const QString dir = QFileDialog::getExistingDirectory(
          this,
          QStringLiteral("고른 도면 %1장을 저장할 폴더")
              .arg(checkedSheetCount()),
          QSettings().value("sheet/batchDir").toString());
      if (dir.isEmpty() || src_ != src)
        return;
      QSettings().setValue("sheet/batchDir", dir);
      QStringList lg;
      const int n = saveCheckedSheets(dir, sp, &lg);
      report(n > 0, QStringLiteral("%1장 저장됨 — %2")
                        .arg(n)
                        .arg(QDir::toNativeSeparators(dir)));
      noteBatchDone(n, dir);
      return;
    }
    const bool svg = sp.format == 4;
    QString base = modelDisplayName(path_) + "_" +
                   sectionName()
                       .replace(QStringLiteral("–"), "-")
                       .replace(QStringLiteral("′"), "'")
                       .remove('\'');
    QString ext =
        svg ? QStringLiteral("svg")
            : QString::fromLatin1(kFormatExt[std::clamp(sp.format, 0, 3)]);
    QString name =
        svg ? QStringLiteral("SVG")
            : QString::fromLatin1(kFormatName[std::clamp(sp.format, 0, 3)]);
    QString filter = QStringLiteral("%1 (*.%2)").arg(name, ext);
    QString f = askSavePath(
        "sheet",
        base + QStringLiteral("_1-%1.").arg(sp.spec.denom, 0, 'f', 0) + ext,
        filter);
    if (f.isEmpty() || src_ != src)
      return; // 저장 창을 띄운 사이 모델이 바뀌었으면 옛 도면을 저장하지
              // 않음(검토 C1)
    if (QFileInfo(f).suffix().isEmpty())
      f += "." + ext;
    runTask(
        QStringLiteral("도면 %1 만드는 중").arg(name),
        [this, doc, src, sp, f, svg](QString *m) {
          if (svg)
            return exportSectionSvg(doc, *src, sp, f, m);
          return exportSheet(
              doc, *src, sp, f, m, &cancelTask_, [this](double x) {
                QMetaObject::invokeMethod(
                    this, [this, x] { setProgress(x); }, Qt::QueuedConnection);
              });
        },
        [this, sp, nm = sectionName()](bool ok, const QString &m) {
          report(ok, m);
          if (ok)
            noteSheetSaved(QStringLiteral("sheet/%1/").arg(nm), sp.spec);
        });
  });
}

PlanSheetParams MainWindow::defaultPlanSheetParams() const {
  PlanSheetParams sp;
  QSettings st;
  sp.spec.paper =
      st.value("plansheet/paper", st.value("sheet/paper", 0).toInt()).toInt() ==
              1
          ? Paper::A3
          : Paper::A4;
  sp.spec.landscape = st.value("plansheet/landscape", true).toBool();
  sp.spec.denom =
      std::max(10.0, std::round(st.value("plansheet/denom", 100.0).toDouble()));
  sp.format = std::clamp(st.value("plansheet/format", 0).toInt(), 0, 3);
  sp.dpi = std::max(600.0, st.value("plansheet/dpi", 600.0).toDouble());
  sp.wholeModel = false;
  if (!plan_->viewRectLocal(sp.area)) {
    sp.wholeModel = true;
    if (src_)
      sp.area = src_->bounds;
  }
  SheetParams sec = defaultSheetParams();
  sp.srsLabel = sec.srsLabel;
  sp.date = sec.date;
  sp.title = modelDisplayName(path_);
  if (sp.title.isEmpty())
    sp.title = QStringLiteral("평면도");
  else
    sp.title += QStringLiteral(" 평면도");
  if (src_ && src_->bounds.valid()) { // 3D 로 기울인 왼쪽 화면은 모델보다 큰
                                      // 상자를 낼 수 있어 먼저 모델로 자름
    sp.area.mn.x = std::max(sp.area.mn.x, src_->bounds.mn.x);
    sp.area.mn.y = std::max(sp.area.mn.y, src_->bounds.mn.y);
    sp.area.mx.x = std::min(sp.area.mx.x, src_->bounds.mx.x);
    sp.area.mx.y = std::min(sp.area.mx.y, src_->bounds.mx.y);
    if (!(sp.area.mx.x > sp.area.mn.x && sp.area.mx.y > sp.area.mn.y))
      sp.area = src_->bounds;
  }
  double rw = std::max(0.0, sp.area.mx.x - sp.area.mn.x),
         rh = std::max(0.0, sp.area.mx.y - sp.area.mn.y);
  if (rw > 0 && rh > 0)
    sp.spec.denom = fitDenomStep10(sp.spec, rw, rh);
  return sp;
}

QDialog *MainWindow::buildPlanSheetDialog(PlanSheetParams &io,
                                          const QImage &preview0, double prevX0,
                                          double prevY1, double prevRes,
                                          bool &accepted) {
  accepted = false;
  auto *d = new QDialog(this);
  d->setWindowTitle(QStringLiteral("평면도"));
  d->setObjectName("sheetDialog");
  auto st = std::make_shared<PlanSheetParams>(io);
  auto img = std::make_shared<QImage>(preview0);
  auto gx0 = std::make_shared<double>(prevX0);
  auto gy1 = std::make_shared<double>(prevY1);
  auto gres = std::make_shared<double>(prevRes);
  auto *outer = new QHBoxLayout(d);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(0);
  auto *pv = new SheetPreview;
  pv->setMinimumSize(470, 330);
  bindImageDrag(pv, &st->imgDxMm, &st->imgDyMm);
  auto *side = new QWidget;
  side->setObjectName("dialogSide");
  side->setAttribute(Qt::WA_StyledBackground);
  side->setMinimumWidth(200);
  side->setMaximumWidth(600);
  auto *sv = new QVBoxLayout(side);
  sv->setContentsMargins(0, 0, 0, 0);
  sv->setSpacing(0);
  auto *scroll = new QScrollArea;
  scroll->setObjectName("sideScroll");
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto *body = new QWidget;
  body->setObjectName("sideBody");
  body->setAttribute(Qt::WA_StyledBackground);
  auto *v = new QVBoxLayout(body);
  v->setContentsMargins(12, 10, 12, 10);
  v->setSpacing(6);
  v->addWidget(lbl(QStringLiteral("고른 도면 · 평면도"), "cap"));
  auto *nameLab = lbl(st->title, "sheetName");
  nameLab->setWordWrap(true);
  v->addWidget(nameLab);
  auto *subLab = lbl(QString(), "hint");
  subLab->setWordWrap(true);
  v->addWidget(subLab);
  // 도면 항목(평면도는 모두 그린다 — FINAL_PLAN §4: 그림 칸에 단면선 없음 ·
  // 진북 필수)
  v->addWidget(sideCaption(QStringLiteral("도면 항목")));
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);
    for (auto *tb :
         {sheetTile(QStringLiteral("image-layer"), QStringLiteral("정사영상"),
                    QStringLiteral("그림 칸에는 위에서 본 정사영상만 — "
                                   "단면선은 그리지 않습니다")),
          sheetTile(QStringLiteral("title-block"), QStringLiteral("표제란"),
                    QStringLiteral("표제란 · 축척 막대 — 단면도와 같은 자리")),
          sheetTile(QStringLiteral("scalebar"), QStringLiteral("축척 막대"),
                    QStringLiteral("축척 막대")),
          sheetTile(QStringLiteral("north"), QStringLiteral("진북"),
                    QStringLiteral("진북 28 mm — 글자는 「진북」만"))}) {
      tb->setChecked(true);
      tb->setEnabled(false);
      h->addWidget(tb);
    }
    h->setSpacing(8);
    h->addStretch();
    v->addWidget(row);
  }
  v->addWidget(sideCaption(QStringLiteral("도면 정보")));
  auto *g = new QGridLayout;
  g->setContentsMargins(0, 0, 0, 0);
  g->setHorizontalSpacing(10);
  g->setVerticalSpacing(8);
  g->setColumnMinimumWidth(0, 40);
  g->setColumnStretch(1, 1);
  int r = 0;
  auto *name = new QLineEdit(st->title);
  g->addWidget(lbl(QStringLiteral("도면명"), "hint"), r, 0);
  g->addWidget(name, r++, 1);
  PaperChips pc = paperChips(st->spec);
  g->addWidget(lbl(QStringLiteral("용지"), "hint"), r, 0);
  g->addWidget(pc.w, r++, 1);
  auto *scale = scaleSpin(st->spec.denom);
  auto *fitLink = new QPushButton;
  fitLink->setObjectName("quietLink");
  fitLink->setFlat(true);
  fitLink->setFocusPolicy(Qt::NoFocus);
  fitLink->setCursor(Qt::PointingHandCursor);
  fitLink->setToolTip(
      QStringLiteral("범위가 그림 칸에 들어가는 가장 작은 10 단위"));
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(6);
    h->addWidget(scaleWithArrows(scale), 1);
    h->addWidget(fitLink);
    g->addWidget(lbl(QStringLiteral("축척"), "hint"), r, 0);
    g->addWidget(row, r++, 1);
  }
  g->addWidget(scaleGridWidget(scale), r++, 1);
  auto *area = new QComboBox;
  area->addItem(QStringLiteral("지금 왼쪽 화면"));
  area->addItem(QStringLiteral("모델 전체"));
  Box3 view;
  bool hasView = plan_->viewRectLocal(view);
  if (!hasView)
    area->setEnabled(false);
  area->setCurrentIndex(st->wholeModel || !hasView ? 1 : 0);
  g->addWidget(lbl(QStringLiteral("범위"), "hint"), r, 0);
  g->addWidget(area, r++, 1);
  v->addLayout(g);
  v->addWidget(sideCaption(QStringLiteral("선 · 글자")));
  {
    auto *g2 = new QGridLayout;
    g2->setContentsMargins(0, 0, 0, 0);
    g2->setHorizontalSpacing(10);
    g2->setVerticalSpacing(6);
    g2->setColumnMinimumWidth(0, 40);
    g2->setColumnStretch(1, 1);
    g2->addWidget(lbl(QStringLiteral("좌표 숫자"), "hint"), 0, 0);
    g2->addWidget(lbl(QStringLiteral("그림 칸 밖 · 4 m마다 · 모노"), "faint"),
                  0, 1);
    g2->addWidget(lbl(QStringLiteral("진북"), "hint"), 1, 0);
    g2->addWidget(lbl(QStringLiteral("28 mm · 글자는 「진북」만"), "faint"), 1,
                  1);
    v->addLayout(g2);
  }
  v->addWidget(sideCaption(QStringLiteral("형식")));
  auto *fmtG = new QButtonGroup(d);
  fmtG->setExclusive(true);
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(2);
    h->addWidget(chipBtn(QStringLiteral("SVG"), fmtG, 4, "first", 48));
    h->addWidget(chipBtn(QStringLiteral("DXF"), fmtG, 1, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("PDF"), fmtG, 0, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("PNG"), fmtG, 2, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("TIFF"), fmtG, 3, "last", 48));
    h->addStretch();
    v->addWidget(row);
  }
  v->addWidget(sideCaption(QStringLiteral("도면 점검 · 저장을 막지 않음")));
  auto *check = new SheetCheckBox;
  v->addWidget(check);
  v->addStretch(1);
  scroll->setWidget(body);
  sv->addWidget(scroll, 1);
  auto *bDefault = new QPushButton(QStringLiteral("☆ 기본으로"));
  bDefault->setObjectName("quiet");
  bDefault->setFocusPolicy(Qt::NoFocus);
  bDefault->setToolTip(QStringLiteral(
      "이 설정(용지 · 축척 · 항목 · 형식 · 선 굵기)을 다음 도면의 기본으로"));
  auto *bSave = new QPushButton;
  bSave->setObjectName("primary");
  bSave->setDefault(true);
  auto *bPdf = new QPushButton(QStringLiteral("PDF"));
  bPdf->setFocusPolicy(Qt::NoFocus);
  auto *bPrint = new QPushButton(QStringLiteral("인쇄…"));
  bPrint->setObjectName("sheetPrint");
  bPrint->setFocusPolicy(Qt::NoFocus);
  bPrint->setToolTip(QStringLiteral(
      "인쇄(Ctrl+P) — 미리보기 창에서 프린터 · 쪽 설정. 도면 용지 그대로 100 "
      "%(줄이지 않음) — 축척이 종이에서도 맞음"));
  {
    QLabel *done = nullptr;
    sv->addWidget(sideBottom(bDefault, bSave, bPdf, &done, bPrint));
    sheetDoneLabel_ = done;
  }
  QObject::connect(bPrint, &QPushButton::clicked, d,
                   [this, st] { printPlanSheet(*st); });
  auto *dlgSplit = new QSplitter(Qt::Horizontal, d);
  dlgSplit->setObjectName("sheetDialogSplitter");
  dlgSplit->setHandleWidth(4);
  dlgSplit->setChildrenCollapsible(false);
  dlgSplit->addWidget(pv);
  dlgSplit->addWidget(side);
  dlgSplit->setStretchFactor(0, 1);
  dlgSplit->setStretchFactor(1, 0);
  dlgSplit->setSizes({700, 320});
  outer->addWidget(dlgSplit, 1);

  auto baseImg = std::make_shared<QImage>(preview0);
  auto baseX0 = std::make_shared<double>(prevX0);
  auto baseY1 = std::make_shared<double>(prevY1);
  auto baseRes = std::make_shared<double>(prevRes);
  if (baseImg->isNull() && src_) {
    QImage q;
    double x0 = 0, y1 = 0, res = 0;
    if (renderPlanPreview(*src_, st->spec, st->area, pv->zoom, st->imgDxMm,
                          st->imgDyMm, q, x0, y1, res)) {
      *baseImg = q;
      *baseX0 = x0;
      *baseY1 = y1;
      *baseRes = res;
      *img = q;
      *gx0 = x0;
      *gy1 = y1;
      *gres = res;
    }
  }
  auto planCancel = std::make_shared<std::shared_ptr<std::atomic<bool>>>();
  auto planGen = std::make_shared<std::atomic<int>>(0);
  QObject::connect(d, &QObject::destroyed, [planCancel] {
    if (*planCancel)
      (*planCancel)->store(true);
  });
  auto *planTimer = new QTimer(d);
  planTimer->setSingleShot(true);
  planTimer->setInterval(200);
  auto schedulePlanReload = [this, st, pv, img, gx0, gy1, gres, baseImg, baseX0,
                             baseY1, baseRes, planCancel, planGen, planTimer] {
    if (!src_)
      return;
    const PlanPlace cover =
        placePlan(st->spec, st->area.mn.x, st->area.mx.x, st->area.mn.y,
                  st->area.mx.y, pv->zoom, st->imgDxMm, st->imgDyMm);
    const double visSpan = std::max(cover.worldX1 - cover.worldX0,
                                    cover.worldYTop - cover.worldYBot);
    if (!baseImg->isNull() && *baseRes > 0 &&
        visSpan / *baseRes >=
            900.0) { // 기본 영상 해상도로 충분하면 즉시 재사용
      if (*planCancel)
        (*planCancel)->store(true);
      planTimer->stop();
      *img = *baseImg;
      *gx0 = *baseX0;
      *gy1 = *baseY1;
      *gres = *baseRes;
      pv->update();
      return;
    }
    planTimer->start();
  };
  QObject::connect(
      planTimer, &QTimer::timeout, d,
      [this, st, pv, img, gx0, gy1, gres, planCancel, planGen] {
        if (!src_)
          return;
        if (*planCancel)
          (*planCancel)->store(true);
        auto cancel = std::make_shared<std::atomic<bool>>(false);
        *planCancel = cancel;
        const int myGen = ++*planGen;
        const SheetSpec specSnap = st->spec;
        const Box3 areaSnap = st->area;
        const double zoomSnap = pv->zoom;
        const double dxSnap = st->imgDxMm, dySnap = st->imgDyMm;
        std::thread([src = src_, specSnap, areaSnap, zoomSnap, dxSnap, dySnap,
                     img, gx0, gy1, gres, cancel, planGen, myGen,
                     pvp = QPointer<SheetPreview>(pv)] {
          QImage q;
          double x0 = 0, y1 = 0, res = 0;
          if (renderPlanPreview(*src, specSnap, areaSnap, zoomSnap, dxSnap,
                                dySnap, q, x0, y1, res, cancel.get()) &&
              !cancel->load()) {
            QMetaObject::invokeMethod(
                qApp,
                [img, gx0, gy1, gres, q, x0, y1, res, pvp, planGen, myGen] {
                  if (planGen->load() != myGen)
                    return;
                  *img = q;
                  *gx0 = x0;
                  *gy1 = y1;
                  *gres = res;
                  if (pvp)
                    pvp->update();
                },
                Qt::QueuedConnection);
          }
        }).detach();
      });
  const QString modelName = modelDisplayName(path_);
  auto refresh = std::make_shared<std::function<void()>>();
  *refresh = [=] {
    st->spec.denom = std::max(10.0, std::round(st->spec.denom));
    if (!scale->hasFocus() &&
        int(std::lround(st->spec.denom)) != scale->value()) {
      QSignalBlocker bl(scale);
      scale->setValue(int(std::lround(st->spec.denom)));
    }
    syncPaperChips(pc, st->spec);
    st->wholeModel = area->currentIndex() == 1 || !hasView;
    Box3 raw =
        st->wholeModel || !hasView ? (src_ ? src_->bounds : st->area) : view;
    if (src_ && src_->bounds.valid()) {
      raw.mn.x = std::max(raw.mn.x, src_->bounds.mn.x);
      raw.mn.y = std::max(raw.mn.y, src_->bounds.mn.y);
      raw.mx.x = std::min(raw.mx.x, src_->bounds.mx.x);
      raw.mx.y = std::min(raw.mx.y, src_->bounds.mx.y);
    }
    st->area = raw;
    if (auto *b = fmtG->button(st->format)) {
      QSignalBlocker bl(b);
      b->setChecked(true);
    }
    PlanWindow W = planWindow(*st, 0);
    double rw = std::max(0.0, st->area.mx.x - st->area.mn.x),
           rh = std::max(0.0, st->area.mx.y - st->area.mn.y);
    double fitD = fitDenomStep10(st->spec, rw, rh);
    fitLink->setText(QStringLiteral("맞춤 %1").arg(denomText(fitD)));
    nameLab->setText(st->title);
    subLab->setText(
        QStringLiteral("%1 · %2 · 그림 칸 %3 × %4 mm · 단면선 없음 · 진북 있음")
            .arg(paperLabel(st->spec), denomText(st->spec.denom))
            .arg(W.L.plotW, 0, 'f', 0)
            .arg(W.L.plotH, 0, 'f', 0));
    std::vector<CheckItem> items;
    if (st->spec.denom + 0.1 < fitD)
      items.push_back(
          {false,
           QStringLiteral(
               "범위 %1 × %2 m 가 %3 칸보다 큽니다 — 가운데만 보입니다")
               .arg(rw, 0, 'f', 1)
               .arg(rh, 0, 'f', 1)
               .arg(denomText(st->spec.denom)),
           QStringLiteral("맞춤 %1").arg(denomText(fitD)),
           [=] { scale->setValue(int(std::lround(fitD))); }});
    else
      items.push_back(
          {true,
           QStringLiteral("범위 %1 × %2 m 가 %3에서 그림 칸을 채웁니다")
               .arg(rw, 0, 'f', 1)
               .arg(rh, 0, 'f', 1)
               .arg(denomText(st->spec.denom)),
           QString(), nullptr});
    items.push_back({true, QStringLiteral("그림 칸에 단면선 없음 · 정사영상만"),
                     QString(), nullptr});
    items.push_back(
        {true,
         QStringLiteral(
             "진북 있음 · 바깥 좌표 숫자 · 범례 · 자는 단면도와 같은 자리"),
         QString(), nullptr});
    {
      const QString ext =
          st->format == 4
              ? QStringLiteral("svg")
              : QString::fromLatin1(kFormatExt[std::clamp(st->format, 0, 3)]);
      const QString fname =
          QFileInfo(qs8(sheetFileName(modelName.toStdString(),
                                      std::string("평면도"), st->spec.denom)))
              .completeBaseName() +
          QLatin1Char('.') + ext;
      items.push_back(
          {true, QStringLiteral("파일 이름 %1 (같은 이름이면 _2)").arg(fname),
           QString(), nullptr});
    }
    check->setItems(items);
    const QString fmtName =
        st->format == 4
            ? QStringLiteral("SVG")
            : QString::fromLatin1(kFormatName[std::clamp(st->format, 0, 3)]);
    bSave->setText(QStringLiteral("%1장 %2 저장")
                       .arg(std::max(1, checkedSheetCount()))
                       .arg(fmtName));
    bPdf->setVisible(st->format != 0);
    pv->update();
  };
  onSheetChecksChanged_ = [refresh] { (*refresh)(); };
  pv->onSettled = [=] {
    if (std::abs(pv->zoom - 1.0) >
        1e-6) { // 조판편집 중 휠로 바꾼 크기 = 새 축척(보이는 그림과 축척
                // 표기가 같게)
      const int vv =
          int(std::lround(snapScaleDenom10(st->spec.denom / pv->zoom)));
      pv->zoom = 1;
      if (scale->value() != vv) {
        scale->setValue(vv);
        return;
      } // valueChanged → 새로 그림
      (*refresh)();
    }
    schedulePlanReload();
    pv->update();
  };
  pv->paintFn = [=](QPainter &p, const QRect &rc) {
    p.fillRect(rc, theme::Desk);
    PlanWindow W = planWindow(*st, 0);
    double sc = std::min((rc.width() - 48.0) / W.L.paperW,
                         (rc.height() - 110.0) / W.L.paperH);
    if (sc <= 0.05)
      return;
    sc *= pv->pageZoom;
    QSizeF pz(W.L.paperW * sc, W.L.paperH * sc);
    QPointF o(rc.left() + (rc.width() - pz.width()) / 2 + pv->viewPan.x(),
              rc.top() + (rc.height() - pz.height()) / 2 + pv->viewPan.y());
    pv->lastPaperO = o;
    pv->lastPaperSize = QPointF(pz.width(), pz.height());
    p.save();
    p.translate(o);
    p.setClipRect(QRectF(QPointF(0, 0), pz));
    Vec3 origin = src_ ? src_->srs.origin : Vec3();
    pv->fitPxPerMm = sc;
    pv->zoomCenter = o + QPointF((W.L.plotX + W.L.plotW / 2) * sc,
                                 (W.L.plotY + W.L.plotH / 2) * sc);
    paintPlanSheet(p, *st, sc, 0, *img, *gx0, *gy1, *gres, origin, pv->zoom, 0,
                   0);
    p.restore();
    paintPaperFrame(p, o, pz);
  };
  auto *editAct = new QAction(QStringLiteral("조판편집"), d);
  editAct->setCheckable(true);
  QObject::connect(editAct, &QAction::toggled, d, [pv](bool on) {
    pv->editing = on;
    pv->setCursor(on ? Qt::SizeAllCursor : Qt::ArrowCursor);
  });
  addDeskFloats(
      pv, editAct,
      [=] {
        int idx = (st->spec.paper == Paper::A3 ? 2 : 0) +
                  (st->spec.landscape ? 0 : 1);
        idx = (idx + 1) % 4;
        st->spec.paper = idx >= 2 ? Paper::A3 : Paper::A4;
        st->spec.landscape = (idx % 2) == 0;
        (*refresh)();
        schedulePlanReload();
      },
      [this] {
        if (sheetSide_)
          sheetSide_->setVisible(!sheetSide_->isVisible());
      });
  QObject::connect(name, &QLineEdit::textChanged, d, [=](const QString &tx) {
    st->title = tx;
    nameLab->setText(tx);
    pv->update();
  });
  QObject::connect(pc.size, &QButtonGroup::idClicked, d, [=](int id) {
    st->spec.paper = id == 1 ? Paper::A3 : Paper::A4;
    (*refresh)();
    schedulePlanReload();
  });
  QObject::connect(pc.orient, &QButtonGroup::idClicked, d, [=](int id) {
    st->spec.landscape = id == 0;
    (*refresh)();
    schedulePlanReload();
  });
  QObject::connect(scale, &QSpinBox::valueChanged, d, [=](int v) {
    st->spec.denom = std::max(10, v);
    (*refresh)();
    schedulePlanReload();
  });
  bindScaleTyping(scale, [=](int v) {
    st->spec.denom = std::max(10, v);
    (*refresh)();
    schedulePlanReload();
  });
  QObject::connect(fitLink, &QPushButton::clicked, d, [=] {
    double rw = std::max(0.0, st->area.mx.x - st->area.mn.x),
           rh = std::max(0.0, st->area.mx.y - st->area.mn.y);
    st->spec.denom = fitDenomStep10(st->spec, rw, rh);
    QSignalBlocker bl(scale);
    scale->setValue(int(std::lround(st->spec.denom)));
    (*refresh)();
    schedulePlanReload();
  });
  QObject::connect(area, &QComboBox::currentIndexChanged, d, [=](int) {
    (*refresh)();
    schedulePlanReload();
  });
  QObject::connect(fmtG, &QButtonGroup::idClicked, d, [=](int id) {
    st->format = id;
    (*refresh)();
  });
  QObject::connect(bDefault, &QPushButton::clicked, d, [=] {
    QSettings s;
    s.setValue("plansheet/paper", st->spec.paper == Paper::A3 ? 1 : 0);
    s.setValue("plansheet/landscape", st->spec.landscape);
    s.setValue("plansheet/denom", st->spec.denom);
    s.setValue("plansheet/format", st->format);
    bDefault->setText(QStringLiteral("★ 저장함"));
  });
  auto doSave = [=, &io, &accepted] {
    QSettings s;
    s.setValue("plansheet/paper", st->spec.paper == Paper::A3 ? 1 : 0);
    s.setValue("plansheet/landscape", st->spec.landscape);
    s.setValue("plansheet/denom", st->spec.denom);
    s.setValue("plansheet/format", st->format);
    io = *st;
    if (d->property("embed").toBool()) {
      saveEmbeddedSheet();
      return;
    } // 검토 C2
    accepted = true;
    d->accept();
  };
  QObject::connect(bSave, &QPushButton::clicked, d, doSave);
  QObject::connect(bPdf, &QPushButton::clicked, d, [=] {
    st->format = 0;
    (*refresh)();
    doSave();
  });
  (*refresh)();
  pv->update();
  d->resize(1000, 690);
  return d;
}

void MainWindow::dlgPlanSheet() {
  if (!src_) {
    showStatus(QStringLiteral("모델을 연 뒤에 평면도를 만드세요"));
    return;
  }
  PlanSheetParams sp = (reusePlanSheet_ && lastPlanSheet_)
                           ? *lastPlanSheet_
                           : defaultPlanSheetParams();
  reusePlanSheet_ = false;
  static const MeshSource *s_cachedSrc = nullptr;
  static QImage s_cachedImg;
  static double s_cachedX0 = 0, s_cachedY1 = 0, s_cachedRes = 0;
  if (s_cachedSrc != src_.get() || s_cachedImg.isNull()) {
    Box3 box = src_->bounds;
    double w = std::max(0.0, box.mx.x - box.mn.x),
           h = std::max(0.0, box.mx.y - box.mn.y);
    double res = std::max({1e-4, 0.002, std::max(w, h) / 1600.0});
    QImage prev;
    double x0 = 0, y1 = 0;
    QString em;
    renderPlanOrtho(*src_, box, res, prev, x0, y1, &em, nullptr);
    s_cachedSrc = src_.get();
    s_cachedImg = prev;
    s_cachedX0 = x0;
    s_cachedY1 = y1;
    s_cachedRes = res;
  }
  auto params = std::make_shared<PlanSheetParams>(sp);
  lastPlanSheet_ = params;
  sheetIsPlan_ = true;
  sheetIsCombo_ = false;
  bool acc = false;
  QDialog *d = buildPlanSheetDialog(*params, s_cachedImg, s_cachedX0,
                                    s_cachedY1, s_cachedRes, acc);
  auto src = src_;
  showSheetTab(d, [this, params, src] {
    PlanSheetParams sp = *params;
    if (checkedSheetCount() > 1) { // v5 E4
      const QString dir = QFileDialog::getExistingDirectory(
          this,
          QStringLiteral("고른 도면 %1장을 저장할 폴더")
              .arg(checkedSheetCount()),
          QSettings().value("sheet/batchDir").toString());
      if (dir.isEmpty() || src_ != src)
        return;
      QSettings().setValue("sheet/batchDir", dir);
      SheetParams base = lastSheet_ ? *lastSheet_ : defaultSheetParams();
      base.format = sp.format;
      QStringList lg;
      const int n = saveCheckedSheets(dir, base, &lg);
      report(n > 0, QStringLiteral("%1장 저장됨 — %2")
                        .arg(n)
                        .arg(QDir::toNativeSeparators(dir)));
      noteBatchDone(n, dir);
      return;
    }
    const bool svg = sp.format == 4;
    QString ext =
        svg ? QStringLiteral("svg")
            : QString::fromLatin1(kFormatExt[std::clamp(sp.format, 0, 3)]);
    QString name =
        svg ? QStringLiteral("SVG")
            : QString::fromLatin1(kFormatName[std::clamp(sp.format, 0, 3)]);
    QString filter = QStringLiteral("%1 (*.%2)").arg(name, ext);
    QString f = askSavePath(
        "plansheet",
        modelDisplayName(path_) +
            QStringLiteral("_평면도_1-%1.").arg(sp.spec.denom, 0, 'f', 0) + ext,
        filter);
    if (f.isEmpty() || src_ != src)
      return; // 저장 창을 띄운 사이 모델이 바뀌었으면 옛 도면을 저장하지
              // 않음(검토 C1)
    if (QFileInfo(f).suffix().isEmpty())
      f += "." + ext;
    runTask(
        QStringLiteral("평면도 %1 만드는 중").arg(name),
        [this, src, sp, f](QString *m) {
          return exportPlanSheet(
              *src, sp, f, m, &cancelTask_, [this](double x) {
                QMetaObject::invokeMethod(
                    this, [this, x] { setProgress(x); }, Qt::QueuedConnection);
              });
        },
        [this, sp](bool ok, const QString &m) {
          report(ok, m);
          if (ok)
            noteSheetSaved(QStringLiteral("sheet/plan/"), sp.spec);
        });
  });
}

void MainWindow::dlgChooseSheet() {
  if (!src_) {
    QMessageBox::information(this, windowTitle(),
                             QStringLiteral("모델을 연 뒤에 도면을 만드세요"));
    return;
  }
  if (section_->hasResult() || plan_->hasLine())
    dlgSheet();
  else
    dlgPlanSheet();
}

bool MainWindow::runSheetCheck(const QString &outDir, QString *log) {
  QDir().mkpath(outDir);
  QStringList lines;
  bool ok = true;
  auto flag = [&](bool v) {
    if (!v)
      ok = false;
    return v ? 1 : 0;
  };
  QSpinBox *planStep = scaleSpin(110);
  QSpinBox *secStep = scaleSpin(40);
  const int ps = planStep->singleStep(), ss = secStep->singleStep();
  delete planStep;
  delete secStep;
  if (!src_ || !src_->bounds.valid()) {
    lines << QStringLiteral("sheet-check RESULT fail");
    if (log)
      *log = lines.join('\n');
    return false;
  }
  const Box3 model = src_->bounds;
  const double mw = std::max(0.0, model.mx.x - model.mn.x),
               mh = std::max(0.0, model.mx.y - model.mn.y);
  Box3 area;
  area.mn.x = model.mn.x + mw * 0.25;
  area.mx.x = model.mx.x - mw * 0.25;
  area.mn.y = model.mn.y + mh * 0.25;
  area.mx.y = model.mx.y - mh * 0.25;
  area.mn.z = model.mn.z;
  area.mx.z = model.mx.z;
  if (!(area.mx.x > area.mn.x && area.mx.y > area.mn.y))
    area = model;
  PlanSheetParams sp = defaultPlanSheetParams();
  sp.area = area;
  sp.wholeModel = false;
  sp.spec.denom =
      fitDenomStep10(sp.spec, area.mx.x - area.mn.x, area.mx.y - area.mn.y);
  QImage base;
  double bx0 = 0, by1 = 0, bres = 0;
  {
    double res = std::max(1e-4, std::max(mw, mh) / 800.0);
    QString em;
    if (!renderPlanOrtho(*src_, model, res, base, bx0, by1, &em, nullptr) ||
        base.isNull()) {
      int W = 64, H = 64;
      base = QImage(W, H, QImage::Format_RGB32);
      base.fill(QColor(0xC4, 0xA5, 0x74));
      bx0 = model.mn.x;
      by1 = model.mx.y;
      bres = std::max(mw, mh) / W;
    } else
      bres = res;
  }
  const double k = 8;
  const Vec3 origin = src_->srs.origin;
  auto paintPlan = [&](double zoom, double dx, QImage &canvas,
                       SheetPaintProbe &probe) {
    PlanSheetParams draw = sp;
    draw.imgDxMm = dx;
    PlanWindow W = planWindow(draw, 0, zoom);
    canvas = QImage(std::max(1, int(std::ceil(W.L.paperW * k))),
                    std::max(1, int(std::ceil(W.L.paperH * k))),
                    QImage::Format_RGB32);
    canvas.fill(Qt::white);
    QPainter p(&canvas);
    QImage pv;
    double px0 = bx0, py1 = by1, pres = bres;
    if (!renderPlanPreview(*src_, draw.spec, draw.area, zoom, draw.imgDxMm,
                           draw.imgDyMm, pv, px0, py1, pres)) {
      pv = base;
      px0 = bx0;
      py1 = by1;
      pres = bres;
    }
    paintPlanSheet(p, draw, k, 0, pv, px0, py1, pres, origin, zoom, 0, 0,
                   &probe);
  };
  auto outside = [](const SheetPaintProbe &pr) {
    if (pr.labels.isEmpty() || pr.plot.isEmpty())
      return false;
    for (const QRectF &r : pr.labels) {
      QRectF i = r.intersected(pr.plot);
      if (i.width() > 0.8 && i.height() > 0.8)
        return false;
    }
    return true;
  };
  auto contentRatio = [](const QImage &im, const QRectF &plot) {
    QRect r =
        plot.toAlignedRect().intersected(im.rect()).adjusted(2, 2, -2, -2);
    if (r.width() < 4 || r.height() < 4)
      return 0.0;
    int n = 0, c = 0;
    for (int y = r.top(); y < r.bottom(); y += 2)
      for (int x = r.left(); x < r.right(); x += 2) {
        QRgb px = im.pixel(x, y);
        int R = qRed(px), G = qGreen(px), B = qBlue(px);
        auto near = [&](int r0, int g0, int b0) {
          return std::abs(R - r0) < 14 && std::abs(G - g0) < 14 &&
                 std::abs(B - b0) < 14;
        };
        ++n;
        if (!near(255, 255, 255) && !near(0xF7, 0xF5, 0xEE) &&
            !near(0xF4, 0xF1, 0xEA))
          ++c;
      }
    return n ? double(c) / n : 0.0;
  };
  auto redCount = [](const QImage &im, const QRectF &plot) {
    QRect r = plot.toAlignedRect().intersected(im.rect());
    int c = 0;
    for (int y = r.top(); y < r.bottom(); ++y)
      for (int x = r.left(); x < r.right(); ++x) {
        QRgb px = im.pixel(x, y);
        if (qRed(px) > 200 && qGreen(px) < 50 && qBlue(px) < 50)
          ++c;
      }
    return c;
  };
  auto imageCovers = [](const QRectF &img, const QRectF &plot) {
    return !img.isEmpty() && img.left() <= plot.left() + 3 &&
           img.top() <= plot.top() + 3 && img.right() >= plot.right() - 3 &&
           img.bottom() >= plot.bottom() - 3;
  };
  QImage c1;
  SheetPaintProbe p1;
  paintPlan(1, 0, c1, p1);
  const QString shot1 =
      QDir(outDir).filePath(QStringLiteral("sheet_plan_z1.png"));
  c1.save(shot1);
  const PlanPlace geo1 = placePlan(sp.spec, sp.area.mn.x, sp.area.mx.x,
                                   sp.area.mn.y, sp.area.mx.y, 1, 0, 0);
  const int fills1 =
      flag(geo1.imageFillsPlot && imageCovers(p1.image, p1.plot) &&
           contentRatio(c1, p1.plot) > 0.5);
  const int whole1 = flag(geo1.rangeInsideImage);
  const int lineOn = p1.drewCutLine ? 1 : 0;
  if (lineOn != 0)
    ok = false;
  const int out1 = flag(outside(p1));
  const int uni1 = flag(p1.uniform);
  lines << QStringLiteral("sheet-check plan zoom=1.00 image-fills-plot=%1 "
                          "image-whole=%2 line-on-plan=%3")
               .arg(fills1)
               .arg(whole1)
               .arg(lineOn);
  lines << QStringLiteral("            labels=%1 labels-outside-plot=%2 "
                          "text-uniform=%3 shot=%4")
               .arg(p1.labels.size())
               .arg(out1 ? p1.labels.size() : 0)
               .arg(uni1)
               .arg(QDir::toNativeSeparators(shot1));
  double pw = 0, ph = 0;
  asec::paperSizeMm(sp.spec.paper, sp.spec.landscape, pw, ph);
  const double a4Ratio =
      (pw > 0 && ph > 0) ? ((geo1.plotW * geo1.plotH) / (pw * ph)) : 0.0;
  const int ratioOk = flag(a4Ratio >= 0.75);
  lines << QStringLiteral("sheet-check a4-plot-ratio=%1 expect>=0.75 ok=%2")
               .arg(a4Ratio, 0, 'f', 4)
               .arg(ratioOk);
  QImage c2;
  SheetPaintProbe p2;
  paintPlan(2.07, 0, c2, p2);
  c2.save(QDir(outDir).filePath(QStringLiteral("sheet_plan_z2.png")));
  const double per1 = p1.worldStep > 0 ? p1.neighborPx / p1.worldStep : 0;
  const double per2 = p2.worldStep > 0 ? p2.neighborPx / p2.worldStep : 0;
  const int grew = flag(per2 > per1 * 1.5 && p2.neighborPx > 0);
  const int fontSame =
      flag(p1.fontPx > 0 && std::abs(p1.fontPx - p2.fontPx) < 0.5);
  const int out2 = flag(outside(p2));
  lines << QStringLiteral("sheet-check plan zoom=2.07 spacing-grew=%1 "
                          "font-px-same=%2 labels-outside-plot=%3")
               .arg(grew)
               .arg(fontSame)
               .arg(out2);
  if (section_ && section_->hasResult()) {
    SheetParams sec = defaultSheetParams();
    sec.withLevels = true; // 판정은 레벨선이 있어야 함(검토 M2)
    sec.withTicks = true;  // 거리 눈금 그리기 판정(기본은 끔 — 켰을 때 모양을 본다)
    SectionDoc doc = section_->doc();
    auto paintSec = [&](double zoom, QImage &canvas, SheetPaintProbe &probe) {
      SheetGeom G = sheetGeom(doc, sec);
      canvas = QImage(std::max(1, int(std::ceil(G.L.paperW * k))),
                      std::max(1, int(std::ceil(G.L.paperH * k))),
                      QImage::Format_RGB32);
      canvas.fill(Qt::white);
      QPainter p(&canvas);
      SectionImgGeo geo{doc.imgS0, doc.imgZ1, doc.imgRes};
      paintSheet(p, doc, sec, k, 0, doc.img, geo, zoom, 0, 0, &probe);
    };
    QImage s1;
    SheetPaintProbe e1;
    paintSec(1, s1, e1);
    s1.save(QDir(outDir).filePath(QStringLiteral("sheet_section_z1.png")));
    const int vis =
        flag(contentRatio(s1, e1.plot) > 0.02 && !e1.image.isEmpty());
    const int red = flag(redCount(s1, e1.plot) > 0);
    const int so = flag(outside(e1));
    lines << QStringLiteral("sheet-check section zoom=1.00 image-visible=%1 "
                            "cut-red-visible=%2 labels-outside-plot=%3")
                 .arg(vis)
                 .arg(red)
                 .arg(so);
    { // 디자인 v5 결정 L: 레벨선 10 cm 0.2 pt · 50 cm 0.5 pt — 종이 위 mm ×
      // (px/mm) 가 실제 펜 굵기와 같아야 한다
      const double expMinor = ptToMm(sec.levelMinorPt) * k,
                   expMajor = ptToMm(sec.levelMajorPt) * k;
      const int lvOk = flag(std::abs(e1.levelMinorPx - expMinor) < 0.05 &&
                            std::abs(e1.levelMajorPx - expMajor) < 0.05 &&
                            e1.levelMajorPx > e1.levelMinorPx);
      lines << QStringLiteral("sheet-check level-pt minor=%1 expect=%2 "
                              "major=%3 expect=%4 ok=%5")
                   .arg(e1.levelMinorPx, 0, 'f', 3)
                   .arg(expMinor, 0, 'f', 3)
                   .arg(e1.levelMajorPx, 0, 'f', 3)
                   .arg(expMajor, 0, 'f', 3)
                   .arg(lvOk);
      // 거리 눈금(T10): 도면에도 세로 눈금이 그려지고, 굵기는 레벨선과 같은
      // pt(0.2 · 0.5)이며, 굵은 눈금이 1 개 이상
      const int dtOk =
          flag(e1.distTickCount >= 2 && e1.distTickMajorCount >= 1 &&
               e1.distTickStepM > 0 &&
               std::abs(e1.distTickMinorPx - expMinor) < 0.05 &&
               std::abs(e1.distTickMajorPx - expMajor) < 0.05 &&
               e1.distTickMajorPx > e1.distTickMinorPx);
      lines << QStringLiteral("sheet-check dist-ticks=%1 major=%2 step=%3m "
                              "weight-px=%4/%5 expect=%6/%7 ok=%8")
                   .arg(e1.distTickCount)
                   .arg(e1.distTickMajorCount)
                   .arg(e1.distTickStepM, 0, 'g', 4)
                   .arg(e1.distTickMinorPx, 0, 'f', 3)
                   .arg(e1.distTickMajorPx, 0, 'f', 3)
                   .arg(expMinor, 0, 'f', 3)
                   .arg(expMajor, 0, 'f', 3)
                   .arg(dtOk);
    }
    QImage s2;
    SheetPaintProbe e2;
    paintSec(2.07, s2, e2);
    s2.save(QDir(outDir).filePath(QStringLiteral("sheet_section_z2.png")));
    const double se1 = e1.worldStep > 0 ? e1.neighborPx / e1.worldStep : 0;
    const double se2 = e2.worldStep > 0 ? e2.neighborPx / e2.worldStep : 0;
    const int sgrew = flag(se2 > se1 * 1.5 && e2.neighborPx > 0);
    const int suni = flag(e2.uniform && e1.fontPx > 0 &&
                          std::abs(e1.fontPx - e2.fontPx) < 0.5);
    const int svis = flag(contentRatio(s2, e2.plot) > 0.02);
    lines << QStringLiteral("sheet-check section zoom=2.07 spacing-grew=%1 "
                            "text-uniform=%2 image-visible=%3")
                 .arg(sgrew)
                 .arg(suni)
                 .arg(svis);
  } else {
    lines << QStringLiteral("sheet-check section zoom=1.00 image-visible=0 "
                            "cut-red-visible=0 labels-outside-plot=0");
    lines << QStringLiteral("sheet-check section zoom=2.07 spacing-grew=0 "
                            "text-uniform=0 image-visible=0");
    ok = false;
  }
  QImage c3;
  SheetPaintProbe p3;
  paintPlan(1, 12, c3, p3);
  c3.save(QDir(outDir).filePath(QStringLiteral("sheet_plan_move.png")));
  const bool slid =
      !p1.labels.isEmpty() && !p3.labels.isEmpty() &&
      std::abs(p1.labels[0].center().x() - p3.labels[0].center().x()) > 2;
  const int chrome = flag(std::abs(p1.compass.x() - p3.compass.x()) < 0.5 &&
                          std::abs(p1.legend.y() - p3.legend.y()) < 0.5 &&
                          std::abs(p1.frame.x() - p3.frame.x()) < 0.5);
  const int changed = flag(p1.labelText != p3.labelText || slid);
  lines << QStringLiteral("sheet-check plan move=12mm chrome-fixed=%1 "
                          "label-values-changed=%2")
               .arg(chrome)
               .arg(changed);
  lines << QStringLiteral("sheet-check scale-step plan=%1 section=%2")
               .arg(ps)
               .arg(ss);
  flag(ps == 10 && ss == 10);
  { // 키보드로 120 을 한 글자씩('1', '2', '0') 입력할 때 중간값 12 가 10 으로
    // 덮어써져 100 이 되지 않고 120 이 그대로 유지되는지 점검
    int typedVal = 0;
    if (hasSection() && src_) {
      SheetParams tp = defaultSheetParams();
      bool acc = false;
      std::unique_ptr<QDialog> dlg(buildSheetDialog(tp, acc));
      QSpinBox *sb = nullptr;
      for (QSpinBox *s : dlg->findChildren<QSpinBox *>())
        if (s->prefix() == QStringLiteral("1:")) {
          sb = s;
          break;
        }
      if (sb) {
        if (auto *le = sb->findChild<QLineEdit *>()) {
          le->setFocus();
          le->setText(QStringLiteral("1:1"));
          emit le->textEdited(le->text());
          le->setText(QStringLiteral("1:12"));
          emit le->textEdited(le->text());
          if (le->text() == QStringLiteral("1:12")) {
            le->setText(QStringLiteral("1:120"));
            emit le->textEdited(le->text());
          }
          sb->interpretText();
          typedVal = sb->value();
        }
      }
    }
    lines << QStringLiteral("sheet-check scale-type-120=%1").arg(typedVal);
    flag(typedVal == 120);
  }
  { // 도면 어디에도 높이 글이 없다(T8): 단면 SVG 의 표제란 · 범례에서 「높이」
    // 글을 센다
    int heightLines = -1;
    if (hasSection() && src_) {
      SheetParams hp = defaultSheetParams();
      hp.format = 4;
      const QString hf =
          QDir(outDir).filePath(QStringLiteral("height-probe.svg"));
      QString hm;
      if (exportSectionSvg(sectionDoc(), *src_, hp, hf, &hm)) {
        QFile sf(hf);
        if (sf.open(QIODevice::ReadOnly))
          heightLines =
              QString::fromUtf8(sf.readAll()).count(QStringLiteral(">높이"));
      }
    }
    lines << QStringLiteral("sheet-check height-lines=%1").arg(heightLines);
    flag(heightLines == 0);
  }
  lines << QStringLiteral("sheet-check RESULT %1").arg(ok ? "ok" : "fail");
  QFile out(QDir(outDir).filePath(QStringLiteral("sheet-check.txt")));
  if (out.open(QIODevice::WriteOnly | QIODevice::Text))
    out.write((lines.join('\n') + '\n').toUtf8());
  if (log)
    *log = lines.join('\n');
  return ok;
}

// ---------------------------------------------------------------- 인쇄(사용자
// 요청 2026-10-12) PDF 저장과 같은 그리기(paintSheet / paintPlanSheet) · 같은
// 출력 영상(prepare*Assets)을 QPrinter 에. 용지 = 도면 용지 · 방향, 여백 0,
// 전체 쪽(fullPage) 100 % — 줄여 맞추지 않는다(1:20 이면 종이에서도 1:20).
// 프린터 용지가 다르거나 못 찍는 가장자리가 테두리 밖 여백보다 넓으면 미리보기
// 창 제목에 경고.
MainWindow::PrintTest MainWindow::printTest;

namespace {
QString paperText(const SheetSpec &s) {
  return QStringLiteral("%1 %2").arg(QString::fromLatin1(paperName(s.paper)),
                                     s.landscape ? QStringLiteral("가로")
                                                 : QStringLiteral("세로"));
}

// 프린터를 도면 용지에 맞춘다(여백 0 · 전체 쪽). 지원하지 않는 용지면
// false(드라이버가 다른 용지를 고름)
bool setupPrinter(QPrinter &pr, const SheetSpec &spec) {
  pr.setFullPage(true);
  const QPageLayout lay(
      QPageSize(spec.paper == Paper::A3 ? QPageSize::A3 : QPageSize::A4),
      spec.landscape ? QPageLayout::Landscape : QPageLayout::Portrait,
      QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter);
  return pr.setPageLayout(lay);
}

// 지금 프린터 설정을 도면과 비교한 경고 글(없으면 빈 글)
QString printWarning(QPrinter &pr, const SheetLayout &L, const SheetSpec &spec,
                     PrintPaperCheck *outCheck = nullptr) {
  QPageLayout lay = pr.pageLayout();
  lay.setUnits(QPageLayout::Millimeter);
  const QRectF full = lay.fullRect();
  const QMarginsF mm = lay.minimumMargins();
  const PrintPaperCheck c =
      checkPrintPaper(L, full.width(), full.height(), mm.left(), mm.top(),
                      mm.right(), mm.bottom());
  if (outCheck)
    *outCheck = c;
  QStringList w;
  if (!c.sizeOk)
    w << QStringLiteral("프린터 용지 %1 × %2 mm 가 도면 용지(%3, %4 × %5 mm)와 "
                        "다릅니다 — 축척이 맞지 않거나 잘립니다")
             .arg(full.width(), 0, 'f', 0)
             .arg(full.height(), 0, 'f', 0)
             .arg(paperText(spec))
             .arg(L.paperW, 0, 'f', 0)
             .arg(L.paperH, 0, 'f', 0);
  if (!c.marginOk)
    w << QStringLiteral(
             "프린터가 가장자리 %1 mm 를 못 찍어 테두리(여백 %2 mm)가 잘립니다")
             .arg(c.maxPrinterMarginMm, 0, 'f', 1)
             .arg(c.frameMarginMm, 0, 'f', 1);
  return w.join(QStringLiteral(" · "));
}

using PagePainter =
    std::function<void(QPainter &, double /*pxPerMm*/, int /*page*/)>;

// 쪽마다 그리기. pxPerMm = 프린터 해상도 / 25.4(전체 쪽이라 원점 = 종이 모서리)
bool paintPages(QPrinter *pr, int pages, const PagePainter &paintPage) {
  QPainter pt;
  if (!pt.begin(pr))
    return false;
  const double pxPerMm = pr->resolution() / 25.4;
  for (int i = 0; i < pages; ++i) {
    if (i > 0)
      pr->newPage();
    paintPage(pt, pxPerMm, i);
  }
  pt.end();
  return true;
}
} // namespace

// 인쇄 미리보기(프린터 고르기 · 쪽 설정은 창 안 단추). 명령줄
// 점검(printTest)이면 창 없이 찍거나 창을 캡처하고 닫는다
static void runPrintPreview(QWidget *parent, const SheetLayout &L,
                            const SheetSpec &spec, const QString &docName,
                            int pages, const PagePainter &paintPage) {
  const QString scaleText = QStringLiteral("1:%1").arg(spec.denom, 0, 'f', 0);
  MainWindow::PrintTest &T = MainWindow::printTest;
  if (!T.printer.isEmpty()) { // 점검: 대화상자 없이 그 프린터로
    QPrinter pr(QPrinterInfo::printerInfo(T.printer), QPrinter::HighResolution);
    pr.setDocName(docName);
    const bool laid = setupPrinter(pr, spec);
    if (!T.outFile.isEmpty()) {
      pr.setOutputFileName(T.outFile);
      pr.setOutputFormat(QPrinter::NativeFormat);
    } // 드라이버 출력을 파일로(.pdf 이름이면 Qt 가 PDF 로 바꾸므로 다른 확장자)
    PrintPaperCheck c;
    const QString warn = printWarning(pr, L, spec, &c);
    QPageLayout lay = pr.pageLayout();
    lay.setUnits(QPageLayout::Millimeter);
    const bool ok = paintPages(&pr, pages, paintPage);
    T.log =
        QStringLiteral(
            "print: printer=\"%1\" ok=%2 layout-set=%3 paper=%4x%5mm dpi=%6 "
            "pages=%7 size-ok=%8 margin-ok=%9 printer-margin=%10mm "
            "frame-margin=%11mm warn=\"%12\" scale=%13 frame=%14x%15mm@%16,%17")
            .arg(pr.printerName())
            .arg(ok ? 1 : 0)
            .arg(laid ? 1 : 0)
            .arg(lay.fullRect().width(), 0, 'f', 1)
            .arg(lay.fullRect().height(), 0, 'f', 1)
            .arg(pr.resolution())
            .arg(pages)
            .arg(c.sizeOk ? 1 : 0)
            .arg(c.marginOk ? 1 : 0)
            .arg(c.maxPrinterMarginMm, 0, 'f', 2)
            .arg(c.frameMarginMm, 0, 'f', 2)
            .arg(warn)
            .arg(scaleText)
            .arg(L.frameW, 0, 'f', 2)
            .arg(L.frameH, 0, 'f', 2)
            .arg(L.frameX, 0, 'f', 2)
            .arg(L.frameY, 0, 'f', 2);
    T.done = true;
    return;
  }
  QPrinter printer(QPrinter::HighResolution);
  printer.setDocName(docName);
  setupPrinter(printer, spec);
  QPrintPreviewDialog dlg(&printer, parent);
  const QString base = QStringLiteral("인쇄 — %1 · %2 · 100 %(줄이지 않음)")
                           .arg(docName, paperText(spec));
  dlg.setWindowTitle(base);
  QObject::connect(
      &dlg, &QPrintPreviewDialog::paintRequested, &dlg, [&](QPrinter *pr) {
        const QString warn = printWarning(*pr, L, spec);
        dlg.setWindowTitle(
            warn.isEmpty() ? base : base + QStringLiteral("  ⚠ ") + warn);
        paintPages(pr, pages, paintPage);
      });
  if (!T.previewShot.isEmpty()) { // 점검: 미리보기가 그려진 뒤 캡처하고 닫는다
    QTimer::singleShot(4000, &dlg, [&dlg, &T] {
      const bool ok = dlg.grab().save(T.previewShot);
      T.log = QStringLiteral("print-preview: shot=%1 title=\"%2\"")
                  .arg(ok ? QStringLiteral("ok") : QStringLiteral("FAILED"),
                       dlg.windowTitle());
      T.done = true;
      dlg.reject();
    });
  }
  dlg.resize(parent ? parent->size() * 0.8 : QSize(1200, 900));
  dlg.exec();
}

void MainWindow::printSectionSheet(const SectionDoc &doc,
                                   const SheetParams &sp0) {
  if (!src_)
    return;
  auto sp = std::make_shared<SheetParams>(sp0);
  auto A = std::make_shared<SheetAssets>();
  auto src = src_;
  runTask(
      QStringLiteral("인쇄 준비 중(출력 해상도 입면 영상)"),
      [this, doc, sp, A, src](QString *m) {
        return prepareSheetAssets(
            doc, *src, *sp, *A, m, &cancelTask_, [this](double x) {
              QMetaObject::invokeMethod(
                  this, [this, x] { setProgress(x); }, Qt::QueuedConnection);
            });
      },
      [this, doc, sp, A](bool ok, const QString &m) {
        if (!ok) {
          report(false, QStringLiteral("인쇄 준비 실패: ") + m);
          return;
        }
        const SheetGeom G = sheetGeom(doc, *sp);
        const int pages = sp->split ? G.L.cols * G.L.rows : 1;
        runPrintPreview(this, G.L, sp->spec, sp->title, pages,
                        [doc, sp, A](QPainter &pt, double pxPerMm, int i) {
                          paintSheet(pt, doc, *sp, pxPerMm, i, A->img, A->geo,
                                     1, 0, 0, nullptr,
                                     A->hasStrip ? &A->strip : nullptr);
                        });
        showStatus(QStringLiteral("인쇄 창을 닫았습니다"));
      });
}

void MainWindow::printPlanSheet(const PlanSheetParams &p) {
  if (!src_)
    return;
  struct PlanAssets {
    PlanSheetParams sp;
    QImage img;
    double x0 = 0, y1 = 0, res = 0;
  };
  auto A = std::make_shared<PlanAssets>();
  auto src = src_;
  const PlanSheetParams p0 = p;
  runTask(
      QStringLiteral("인쇄 준비 중(출력 해상도 정사영상)"),
      [this, p0, A, src](QString *m) {
        return preparePlanAssets(*src, p0, A->sp, A->img, A->x0, A->y1, A->res,
                                 m, &cancelTask_, [this](double x) {
                                   QMetaObject::invokeMethod(
                                       this, [this, x] { setProgress(x); },
                                       Qt::QueuedConnection);
                                 });
      },
      [this, A, src](bool ok, const QString &m) {
        if (!ok) {
          report(false, QStringLiteral("인쇄 준비 실패: ") + m);
          return;
        }
        const PlanWindow W0 = planWindow(A->sp, 0);
        const int pages = A->sp.split ? W0.L.cols * W0.L.rows : 1;
        const Vec3 origin = src->srs.origin;
        runPrintPreview(this, W0.L, A->sp.spec, A->sp.title, pages,
                        [A, origin](QPainter &pt, double pxPerMm, int i) {
                          paintPlanSheet(pt, A->sp, pxPerMm, i, A->img, A->x0,
                                         A->y1, A->res, origin);
                        });
        showStatus(QStringLiteral("인쇄 창을 닫았습니다"));
      });
}

void MainWindow::printCurrentSheet() {
  if (!sheetHost_) {
    showStatus(QStringLiteral("도면 탭에서 인쇄하세요"));
    return;
  }
  if (auto *b =
          sheetHost_->findChild<QPushButton *>(QStringLiteral("sheetPrint")))
    b->click();
}

// ============================================================================
// 평·단면도 한 장(2026-10-16, 사용자 「3장이 출력될 걸 한 장에 축척 · 방향을
// 맞춰」 · 「중앙 화살표 방향에 단면」 · 「용지 크기는 신경 쓰지 말고 최적
// 크기로 배치만」 · 「표고 여백을 너무 두지 마라」). 배치 규칙은
// asec::layoutCombo(core, 시험 test_combo.cpp). 여기는 영상 준비 · 그리기 ·
// 저장 · 조판 판
namespace {
constexpr double kComboGapMm = 8.0;    // 평면 칸 ~ 첫 단면 띠
constexpr double kComboStackMm = 4.0;  // 같은 쪽 단면끼리
constexpr double kComboMarginMm = 10.0; // 내용 바깥 여백(종이 = 내용 + 여백)
constexpr double kComboTitleMm = 16.0;  // 아래 축척 막대 · 도면명 띠
constexpr double kComboPlanLineMm =
    0.18; // 평면 위 단면선(사용자 「단면선이 너무 두껍다 · 얇게」)

SheetSpec comboSpec(double denom) {
  SheetSpec s;
  s.denom = std::max(1.0, denom);
  s.gutterLeftMm = 14.0; // 표고 글자 한 줄(「EL. 23.4 m」)
  s.gutterRightMm = 3.0;
  s.gutterTopMm = 6.0; // A · A′ 글자
  s.gutterBottomMm = 2.0;
  return s;
}

bool openProfile(const std::vector<Polyline> &profile) {
  for (const auto &pl : profile)
    if (pl.size() >= 2 && !(pl.size() > 3 && (pl.front() - pl.back()).len() < 1e-9))
      return true;
  return false;
}

void splitSectionName(const QString &name, QString &a, QString &b) {
  const int dash = name.indexOf(QChar(0x2013)); // –
  if (dash > 0) {
    a = name.left(dash).trimmed();
    b = name.mid(dash + 1).trimmed();
  } else {
    a = name.isEmpty() ? QStringLiteral("A") : name;
    b = a + QChar(0x2032); // ′
  }
}

QString dxfAscii(QString s) {
  s.replace(QChar(0x2032), QLatin1Char('\''));
  s.replace(QChar(0x2013), QLatin1Char('-'));
  QString o;
  for (QChar c : s)
    if (c.unicode() < 128)
      o += c;
  return o;
}

/// 입면 영상을 그림 칸(s 0..L, 표고 zMin..zMax 로컬)에 걸치는 픽셀만 남긴다 —
/// SVG · DXF 는 잘라 그리기(clip)가 없으므로 미리 자른다
void cropComboImage(ComboSecAsset &a, double L) {
  if (a.img.isNull() || !(a.imgRes > 0))
    return;
  const double r = a.imgRes;
  const int i0 = std::max(0, int(std::floor((0.0 - a.imgS0) / r + 1e-9)));
  const int i1 =
      std::min(a.img.width(), int(std::ceil((L - a.imgS0) / r - 1e-9)));
  const int j0 = std::max(0, int(std::floor((a.imgZ1 - a.r.zMax) / r + 1e-9)));
  const int j1 =
      std::min(a.img.height(), int(std::ceil((a.imgZ1 - a.r.zMin) / r - 1e-9)));
  if (i1 <= i0 || j1 <= j0) {
    a.img = QImage();
    return;
  }
  if (i0 > 0 || j0 > 0 || i1 < a.img.width() || j1 < a.img.height())
    a.img = a.img.copy(i0, j0, i1 - i0, j1 - j0);
  a.imgS0 += i0 * r;
  a.imgZ1 -= j0 * r;
}

/// 표고 글자를 붙일 레벨선: A 끝(s = 0)에서 지면 바로 아래 선(글자는 그림 칸
/// 왼쪽 밖) → 없으면 땅속 구간이 있는 가장 높은 선(글자는 그 구간 시작 위)
struct ComboElevLabel {
  bool ok = false, atLeft = false;
  double zAbs = 0, s0 = 0;
};
ComboElevLabel pickElevLabel(const ComboSecAsset &a, double L) {
  ComboElevLabel out;
  const double oz = a.r.srs.origin.z;
  const std::vector<LevelLine> lv =
      levelLines(a.r.zMin + oz - 1e-9, a.r.zMax + oz + 1e-9, 10);
  for (auto it = lv.rbegin(); it != lv.rend(); ++it) {
    const std::vector<SGap> g = levelLineSpans(a.r.profile, it->z - oz, 0.0, L);
    if (g.empty())
      continue;
    if (!out.ok) {
      out.ok = true;
      out.zAbs = it->z;
      out.s0 = g.front().s0;
    }
    if (g.front().s0 <= 1e-6) {
      out.zAbs = it->z;
      out.s0 = 0;
      out.atLeft = true;
      return out;
    }
  }
  return out;
}

/// 단면 하나(단면 자체 좌표: x = 거리 s 오른쪽 · y = 아래로 표고 낮아짐, 원점 =
/// A · 그림 칸 위). 호출 전에 painter 를 종이 자리로 옮기고 돌려 둔다.
/// 순서: 레벨선(땅속만) → 입면 영상 → 잘린 선(빨강) → A · A′ → 표고 글자
void paintComboSection(QPainter &p, const ComboSecAsset &a,
                       const ComboPlaced &pl, const ComboSheetParams &sp,
                       double mmPerM, double k, double minLinePx) {
  const double oz = a.r.srs.origin.z;
  const double L = pl.lenM;
  const double zTop = a.r.zMax + oz;
  auto X = [&](double s) { return s * mmPerM * k; };
  auto Y = [&](double zAbs) { return (zTop - zAbs) * mmPerM * k; };
  if (sp.withLevels) {
    const bool ground = openProfile(a.r.profile);
    const double wMinor = ptToMm(clampLineWeightPt(sp.levelMinorPt)) * k;
    const double wMajor = ptToMm(clampLineWeightPt(sp.levelMajorPt)) * k;
    for (const LevelLine &lv :
         levelLines(a.r.zMin + oz - 1e-9, a.r.zMax + oz + 1e-9, 10)) {
      const bool major = lv.cls != LevelClass::Minor;
      QPen pen(major ? theme::LevelMajor : theme::LevelMinor,
               major ? wMajor : wMinor);
      pen.setCapStyle(Qt::FlatCap);
      p.setPen(pen);
      const double y = Y(lv.z);
      if (ground) {
        for (const SGap &g : levelLineSpans(a.r.profile, lv.z - oz, 0.0, L))
          p.drawLine(QPointF(X(g.s0), y), QPointF(X(g.s1), y));
      } else
        p.drawLine(QPointF(X(0), y), QPointF(X(L), y));
    }
  }
  if (sp.withSectionImage && !a.img.isNull() && a.imgRes > 0) {
    p.setOpacity(std::clamp(sp.imageOpacity, 0.0, 1.0));
    p.drawImage(QRectF(X(a.imgS0), Y(a.imgZ1 + oz),
                       a.img.width() * a.imgRes * mmPerM * k,
                       a.img.height() * a.imgRes * mmPerM * k),
                a.img);
    p.setOpacity(1.0);
  }
  p.setPen(QPen(theme::ProfileRed,
                std::max(minLinePx, exportLineMm(sp.lineWidthPx) * k),
                Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setBrush(Qt::NoBrush);
  for (const auto &pl0 : a.r.profile) {
    if (pl0.size() < 2)
      continue;
    const bool closed =
        pl0.size() > 3 && (pl0.front() - pl0.back()).len() < 1e-9;
    QPainterPath path;
    path.moveTo(X(pl0[0].x), Y(pl0[0].y + oz));
    const size_t n = closed ? pl0.size() - 1 : pl0.size();
    for (size_t i = 1; i < n; ++i)
      path.lineTo(X(pl0[i].x), Y(pl0[i].y + oz));
    if (closed)
      path.closeSubpath();
    p.drawPath(path);
  }
  QFont fl = theme::uiFont(std::max(4, int(std::lround(3.0 * k))), true);
  p.setFont(fl);
  p.setPen(theme::Ink);
  p.drawText(QRectF(X(0) - 10 * k, -6.0 * k, 20 * k, 5.2 * k),
             Qt::AlignHCenter | Qt::AlignBottom, a.nameA);
  p.drawText(QRectF(X(L) - 10 * k, -6.0 * k, 20 * k, 5.2 * k),
             Qt::AlignHCenter | Qt::AlignBottom, a.nameB);
  if (sp.withLevels) {
    const ComboElevLabel el = pickElevLabel(a, L);
    if (el.ok) {
      QFont fe = theme::monoFont(std::max(4, int(std::lround(2.1 * k))));
      p.setFont(fe);
      p.setPen(theme::LevelText);
      const QString t = QStringLiteral("EL. %1 m")
                            .arg(qs8(formatElevation(el.zAbs, 1)));
      const double y = Y(el.zAbs);
      if (el.atLeft)
        p.drawText(QRectF(-14.0 * k, y - 1.6 * k, 13.0 * k, 3.2 * k),
                   Qt::AlignRight | Qt::AlignVCenter, t);
      else
        p.drawText(QRectF(X(el.s0) + 0.6 * k, y - 3.0 * k, 30 * k, 2.8 * k),
                   Qt::AlignLeft | Qt::AlignBottom, t);
    }
  }
}

/// 진북(돌린 평면에 맞춰 deg 만큼 시계 방향). 글자는 똑바로
void paintComboNorth(QPainter &p, double cxMm, double cyMm, double rMm,
                     double deg, double k) {
  const QPointF c(cxMm * k, cyMm * k);
  const double R = rMm * k;
  p.save();
  p.setPen(QPen(theme::Ink, 0.2 * k));
  p.setBrush(Qt::white);
  p.drawEllipse(c, R, R);
  p.translate(c);
  p.rotate(deg);
  QPolygonF arrow;
  arrow << QPointF(0, -R * 0.78) << QPointF(-R * 0.28, R * 0.15)
        << QPointF(0, -R * 0.05) << QPointF(R * 0.28, R * 0.15);
  p.setBrush(theme::Ink);
  p.drawPolygon(arrow);
  p.restore();
  const double a = deg * 3.14159265358979323846 / 180.0;
  const QPointF tip = c + QPointF(std::sin(a), -std::cos(a)) * (R + 2.0 * k);
  p.setFont(theme::uiFont(std::max(4, int(std::lround(2.4 * k))), true));
  p.setPen(theme::Ink);
  p.drawText(QRectF(tip.x() - 3 * k, tip.y() - 1.6 * k, 6 * k, 3.2 * k),
             Qt::AlignCenter, QStringLiteral("N"));
}

/// 평면 칸: 돌린 정사영상 → 테두리 → 단면선(얇은 빨강) · 가운데 화살표(바라보는
/// 쪽 = 단면이 놓인 쪽) · A/A′ 글자 → 진북
void paintComboPlan(QPainter &p, const ComboLayout &c, const ComboAssets &a,
                    const ComboSheetParams &sp, double k, double minLinePx) {
  const QRectF pr(c.planX * k, c.planY * k, c.planW * k, c.planH * k);
  if (sp.withPlanImage && !a.plan.isNull() && a.planRes > 0)
    p.drawImage(QRectF(c.xMm(a.planS0) * k, c.yMm(a.planT1) * k,
                       a.plan.width() * a.planRes * c.mmPerM * k,
                       a.plan.height() * a.planRes * c.mmPerM * k),
                a.plan);
  p.setPen(QPen(theme::Ink2, 0.13 * k));
  p.setBrush(Qt::NoBrush);
  p.drawRect(pr);
  QFont fl = theme::uiFont(std::max(4, int(std::lround(2.8 * k))), true);
  for (size_t i = 0; i < c.sections.size() && i < a.secs.size(); ++i) {
    const ComboPlaced &pl = c.sections[i];
    const QPointF A(c.xMm(pl.sA) * k, c.yMm(pl.tA) * k),
        B(c.xMm(pl.sB) * k, c.yMm(pl.tB) * k);
    const QPointF d = B - A;
    const double dl = std::hypot(d.x(), d.y());
    if (dl < 1e-9)
      continue;
    const QPointF u = d / dl, n(u.y(), -u.x()); // 종이 위 왼쪽 법선 = 바라보는 쪽
    p.setPen(QPen(theme::SectionRed, std::max(minLinePx, kComboPlanLineMm * k),
                  Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(A, B);
    const QPointF m = (A + B) / 2, a0 = m + n * (0.8 * k), a1 = m + n * (4.0 * k);
    p.drawLine(a0, a1);
    p.drawLine(a1, a1 - n * (1.2 * k) + u * (0.8 * k));
    p.drawLine(a1, a1 - n * (1.2 * k) - u * (0.8 * k));
    p.setFont(fl);
    p.setPen(theme::Ink);
    const QPointF ca = A - u * (2.8 * k), cb = B + u * (2.8 * k);
    p.drawText(QRectF(ca.x() - 6 * k, ca.y() - 1.8 * k, 12 * k, 3.6 * k),
               Qt::AlignCenter, a.secs[i].nameA);
    p.drawText(QRectF(cb.x() - 6 * k, cb.y() - 1.8 * k, 12 * k, 3.6 * k),
               Qt::AlignCenter, a.secs[i].nameB);
  }
  if (c.planW > 20 && c.planH > 20)
    paintComboNorth(p, c.planX + c.planW - 7.0, c.planY + 7.0, 3.5,
                    c.northDeg(), k);
  else
    paintComboNorth(p, c.planX + c.planW + 6.0, c.planY + 4.0, 3.0,
                    c.northDeg(), k);
}

void insertSvgEmptyLayers(const QString &path) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly))
    return;
  QString svg = QString::fromUtf8(f.readAll());
  f.close();
  const int at = svg.indexOf(QStringLiteral("<svg"));
  const int gt = at < 0 ? -1 : svg.indexOf('>', at);
  if (gt <= 0)
    return;
  svg.insert(gt + 1,
             QStringLiteral("\n<g id=\"DRAW_SOIL\"/>\n<g id=\"DRAW_OUTLINE\"/>\n"));
  if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    f.write(svg.toUtf8());
}

/// DXF(종이 mm, 왼쪽 아래 원점 · y 위). 영상은 종이 방향으로 미리 돌려 별도 PNG
bool writeComboDxf(const ComboLayout &c, const ComboAssets &a,
                   const ComboSheetParams &sp, const QString &path,
                   QString *msg, QStringList *files) {
  const SheetLayout &L = c.L;
  DxfWriter d;
  d.addLayer("DRAW_SOIL", 7, 25);
  d.addLayer("DRAW_OUTLINE", 7, 35);
  d.addLayer("FRAME", 7);
  d.addLayer("PLAN_IMAGE", 7);
  d.addLayer("PLAN_SECTION_LINE", 1, 18);
  d.addLayer("PLAN_TEXT", 7, 18);
  d.addLayer("NORTH", 7, 18);
  d.addLayer("SECTION_IMAGE", 7);
  d.addLayer("SECTION_PROFILE", 1,
             dxfLineWeight(exportLineMm(sp.lineWidthPx) * 72.0 / 25.4));
  d.addLayer("LEVEL_10CM", 9, dxfLineWeight(sp.levelMinorPt));
  d.addLayer("LEVEL_50CM", 8, dxfLineWeight(sp.levelMajorPt));
  d.addLayer("LEVEL_TEXT", 7, 18);
  d.addLayer("SECTION_TEXT", 7, 25);
  d.addLayer("TITLE", 7, 18);
  auto P = [&](double x, double y) { return Vec3(x, L.paperH - y, 0); };
  auto P2 = [&](double x, double y) { return Vec2(x, L.paperH - y); };
  d.polyline2d("FRAME",
               {P2(L.frameX, L.frameY), P2(L.frameX + L.frameW, L.frameY),
                P2(L.frameX + L.frameW, L.frameY + L.frameH),
                P2(L.frameX, L.frameY + L.frameH)},
               true);
  const QFileInfo fi(path);
  const QString base = fi.completeBaseName();
  auto putImage = [&](const char *layer, const QImage &img0, double leftMm,
                      double topMm, double wMm, double hMm,
                      const QString &suffix) {
    if (img0.isNull() || wMm <= 0 || hMm <= 0)
      return true;
    const QString png = base + suffix + QStringLiteral(".png");
    const QImage img = img0.mirrored(false, true); // 삽입점(왼쪽 아래)에서 +Y 가 다음 행
    if (!img.save(fi.absolutePath() + QLatin1Char('/') + png, "PNG")) {
      if (msg)
        *msg = QStringLiteral("PNG 저장 실패: %1").arg(png);
      return false;
    }
    if (files)
      *files << png;
    d.image(layer, png.toStdString(), img.width(), img.height(),
            P(leftMm, topMm + hMm), Vec3(wMm / img.width(), 0, 0),
            Vec3(0, hMm / img.height(), 0));
    return true;
  };
  // 평면
  if (sp.withPlanImage && !a.plan.isNull() && a.planRes > 0)
    if (!putImage("PLAN_IMAGE", a.plan, c.xMm(a.planS0), c.yMm(a.planT1),
                  a.plan.width() * a.planRes * c.mmPerM,
                  a.plan.height() * a.planRes * c.mmPerM,
                  QStringLiteral("_plan")))
      return false;
  d.polyline2d("FRAME",
               {P2(c.planX, c.planY), P2(c.planX + c.planW, c.planY),
                P2(c.planX + c.planW, c.planY + c.planH),
                P2(c.planX, c.planY + c.planH)},
               true);
  for (size_t i = 0; i < c.sections.size() && i < a.secs.size(); ++i) {
    const ComboPlaced &pl = c.sections[i];
    const double ax = c.xMm(pl.sA), ay = c.yMm(pl.tA), bx = c.xMm(pl.sB),
                 by = c.yMm(pl.tB);
    const double dl = std::hypot(bx - ax, by - ay);
    if (dl < 1e-9)
      continue;
    const double ux = (bx - ax) / dl, uy = (by - ay) / dl, nx = uy, ny = -ux;
    d.line("PLAN_SECTION_LINE", P(ax, ay), P(bx, by));
    const double mx = (ax + bx) / 2, my = (ay + by) / 2;
    const double t0x = mx + nx * 0.8, t0y = my + ny * 0.8, t1x = mx + nx * 4.0,
                 t1y = my + ny * 4.0;
    d.line("PLAN_SECTION_LINE", P(t0x, t0y), P(t1x, t1y));
    d.line("PLAN_SECTION_LINE", P(t1x, t1y),
           P(t1x - nx * 1.2 + ux * 0.8, t1y - ny * 1.2 + uy * 0.8));
    d.line("PLAN_SECTION_LINE", P(t1x, t1y),
           P(t1x - nx * 1.2 - ux * 0.8, t1y - ny * 1.2 - uy * 0.8));
    d.text("PLAN_TEXT", P(ax - ux * 2.8, ay - uy * 2.8), 2.8,
           dxfAscii(a.secs[i].nameA).toStdString(), 1, 2);
    d.text("PLAN_TEXT", P(bx + ux * 2.8, by + uy * 2.8), 2.8,
           dxfAscii(a.secs[i].nameB).toStdString(), 1, 2);
  }
  {
    const bool inside = c.planW > 20 && c.planH > 20;
    const double cx = inside ? c.planX + c.planW - 7.0 : c.planX + c.planW + 6.0;
    const double cy = inside ? c.planY + 7.0 : c.planY + 4.0;
    const double R = inside ? 3.5 : 3.0;
    const double ang = c.northDeg() * 3.14159265358979323846 / 180.0;
    const double dx = std::sin(ang), dy = -std::cos(ang); // 종이(y 아래) 위 진북
    d.line("NORTH", P(cx - dx * R * 0.7, cy - dy * R * 0.7),
           P(cx + dx * R * 0.85, cy + dy * R * 0.85));
    d.line("NORTH", P(cx + dx * R * 0.85, cy + dy * R * 0.85),
           P(cx + dx * R * 0.45 - dy * R * 0.25, cy + dy * R * 0.45 + dx * R * 0.25));
    d.line("NORTH", P(cx + dx * R * 0.85, cy + dy * R * 0.85),
           P(cx + dx * R * 0.45 + dy * R * 0.25, cy + dy * R * 0.45 - dx * R * 0.25));
    d.text("NORTH", P(cx + dx * (R + 2.0), cy + dy * (R + 2.0)), 2.4, "N", 1, 2);
  }
  // 단면
  for (size_t i = 0; i < c.sections.size() && i < a.secs.size(); ++i) {
    const ComboPlaced &pl = c.sections[i];
    const ComboSecAsset &as = a.secs[i];
    const double oz = as.r.srs.origin.z, zTop = as.r.zMax + oz, L0 = pl.lenM,
                 mm = c.mmPerM;
    auto Q = [&](double x, double y) {
      double px, py;
      pl.toPaper(x, y, px, py);
      return P(px, py);
    };
    auto Q2 = [&](double x, double y) {
      double px, py;
      pl.toPaper(x, y, px, py);
      return P2(px, py);
    };
    auto X = [&](double s) { return s * mm; };
    auto Y = [&](double zAbs) { return (zTop - zAbs) * mm; };
    const double textRot = pl.rotDeg == -90 ? 90.0 : pl.rotDeg == 90 ? -90.0 : 0.0;
    if (sp.withLevels) {
      const bool ground = openProfile(as.r.profile);
      for (const LevelLine &lv :
           levelLines(as.r.zMin + oz - 1e-9, as.r.zMax + oz + 1e-9, 10)) {
        const char *layer =
            lv.cls == LevelClass::Minor ? "LEVEL_10CM" : "LEVEL_50CM";
        const double y = Y(lv.z);
        if (ground) {
          for (const SGap &g : levelLineSpans(as.r.profile, lv.z - oz, 0.0, L0))
            d.line(layer, Q(X(g.s0), y), Q(X(g.s1), y));
        } else
          d.line(layer, Q(0, y), Q(X(L0), y));
      }
      const ComboElevLabel el = pickElevLabel(as, L0);
      if (el.ok) {
        const std::string t = "EL. " + formatElevation(el.zAbs, 1) + " m";
        if (el.atLeft)
          d.text("LEVEL_TEXT", Q(-1.0, Y(el.zAbs)), 2.0, t, 2, 2, textRot);
        else
          d.text("LEVEL_TEXT", Q(X(el.s0) + 0.6, Y(el.zAbs) - 0.4), 2.0, t, 0,
                 1, textRot);
      }
    }
    if (sp.withSectionImage && !as.img.isNull() && as.imgRes > 0) {
      const double x0 = X(as.imgS0), y0 = Y(as.imgZ1 + oz),
                   x1 = x0 + as.img.width() * as.imgRes * mm,
                   y1 = y0 + as.img.height() * as.imgRes * mm;
      double l = 1e300, t = 1e300, r = -1e300, b = -1e300;
      for (double x : {x0, x1})
        for (double y : {y0, y1}) {
          double px, py;
          pl.toPaper(x, y, px, py);
          l = std::min(l, px);
          r = std::max(r, px);
          t = std::min(t, py);
          b = std::max(b, py);
        }
      const QImage rot =
          pl.rotDeg == 0
              ? as.img
              : as.img.transformed(QTransform().rotate(pl.rotDeg));
      if (!putImage("SECTION_IMAGE", rot, l, t, r - l, b - t,
                    QStringLiteral("_%1").arg(i + 1)))
        return false;
    }
    for (const auto &pl0 : as.r.profile) {
      if (pl0.size() < 2)
        continue;
      const bool closed =
          pl0.size() > 3 && (pl0.front() - pl0.back()).len() < 1e-9;
      std::vector<Vec2> pts;
      const size_t n = closed ? pl0.size() - 1 : pl0.size();
      pts.reserve(n);
      for (size_t j = 0; j < n; ++j)
        pts.push_back(Q2(X(pl0[j].x), Y(pl0[j].y + oz)));
      d.polyline2d("SECTION_PROFILE", pts, closed);
    }
    d.text("SECTION_TEXT", Q(X(0), -1.0), 3.0, dxfAscii(as.nameA).toStdString(),
           1, 1, textRot);
    d.text("SECTION_TEXT", Q(X(L0), -1.0), 3.0,
           dxfAscii(as.nameB).toStdString(), 1, 1, textRot);
  }
  d.text("TITLE", P(L.frameX + 3.0, L.titleY + 6.0), 2.4,
         "PLAN + SECTIONS  S=1:" + std::to_string(int(std::lround(sp.denom))),
         0, 2);
  std::string err;
  if (!d.save(toFsS(path), &err)) {
    if (msg)
      *msg = qs8(err);
    return false;
  }
  return true;
}
} // namespace

ComboLayout MainWindow::comboLayoutFor(const ComboAssets &a,
                                       const ComboSheetParams &sp) {
  std::vector<ComboSection> secs;
  secs.reserve(a.secs.size());
  for (const ComboSecAsset &s : a.secs) {
    ComboSection c;
    c.ax = s.r.line.a.x;
    c.ay = s.r.line.a.y;
    c.bx = s.r.line.b.x;
    c.by = s.r.line.b.y;
    c.heightM = std::max(0.0, s.r.zMax - s.r.zMin);
    secs.push_back(c);
  }
  ComboLayout c = layoutCombo(comboSpec(sp.denom), secs, sp.planMarginM,
                              kComboGapMm, kComboStackMm);
  freeComboPage(c, kComboMarginMm, kComboTitleMm);
  return c;
}

bool MainWindow::computeComboSections(
    MeshSource &src, const std::vector<ComboSecInput> &in,
    const ComboSheetParams &sp, bool preview, ComboAssets &out, QString *msg,
    const std::atomic<bool> *cancel,
    const std::function<void(double)> &progress) {
  out.secs.clear();
  out.skipped.clear();
  out.origin = src.srs.origin;
  if (in.empty()) {
    if (msg)
      *msg = QStringLiteral("단면이 없습니다. 평면에서 단면선을 그린 뒤 여세요");
    return false;
  }
  for (size_t i = 0; i < in.size(); ++i) {
    const SectionLine &ln = in[i].line;
    const double L = SectionFrame(ln).L;
    SectionRequest rq;
    rq.line = ln;
    rq.zRangeFromProfile = true;
    rq.cleanup.smooth = sp.smooth;
    rq.depthFade = sp.depthFade ? kDepthFadeStrength : 0.0;
    rq.wantImage = sp.withSectionImage;
    if (preview) {
      rq.imageRes = std::max(0.004, L / 1500.0);
      rq.meshRes = rq.imageRes; // 미리보기: 영상은 거친 LOD(잘린 선은 잎)
      rq.maxImagePixels = size_t(6) << 20;
    } else {
      rq.imageRes = groundResolution(std::max(1.0, sp.denom), sp.dpi);
      rq.maxImagePixels = size_t(80) << 20;
    }
    SectionOutput o;
    std::string e;
    if (!computeSection(src, rq, o, &e, cancel)) {
      if (cancel && cancel->load()) {
        if (msg)
          *msg = QStringLiteral("취소됨");
        return false;
      }
      out.skipped << QStringLiteral("%1: %2").arg(in[i].name, qs8(e));
      continue; // 한 단면이 메시 밖이어도 나머지로 한 장을 만든다
    }
    ComboSecAsset as;
    as.r = o.result;
    if (rq.wantImage) {
      as.img = toQImageS(o.image.img);
      as.imgS0 = o.image.s0;
      as.imgZ1 = o.image.z1;
      as.imgRes = o.image.res;
      cropComboImage(as, L);
    }
    splitSectionName(in[i].name, as.nameA, as.nameB);
    out.secs.push_back(std::move(as));
    if (progress)
      progress(double(i + 1) / double(in.size()));
    if (cancel && cancel->load()) {
      if (msg)
        *msg = QStringLiteral("취소됨");
      return false;
    }
  }
  if (out.secs.empty()) {
    if (msg)
      *msg = QStringLiteral("단면을 계산하지 못했습니다 — ") +
             out.skipped.join(QStringLiteral(" · "));
    return false;
  }
  return true;
}

bool MainWindow::renderComboPlan(MeshSource &src, const ComboSheetParams &sp,
                                 double res, ComboAssets &out, QString *msg,
                                 const std::atomic<bool> *cancel) {
  out.plan = QImage();
  out.planRes = 0;
  out.planMarginM = sp.planMarginM;
  if (out.secs.empty()) {
    if (msg)
      *msg = QStringLiteral("단면이 없습니다");
    return false;
  }
  const ComboLayout c = comboLayoutFor(out, sp);
  if (!(c.sMax > c.sMin) || !(c.tMax > c.tMin)) {
    if (msg)
      *msg = QStringLiteral("평면 범위가 비어 있습니다");
    return false;
  }
  const PlanStripArea pa = comboPlanArea(c);
  res = std::max(1e-4, res);
  // 돌린 평면을 바로 그린다(다 그린 정사영상을 다시 돌려 표본화하면 흐려짐 —
  // 사용자 2026-10-16 「평면이 화질저하」). 픽셀 수 상한은 단면 영상과 같은 80 MP
  const double pix = (c.sMax - c.sMin) / res * ((c.tMax - c.tMin) / res);
  if (pix > 80e6)
    res *= std::sqrt(pix / 80e6);
  Box3 box;
  box.mn = Vec3(std::max(pa.x0, src.bounds.mn.x),
                std::max(pa.y0, src.bounds.mn.y), src.bounds.mn.z);
  box.mx = Vec3(std::min(pa.x1, src.bounds.mx.x),
                std::min(pa.y1, src.bounds.mx.y), src.bounds.mx.z);
  if (!(box.mx.x > box.mn.x && box.mx.y > box.mn.y)) {
    if (msg)
      *msg = QStringLiteral("내보낼 범위가 비어 있습니다");
    return false;
  }
  std::vector<MeshPtr> meshes;
  if (!collectPlanMeshes(src, box, res, meshes, msg, cancel))
    return false;
  const int W = std::max(1, int(std::ceil((c.sMax - c.sMin) / res)));
  const int H = std::max(1, int(std::ceil((c.tMax - c.tMin) / res)));
  QImage rot(W, H, QImage::Format_RGB32);
  {
    RgbaImage img;
    if (!renderPlanFrame(meshes, c.ax, c.ay, c.ux, c.uy, c.nx, c.ny, c.sMin,
                         c.tMax, res, W, H, img, cancel)) {
      if (msg)
        *msg = QStringLiteral("평면 영상 생성 실패(취소 또는 메모리)");
      return false;
    }
    for (int y = 0; y < H; ++y) { // 빈 곳(메시 없음)은 흰 종이
      QRgb *o = reinterpret_cast<QRgb *>(rot.scanLine(y));
      const uint8_t *q = &img.px[size_t(y) * W * 4];
      for (int x = 0; x < W; ++x, q += 4)
        o[x] = q[3] ? qRgb(q[0], q[1], q[2]) : qRgb(255, 255, 255);
    }
  }
  out.plan = rot;
  out.planS0 = c.sMin;
  out.planT1 = c.tMax;
  out.planRes = res;
  return true;
}

void MainWindow::paintComboSheet(QPainter &p, const ComboAssets &a,
                                 const ComboSheetParams &sp, double k,
                                 double minLinePx) {
  const ComboLayout c = comboLayoutFor(a, sp);
  const SheetLayout &L = c.L;
  p.save();
  p.setRenderHint(QPainter::Antialiasing);
  p.setRenderHint(QPainter::TextAntialiasing);
  p.setRenderHint(QPainter::SmoothPixmapTransform);
  p.fillRect(QRectF(0, 0, L.paperW * k, L.paperH * k), Qt::white);
  if (!c.sections.empty()) {
    paintComboPlan(p, c, a, sp, k, minLinePx);
    for (size_t i = 0; i < c.sections.size() && i < a.secs.size(); ++i) {
      const ComboPlaced &pl = c.sections[i];
      p.save();
      p.translate(pl.originX * k, pl.originY * k);
      p.rotate(pl.rotDeg);
      paintComboSection(p, a.secs[i], pl, sp, c.mmPerM, k, minLinePx);
      p.restore();
    }
  }
  SheetChrome ch;
  ch.title = sp.title;
  ch.srs = sp.srsLabel;
  ch.date = sp.date;
  ch.kind = QStringLiteral("평·단면도");
  ch.denom = sp.denom;
  ch.showTitle = true;
  paintSheetChrome(p, L, k, ch, false);
  p.restore();
}

bool MainWindow::exportComboSheet(MeshSource &src,
                                  const std::vector<ComboSecInput> &in,
                                  const ComboSheetParams &sp0,
                                  const QString &path, QString *msg,
                                  const std::atomic<bool> *cancel,
                                  const std::function<void(double)> &progress) {
  ComboSheetParams sp = sp0;
  sp.denom = std::max(1.0, std::round(sp.denom));
  sp.dpi = std::clamp(sp.dpi, 72.0, 1200.0);
  ComboAssets a;
  if (!computeComboSections(src, in, sp, false, a, msg, cancel,
                            [&](double x) {
                              if (progress)
                                progress(0.6 * x);
                            }))
    return false;
  if (sp.withPlanImage &&
      !renderComboPlan(src, sp, groundResolution(sp.denom, sp.dpi), a, msg,
                       cancel))
    return false;
  if (progress)
    progress(0.75);
  const ComboLayout c = comboLayoutFor(a, sp);
  const SheetLayout &L = c.L;
  QStringList files;
  files << QFileInfo(path).fileName();
  if (sp.format == 1) {
    if (!writeComboDxf(c, a, sp, path, msg, &files))
      return false;
  } else if (sp.format == 4) {
    const double k = 96.0 / 25.4;
    QSvgGenerator gen(QSvgGenerator::SvgVersion::Svg11);
    gen.setFileName(path);
    gen.setResolution(96);
    gen.setTitle(sp.title);
    gen.setDescription(
        QStringLiteral("DRAW_SOIL and DRAW_OUTLINE are empty drawing layers"));
    gen.setSize(QSize(std::max(1, int(std::lround(L.paperW * k))),
                      std::max(1, int(std::lround(L.paperH * k)))));
    gen.setViewBox(QRectF(0, 0, L.paperW * k, L.paperH * k));
    QPainter pt;
    if (!pt.begin(&gen)) {
      if (msg)
        *msg = QStringLiteral("SVG 를 만들 수 없습니다: %1").arg(path);
      return false;
    }
    paintComboSheet(pt, a, sp, k);
    pt.end();
    insertSvgEmptyLayers(path);
  } else if (sp.format == 0) {
    QPdfWriter w(path);
    w.setResolution(int(std::lround(sp.dpi)));
    w.setTitle(QStringLiteral("%1 — %2").arg(sp.title, sp.srsLabel));
    w.setCreator(QStringLiteral("Trench %1").arg(QString::fromUtf8(kVersion)));
    const bool wide = L.paperW > L.paperH; // 쪽 크기는 세로로 정의하고 방향으로 돌린다
    w.setPageLayout(QPageLayout(
        QPageSize(QSizeF(std::min(L.paperW, L.paperH),
                         std::max(L.paperW, L.paperH)),
                  QPageSize::Millimeter,
                  QStringLiteral("Trench %1x%2 mm")
                      .arg(L.paperW, 0, 'f', 0)
                      .arg(L.paperH, 0, 'f', 0),
                  QPageSize::ExactMatch),
        wide ? QPageLayout::Landscape : QPageLayout::Portrait,
        QMarginsF(0, 0, 0, 0), QPageLayout::Millimeter));
    QPainter pt;
    if (!pt.begin(&w)) {
      if (msg)
        *msg = QStringLiteral("PDF 를 만들 수 없습니다: %1").arg(path);
      return false;
    }
    paintComboSheet(pt, a, sp, w.resolution() / 25.4);
    pt.end();
  } else {
    const double k = sp.dpi / 25.4;
    const QSize sz(int(std::lround(L.paperW * k)), int(std::lround(L.paperH * k)));
    if (double(sz.width()) * sz.height() > 400e6) {
      if (msg)
        *msg = QStringLiteral("출력 영상이 너무 큽니다(%1 × %2 px). DPI 를 "
                              "낮추거나 축척 분모를 키우세요.")
                   .arg(sz.width())
                   .arg(sz.height());
      return false;
    }
    QImage canvas(sz, QImage::Format_RGB32);
    canvas.fill(Qt::white);
    {
      QPainter pt(&canvas);
      paintComboSheet(pt, a, sp, k);
    }
    const int dpm = int(std::lround(sp.dpi / 0.0254));
    canvas.setDotsPerMeterX(dpm);
    canvas.setDotsPerMeterY(dpm);
    if (sp.format == 2) {
      if (!canvas.save(path, "PNG")) {
        if (msg)
          *msg = QStringLiteral("PNG 저장 실패: %1").arg(path);
        return false;
      }
    } else {
      TiffOptions o;
      o.dpi = sp.dpi;
      o.alpha = false;
      o.description = "Plan and sections sheet " + src.srs.shortLabel();
      std::string err;
      if (!writeTiff(toFsS(path), fromQImageS(canvas), o, &err)) {
        if (msg)
          *msg = qs8(err);
        return false;
      }
    }
  }
  if (progress)
    progress(1.0);
  if (msg) {
    QStringList where;
    for (const ComboPlaced &pl : c.sections)
      where << (pl.slot == ComboSlot::Top      ? QStringLiteral("위")
                : pl.slot == ComboSlot::Bottom ? QStringLiteral("아래")
                : pl.slot == ComboSlot::Left   ? QStringLiteral("왼쪽")
                                               : QStringLiteral("오른쪽"));
    QStringList names;
    for (const ComboSecAsset &s : a.secs)
      names << s.nameA + QChar(0x2013) + s.nameB;
    QStringList pairs;
    for (int i = 0; i < names.size() && i < where.size(); ++i)
      pairs << names[i] + QStringLiteral(" ") + where[i];
    *msg = QStringLiteral("%1\n\n평·단면도 · %2 · 종이 %3 × %4 mm(내용 크기) · "
                          "단면 %5개(%6)")
               .arg(files.join(QStringLiteral(", ")), denomText(sp.denom))
               .arg(L.paperW, 0, 'f', 0)
               .arg(L.paperH, 0, 'f', 0)
               .arg(a.secs.size())
               .arg(pairs.join(QStringLiteral(" · ")));
    if (!a.skipped.isEmpty())
      *msg += QStringLiteral("\n뺀 단면: ") + a.skipped.join(QStringLiteral(" · "));
  }
  return true;
}

ComboSheetParams MainWindow::defaultComboSheetParams() const {
  ComboSheetParams sp;
  QSettings st;
  sp.denom =
      std::max(10.0, std::round(st.value("combosheet/denom", 30.0).toDouble()));
  sp.format = std::clamp(st.value("combosheet/format", 4).toInt(), 0, 4);
  sp.dpi = std::clamp(st.value("combosheet/dpi", 300.0).toDouble(), 72.0, 1200.0);
  sp.planMarginM =
      std::clamp(st.value("combosheet/planMargin", 1.0).toDouble(), 0.0, 20.0);
  sp.withPlanImage = st.value("combosheet/withPlanImage", true).toBool();
  sp.withSectionImage = st.value("combosheet/withSectionImage", true).toBool();
  sp.withLevels = st.value("combosheet/withLevels", true).toBool();
  if (section_) {
    const SectionDoc d = section_->doc(); // 화면의 선 굵기 · 레벨선 pt · 불투명도 그대로
    sp.lineWidthPx = d.st.lineWidthPx;
    sp.imageOpacity = d.st.imageOpacity;
    sp.levelMinorPt = clampLineWeightPt(d.st.levelMinorPt);
    sp.levelMajorPt = clampLineWeightPt(d.st.levelMajorPt);
    sp.depthFade = d.st.depthFade;
  }
  if (auto f = act_.find(QStringLiteral("smooth")); f != act_.end() && f->second)
    sp.smooth = f->second->isChecked();
  const SheetParams sec = defaultSheetParams();
  sp.srsLabel = sec.srsLabel;
  sp.date = sec.date;
  const QString model = modelDisplayName(path_);
  sp.title = model.isEmpty() ? QStringLiteral("평·단면도")
                             : model + QStringLiteral(" 평·단면도");
  return sp;
}

std::vector<ComboSecInput> MainWindow::comboInputs() {
  syncCurrentSection();
  std::vector<ComboSecInput> v;
  if (!src_)
    return v;
  const Vec3 o = srs().origin;
  for (const SavedSection &s : sections_) {
    ComboSecInput c;
    c.line.a = Vec2(s.ax - o.x, s.ay - o.y);
    c.line.b = Vec2(s.bx - o.x, s.by - o.y);
    c.line.front = s.front;
    c.line.back = s.back;
    if (SectionFrame(c.line).L < 0.01)
      continue;
    c.name = qs8(s.name);
    v.push_back(c);
  }
  return v;
}

void MainWindow::dlgComboSheet() {
  if (!src_) {
    showStatus(QStringLiteral("모델을 연 뒤에 도면을 만드세요"));
    return;
  }
  auto inputs = std::make_shared<std::vector<ComboSecInput>>(comboInputs());
  if (inputs->empty()) {
    showStatus(QStringLiteral(
        "단면이 없습니다. 평면에서 단면선을 그린 뒤 평·단면도를 여세요"));
    return;
  }
  ComboSheetParams sp0 = defaultComboSheetParams();
  if (lastCombo_) { // 다시 열 때 축척 · 형식 · 여유 · 넣을 것은 그대로
    sp0.denom = lastCombo_->denom;
    sp0.format = lastCombo_->format;
    sp0.planMarginM = lastCombo_->planMarginM;
    sp0.withPlanImage = lastCombo_->withPlanImage;
    sp0.withSectionImage = lastCombo_->withSectionImage;
    sp0.withLevels = lastCombo_->withLevels;
    sp0.title = lastCombo_->title;
  }
  auto st = std::make_shared<ComboSheetParams>(sp0);
  lastCombo_ = st;
  sheetIsPlan_ = false;
  sheetIsCombo_ = true;

  auto *d = new QDialog(this);
  d->setWindowTitle(QStringLiteral("평·단면도"));
  d->setObjectName("sheetDialog");
  auto assets = std::make_shared<ComboAssets>();
  auto busy = std::make_shared<bool>(true);
  auto failMsg = std::make_shared<QString>();
  auto *outer = new QHBoxLayout(d);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(0);
  auto *pv = new SheetPreview;
  pv->setMinimumSize(470, 330);
  auto *side = new QWidget;
  side->setObjectName("dialogSide");
  side->setAttribute(Qt::WA_StyledBackground);
  side->setMinimumWidth(200);
  side->setMaximumWidth(600);
  auto *sv = new QVBoxLayout(side);
  sv->setContentsMargins(0, 0, 0, 0);
  sv->setSpacing(0);
  auto *scroll = new QScrollArea;
  scroll->setObjectName("sideScroll");
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  auto *body = new QWidget;
  body->setObjectName("sideBody");
  body->setAttribute(Qt::WA_StyledBackground);
  auto *v = new QVBoxLayout(body);
  v->setContentsMargins(12, 10, 12, 10);
  v->setSpacing(6);
  v->addWidget(lbl(QStringLiteral("고른 도면 · 평·단면도 한 장"), "cap"));
  auto *nameLab = lbl(st->title, "sheetName");
  nameLab->setWordWrap(true);
  v->addWidget(nameLab);
  auto *subLab = lbl(QString(), "hint");
  subLab->setWordWrap(true);
  v->addWidget(subLab);
  v->addWidget(sideCaption(QStringLiteral("도면 정보")));
  auto *g = new QGridLayout;
  g->setContentsMargins(0, 0, 0, 0);
  g->setHorizontalSpacing(10);
  g->setVerticalSpacing(8);
  g->setColumnMinimumWidth(0, 40);
  g->setColumnStretch(1, 1);
  int r = 0;
  auto *name = new QLineEdit(st->title);
  g->addWidget(lbl(QStringLiteral("도면명"), "hint"), r, 0);
  g->addWidget(name, r++, 1);
  auto *scale = scaleSpin(st->denom);
  g->addWidget(lbl(QStringLiteral("축척"), "hint"), r, 0);
  g->addWidget(scaleWithArrows(scale), r++, 1);
  g->addWidget(scaleGridWidget(scale), r++, 1);
  auto *margin = new QDoubleSpinBox;
  margin->setRange(0.0, 20.0);
  margin->setDecimals(1);
  margin->setSingleStep(0.5);
  margin->setSuffix(QStringLiteral(" m"));
  margin->setValue(st->planMarginM);
  margin->setKeyboardTracking(false);
  margin->setToolTip(
      QStringLiteral("평면 칸: 단면선 끝에서 더 보일 거리(실제 m)"));
  g->addWidget(lbl(QStringLiteral("평면 여유"), "hint"), r, 0);
  g->addWidget(margin, r++, 1);
  v->addLayout(g);
  v->addWidget(sideCaption(QStringLiteral("넣을 것")));
  auto *cPlan = new QCheckBox(QStringLiteral("평면 정사영상"));
  auto *cImg = new QCheckBox(QStringLiteral("단면 입면 영상"));
  auto *cLev = new QCheckBox(QStringLiteral("레벨선(잘린 선 아래 땅속만)"));
  cPlan->setChecked(st->withPlanImage);
  cImg->setChecked(st->withSectionImage);
  cLev->setChecked(st->withLevels);
  for (auto *c : {cPlan, cImg, cLev}) {
    c->setFocusPolicy(Qt::NoFocus);
    v->addWidget(c);
  }
  v->addWidget(sideCaption(QStringLiteral("형식")));
  auto *fmtG = new QButtonGroup(d);
  fmtG->setExclusive(true);
  {
    auto *row = new QWidget;
    auto *h = new QHBoxLayout(row);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(2);
    h->addWidget(chipBtn(QStringLiteral("SVG"), fmtG, 4, "first", 48));
    h->addWidget(chipBtn(QStringLiteral("DXF"), fmtG, 1, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("PDF"), fmtG, 0, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("PNG"), fmtG, 2, nullptr, 48));
    h->addWidget(chipBtn(QStringLiteral("TIFF"), fmtG, 3, "last", 48));
    h->addStretch();
    v->addWidget(row);
  }
  v->addWidget(sideCaption(QStringLiteral("배치 · 저장을 막지 않음")));
  auto *check = new SheetCheckBox;
  v->addWidget(check);
  v->addStretch(1);
  scroll->setWidget(body);
  sv->addWidget(scroll, 1);
  auto *bDefault = new QPushButton(QStringLiteral("☆ 기본으로"));
  bDefault->setObjectName("quiet");
  bDefault->setFocusPolicy(Qt::NoFocus);
  bDefault->setToolTip(QStringLiteral(
      "이 설정(축척 · 평면 여유 · 넣을 것 · 형식)을 다음 평·단면도의 기본으로"));
  auto *bSave = new QPushButton;
  bSave->setObjectName("primary");
  bSave->setDefault(true);
  {
    QLabel *done = nullptr;
    sv->addWidget(sideBottom(bDefault, bSave, nullptr, &done));
    sheetDoneLabel_ = done;
  }
  auto *dlgSplit = new QSplitter(Qt::Horizontal, d);
  dlgSplit->setObjectName("sheetDialogSplitter");
  dlgSplit->setHandleWidth(4);
  dlgSplit->setChildrenCollapsible(false);
  dlgSplit->addWidget(pv);
  dlgSplit->addWidget(side);
  dlgSplit->setStretchFactor(0, 1);
  dlgSplit->setStretchFactor(1, 0);
  dlgSplit->setSizes({700, 320});
  outer->addWidget(dlgSplit, 1);

  auto refresh = std::make_shared<std::function<void()>>();
  *refresh = [=] {
    st->denom = std::max(10.0, std::round(st->denom));
    if (!scale->hasFocus() && int(std::lround(st->denom)) != scale->value()) {
      QSignalBlocker bl(scale);
      scale->setValue(int(std::lround(st->denom)));
    }
    if (auto *b = fmtG->button(st->format)) {
      QSignalBlocker bl(b);
      b->setChecked(true);
    }
    nameLab->setText(st->title);
    std::vector<CheckItem> items;
    if (assets->secs.empty()) {
      subLab->setText(
          QStringLiteral("%1 · 단면 %2개 계산 중…")
              .arg(denomText(st->denom))
              .arg(inputs->size()));
      if (!failMsg->isEmpty())
        items.push_back({false, *failMsg, QString(), nullptr});
    } else {
      const ComboLayout c = comboLayoutFor(*assets, *st);
      subLab->setText(QStringLiteral("%1 · 종이 %2 × %3 mm(내용 크기 — 용지 "
                                     "맞춤 없음) · 단면 %4개")
                          .arg(denomText(st->denom))
                          .arg(c.L.paperW, 0, 'f', 0)
                          .arg(c.L.paperH, 0, 'f', 0)
                          .arg(c.sections.size()));
      for (size_t i = 0; i < c.sections.size() && i < assets->secs.size(); ++i) {
        const ComboPlaced &pl = c.sections[i];
        const ComboSecAsset &as = assets->secs[i];
        const QString where =
            pl.slot == ComboSlot::Top
                ? QStringLiteral("평면 위(가로)")
                : pl.slot == ComboSlot::Bottom
                      ? QStringLiteral("평면 아래(가로)")
                      : pl.slot == ComboSlot::Left
                            ? QStringLiteral("평면 왼쪽(90° 돌림)")
                            : QStringLiteral("평면 오른쪽(90° 돌림)");
        items.push_back(
            {true,
             QStringLiteral("%1%2%3 → %4 · 길이 %5 m · 표고 %6–%7 m%8")
                 .arg(as.nameA, QChar(0x2013), as.nameB, where)
                 .arg(pl.lenM, 0, 'f', 2)
                 .arg(as.r.zMin + as.r.srs.origin.z, 0, 'f', 1)
                 .arg(as.r.zMax + as.r.srs.origin.z, 0, 'f', 1)
                 .arg(i == 0 ? QStringLiteral(" · 평면 회전 기준")
                             : QString()),
             QString(), nullptr});
      }
      items.push_back({true,
                       QStringLiteral("단면은 평면 단면선 가운데 화살표 쪽에 · "
                                      "평면 A · A′ 와 거리를 맞춤"),
                       QString(), nullptr});
      items.push_back({true,
                       QStringLiteral("표고 범위 = 잘린 선 위아래 10 cm 선(아래 "
                                      "세 칸 더) · 레벨선은 땅속만"),
                       QString(), nullptr});
      for (const QString &sk : assets->skipped)
        items.push_back(
            {false, QStringLiteral("뺀 단면 — %1").arg(sk), QString(), nullptr});
      if (!failMsg->isEmpty())
        items.push_back({false, *failMsg, QString(), nullptr});
    }
    check->setItems(items);
    const QString fmtName =
        st->format == 4
            ? QStringLiteral("SVG")
            : QString::fromLatin1(kFormatName[std::clamp(st->format, 0, 3)]);
    bSave->setText(QStringLiteral("1장 %1 저장").arg(fmtName));
    pv->update();
  };
  onSheetChecksChanged_ = [refresh] { (*refresh)(); };

  // 미리보기 영상(거친 해상도)은 작업 스레드에서: 단면(잘린 선 · 입면) → 돌린
  // 평면. 넣을 것 끄기 · 축척은 다시 그리기만, 평면 여유는 평면만 다시
  auto gen = std::make_shared<std::atomic<int>>(0);
  auto cancelRef = std::make_shared<std::shared_ptr<std::atomic<bool>>>();
  QObject::connect(d, &QObject::destroyed, [cancelRef] {
    if (*cancelRef)
      (*cancelRef)->store(true);
  });
  auto reload = std::make_shared<std::function<void(bool)>>();
  *reload = [this, st, inputs, assets, busy, failMsg, gen, cancelRef, refresh,
             pvp = QPointer<SheetPreview>(pv)](bool sectionsToo) {
    if (!src_)
      return;
    if (*cancelRef)
      (*cancelRef)->store(true);
    auto cancel = std::make_shared<std::atomic<bool>>(false);
    *cancelRef = cancel;
    const int my = ++*gen;
    *busy = true;
    ComboSheetParams snap = *st;
    snap.withSectionImage = true; // 미리보기는 늘 만들어 두고 칸으로 숨기기만
    snap.withPlanImage = true;
    const bool again = sectionsToo || assets->secs.empty();
    ComboAssets base = again ? ComboAssets() : *assets;
    std::thread([src = src_, snap, in = *inputs, base, again, cancel, gen, my,
                 assets, busy, failMsg, refresh, pvp]() mutable {
      ComboAssets a = base;
      QString m, mp;
      bool ok = true;
      if (again)
        ok = computeComboSections(*src, in, snap, true, a, &m, cancel.get());
      if (ok && !cancel->load()) {
        const ComboLayout c = comboLayoutFor(a, snap);
        const double ext = std::max(c.sMax - c.sMin, c.tMax - c.tMin);
        if (!renderComboPlan(*src, snap, std::max(0.002, ext / 2400.0), a, &mp,
                             cancel.get()))
          a.planMarginM = snap.planMarginM;
      }
      if (cancel->load())
        return;
      QMetaObject::invokeMethod(
          qApp,
          [a, ok, m, mp, gen, my, assets, busy, failMsg, refresh, pvp] {
            if (gen->load() != my || !pvp)
              return;
            if (ok)
              *assets = a;
            *failMsg = !ok ? m
                           : (mp.isEmpty() ? QString()
                                           : QStringLiteral("평면 영상: ") + mp);
            *busy = false;
            (*refresh)();
          },
          Qt::QueuedConnection);
    }).detach();
  };
  auto *marginTimer = new QTimer(d);
  marginTimer->setSingleShot(true);
  marginTimer->setInterval(300);
  QObject::connect(marginTimer, &QTimer::timeout, d,
                   [reload] { (*reload)(false); });

  pv->paintFn = [=](QPainter &p, const QRect &rc) {
    p.fillRect(rc, theme::Desk);
    if (assets->secs.empty()) {
      p.setPen(theme::Muted);
      p.setFont(theme::uiFont(13));
      p.drawText(rc, Qt::AlignCenter,
                 failMsg->isEmpty() ? QStringLiteral("단면 %1개 계산 중…")
                                          .arg(inputs->size())
                                    : *failMsg);
      return;
    }
    const ComboLayout c = comboLayoutFor(*assets, *st);
    double sc = std::min((rc.width() - 48.0) / c.L.paperW,
                         (rc.height() - 110.0) / c.L.paperH);
    if (sc <= 0.02)
      return;
    sc *= pv->pageZoom;
    const QSizeF pz(c.L.paperW * sc, c.L.paperH * sc);
    const QPointF o(rc.left() + (rc.width() - pz.width()) / 2 + pv->viewPan.x(),
                    rc.top() + (rc.height() - pz.height()) / 2 +
                        pv->viewPan.y());
    pv->lastPaperO = o;
    pv->lastPaperSize = QPointF(pz.width(), pz.height());
    pv->fitPxPerMm = sc;
    p.save();
    p.translate(o);
    p.setClipRect(QRectF(QPointF(0, 0), pz));
    paintComboSheet(p, *assets, *st, sc, 1.0); // 화면: 선은 1 px 아래로 가늘지 않게
    p.restore();
    paintPaperFrame(p, o, pz);
    if (*busy) {
      p.setPen(theme::Muted);
      p.setFont(theme::uiFont(12));
      p.drawText(QRectF(rc.left() + 12, rc.bottom() - 100, rc.width() - 24, 20),
                 Qt::AlignHCenter | Qt::AlignVCenter,
                 QStringLiteral("영상 준비 중…"));
    }
  };
  auto *editAct = new QAction(QStringLiteral("조판편집"), d);
  editAct->setCheckable(true);
  editAct->setEnabled(false); // 평·단면도는 배치가 정해져 있어 칸 안 이동 없음
  addDeskFloats(
      pv, editAct,
      [this] {
        showStatus(QStringLiteral(
            "평·단면도는 용지를 고르지 않습니다 — 종이 = 내용 크기"));
      },
      [this] {
        if (sheetSide_)
          sheetSide_->setVisible(!sheetSide_->isVisible());
      });
  QObject::connect(name, &QLineEdit::textChanged, d, [=](const QString &tx) {
    st->title = tx;
    nameLab->setText(tx);
    pv->update();
  });
  QObject::connect(scale, &QSpinBox::valueChanged, d, [=](int val) {
    st->denom = std::max(10, val);
    (*refresh)();
  });
  bindScaleTyping(scale, [=](int val) {
    st->denom = std::max(10, val);
    (*refresh)();
  });
  QObject::connect(margin, &QDoubleSpinBox::valueChanged, d, [=](double val) {
    st->planMarginM = std::clamp(val, 0.0, 20.0);
    (*refresh)();
    marginTimer->start();
  });
  QObject::connect(cPlan, &QCheckBox::toggled, d, [=](bool on) {
    st->withPlanImage = on;
    (*refresh)();
  });
  QObject::connect(cImg, &QCheckBox::toggled, d, [=](bool on) {
    st->withSectionImage = on;
    (*refresh)();
  });
  QObject::connect(cLev, &QCheckBox::toggled, d, [=](bool on) {
    st->withLevels = on;
    (*refresh)();
  });
  QObject::connect(fmtG, &QButtonGroup::idClicked, d, [=](int id) {
    st->format = id;
    (*refresh)();
  });
  auto saveDefaults = [st] {
    QSettings s;
    s.setValue("combosheet/denom", st->denom);
    s.setValue("combosheet/format", st->format);
    s.setValue("combosheet/planMargin", st->planMarginM);
    s.setValue("combosheet/withPlanImage", st->withPlanImage);
    s.setValue("combosheet/withSectionImage", st->withSectionImage);
    s.setValue("combosheet/withLevels", st->withLevels);
  };
  QObject::connect(bDefault, &QPushButton::clicked, d, [=] {
    saveDefaults();
    bDefault->setText(QStringLiteral("★ 저장함"));
  });
  QObject::connect(bSave, &QPushButton::clicked, d, [this] {
    saveEmbeddedSheet();
  });
  (*refresh)();
  (*reload)(true);
  d->resize(1000, 690);

  auto src = src_;
  showSheetTab(d, [this, st, inputs, src] {
    const ComboSheetParams sp = *st;
    const bool svg = sp.format == 4;
    const QString ext =
        svg ? QStringLiteral("svg")
            : QString::fromLatin1(kFormatExt[std::clamp(sp.format, 0, 3)]);
    const QString fname =
        svg ? QStringLiteral("SVG")
            : QString::fromLatin1(kFormatName[std::clamp(sp.format, 0, 3)]);
    QString f = askSavePath(
        "combosheet",
        modelDisplayName(path_) +
            QStringLiteral("_평단면도_1-%1.").arg(sp.denom, 0, 'f', 0) + ext,
        QStringLiteral("%1 (*.%2)").arg(fname, ext));
    if (f.isEmpty() || src_ != src)
      return;
    if (QFileInfo(f).suffix().isEmpty())
      f += "." + ext;
    const std::vector<ComboSecInput> in = *inputs;
    runTask(
        QStringLiteral("평·단면도 %1 만드는 중").arg(fname),
        [this, src, sp, in, f](QString *m) {
          return exportComboSheet(
              *src, in, sp, f, m, &cancelTask_, [this](double x) {
                QMetaObject::invokeMethod(
                    this, [this, x] { setProgress(x); }, Qt::QueuedConnection);
              });
        },
        [this](bool ok, const QString &m) { report(ok, m); });
  });
  refreshSheetList();
}
