#include "sectionview.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <cmath>
#include "theme.hpp"

using namespace asec;

SectionView::SectionView(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
    setMinimumSize(320, 240);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void SectionView::clear() { has_ = false; img_ = QImage(); update(); }

void SectionView::setResult(const SectionResult& r, const QImage& img, double s0, double z1, double res, bool keepView) {
    bool first = !has_;
    double oldL = has_ ? SectionFrame(r_.line).L : 0;
    r_ = r; img_ = img; imgS0_ = s0; imgZ1_ = z1; imgRes_ = res; has_ = true;
    double L = SectionFrame(r.line).L;
    if (first || !keepView || std::fabs(L - oldL) > 0.02 * std::max(L, oldL)) fit();
    update();
}

QRectF SectionView::plotRect(const QRectF& a, double ui) const {
    return a.adjusted(74 * ui, 46 * ui, -54 * ui, -44 * ui);
}

void SectionView::fit() {
    if (!has_) return;
    QRectF pr = plotRect(rect(), 1.0);
    double L = SectionFrame(r_.line).L, zr = std::max(0.2, r_.zMax - r_.zMin);
    xf_.ppm = std::max(1e-3, std::min(pr.width() / (L * 1.03), pr.height() / (zr * 1.06)));
    double oz = r_.srs.origin.z;
    xf_.s0 = L / 2 - pr.width() / 2 / xf_.ppm;
    xf_.zTop = (r_.zMin + r_.zMax) / 2 + oz + pr.height() / 2 / xf_.ppm;
    xf_.plot = pr;
}

void SectionView::resizeEvent(QResizeEvent*) {
    QRectF pr = plotRect(rect(), 1.0);
    if (has_ && xf_.plot.isValid()) {
        // 가운데 유지
        double sc = xf_.s0 + xf_.plot.width() / 2 / xf_.ppm, zc = xf_.zTop - xf_.plot.height() / 2 / xf_.ppm;
        xf_.plot = pr;
        xf_.s0 = sc - pr.width() / 2 / xf_.ppm;
        xf_.zTop = zc + pr.height() / 2 / xf_.ppm;
    }
    xf_.plot = pr;
}

static QString fmtDist(double s, double step) {
    int dec = step >= 1 ? 0 : step >= 0.1 ? 1 : 2;
    return QString::number(s, 'f', dec);
}

SectionDoc SectionView::doc() const {
    SectionDoc d; d.r = r_; d.st = st_; d.img = img_; d.imgS0 = imgS0_; d.imgZ1 = imgZ1_; d.imgRes = imgRes_;
    return d;
}

void paintSectionDoc(QPainter& p, const SectionDoc& d, const QRectF& area, const SectionXf& xf, double ui, const QImage& img, bool forExport,
                     const QString& footer, const SectionImgGeo* geo, bool busy_) {
    const SectionResult& r_ = d.r;
    const SectionStyle& st_ = d.st;
    const SectionImgGeo ig = geo ? *geo : SectionImgGeo{d.imgS0, d.imgZ1, d.imgRes};
    p.save();
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    p.fillRect(area, Qt::white);
    const QRectF pr = xf.plot;
    const double oz = r_.srs.origin.z;
    const SectionFrame f(r_.line);
    auto X = [&](double s) { return pr.left() + (s - xf.s0) * xf.ppm; };
    auto Y = [&](double zAbs) { return pr.top() + (xf.zTop - zAbs) * xf.ppm; };
    const double sVis0 = xf.s0, sVis1 = xf.s0 + pr.width() / xf.ppm;
    const double zVis1 = xf.zTop, zVis0 = xf.zTop - pr.height() / xf.ppm;
    QFont small(theme::fontFamily()); small.setPixelSize(std::max(8, int(std::lround(11.5 * ui))));
    QFont bold = small; bold.setBold(true);
    QFont title(theme::fontFamily()); title.setPixelSize(int(std::lround(14 * ui))); title.setBold(true);
    QFontMetricsF fm(small);

    // 1) 세로 격자(거리) — 아주 옅게
    double dStep = niceStep(std::max(1e-3, sVis1 - sVis0), std::max(2, int(pr.width() / (90 * ui))));
    p.save();
    p.setClipRect(pr);
    {
        QPen g(QColor(0xEE, 0xED, 0xE9), 1.0 * ui);
        p.setPen(g);
        for (long k = long(std::ceil(sVis0 / dStep)); k * dStep <= sVis1; ++k) p.drawLine(QPointF(X(k * dStep), pr.top()), QPointF(X(k * dStep), pr.bottom()));
    }
    // 2) 레벨선(맨 아래 층 — 영상·단면선 밑): 10 cm 얇고 옅게, 50 cm·1 m 조금 진하게. 너무 촘촘하면 생략
    std::vector<LevelLine> levels;
    if (st_.showLevels) {
        levels = levelLines(zVis0, zVis1, 10);
        double gap10 = 0.1 * xf.ppm;
        for (auto& lv : levels) {
            double y = Y(lv.z);
            QColor c; double w;
            if (lv.cls == LevelClass::Master) { c = QColor(60, 60, 58, 120); w = 1.1; }
            else if (lv.cls == LevelClass::Major) { c = QColor(60, 60, 58, 80); w = 0.9; if (gap10 * 5 < 5 * ui) continue; }
            else { c = QColor(60, 60, 58, 42); w = 0.6; if (gap10 < 5 * ui) continue; }
            p.setPen(QPen(c, w * ui));
            p.drawLine(QPointF(pr.left(), y), QPointF(pr.right(), y));
        }
    }
    // 3) 입면 영상(레벨선 위)
    if (st_.showImage && !img.isNull()) {
        QRectF tr(X(ig.s0), Y(ig.z1Local + oz), img.width() * ig.res * xf.ppm, img.height() * ig.res * xf.ppm);
        p.setOpacity(st_.imageOpacity);
        p.drawImage(tr, img);
        p.setOpacity(1.0);
    }
    // 4) 단면 한계(A, A′) 세로 점선
    {
        QPen lim(QColor(255, 0, 0, 90), 1.0 * ui, Qt::DashLine);
        p.setPen(lim);
        p.drawLine(QPointF(X(0), pr.top()), QPointF(X(0), pr.bottom()));
        p.drawLine(QPointF(X(f.L), pr.top()), QPointF(X(f.L), pr.bottom()));
    }
    // 5) 단면선: 순수 빨강 약 2 px, 안티에일리어싱, 둥근 이음. 닫힌 고리(나무·돌 덩어리)는 닫아서 그림
    if (st_.showLine) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(theme::ProfileRed, 2.0 * ui, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        for (auto& pl : r_.profile) {
            if (pl.size() < 2) continue;
            const bool closed = pl.size() > 3 && (pl.front() - pl.back()).len() < 1e-9;
            QPainterPath path;
            path.moveTo(X(pl[0].x), Y(pl[0].y + oz));
            const size_t n = closed ? pl.size() - 1 : pl.size();
            for (size_t i = 1; i < n; ++i) path.lineTo(X(pl[i].x), Y(pl[i].y + oz));
            if (closed) path.closeSubpath();
            p.drawPath(path);
        }
    }
    p.restore();  // clip

    // 6) 테두리
    p.setPen(QPen(QColor(0xC9, 0xC8, 0xC3), 1.0 * ui));
    p.setBrush(Qt::NoBrush);
    p.drawRect(pr);

    // 7) 표고 라벨(왼쪽·오른쪽), 겹치지 않는 간격
    if (st_.showLevels) {
        int lab = labelStepCm(xf.ppm, fm.height() * 1.35);
        for (auto& lv : levels) {
            if (lv.cm % lab != 0) continue;
            double y = Y(lv.z);
            if (y < pr.top() + fm.height() * 0.3 || y > pr.bottom() - fm.height() * 0.3) continue;
            p.setFont(lv.cls == LevelClass::Master ? bold : small);
            p.setPen(lv.cls == LevelClass::Minor ? theme::InkSub : theme::Ink);
            QString t = QString::fromStdString(formatElevation(lv.z));
            p.drawText(QRectF(area.left(), y - fm.height() / 2, pr.left() - area.left() - 8 * ui, fm.height()), Qt::AlignRight | Qt::AlignVCenter, t);
            p.setPen(QPen(theme::Outline, 1.0 * ui));
            p.drawLine(QPointF(pr.left() - 5 * ui, y), QPointF(pr.left(), y));
            p.drawLine(QPointF(pr.right(), y), QPointF(pr.right() + 5 * ui, y));
            p.setPen(lv.cls == LevelClass::Minor ? theme::InkSub : theme::Ink);
            p.drawText(QRectF(pr.right() + 8 * ui, y - fm.height() / 2, 60 * ui, fm.height()), Qt::AlignLeft | Qt::AlignVCenter, t);
        }
        p.setFont(small); p.setPen(theme::Idle);
        p.drawText(QRectF(area.left() + 4 * ui, pr.top() - fm.height() - 4 * ui, pr.left() - area.left(), fm.height()), Qt::AlignLeft, QStringLiteral("표고(m)"));
    }
    // 8) 거리축
    p.setFont(small);
    for (long k = long(std::ceil(sVis0 / dStep)); k * dStep <= sVis1 + 1e-9; ++k) {
        double s = k * dStep, x = X(s);
        if (x < pr.left() - 0.5 || x > pr.right() + 0.5) continue;
        p.setPen(QPen(theme::Outline, 1.0 * ui));
        p.drawLine(QPointF(x, pr.bottom()), QPointF(x, pr.bottom() + 5 * ui));
        p.setPen(theme::InkSub);
        p.drawText(QRectF(x - 40 * ui, pr.bottom() + 6 * ui, 80 * ui, fm.height()), Qt::AlignHCenter | Qt::AlignTop, fmtDist(s, dStep));
    }
    p.setPen(theme::Idle);
    p.drawText(QRectF(pr.right() - 160 * ui, pr.bottom() + 6 * ui + fm.height(), 160 * ui, fm.height()), Qt::AlignRight, QStringLiteral("A 로부터 거리(m)"));
    // 9) A / A′ 표시
    p.setFont(title);
    for (int k = 0; k < 2; ++k) {
        double x = X(k ? f.L : 0);
        if (x < pr.left() - 20 || x > pr.right() + 20) continue;
        QRectF tr(x - 18 * ui, pr.top() - 24 * ui, 36 * ui, 20 * ui);
        p.setPen(theme::SectionRed);
        p.drawText(tr, Qt::AlignCenter, k ? QStringLiteral("A′") : QStringLiteral("A"));
    }
    // 10) 축척 막대(그림 안 오른쪽 아래)
    {
        double len = niceStep(110 * ui / xf.ppm, 1);
        double px = len * xf.ppm;
        QRectF sb(pr.right() - px - 14 * ui, pr.bottom() - 22 * ui, px, 5 * ui);
        p.setPen(Qt::NoPen); p.setBrush(QColor(255, 255, 255, 220));
        p.drawRoundedRect(sb.adjusted(-8 * ui, -16 * ui, 8 * ui, 8 * ui), 4 * ui, 4 * ui);
        p.setPen(QPen(theme::Ink, 1.0 * ui)); p.setBrush(Qt::white); p.drawRect(sb);
        p.setBrush(theme::Ink);
        for (int i = 0; i < 4; i += 2) p.drawRect(QRectF(sb.x() + px * i / 4, sb.y(), px / 4, sb.height()));
        p.setFont(small); p.setPen(theme::Ink);
        QString lab = len >= 1 ? QString::number(len, 'g', 4) + " m" : QString::number(len * 100, 'g', 4) + " cm";
        p.drawText(QRectF(sb.left(), sb.top() - 15 * ui, px, 13 * ui), Qt::AlignHCenter | Qt::AlignBottom, lab);
    }
    // 11) 제목
    p.setFont(title); p.setPen(theme::Ink);
    QString t1 = QStringLiteral("단면 A–A′");
    p.drawText(QRectF(area.left() + 12 * ui, area.top() + 8 * ui, 200 * ui, 20 * ui), Qt::AlignLeft | Qt::AlignVCenter, t1);
    p.setFont(small); p.setPen(theme::InkSub);
    QString t2 = QStringLiteral("길이 %1 m  ·  두께 앞 %2 / 뒤 %3 m  ·  %4  ·  가로:세로 1:1")
                     .arg(f.L, 0, 'f', 2).arg(r_.line.front, 0, 'f', 2).arg(r_.line.back, 0, 'f', 2)
                     .arg(r_.srs.srs.empty() ? QStringLiteral("좌표계 미상") : QString::fromStdString(r_.srs.srs));
    p.drawText(QRectF(area.left() + 12 * ui + QFontMetricsF(title).horizontalAdvance(t1) + 12 * ui, area.top() + 8 * ui, area.width(), 20 * ui), Qt::AlignLeft | Qt::AlignVCenter, t2);
    if (!footer.isEmpty()) {
        p.setPen(theme::Idle);
        p.drawText(QRectF(area.left() + 12 * ui, area.bottom() - fm.height() - 4 * ui, area.width() - 24 * ui, fm.height()), Qt::AlignRight, footer);
    }
    if (!forExport && busy_) {
        p.setFont(small);
        QRectF b(pr.right() - 110 * ui, pr.top() + 8 * ui, 100 * ui, 22 * ui);
        p.setPen(Qt::NoPen); p.setBrush(QColor(17, 17, 17, 200)); p.drawRoundedRect(b, 11 * ui, 11 * ui);
        p.setPen(Qt::white); p.drawText(b, Qt::AlignCenter, QStringLiteral("계산 중…"));
    }
    p.restore();
}

QSize sectionExportLayout(const SectionDoc& d, double ppm, double ui, SectionXf& xf) {
    const SectionResult& r_ = d.r;
    double L = SectionFrame(r_.line).L, zr = r_.zMax - r_.zMin;
    // 영상 픽셀이 출력 픽셀과 1:1 로 겹치도록 여백·원점을 정수 픽셀에 맞춤(재표본 흐림 없음)
    double padS = std::round(std::max(0.02 * L, 12 * ui / ppm) * ppm) / ppm;
    double w = std::ceil((L + 2 * padS) * ppm), h = std::ceil(zr * ppm);
    double left = std::round(74 * ui), top = std::round(46 * ui);
    QSize sz(int(left + w + std::ceil(54 * ui)), int(top + h + std::ceil((44 + 18) * ui)));
    xf.ppm = ppm;
    xf.plot = QRectF(left, top, w, h);
    xf.s0 = -padS;
    xf.zTop = r_.zMax + r_.srs.origin.z;
    return sz;
}

void SectionView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    if (!has_) {
        p.fillRect(rect(), Qt::white);
        p.setRenderHint(QPainter::Antialiasing);
        // 빈 격자 + 안내
        p.setPen(QPen(QColor(0xEE, 0xED, 0xE9), 1));
        for (int x = 0; x < width(); x += 24) p.drawLine(x, 0, x, height());
        for (int y = 0; y < height(); y += 24) p.drawLine(0, y, width(), y);
        QRectF r(0, 0, std::min(400, width() - 40), 110);
        r.moveCenter(QPointF(width() / 2.0, height() / 2.0));
        p.setPen(QPen(theme::Line, 1)); p.setBrush(theme::Card); p.drawRoundedRect(r, 14, 14);
        QFont f(theme::fontFamily()); f.setPointSizeF(11.5); f.setBold(true); p.setFont(f); p.setPen(theme::Ink);
        p.drawText(r.adjusted(16, 18, -16, -50), Qt::AlignHCenter | Qt::AlignTop, QStringLiteral("단면선을 그리세요"));
        f.setPointSizeF(9.5); f.setBold(false); p.setFont(f); p.setPen(theme::InkSub);
        p.drawText(r.adjusted(16, 52, -16, -8), Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                   QStringLiteral("「단면선 그리기」(S) 후 평면에서 A, A′ 두 점을 클릭\n끝점을 끌면 단면이 바로 바뀝니다"));
        return;
    }
    SectionDoc d; d.r = r_; d.st = st_; d.imgS0 = imgS0_; d.imgZ1 = imgZ1_; d.imgRes = imgRes_;
    paintSectionDoc(p, d, rect(), xf_, 1.0, img_, false, QString(), nullptr, busy_);
}

