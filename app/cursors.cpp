#include "cursors.hpp"

#include <QFontMetricsF>
#include <QHash>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QStringList>
#include <algorithm>
#include <cmath>

#include "theme.hpp"

namespace kerf {
namespace {
constexpr double kC = kCursorHot;                       // 십자 가운데(논리 px)
constexpr double kGap = 5.0, kArm = 14.0;               // 십자 안쪽 틈 · 팔 끝(가운데에서)

QPixmap bake(ToolCursor kind, qreal dpr, bool shift, bool hc) {
    const int px = int(std::lround(kCursorLogical * dpr));
    QPixmap pm(px, px);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const bool bold = kind == ToolCursor::DrawB && shift;                                     // Shift = 축 맞춤: 십자가 축 색(Ink2)으로 굵어짐
    const double lineW = (hc ? 2.0 : 1.5) + (bold ? 1.0 : 0.0), haloW = hc ? 1.5 : 1.0;      // 고대비: 선 2 · 흰 테 1.5
    const QColor ink = bold ? theme::Ink2 : theme::Ink;
    // 십자(안쪽 5 px 는 비움 — 찍을 점이 안 가려진다). 흰 테를 먼저, 그 위에 Ink
    QPainterPath cross;
    cross.moveTo(kC - kArm, kC); cross.lineTo(kC - kGap, kC); cross.moveTo(kC + kGap, kC); cross.lineTo(kC + kArm, kC);
    cross.moveTo(kC, kC - kArm); cross.lineTo(kC, kC - kGap); cross.moveTo(kC, kC + kGap); cross.lineTo(kC, kC + kArm);
    p.setPen(QPen(theme::Ground, lineW + 2 * haloW, Qt::SolidLine, Qt::SquareCap)); p.drawPath(cross);
    p.setPen(QPen(ink, lineW, Qt::SolidLine, Qt::FlatCap)); p.drawPath(cross);
    if (kind == ToolCursor::Handle) {   // 빨간 고리 = 이 점을 끌 수 있음. 고리 안쪽(반지름 5 이하)은 비어 있다
        const double rr = 7.0;
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(theme::Ground, lineW + 2 * haloW)); p.drawEllipse(QPointF(kC, kC), rr, rr);
        p.setPen(QPen(theme::SectionRed, lineW)); p.drawEllipse(QPointF(kC, kC), rr, rr);
    } else {
        // 배지: 오른쪽 아래, 핫스폿에서 2 px — 흰 알약 14×12 · Ink 테 1
        const QRectF badgeOuter(kC + 2, kC + 2, 14, 12), badge = badgeOuter.adjusted(0.5, 0.5, -0.5, -0.5);   // 테 1 px 가 칸 안에 들어오게 안쪽으로 반 화소(바깥 크기 14×12)
        QPainterPath pill; pill.addRoundedRect(badge, 6, 6);
        p.setPen(QPen(ink, 1.0)); p.setBrush(theme::Card); p.drawPath(pill);
        if (kind == ToolCursor::DrawA || kind == ToolCursor::DrawB) {
            QFont f = theme::uiFont(9, true); f.setPixelSize(9);
            const QString t = kind == ToolCursor::DrawA ? QStringLiteral("A") : QStringLiteral("A′");
            while (f.pixelSize() > 6 && QFontMetricsF(f).horizontalAdvance(t) > badge.width() - 2) f.setPixelSize(f.pixelSize() - 1);
            p.setFont(f); p.setPen(theme::SectionRed);   // 글자는 평면 위 A · A′ 이름표와 같은 빨강
            p.drawText(badge, Qt::AlignCenter, t);
        } else if (kind == ToolCursor::Hatch) {
            p.save(); p.setClipPath(pill); p.setPen(QPen(ink, 1.0, Qt::SolidLine, Qt::FlatCap));
            for (double d = -8; d <= 16; d += 4) p.drawLine(QPointF(badge.left() + d, badge.bottom()), QPointF(badge.left() + d + 12, badge.top()));
            p.restore();
        } else {   // Outline: 점 — Shift(가장자리 붙음)이면 고리
            const QPointF c = badge.center();
            if (shift) { p.setBrush(Qt::NoBrush); p.setPen(QPen(ink, 1.3)); p.drawEllipse(c, 3.0, 3.0); }
            else { p.setBrush(ink); p.setPen(Qt::NoPen); p.drawEllipse(c, 2.2, 2.2); }
        }
    }
    p.end();
    return pm;
}
}  // namespace

QPixmap toolCursorPixmap(ToolCursor kind, qreal dpr, bool shift, bool highContrast) { return bake(kind, dpr, shift, highContrast); }

QCursor toolCursor(ToolCursor kind, qreal dpr, bool shift, bool highContrast) {
    static QHash<quint64, QCursor> cache;
    const quint64 key = (quint64(kind) << 32) | (quint64(shift ? 1 : 0) << 31) | (quint64(highContrast ? 1 : 0) << 30) | quint64(std::lround(dpr * 100));
    auto it = cache.find(key);
    if (it != cache.end()) return it.value();
    return *cache.insert(key, QCursor(toolCursorPixmap(kind, dpr, shift, highContrast), kCursorHot, kCursorHot));
}

