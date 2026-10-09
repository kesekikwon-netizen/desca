// 아이콘 그리기: generated/kerf_icons.hpp 의 SVG 글자를 QSvgRenderer 로 그린다(디자인 v5 §11 · Strata KaIconsMockupEngine 과 같은 방식).
// 선 아이콘은 currentColor 만 바꿔 그리고, 리본 칩은 둥근 타일(한 변의 1/4 둥글기) 위에 아이콘을 굽는다.
// 배율: dpr 1 · 1.5 · 2 · 3 네 장을 QIcon 에 넣어 100–300 % 화면 어디서나 흐리지 않게(스펙 §2.1).
#include "icons.hpp"
#include "kerf_icons.hpp"

#include <QDebug>
#include <QPainter>
#include <QPolygonF>
#include <QPainterPath>
#include <QPen>
#include <QSet>
#include <QStringList>
#include <QSvgRenderer>
#include <cmath>

namespace kerf {
namespace {
constexpr qreal kDprs[] = {1.0, 1.5, 2.0, 3.0};
constexpr qreal kTileRadius = 0.25;     // 타일 한 변의 1/4 — 50 px 타일에서 12.5(Strata 와 같음)
constexpr qreal kDisabledGlyph = 0.45;  // 꺼진 칩의 아이콘 불투명도(tokens.json glyphDisabledOpacity)

const kerficons::Svg* find(const QString& name) {
    const QByteArray n = name.toLatin1();
    for (int i = 0; i < kerficons::kCount; ++i)
        if (n == kerficons::kAll[i].name) return &kerficons::kAll[i];
    return nullptr;
}

QByteArray svgWithInk(const kerficons::Svg& s, const QColor& ink) {
    QByteArray b(s.svg);
    b.replace("currentColor", ink.name(QColor::HexRgb).toLatin1());
    return b;
}

QPixmap blank(int px, qreal dpr) {
    QPixmap pm(int(std::lround(px * dpr)), int(std::lround(px * dpr)));
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    return pm;
}

// 선 아이콘 한 장(px 논리 크기, dpr 배율). 흐린 아이콘은 한 층으로 그린 뒤 불투명도로 얹는다 — 겹치는 획이 서로 어두워지지 않게
QPixmap renderGlyph(const kerficons::Svg* s, int px, const QColor& ink, qreal dpr, qreal opacity) {
    QPixmap pm = blank(px, dpr);
    if (!s) return pm;
    QSvgRenderer r(svgWithInk(*s, ink));
    if (!r.isValid()) return pm;
    if (opacity >= 1.0) {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        r.render(&p, QRectF(0, 0, px, px));
        return pm;
    }
    QPixmap layer = blank(px, dpr);
    {
        QPainter lp(&layer);
        lp.setRenderHint(QPainter::Antialiasing);
        r.render(&lp, QRectF(0, 0, px, px));
    }
    QPainter p(&pm);
    p.setOpacity(opacity);
    p.drawPixmap(0, 0, layer);
    return pm;
}

struct Look { QColor tile, border, glyph; qreal glyphOpacity = 1.0; };

// 스펙 §3.2 표: 보통 · 마우스 위 · 꺼짐 · 도구 켜짐 · 보기 켜짐 · 주 단추
Look lookFor(ChipKind kind, QIcon::Mode mode, QIcon::State state, const ChipColors& c) {
    Look L{c.tile, QColor(), c.glyph, 1.0};
    if (kind == ChipKind::Primary) {
        L.tile = mode == QIcon::Active ? c.tilePrimaryHover : c.tilePrimary;
        L.glyph = c.glyphPrimary;
    } else if (state == QIcon::On) {
        if (kind == ChipKind::Tool) { L.tile = c.tileOn; L.border = c.borderOn; L.glyph = c.glyphOn; }
        else { L.tile = c.tileShown; L.border = c.borderShown; L.glyph = c.glyph; }
    } else if (mode == QIcon::Active) {
        L.tile = c.tileHover;
    }
    if (mode == QIcon::Disabled) { L.tile = c.tileDisabled; L.border = QColor(); L.glyph = c.glyph; L.glyphOpacity = kDisabledGlyph; }
    return L;
}

QPixmap renderChip(const kerficons::Svg* s, int tile, int glyphPx, const Look& L, qreal dpr, bool caret) {
    QPixmap pm = blank(tile, dpr);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    const qreal r = tile * kTileRadius;
    p.setPen(Qt::NoPen);
    p.setBrush(L.tile);
    p.drawRoundedRect(QRectF(0, 0, tile, tile), r, r);
    if (L.border.isValid()) {   // 테는 안쪽에: 반 픽셀 들여 그려야 타일 밖으로 번지지 않는다
        p.setPen(QPen(L.border, 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(0.5, 0.5, tile - 1, tile - 1), r - 0.5, r - 0.5);
    }
    const qreal off = (tile - glyphPx) / 2.0;
    p.drawPixmap(QPointF(off, off), renderGlyph(s, glyphPx, L.glyph, dpr, L.glyphOpacity));
    if (caret) {   // ▾ 6 px, 타일 오른쪽 아래(스펙 §3.1)
        QColor ink = L.glyph; ink.setAlphaF(L.glyphOpacity);
        p.setPen(Qt::NoPen); p.setBrush(ink);
        const qreal x = tile - 11, y = tile - 10;
        QPolygonF tri; tri << QPointF(x, y) << QPointF(x + 6, y) << QPointF(x + 3, y + 4);
        p.drawPolygon(tri);
    }
    return pm;
}
}  // namespace

bool hasIcon(const QString& name) { return find(name) != nullptr; }

QStringList iconNames() {
    QStringList out;
    for (int i = 0; i < kerficons::kCount; ++i) out << QString::fromLatin1(kerficons::kAll[i].name);
    return out;
}

QPixmap glyph(const QString& name, int px, const QColor& ink, qreal dpr) { return renderGlyph(find(name), px, ink, dpr, 1.0); }

QIcon icon(const QString& name, int px, const QColor& ink) {
    const kerficons::Svg* s = find(name);
    if (!s) {
        static QSet<QString> warned;
        if (!warned.contains(name)) { warned.insert(name); qWarning().noquote() << QStringLiteral("[icon] 없음: %1").arg(name); }
        return {};
    }
    QIcon ic;
    for (qreal d : kDprs) ic.addPixmap(renderGlyph(s, px, ink, d, 1.0));
    return ic;
}

QIcon chipIcon(const QString& name, int tile, int glyphPx, ChipKind kind, const ChipColors& c, bool caret) {
    const kerficons::Svg* s = find(name);
    QIcon ic;
    for (auto mode : {QIcon::Normal, QIcon::Active, QIcon::Disabled, QIcon::Selected})
        for (auto state : {QIcon::Off, QIcon::On}) {
            const Look L = lookFor(kind, mode == QIcon::Selected ? QIcon::Normal : mode, state, c);
            for (qreal d : kDprs) ic.addPixmap(renderChip(s, tile, glyphPx, L, d, caret), mode, state);
        }
    return ic;
}
}  // namespace kerf