void SectionView::wheelEvent(QWheelEvent* e) {
    if (!has_) return;
    QPointF m = e->position();
    double s = xf_.s0 + (m.x() - xf_.plot.left()) / xf_.ppm, z = xf_.zTop - (m.y() - xf_.plot.top()) / xf_.ppm;
    xf_.ppm = std::clamp(xf_.ppm * std::pow(1.18, e->angleDelta().y() / 120.0), 0.5, 50000.0);
    xf_.s0 = s - (m.x() - xf_.plot.left()) / xf_.ppm;
    xf_.zTop = z + (m.y() - xf_.plot.top()) / xf_.ppm;
    update();
}

void SectionView::mousePressEvent(QMouseEvent* e) { panning_ = true; last_ = e->pos(); setCursor(Qt::ClosedHandCursor); }
void SectionView::mouseReleaseEvent(QMouseEvent*) { panning_ = false; setCursor(Qt::ArrowCursor); }
void SectionView::mouseDoubleClickEvent(QMouseEvent*) { fit(); update(); }
void SectionView::leaveEvent(QEvent*) { if (onCursor) onCursor(0, 0, 0, 0, false); }

void SectionView::mouseMoveEvent(QMouseEvent* e) {
    if (panning_) {
        QPoint d = e->pos() - last_; last_ = e->pos();
        xf_.s0 -= d.x() / xf_.ppm; xf_.zTop += d.y() / xf_.ppm;
        update();
    }
    if (has_ && onCursor) {
        double s = xf_.s0 + (e->position().x() - xf_.plot.left()) / xf_.ppm, z = xf_.zTop - (e->position().y() - xf_.plot.top()) / xf_.ppm;
        Vec3 w = sectionToWorld(r_, s, z - r_.srs.origin.z);
        onCursor(s, z, w.x, w.y, xf_.plot.contains(e->position()));
    }
}

void SectionView::zoomBy(double f) {
    if (!has_) return;
    QPointF m = xf_.plot.center();
    double s = xf_.s0 + (m.x() - xf_.plot.left()) / xf_.ppm, z = xf_.zTop - (m.y() - xf_.plot.top()) / xf_.ppm;
    xf_.ppm = std::clamp(xf_.ppm * f, 0.5, 50000.0);
    xf_.s0 = s - (m.x() - xf_.plot.left()) / xf_.ppm;
    xf_.zTop = z + (m.y() - xf_.plot.top()) / xf_.ppm;
    update();
}