bool writeCursorSheet(const QString& pngPath, QString* report) {
    struct Row { ToolCursor k; bool shift; const char* name; };
    const Row rows[] = {{ToolCursor::DrawA, false, "draw-A"}, {ToolCursor::DrawB, false, "draw-A'"}, {ToolCursor::DrawB, true, "draw-A'+shift"},
                        {ToolCursor::Hatch, false, "hatch"}, {ToolCursor::Outline, false, "outline"}, {ToolCursor::Outline, true, "outline+shift"}, {ToolCursor::Handle, false, "handle"}};
    const qreal dprs[] = {1.0, 2.0, 3.0};
    int bad = 0, n = 0;
    QStringList lines;
    const int cell = kCursorLogical * 3 + 16;   // 가장 큰 배율 칸
    QImage sheet(cell * 3 * 2, cell * int(std::size(rows)), QImage::Format_ARGB32_Premultiplied);
    sheet.fill(Qt::white);
    QPainter sp(&sheet);
    for (int r = 0; r < int(std::size(rows)); ++r)
        for (int c = 0; c < 3; ++c) {
            const QPixmap pm = toolCursorPixmap(rows[r].k, dprs[c], rows[r].shift, false);
            ++n;
            QString why;
            const int want = int(std::lround(kCursorLogical * dprs[c]));
            if (pm.isNull()) why = QStringLiteral("null");
            else if (pm.width() != want || pm.height() != want) why = QStringLiteral("size %1x%2 want %3").arg(pm.width()).arg(pm.height()).arg(want);
            else if (!qFuzzyCompare(pm.devicePixelRatio(), dprs[c])) why = QStringLiteral("dpr");
            else {
                const QImage im = pm.toImage().convertToFormat(QImage::Format_ARGB32);
                const double k = dprs[c];
                auto alphaAt = [&](double lx, double ly) { return qAlpha(im.pixel(std::clamp(int(lx * k), 0, im.width() - 1), std::clamp(int(ly * k), 0, im.height() - 1))); };
                if (alphaAt(kCursorHot, kCursorHot) != 0 || alphaAt(kCursorHot + 1, kCursorHot) != 0 || alphaAt(kCursorHot, kCursorHot - 1) != 0 || alphaAt(kCursorHot - 1, kCursorHot) != 0 || alphaAt(kCursorHot, kCursorHot + 1) != 0) why = QStringLiteral("center-covered");
                else if (alphaAt(kCursorHot, 6) == 0 || alphaAt(6, kCursorHot) == 0) why = QStringLiteral("no-cross");
                else {
                    int red = 0;   // 배지(오른쪽 아래) 또는 손잡이 고리의 빨강
                    for (int y = 0; y < im.height(); ++y) for (int x = 0; x < im.width(); ++x) { const QRgb p = im.pixel(x, y); if (qAlpha(p) > 128 && qRed(p) > 200 && qGreen(p) < 80 && qBlue(p) < 80) ++red; }
                    const bool wantRed = rows[r].k == ToolCursor::DrawA || rows[r].k == ToolCursor::DrawB || rows[r].k == ToolCursor::Handle;
                    if (wantRed && red == 0) why = QStringLiteral("no-red");
                }
            }
            if (!why.isEmpty()) { ++bad; lines << QStringLiteral("cursor-sheet FAIL %1 dpr=%2: %3").arg(QString::fromLatin1(rows[r].name)).arg(dprs[c]).arg(why); }
            if (!pm.isNull()) {
                const QRectF dst(c * cell * 2 + 8, r * cell + 8, pm.width(), pm.height());
                sp.fillRect(QRectF(c * cell * 2, r * cell, cell, cell), QColor(0xF5, 0xF4, 0xED));           // 밝은 바탕(흙 위 흰 테)
                sp.fillRect(QRectF(c * cell * 2 + cell, r * cell, cell, cell), QColor(0x6B, 0x4A, 0x32));      // 어두운 흙색 바탕(Ink 만으로는 안 보이는 자리)
                QPixmap phys = pm; phys.setDevicePixelRatio(1.0);   // 물리 화소 그대로(2 · 3 배율의 선이 어떻게 구워졌는지 보이게)
                sp.drawPixmap(QPointF(dst.x(), dst.y()), phys);
                sp.drawPixmap(QPointF(dst.x() + cell, dst.y()), phys);
            }
        }
    sp.end();
    const bool saved = sheet.save(pngPath);
    lines.prepend(QStringLiteral("cursor-sheet n=%1 bad=%2 saved=%3 hotspot=%4,%5%6").arg(n).arg(bad).arg(saved ? 1 : 0).arg(kCursorHot).arg(kCursorHot).arg(bad || !saved ? QStringLiteral("  RESULT fail") : QStringLiteral("  RESULT ok")));
    if (report) *report = lines.join(QLatin1Char('\n'));
    return bad == 0 && saved;
}
}  // namespace kerf
