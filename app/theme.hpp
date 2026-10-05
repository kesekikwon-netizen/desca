// 측량앱(LightTokens)과 같은 색·글꼴. 아이보리 바탕 + 테라코타 강조, 맑은 고딕.
#pragma once
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QString>

namespace theme {
inline const QColor Bg{0xF7, 0xF6, 0xF2}, Card{0xFF, 0xFF, 0xFF}, Line{0xDA, 0xD9, 0xD4}, Outline{0x76, 0x76, 0x72};
inline const QColor Ink{0x11, 0x11, 0x11}, InkSub{0x47, 0x47, 0x44}, Chip{0xED, 0xEC, 0xE7}, Idle{0x8A, 0x8A, 0x86};
inline const QColor Brand{0xC6, 0x61, 0x3F}, BrandSoft{0xFB, 0xED, 0xE7}, Ok{0x1E, 0x7A, 0x46}, Caution{0xB0, 0x7A, 0x00};
inline const QColor SectionRed{0xFF, 0x00, 0x00};  // 평면의 단면선·괄호선: 순수 빨강
inline const QColor ProfileRed{0xFF, 0x00, 0x00};  // 단면 보기의 절단 윤곽선: 순수 빨강(DXF 레이어 색 1 과 같음)

inline QString fontFamily() {
#ifdef Q_OS_WIN
    return QStringLiteral("Malgun Gothic");
#else
    return QStringLiteral("Noto Sans CJK KR");
#endif
}

// 데스크톱 측량 소프트웨어 형식의 연회색 크롬(리본·창 제목줄·하단 좌표줄) + 흰 보기 창. 강조색은 측량앱 테라코타.
inline QString styleSheet() {
    return QStringLiteral(R"(
QMainWindow, QWidget#central { background: #E9E9E7; }
QWidget { color: #111111; font-size: 9pt; }
QWidget#ribbon { background: #F5F5F3; border-bottom: 1px solid #C9C9C6; }
QTabBar#ribbonTabs { background: #E9E9E7; }
QTabBar#ribbonTabs::tab { background: transparent; border: 1px solid transparent; border-bottom: none; padding: 4px 14px 5px 14px; margin-right: 1px; color: #222222; }
QTabBar#ribbonTabs::tab:hover { background: #F0F0EE; }
QTabBar#ribbonTabs::tab:selected { background: #F5F5F3; border-color: #C9C9C6; border-top: 2px solid #C6613F; color: #111111; font-weight: 600; }
QTabBar#ribbonTabs::tab:first { background: #C6613F; color: #FFFFFF; font-weight: 600; margin-right: 6px; }
QTabBar#ribbonTabs::tab:first:selected { background: #A94F31; border-color: #A94F31; color: #FFFFFF; }
QLabel#brand { color: #474744; font-size: 8pt; padding-right: 10px; }
QLabel#groupLabel { color: #5E5E5B; font-size: 8pt; }
QFrame#groupSep { background: #D3D3D0; max-width: 1px; min-width: 1px; }
QToolButton { border: 1px solid transparent; border-radius: 3px; padding: 2px 4px; background: transparent; color: #111111; }
QToolButton:hover { background: #FBEDE7; border-color: #E2B7A6; }
QToolButton:pressed { background: #F4DDD3; }
QToolButton:checked { background: #F8E1D7; border-color: #C6613F; }
QToolButton:disabled { color: #A9A9A6; }
QWidget#viewFrame { background: #FFFFFF; border: 1px solid #A9A9A6; }
QWidget#viewTitle { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #F2F2F0, stop:1 #E1E1DE); border-bottom: 1px solid #C2C2BF; }
QWidget#viewTitle[active="true"] { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #FBEDE7, stop:1 #F1D6CB); }
QLabel#viewTitleText { font-weight: 600; color: #222222; padding-left: 4px; }
QWidget#viewBar { background: #F7F7F5; border-bottom: 1px solid #DADAD7; }
QWidget#viewBar QToolButton { padding: 1px; border-radius: 2px; }
QWidget#coordBar { background: #F0F0EE; border-top: 1px solid #C2C2BF; }
QLabel#coordKey { color: #474744; font-weight: 600; padding: 0 2px 0 8px; }
QLineEdit#coord { font-family: Consolas, "DejaVu Sans Mono", monospace; background: #FFFFFF; border: 1px solid #B9B9B6; border-radius: 0; padding: 1px 4px; min-height: 18px; }
QLabel#statusMsg { color: #333333; padding: 0 8px; }
QLabel#statusInfo { color: #5E5E5B; padding: 0 8px; }
QToolButton#viewNum { min-width: 18px; max-width: 18px; min-height: 18px; max-height: 18px; padding: 0; border: 1px solid #B9B9B6; border-radius: 0; background: #FFFFFF; font-size: 8pt; }
QToolButton#viewNum:checked { background: #C6613F; color: #FFFFFF; border-color: #A94F31; }
QToolButton#viewNum:disabled { background: #EDEDEB; color: #B5B5B2; border-color: #D3D3D0; }
QDoubleSpinBox, QComboBox, QSpinBox, QLineEdit { background: #FFFFFF; border: 1px solid #B9B9B6; border-radius: 2px; padding: 1px 4px; min-height: 18px; }
QDoubleSpinBox:focus, QComboBox:focus, QSpinBox:focus, QLineEdit:focus { border-color: #C6613F; }
QCheckBox { spacing: 5px; }
QCheckBox::indicator { width: 13px; height: 13px; border: 1px solid #8E8E8B; border-radius: 2px; background: #FFFFFF; }
QCheckBox::indicator:checked { background: #C6613F; border-color: #A94F31; image: none; }
QRadioButton::indicator { width: 13px; height: 13px; }
QSlider::groove:horizontal { height: 4px; background: #D3D3D0; border-radius: 2px; }
QSlider::sub-page:horizontal { background: #C6613F; border-radius: 2px; }
QSlider::handle:horizontal { width: 12px; margin: -5px 0; border-radius: 6px; background: #FFFFFF; border: 1px solid #A94F31; }
QSplitter::handle { background: #E9E9E7; width: 5px; }
QPushButton { background: #FFFFFF; border: 1px solid #8E8E8B; border-radius: 3px; padding: 5px 16px; }
QPushButton:hover { background: #FBEDE7; border-color: #C6613F; }
QPushButton#primary { background: #C6613F; color: #FFFFFF; border-color: #A94F31; font-weight: 600; }
QPushButton#primary:hover { background: #A94F31; }
QDialog { background: #F5F5F3; }
QGroupBox { border: 1px solid #D3D3D0; border-radius: 3px; margin-top: 10px; padding-top: 6px; background: #FFFFFF; }
QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: #474744; font-weight: 600; }
QLabel#hint { color: #5E5E5B; }
QLabel#calc { background: #FBEDE7; border: 1px solid #E2B7A6; border-radius: 3px; padding: 6px 8px; color: #5A2A18; }
QProgressBar { border: 1px solid #B9B9B6; border-radius: 0; background: #FFFFFF; max-height: 12px; min-width: 120px; text-align: center; font-size: 7pt; }
QProgressBar::chunk { background: #C6613F; }
QToolTip { background: #FFFFE8; color: #111111; border: 1px solid #8E8E8B; padding: 3px 6px; }
)");
}

// ---- 리본 아이콘(벡터로 그려 선명) ----
enum class Ico { Open, Draw, Flip, Plan, Fit, Orbit, Image, Line, Levels, Dxf, Png, Smooth, Tif, Geo, Xyz, Las, Csv, ZoomIn, ZoomOut, Close, Info, Clear, View1, View2, Lang, Band };

inline QIcon icon(Ico k, int S = 32) {
    QIcon ic;
    for (int scale : {1, 2}) {
        int s = S * scale;
        QPixmap pm(s, s);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.scale(s / 32.0, s / 32.0);
        QPen ink(Ink, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin), br(Brand, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QPen red(SectionRed, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin), thin(QColor(0xA9, 0xA8, 0xA3), 1.2);
        switch (k) {
        case Ico::Open: {
            p.setPen(ink); p.setBrush(BrandSoft);
            QPainterPath f; f.moveTo(4, 9); f.lineTo(12, 9); f.lineTo(14, 12); f.lineTo(28, 12); f.lineTo(28, 25); f.lineTo(4, 25); f.closeSubpath();
            p.drawPath(f);
            p.setPen(br); p.drawLine(10, 19, 22, 19); p.drawLine(18, 15, 22, 19); p.drawLine(18, 23, 22, 19);
            break; }
        case Ico::Draw:
            p.setPen(thin); for (int y : {9, 16, 23}) p.drawLine(4, y, 28, y);
            p.setPen(red); p.drawLine(6, 24, 26, 8);
            p.setPen(QPen(Ink, 1.6)); p.setBrush(Qt::white); p.drawEllipse(QPointF(6, 24), 3, 3); p.drawEllipse(QPointF(26, 8), 3, 3);
            break;
        case Ico::Flip:
            p.setPen(red); p.drawLine(5, 16, 27, 16);
            p.setPen(ink); p.drawLine(16, 6, 16, 11); p.drawLine(13, 9, 16, 6); p.drawLine(19, 9, 16, 6);
            p.drawLine(16, 21, 16, 26); p.drawLine(13, 23, 16, 26); p.drawLine(19, 23, 16, 26);
            break;
        case Ico::Plan:
            p.setPen(ink); p.setBrush(BrandSoft); p.drawRoundedRect(QRectF(5, 5, 22, 22), 3, 3);
            p.setPen(thin); p.drawLine(5, 16, 27, 16); p.drawLine(16, 5, 16, 27);
            p.setPen(br); p.drawLine(16, 9, 16, 14); p.drawLine(13.5, 11.5, 16, 9); p.drawLine(18.5, 11.5, 16, 9);
            break;
        case Ico::Fit:
            p.setPen(ink);
            for (auto c : {QPointF(5, 5), QPointF(27, 5), QPointF(5, 27), QPointF(27, 27)}) {
                double sx = c.x() < 16 ? 1 : -1, sy = c.y() < 16 ? 1 : -1;
                p.drawLine(c, c + QPointF(6 * sx, 0)); p.drawLine(c, c + QPointF(0, 6 * sy));
            }
            p.setPen(Qt::NoPen); p.setBrush(Brand); p.drawRoundedRect(QRectF(11, 11, 10, 10), 2, 2);
            break;
        case Ico::Orbit:
            p.setPen(ink); p.setBrush(Qt::NoBrush); p.drawEllipse(QRectF(5, 10, 22, 12));
            p.setPen(br); p.drawArc(QRectF(8, 4, 16, 24), 30 * 16, 120 * 16);
            p.drawLine(QPointF(23, 9), QPointF(22.5, 5)); p.drawLine(QPointF(23, 9), QPointF(19.5, 8));
            break;
        case Ico::Image: {
            p.setPen(ink); p.setBrush(QColor(0xE8, 0xD3, 0xC2)); p.drawRoundedRect(QRectF(4, 7, 24, 18), 3, 3);
            QPainterPath m; m.moveTo(6, 23); m.lineTo(12, 15); m.lineTo(16, 19); m.lineTo(21, 12); m.lineTo(26, 23); m.closeSubpath();
            p.setPen(Qt::NoPen); p.setBrush(Brand); p.drawPath(m);
            break; }
        case Ico::Line: {
            QPainterPath pp; pp.moveTo(4, 14); pp.lineTo(10, 13); pp.lineTo(13, 22); pp.lineTo(20, 22); pp.lineTo(23, 12); pp.lineTo(28, 11);
            p.setPen(QPen(SectionRed, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p.drawPath(pp);
            break; }
        case Ico::Levels:
            p.setPen(QPen(QColor(0x9A, 0x99, 0x94), 1.2)); for (int y = 7; y <= 25; y += 3) p.drawLine(9, y, 28, y);
            p.setPen(QPen(Ink, 2)); p.drawLine(9, 16, 28, 16); p.drawLine(9, 25, 28, 25);
            p.setPen(QPen(Brand, 2)); p.drawLine(3, 16, 7, 16); p.drawLine(3, 25, 7, 25);
            break;
        case Ico::Dxf: case Ico::Png: case Ico::Tif: case Ico::Geo: case Ico::Xyz: case Ico::Las: case Ico::Csv: {
            p.setPen(ink); p.setBrush(Qt::white);
            QPainterPath d; d.moveTo(7, 3); d.lineTo(20, 3); d.lineTo(26, 9); d.lineTo(26, 29); d.lineTo(7, 29); d.closeSubpath();
            p.drawPath(d);
            p.setPen(QPen(Ink, 1.2)); p.drawLine(QPointF(20, 3), QPointF(20, 9)); p.drawLine(QPointF(20, 9), QPointF(26, 9));
            const char* t = k == Ico::Dxf ? "DXF" : k == Ico::Png ? "PNG" : k == Ico::Tif ? "TIF" : k == Ico::Geo ? "GEO" : k == Ico::Xyz ? "XYZ" : k == Ico::Las ? "LAS" : "CSV";
            QColor bc = (k == Ico::Dxf || k == Ico::Geo) ? Brand : (k == Ico::Xyz || k == Ico::Las) ? QColor(0x2F, 0x6B, 0x4F) : Ink;
            p.setPen(Qt::NoPen); p.setBrush(bc); p.drawRoundedRect(QRectF(2, 15, 22, 10), 2, 2);
            QFont f; f.setPixelSize(8); f.setBold(true); p.setFont(f); p.setPen(Qt::white);
            p.drawText(QRectF(2, 15, 22, 10), Qt::AlignCenter, t);
            if (k == Ico::Xyz || k == Ico::Las) { p.setBrush(bc); for (auto q : {QPointF(12, 7), QPointF(16, 9), QPointF(11, 11), QPointF(19, 12)}) p.drawEllipse(q, 1.3, 1.3); }
            break; }
        case Ico::ZoomIn: case Ico::ZoomOut:
            p.setPen(ink); p.setBrush(Qt::white); p.drawEllipse(QPointF(13, 13), 8, 8);
            p.setPen(QPen(Ink, 3.2, Qt::SolidLine, Qt::RoundCap)); p.drawLine(QPointF(19, 19), QPointF(27, 27));
            p.setPen(br); p.drawLine(QPointF(9, 13), QPointF(17, 13)); if (k == Ico::ZoomIn) p.drawLine(QPointF(13, 9), QPointF(13, 17));
            break;
        case Ico::Close:
            p.setPen(ink); p.setBrush(Qt::white);
            { QPainterPath f; f.moveTo(4, 9); f.lineTo(12, 9); f.lineTo(14, 12); f.lineTo(28, 12); f.lineTo(28, 25); f.lineTo(4, 25); f.closeSubpath(); p.drawPath(f); }
            p.setPen(br); p.drawLine(12, 15, 20, 23); p.drawLine(20, 15, 12, 23);
            break;
        case Ico::Info:
            p.setPen(ink); p.setBrush(BrandSoft); p.drawEllipse(QPointF(16, 16), 12, 12);
            p.setPen(QPen(Brand, 3, Qt::SolidLine, Qt::RoundCap)); p.drawLine(QPointF(16, 14), QPointF(16, 22)); p.drawPoint(QPointF(16, 9.5));
            break;
        case Ico::Clear:
            p.setPen(thin); for (int y : {9, 16, 23}) p.drawLine(4, y, 28, y);
            p.setPen(QPen(SectionRed, 2.4, Qt::DashLine)); p.drawLine(6, 24, 26, 8);
            p.setPen(ink); p.drawLine(18, 18, 27, 27); p.drawLine(27, 18, 18, 27);
            break;
        case Ico::View1: case Ico::View2: {
            p.setPen(ink); p.setBrush(Qt::white); p.drawRect(QRectF(4, 6, 24, 20));
            p.setBrush(QColor(0xE6, 0xE6, 0xE3)); p.drawRect(QRectF(4, 6, 24, 5));
            QFont f; f.setPixelSize(12); f.setBold(true); p.setFont(f); p.setPen(Brand);
            p.drawText(QRectF(4, 11, 24, 15), Qt::AlignCenter, k == Ico::View1 ? "1" : "2");
            break; }
        case Ico::Lang: {
            p.setPen(ink); p.setBrush(Qt::white); p.drawRoundedRect(QRectF(3, 6, 18, 14), 2, 2);
            QFont f; f.setPixelSize(10); f.setBold(true); p.setFont(f); p.drawText(QRectF(3, 6, 18, 14), Qt::AlignCenter, QString::fromUtf8("가"));
            p.setBrush(BrandSoft); p.drawRoundedRect(QRectF(12, 14, 18, 14), 2, 2);
            p.setPen(Brand); p.drawText(QRectF(12, 14, 18, 14), Qt::AlignCenter, "A");
            break; }
        case Ico::Band:
            p.setPen(Qt::NoPen); p.setBrush(QColor(211, 47, 47, 40)); p.drawRect(QRectF(4, 12, 24, 9));
            p.setPen(red); p.drawLine(4, 12, 28, 12);
            p.setPen(QPen(SectionRed, 1.4)); p.drawLine(4, 21, 28, 21); p.drawLine(4, 12, 4, 21); p.drawLine(28, 12, 28, 21);
            break;
        case Ico::Smooth: {
            QPainterPath s; s.moveTo(4, 20); s.cubicTo(10, 6, 20, 28, 28, 12);
            p.setPen(br); p.drawPath(s);
            break; }
        }
        p.end();
        ic.addPixmap(pm);
    }
    return ic;
}
}  // namespace theme
