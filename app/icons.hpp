// 아이콘: SVG 글자(generated/kerf_icons.hpp, app/icons/)를 QSvgRenderer 로 그린다. 디자인 v5 §11 · Strata KaIcons 와 같은 규격(24 격자 · 선 2).
//  icon()     = 선 아이콘만(탭 · 메뉴 · 떠 있는 단추 · 화면 머리). currentColor → ink.
//  chipIcon() = 리본 칩: 둥근 타일(desk)에 아이콘을 굽는다. 상태(Normal · Active · Disabled × Off · On)별 색은 §3.2 표.
// 색은 app/theme.hpp 토큰만 쓴다(이 파일은 theme.hpp 를 포함하지 않으므로 호출하는 쪽이 넘긴다).
#pragma once
#include <QColor>
#include <QIcon>
#include <QList>
#include <QPixmap>
#include <QString>

namespace kerf {
enum class ChipKind { Normal, Tool, Shown, Primary };

/// 칩 타일 색 묶음(theme 토큰을 호출하는 쪽이 채운다 — mainwindow_ui.cpp 의 chipLook())
struct ChipColors {
    QColor tile, tileHover, tilePressed, tileDisabled, glyph, glyphOn, tileOn, borderOn, tileShown, borderShown, tilePrimary, tilePrimaryHover, glyphPrimary;
};

/// 이 이름의 SVG 가 묶음에 있는가(감사용)
[[nodiscard]] bool hasIcon(const QString& name);
/// 묶음의 모든 이름(감사 · --icon-sheet)
[[nodiscard]] QStringList iconNames();
/// QIcon 에 굽는 배율 목록 {1, 1.25, 1.5, 1.75, 2, 3} — Windows 기본 배율 100 · 125 · 150 · 175 · 200 %(스펙 §2.1). icon · chipIcon · 리본 forceOn 칩이 모두 이것만 쓴다(B5)
[[nodiscard]] QList<qreal> iconDprs();
/// 선 아이콘 QIcon(1× · 2×). 없는 이름이면 빈 QIcon + qWarning 한 번
[[nodiscard]] QIcon icon(const QString& name, int px, const QColor& ink);
/// 한 장 바로 그리기(ink 색, px 논리 픽셀, dpr 배율)
[[nodiscard]] QPixmap glyph(const QString& name, int px, const QColor& ink, qreal dpr = 1.0);
/// 리본 칩 아이콘: tile 한 변(px) 안에 glyph 크기 아이콘, 둥글기 tile/4. kind 에 따라 켜짐 색이 다르다
[[nodiscard]] QIcon chipIcon(const QString& name, int tile, int glyphPx, ChipKind kind, const ChipColors& c, bool caret = false);   // caret = 메뉴 칩 ▾(타일 오른쪽 아래)
}  // namespace kerf
