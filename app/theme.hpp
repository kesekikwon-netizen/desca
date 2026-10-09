// Strata 「조용한 도판」 색·글꼴(research/ui-ref/UI-SPEC.md §2, strata.qss). 바탕 ground + 먹색 글자, 흙색은 주 단추 하나에만.
// 데이터 색(단면선 #FF0000, 레벨선 회색)은 그대로 둔다.
#pragma once
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include "icons.hpp"

namespace theme {
// ---- 토큰
inline const QColor Ground{0xFA, 0xF9, 0xF5}, Card{0xFF, 0xFF, 0xFF}, Wash{0xF5, 0xF4, 0xED}, Desk{0xF0, 0xEE, 0xE6}, Press{0xE8, 0xE6, 0xDC}, Oat{0xE3, 0xDA, 0xCC};
inline const QColor Ink{0x14, 0x14, 0x13}, Hand{0x30, 0x30, 0x2E}, Ink2{0x3D, 0x3D, 0x3A}, Muted{0x5E, 0x5D, 0x59}, Faint{0x73, 0x72, 0x6C};
inline const QColor Line{0xDE, 0xDC, 0xD1}, Edge{0xC2, 0xC0, 0xB6}, Ring{0xB0, 0xAE, 0xA5}, CheckEdge{0x87, 0x86, 0x7F};
inline const QColor Action{0xB5, 0x57, 0x3A}, ActionHover{0xA4, 0x4D, 0x32}, ChipPressed{0xE3, 0xE0, 0xD4}, ClayText{0x9C, 0x4A, 0x2F}, Block{0xA3, 0x3B, 0x3B}, BlockWash{0xF3, 0xDE, 0xDA}, Caution{0x7A, 0x5A, 0x00}, Done{0x3F, 0x6B, 0x31};
inline const QColor LevelMinor{0xDE, 0xDC, 0xD1}, LevelMajor{0x9C, 0x9A, 0x92}, LevelText{0x3D, 0x3D, 0x3A};
// 옛 이름(다른 파일 호환)
inline const QColor Bg = Ground, InkSub = Muted, Chip = Desk, Idle = Faint, Outline = CheckEdge, Brand = Action, BrandSoft = Wash, Ok = Done;
inline const QColor SectionRed{0xFF, 0x00, 0x00};  // 평면의 단면선·괄호선: 순수 빨강(DXF 색 1) — 바꾸지 않음
inline const QColor ProfileRed{0xFF, 0x00, 0x00};  // 단면 보기의 절단 윤곽선: 순수 빨강

inline QStringList fontFamilies() { return {QStringLiteral("Malgun Gothic"), QStringLiteral("Noto Sans CJK KR"), QStringLiteral("Noto Sans KR"), QStringLiteral("Apple SD Gothic Neo")}; }
inline QString fontFamily() {
#ifdef Q_OS_WIN
    return QStringLiteral("Malgun Gothic");
#else
    return QStringLiteral("Noto Sans CJK KR");
#endif
}
inline QFont uiFont(int px, bool bold = false) { QFont f; f.setFamilies(fontFamilies()); f.setPixelSize(px); f.setBold(bold); return f; }
inline QFont monoFont(int px) {
    QFont f; f.setFamilies({QStringLiteral("Consolas"), QStringLiteral("DejaVu Sans Mono"), QStringLiteral("Noto Sans Mono"), QStringLiteral("monospace")});
    f.setStyleHint(QFont::Monospace); f.setPixelSize(px); return f;
}

// 체크 표시 그림: rcc 없이 실행 중 임시 PNG 로 만들어 QSS url() 로 씀
inline QString checkImagePath(const QColor& c, const QString& name) {
    QString dir = QDir::tempPath() + "/ExcavSection-ui";
    QDir().mkpath(dir);
    QString path = dir + "/" + name + ".png";
    if (!QFile::exists(path)) {
        QPixmap pm(28, 28); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c, 3.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath t; t.moveTo(6, 14.5); t.lineTo(11.5, 20); t.lineTo(22, 8); p.drawPath(t);
        p.end(); pm.save(path, "PNG");
    }
    return QDir::fromNativeSeparators(path);
}

inline QString arrowImagePath(const QColor& c, const QString& name) {
    QString dir = QDir::tempPath() + "/ExcavSection-ui";
    QDir().mkpath(dir);
    QString path = dir + "/" + name + ".png";
    if (!QFile::exists(path)) {
        QPixmap pm(20, 20); pm.fill(Qt::transparent);
        QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(c, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath t; t.moveTo(5, 8); t.lineTo(10, 13); t.lineTo(15, 8); p.drawPath(t);
        p.end(); pm.save(path, "PNG");
    }
    return QDir::fromNativeSeparators(path);
}

inline QString styleSheet(bool highContrast = false) {
    QString s = QStringLiteral(R"(
QMainWindow, QWidget#central, QWidget#workArea { background: #FAF9F5; }
QWidget { color: #141413; font-size: 13px; }
QLabel#hint, QLabel#statusInfo { color: #5E5D59; font-size: 12px; }
QLabel#faint { color: #73726C; font-size: 11px; }
QLabel#title { font-size: 17px; font-weight: 700; color: #141413; }
QLabel#bigTitle { font-size: 26px; font-weight: 700; color: #141413; }
QLabel#heroName { font-family: Batang, "Noto Serif KR", "Malgun Gothic"; font-size: 40px; font-weight: 500; color: #141413; }
QToolButton#stepBtn { color: #73726C; font-size: 12px; border: none; padding: 2px 4px; background: transparent; }
QToolButton#stepBtn[state="now"] { color: #141413; font-weight: 700; }
QToolButton#stepBtn[state="ok"] { color: #3D3D3A; }
QLabel#mono, QLineEdit#mono { font-family: Consolas, "DejaVu Sans Mono", monospace; font-size: 12px; }
QLabel#secName { font-size: 13px; font-weight: 700; color: #141413; }
QWidget#dialogMain { background: #FAF9F5; }
QLabel#monoFaint { font-family: Consolas, "DejaVu Sans Mono", monospace; font-size: 11px; color: #73726C; }
QLabel#sectionHead { font-size: 13px; font-weight: 700; color: #141413; }
QLabel#facing { color: #5E5D59; font-size: 12px; }

QWidget#ribbon { background: #FAF9F5; border-bottom: 1px solid #DEDCD1; }
QFrame#ribbonGroup { background: transparent; border: none; border-right: 1px solid #DEDCD1; }
QFrame#ribbonGroup[last="true"] { border-right: none; }
QLabel#ribbonGroupCaption { color: #5E5D59; font-size: 11px; font-weight: 700; padding: 0 2px; background: transparent; }
QLabel#ribbonGroupCaption[dim="true"] { color: #C2C0B6; }
QToolButton#ribbonChip { margin: 0; padding: 1px; background: transparent; color: #141413; font-size: 13px; border: 1px solid transparent; border-radius: 8px; text-align: center; }
QToolButton#ribbonChip:hover { background: transparent; border: 1px solid transparent; }
QToolButton#ribbonChip:pressed { background: #E8E6DC; border-color: #DEDCD1; }
QToolButton#ribbonChip:checked, QToolButton#ribbonChip:checked:hover { background: transparent; color: #141413; font-weight: 700; border: 1px solid transparent; }
QToolButton#ribbonChip[primary="true"] { font-weight: 700; }
QToolButton#ribbonChip:disabled { color: #696862; }
QToolButton#ribbonChip:checked:disabled, QToolButton#ribbonChip[primary="true"]:disabled { font-weight: 400; color: #696862; }
QToolButton#ribbonChip[kbdFocus="true"]:focus { border: 1px solid #141413; }
QToolButton#ribbonChip::menu-indicator { image: none; width: 0px; height: 0px; }
QToolButton#ribbonCorner { min-height: 38px; padding: 0 6px; border: 1px solid #C2C0B6; border-radius: 8px; background: #FFFFFF; color: #141413; font-size: 13px; }
QToolButton#ribbonCorner:hover { background: #F5F4ED; border-color: #B0AEA5; }
QLineEdit#ribbonSearch { min-height: 32px; max-height: 32px; min-width: 150px; max-width: 150px; border: 1px solid #C2C0B6; border-radius: 8px; background: #FFFFFF; padding: 0 10px; font-size: 12px; }
QLineEdit#ribbonSearch:focus { border: 2px solid #141413; padding: 0 9px; }

QWidget#docTabsRow { background: #F0EEE6; border-bottom: 1px solid #DEDCD1; }
QLabel#docTabCorner { color: #5E5D59; font-size: 13px; padding-right: 8px; }
QTabBar#docTabs { background: transparent; border: none; }
QTabBar#docTabs::tab { background: transparent; color: #3D3D3A; border: 1px solid transparent; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; padding: 6px 14px; margin: 4px 2px 0 0; font-size: 13px; }
QTabBar#docTabs::tab:disabled { color: #9C9A92; }
QTabBar#docTabs::tab:selected { background: #FAF9F5; color: #141413; font-weight: 700; border-color: #DEDCD1; }
QTabBar#docTabs::tab:hover:!selected { background: #F5F4ED; }
QWidget#viewToggles { border: 1px solid #DEDCD1; border-radius: 6px; background: #FFFFFF; }
QToolButton#viewToggle { border: 1px solid transparent; border-radius: 4px; padding: 0; background: transparent; }
QToolButton#viewToggle:checked { background: #FFFFFF; border-color: #B0AEA5; }
QToolButton#viewToggle:!checked { background: #F5F4ED; }
QToolButton#viewToggle:hover { border-color: #C2C0B6; }

QToolButton { border: 1px solid transparent; border-radius: 4px; padding: 2px 6px; background: transparent; color: #141413; }
QToolButton:hover { background: #F5F4ED; border-color: #C2C0B6; }
QToolButton:pressed { background: #E8E6DC; }
QToolButton:checked { background: #E3DACC; border-color: #B0AEA5; }
QToolButton:disabled { color: #9C9A92; }
QToolButton::menu-indicator { image: none; width: 0px; }
QToolButton#smallRow { text-align: left; padding: 1px 6px; border: 1px solid #DEDCD1; background: #FFFFFF; }
QToolButton#smallRow:hover { background: #F5F4ED; border-color: #C2C0B6; }
QToolButton#undoBtn { border: 1px solid #C2C0B6; border-radius: 6px; background: #FFFFFF; padding: 2px 8px; }
QToolButton#undoBtn:hover { background: #F5F4ED; }

QToolButton#chip { min-height: 20px; padding: 0 7px; border: 1px solid #C2C0B6; border-radius: 0; background: #FFFFFF; font-family: Consolas, "DejaVu Sans Mono", monospace; font-size: 12px; }
QToolButton#chip:hover { background: #F5F4ED; }
QToolButton#chip:checked { background: #E3DACC; border-color: #141413; }
QToolButton#chip[pos="first"] { border-top-left-radius: 6px; border-bottom-left-radius: 6px; }
QToolButton#chip[pos="last"] { border-top-right-radius: 6px; border-bottom-right-radius: 6px; }

QFrame#guideBand { background: #FFFFFF; border: 1px solid #C2C0B6; border-radius: 8px; }
QLabel#guideTitle { font-size: 13px; color: #141413; background: transparent; }
QLabel#guideHint { font-size: 12px; color: #5E5D59; background: transparent; }
QFrame#guideRule { background: #DEDCD1; }
QFrame#floatButtons { background: #FFFFFF; border: 1px solid #C2C0B6; border-radius: 8px; }
QToolButton#floatBtn, QToolButton#floatCell { border: none; border-radius: 0; background: transparent; color: #3D3D3A; padding: 0; font-size: 11px; }
QToolButton#floatBtn[after="true"], QToolButton#floatCell[after="true"] { border-left: 1px solid #DEDCD1; }
QToolButton#floatBtn:hover, QToolButton#floatCell:hover { background: #F5F4ED; }
QToolButton#floatBtn:pressed, QToolButton#floatCell:pressed { background: #E8E6DC; }
QToolButton#floatBtn:disabled, QToolButton#floatCell:disabled { color: #B0AEA5; }
QToolButton#floatCell:checked { background: #30302E; color: #FAF9F5; }
QToolButton#chip[tight="true"] { padding: 0 2px; }
QLabel#kbd { font-family: Consolas, "DejaVu Sans Mono", monospace; font-size: 11px; color: #3D3D3A; background: #F5F4ED; border: 1px solid #C2C0B6; border-bottom-width: 2px; border-radius: 4px; padding: 0 5px; }

QWidget#viewFrame { background: #FFFFFF; border: 1px solid #DEDCD1; }
QWidget#viewTitle { background: #FFFFFF; border-bottom: 1px solid #DEDCD1; }
QWidget#viewTitle[active="true"] { border-bottom: 2px solid #141413; }
QLabel#viewTitleText { font-weight: 700; color: #141413; padding-left: 2px; }
QWidget#infoStrip { background: #F5F4ED; border-bottom: 1px solid #DEDCD1; }
QWidget#infoStrip QLabel { font-size: 12px; color: #3D3D3A; }
QWidget#viewBar { background: #F5F4ED; border-bottom: 1px solid #DEDCD1; }
QWidget#viewTitle QToolButton { padding: 2px; border-radius: 4px; }

QToolButton#statusBadge, QToolButton#heightBadge { min-height: 22px; max-height: 22px; background: #FFFFFF; border: 1px solid #B5573A; border-radius: 8px; padding: 0 8px; font-size: 12px; font-weight: 700; color: #9C4A2F; }
QToolButton#statusBadge:hover, QToolButton#heightBadge:hover { background: #F5F4ED; }
QToolButton#statusBadge[state="warn"], QToolButton#heightBadge[state="warn"] { border-color: #7A5A00; color: #7A5A00; }
QToolButton#statusBadge[state="error"], QToolButton#heightBadge[state="error"] { background: #F3DEDA; border-color: #A33B3B; color: #A33B3B; }
QToolButton#statusBadge[state="none"], QToolButton#heightBadge[state="none"] { background: #F0EEE6; border-color: #DEDCD1; color: #73726C; font-weight: 400; }
QToolButton#vexBtn { background: transparent; border: 1px solid transparent; border-radius: 4px; padding: 0px 6px; font-size: 12px; color: #3D3D3A; }
QToolButton#vexBtn:hover { border-color: #C2C0B6; }
QToolButton#vexBtn::menu-indicator { image: none; width: 0px; }
QToolButton#vexBtn[state="on"] { background: #FFF4D6; border-color: #C98A1B; color: #7A5A00; font-weight: bold; }
QToolButton#vexBtn[state="hint"] { border-color: transparent; color: #3D3D3A; }

QToolButton#viewNum { min-width: 20px; max-width: 20px; min-height: 20px; max-height: 20px; padding: 0; border: 1px solid #C2C0B6; border-radius: 4px; background: #FFFFFF; font-family: Consolas, "DejaVu Sans Mono", monospace; font-size: 11px; }
QToolButton#viewNum:checked { background: #30302E; color: #FAF9F5; border-color: #30302E; }
QToolButton#viewNum:disabled { background: #F0EEE6; color: #73726C; border-color: #DEDCD1; }

QListWidget#sectionList { background: #FFFFFF; border: none; outline: 0; }
QListWidget#sectionList::item { border-bottom: 1px solid #DEDCD1; color: #141413; padding: 0; }
QListWidget#sectionList::item:hover { background: #F5F4ED; }
QListWidget#sectionList::item:selected { background: #E3DACC; color: #141413; border-left: 3px solid #141413; }
QWidget#sidePanel { background: #FFFFFF; border-right: 1px solid #DEDCD1; }

QWidget#statusBar { background: #FAF9F5; border-top: 1px solid #DEDCD1; }
QFrame#statusRule { background: #DEDCD1; }
QLabel#coordKey { color: #5E5D59; font-size: 11px; padding: 0 6px 0 0; }
QLineEdit#coord { font-family: Consolas, "DejaVu Sans Mono", monospace; font-size: 12px; color: #141413; background: transparent; border: none; padding: 0; min-height: 20px; }
QLabel#statusMsg { color: #5E5D59; font-size: 12px; padding: 0; }
QLabel#zSrc { color: #5E5D59; font-size: 11px; padding: 0 0 0 6px; }
QLabel#zSrc[coarse="true"] { color: #7A5A00; }

QDoubleSpinBox, QSpinBox, QComboBox, QLineEdit { background: #FFFFFF; border: 1px solid #C2C0B6; border-radius: 4px; padding: 1px 6px; min-height: 20px; selection-background-color: #E3DACC; selection-color: #141413; }
QDoubleSpinBox, QSpinBox { font-family: Consolas, "DejaVu Sans Mono", monospace; font-size: 12px; }
QDoubleSpinBox:hover, QSpinBox:hover, QComboBox:hover, QLineEdit:hover { border-color: #B0AEA5; }
QDoubleSpinBox:focus, QSpinBox:focus, QComboBox:focus, QLineEdit:focus { border: 1px solid #141413; }
QDoubleSpinBox:disabled, QSpinBox:disabled, QComboBox:disabled, QLineEdit:disabled { background: #F0EEE6; color: #73726C; }
QComboBox::drop-down { width: 20px; border: none; }
QComboBox::down-arrow { image: url(%ARROW%); width: 10px; height: 10px; }
QAbstractSpinBox::up-button, QAbstractSpinBox::down-button { width: 0px; border: none; }
QAbstractSpinBox { padding-right: 6px; }
QComboBox QAbstractItemView { background: #FFFFFF; border: 1px solid #C2C0B6; selection-background-color: #E3DACC; selection-color: #141413; outline: 0; }

QCheckBox, QRadioButton { spacing: 8px; }
QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid #87867F; border-radius: 2px; background: #FFFFFF; }
QCheckBox::indicator:checked { background: #141413; border-color: #141413; image: url(%CHECK%); }
QCheckBox::indicator:disabled { background: #F0EEE6; border-color: #DEDCD1; }
QRadioButton::indicator { width: 14px; height: 14px; border: 1px solid #87867F; border-radius: 7px; background: #FFFFFF; }
QRadioButton::indicator:checked { border: 4px solid #141413; background: #FFFFFF; width: 8px; height: 8px; }

QSlider::groove:horizontal { height: 4px; background: #E8E6DC; border-radius: 2px; }
QSlider::sub-page:horizontal { background: #141413; border-radius: 2px; }
QSlider::handle:horizontal { width: 14px; margin: -6px 0; border-radius: 7px; background: #FFFFFF; border: 1px solid #87867F; }

QSplitter::handle { background: #DEDCD1; }
QSplitter::handle:horizontal { width: 1px; }

QPushButton { background: #FFFFFF; border: 1px solid #C2C0B6; border-radius: 8px; padding: 6px 16px; min-height: 18px; color: #141413; }
QPushButton:hover { background: #F5F4ED; border-color: #B0AEA5; }
QPushButton:pressed { background: #E8E6DC; }
QPushButton:disabled { color: #73726C; background: #F0EEE6; border-color: #DEDCD1; }
QPushButton#primary { background: #B5573A; color: #FFFFFF; border-color: #B5573A; font-weight: 700; }
QPushButton#primary:hover { background: #A44D32; border-color: #A44D32; }
QPushButton#primary:pressed { background: #8F4229; }
QPushButton#quiet { background: transparent; border-color: transparent; }
QPushButton#quiet:hover { background: #F5F4ED; }
QPushButton#danger { background: #FFFFFF; color: #A33B3B; border-color: #A33B3B; }

QDialog { background: #FAF9F5; }
QWidget#dialogHead { background: #FFFFFF; border-bottom: 1px solid #DEDCD1; }
QWidget#dialogSide { background: #F5F4ED; border-left: 1px solid #DEDCD1; }
QWidget#dialogFoot { background: #FFFFFF; border-top: 1px solid #DEDCD1; }
QLabel#sideHead { color: #5E5D59; font-size: 11px; }
QGroupBox { border: none; border-top: 1px solid #DEDCD1; margin-top: 18px; padding-top: 10px; background: transparent; }
QGroupBox::title { subcontrol-origin: margin; left: 0; padding: 0 8px 0 0; color: #5E5D59; font-size: 11px; }
QLabel#calc { background: #FFFFFF; border: none; border-top: 1px solid #141413; border-bottom: 1px solid #DEDCD1; padding: 6px 2px; color: #141413; }
QFrame#warnBox { background: #FFFFFF; border: 1px solid #C2C0B6; border-radius: 8px; }
QFrame#warnBox QLabel { font-size: 12px; }

QFrame#notice { background: #FFFFFF; border: none; border-bottom: 1px solid #DEDCD1; }
QFrame#notice[level="block"] { background: #F3DEDA; }
QFrame#notice[level="block"] QLabel { color: #A33B3B; }
QLabel#noticeIcon { color: #7A5A00; font-size: 13px; }

QProgressBar { border: none; background: #E8E6DC; border-radius: 2px; max-height: 4px; min-height: 4px; }
QProgressBar::chunk { background: #141413; border-radius: 2px; }

QMenu { background: #FFFFFF; border: 1px solid #C2C0B6; padding: 4px; }
QMenu::item { min-height: 22px; padding: 3px 28px 3px 12px; border-radius: 4px; }
QMenu::item:selected { background: #F5F4ED; color: #141413; }
QMenu::item:disabled { color: #73726C; }
QMenu::separator { height: 1px; background: #DEDCD1; margin: 4px 8px; }
QToolTip { background: #30302E; color: #FAF9F5; border: none; padding: 4px 8px; font-size: 12px; }

QWidget#startPage { background: #FAF9F5; }
QLabel#homeSub { font-size: 15px; color: #5E5D59; border-left: 1px solid #C2C0B6; padding-left: 14px; }
QLabel#homeLead { font-size: 15px; color: #5E5D59; }
QLabel#cap { font-size: 11px; color: #5E5D59; }
QLabel#lab { font-size: 12px; color: #5E5D59; }
QLabel#faintName { font-size: 13px; color: #73726C; }
QLabel#thumbName { font-family: Batang, "Noto Serif KR", "Malgun Gothic"; font-size: 12px; color: #141413; }
QPushButton#homeBtn, QToolButton#homeBtn { background: #FFFFFF; border: 1px solid #C2C0B6; border-radius: 8px; padding: 0 20px; font-size: 15px; color: #141413; }
QPushButton#homeBtn:hover, QToolButton#homeBtn:hover { background: #F5F4ED; border-color: #B0AEA5; }
QToolButton#homeBtn::menu-indicator { image: none; width: 0px; }
QFrame#continueCard { background: #FFFFFF; border: 1px solid #141413; }
QWidget#ccCells { border-top: 1px solid #DEDCD1; }
QWidget#ccCell { border-left: 1px solid #DEDCD1; }
QWidget#ccSide { border-left: 1px solid #DEDCD1; }
QToolButton#thumb { border: 1px solid #DEDCD1; background: #FFFFFF; padding: 1px; }
QToolButton#thumb:hover { border-color: #141413; }
QLineEdit#homeFind { min-height: 32px; max-height: 32px; border: 1px solid #C2C0B6; border-radius: 8px; background: #FFFFFF; padding: 0 10px; font-size: 13px; }
QLabel#stepMark { border: 1px solid #87867F; border-radius: 11px; font-size: 11px; color: #141413; background: transparent; }
QLabel#stepMark[state="ok"] { background: #141413; border-color: #141413; }
QLabel#stepMark[state="now"] { border-color: #D97757; color: #9C4A2F; }
QLabel#nextMark { font-size: 12px; color: #9C4A2F; }
QWidget#homeKeys { border-top: 1px solid #DEDCD1; }
QPushButton#quietLink { background: transparent; border: none; color: #9C4A2F; text-decoration: underline; padding: 0 4px; }
QFrame#card { background: #FFFFFF; border: 1px solid #C2C0B6; border-radius: 8px; }
QFrame#cardSide { background: #FFFFFF; border: none; border-left: 1px solid #DEDCD1; }
QFrame#dropZone { background: #F5F4ED; border: 1px dashed #B0AEA5; border-radius: 8px; }
QTableWidget#recentTable { background: #FAF9F5; border: none; border-top: 1px solid #141413; border-bottom: 1px solid #141413; gridline-color: #E8E6DC; outline: 0; }
QTableWidget#recentTable::item { border-bottom: 1px solid #E8E6DC; padding: 4px 8px; }
QTableWidget#recentTable::item:selected { background: #E3DACC; color: #141413; }
QHeaderView::section { background: #FFFFFF; color: #5E5D59; border: none; border-bottom: 1px solid #DEDCD1; padding: 4px 8px; font-size: 11px; }
QLabel#stepNum { border: 1px solid #141413; border-radius: 11px; min-width: 20px; max-width: 20px; min-height: 20px; max-height: 20px; font-size: 11px; qproperty-alignment: AlignCenter; }

QScrollBar:vertical { width: 10px; background: transparent; margin: 0; }
QScrollBar::handle:vertical { background: #C2C0B6; border-radius: 4px; min-height: 24px; margin: 2px; }
QScrollBar:horizontal { height: 10px; background: transparent; margin: 0; }
QScrollBar::handle:horizontal { background: #C2C0B6; border-radius: 4px; min-width: 24px; margin: 2px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
)");
    if (highContrast)
        s += QStringLiteral(R"(
QWidget#ribbon, QWidget#viewTitle, QWidget#statusBar, QWidget#docTabsRow { border-color: #87867F; }
QDoubleSpinBox, QSpinBox, QComboBox, QLineEdit, QPushButton { border-color: #73726C; }
QLabel#hint, QLabel#statusInfo, QLabel#coordKey, QLabel#faint { color: #3D3D3A; }
QToolButton:checked, QToolButton#chip:checked { border-color: #141413; }
)");
    s.replace(QStringLiteral("%CHECK%"), checkImagePath(QColor(0xFF, 0xFF, 0xFF), QStringLiteral("check-white")));
    s.replace(QStringLiteral("%ARROW%"), arrowImagePath(QColor(0x5E, 0x5D, 0x59), QStringLiteral("arrow-down")));
    return s;
}

// ---- 아이콘: 먹색 선(데이터 색 아이콘만 빨강·회색 획 유지)
enum class Ico { Open, Draw, Flip, Plan, Fit, Orbit, Image, Line, Levels, Dxf, Png, Smooth, Tif, Geo, Xyz, Las, Csv, ZoomIn, ZoomOut, Close, Info, Clear, View1, View2, Lang, Band,
                 Recent, Sheet, Undo, Redo, Measure, Height, Move, Add, Max, Picture, Keys, Vex };

/// Ico → SVG 이름(app/icons/, 디자인 v5 §11). 새 Ico 를 더하면 여기와 icons.cmake 목록에 같이 더한다
inline const char* iconName(Ico k) {
    switch (k) {
    case Ico::Open: return "folder";        case Ico::Draw: return "section-line"; case Ico::Flip: return "flip";
    case Ico::Plan: return "map";           case Ico::Fit: return "square-dashed";      case Ico::Orbit: return "top-view";
    case Ico::Image: return "image-layer";  case Ico::Line: return "pen-line";      case Ico::Levels: return "levels";
    case Ico::Dxf: return "file-text";      case Ico::Png: return "image";         case Ico::Smooth: return "pencil";
    case Ico::Tif: return "image";          case Ico::Geo: return "geotiff";       case Ico::Xyz: return "points-xyz";
    case Ico::Las: return "cloud-las";      case Ico::Csv: return "csv";           case Ico::ZoomIn: return "zoom-in";
    case Ico::ZoomOut: return "zoom-out";   case Ico::Close: return "x";           case Ico::Info: return "info";
    case Ico::Clear: return "x";            case Ico::View1: return "map";         case Ico::View2: return "profile";
    case Ico::Lang: return "file-text";     case Ico::Band: return "fade";     case Ico::Recent: return "clock";
    case Ico::Sheet: return "sheet";        case Ico::Undo: return "undo-2";       case Ico::Redo: return "redo-2";
    case Ico::Measure: return "crosshair";  case Ico::Height: return "height";     case Ico::Move: return "move";
    case Ico::Add: return "plus";           case Ico::Max: return "maximize";      case Ico::Picture: return "image";
    case Ico::Keys: return "keyboard";      case Ico::Vex: return "vex";
    }
    return "x";
}
inline constexpr int kIcoCount = int(Ico::Vex) + 1;

/// 리본 칩 타일 색(디자인 v5 §3.2 · Strata tokens.json ribbon) — 토큰에서만
inline kerf::ChipColors chipColors() {
    kerf::ChipColors c;
    c.tile = Desk; c.tileHover = Press; c.tilePressed = ChipPressed; c.tileDisabled = Wash; c.glyph = Hand;
    c.tileOn = Hand; c.borderOn = Hand; c.glyphOn = Ground;
    c.tileShown = Oat; c.borderShown = Ring;
    c.tilePrimary = Action; c.tilePrimaryHover = ActionHover; c.glyphPrimary = Card;
    return c;
}

/// 선 아이콘(1× · 2×). 색은 Hand(Strata 아이콘 먹색) — 떠 있는 단추 · 탭 · 메뉴 · 화면 머리에 쓴다
inline QIcon icon(Ico k, int S = 32, QColor inkColor = Hand) { return kerf::icon(QString::fromLatin1(iconName(k)), S, inkColor); }
}  // namespace theme
