// 떠 있는 안내 · 단추 묶음 구현(디자인 v5 §5). 모양은 theme.hpp QSS(#guideBand · #floatButtons)가 그린다.
#include "guideband.hpp"
#include "icons.hpp"
#include "theme.hpp"

#include <QAction>
#include <QBoxLayout>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QToolButton>
#include <QVBoxLayout>
#include <QFontMetrics>
#include <algorithm>

namespace {
constexpr int kBandHeight = 36;
constexpr int kMargin = 12;
}

GuideBand::GuideBand(QWidget* host) : QFrame(host), host_(host) {
    setObjectName(QStringLiteral("guideBand"));
    setAttribute(Qt::WA_StyledBackground);
    setFixedHeight(kBandHeight);
    auto* h = new QHBoxLayout(this);
    h->setContentsMargins(12, 0, 12, 0);
    h->setSpacing(8);
    icon_ = new QLabel; icon_->setFixedSize(16, 16);
    title_ = new QLabel; title_->setObjectName(QStringLiteral("guideTitle"));
    auto* rule = new QFrame; rule->setObjectName(QStringLiteral("guideRule")); rule->setFixedSize(1, 16); rule_ = rule;
    hint_ = new QLabel; hint_->setObjectName(QStringLiteral("guideHint"));
    keysBox_ = new QWidget; keysLay_ = new QHBoxLayout(keysBox_); keysLay_->setContentsMargins(0, 0, 0, 0); keysLay_->setSpacing(4);
    h->addWidget(icon_); h->addWidget(title_); h->addWidget(rule); h->addWidget(hint_); h->addWidget(keysBox_);
    host_->installEventFilter(this);
    hide();
}

void GuideBand::bakeIcon() {
    if (iconName_.isEmpty()) return;
    iconDpr_ = devicePixelRatioF();
    icon_->setPixmap(kerf::glyph(iconName_, 16, theme::Hand, iconDpr_));
}

void GuideBand::setTool(const QString& iconName, const QString& title) {
    if (iconName != iconName_) { iconName_ = iconName; bakeIcon(); }   // 같은 아이콘이면 SVG 를 다시 그리지 않음(검토 I3)
    if (fullTitle_ == title) return;
    fullTitle_ = title; title_->setText(title);
    adjustSize(); place();
}

void GuideBand::setHint(const QString& text) {
    if (text == fullHint_) return;
    fullHint_ = text; hint_->setText(text); adjustSize(); place();
}

// 마우스가 움직일 때마다 불려도 라벨을 다시 만들지 않는다(검토 I3). 라벨은 한 번 만들어 글만 바꾸고, 남는 것은 숨긴다
void GuideBand::setKeys(const QStringList& keys) {
    if (keys == keys_) return;
    keys_ = keys;
    while (keyLabels_.size() < keys.size()) {
        auto* l = new QLabel(keysBox_); l->setObjectName(QStringLiteral("kbd")); l->setFixedHeight(18); l->setAlignment(Qt::AlignCenter);   // 키 캡 18(스펙 §5)
        keysLay_->addWidget(l, 0, Qt::AlignVCenter); keyLabels_ << l;
        if (isVisible()) l->show();   // 보이는 칩 안에 더한 라벨은 바로 보여야 adjustSize 가 폭을 맞게 잼
        l->ensurePolished();          // 시트(모노 11 · 테)를 먼저 입혀야 sizeHint 가 맞는다 — 아니면 칩이 25 px 좁아 문장이 잘림(실측)
    }
    for (int i = 0; i < keyLabels_.size(); ++i) { keyLabels_[i]->setVisible(i < keys.size()); if (i < keys.size()) keyLabels_[i]->setText(keys.at(i)); }
    adjustSize(); place();
}

// 칩은 호스트 폭 안에 들어간다. 좁으면 ① 도구 이름을 단계만(「A′ 찾는 중」) ② 도구 이름 숨김(아이콘이 도구를 말한다) ③ 문장 줄임표 순서로 줄인다.
// 되돌리기 · 다시는 오른쪽에 자리(reserveRight_)가 있으면 거기, 없으면 칩 아래로 간다(FloatButtons::place). 문장은 되도록 그대로 보인다(스펙 §5 · 검토 UI 3)
void GuideBand::place() {
    if (!iconName_.isEmpty() && !qFuzzyCompare(iconDpr_, devicePixelRatioF())) bakeIcon();   // 안전판(B5): 호스트 크기가 바뀔 때 구운 배율 ≠ 지금 배율이면 다시 굽는다
    const int hostW = std::max(160, host_->width() - 2 * kMargin);
    const int withBtns = std::max(160, hostW - reserveRight_);
    hint_->setText(fullHint_);
    title_->setText(fullTitle_); title_->setVisible(true); rule_->setVisible(true);
    adjustSize();
    if (width() > withBtns) {
        const int cut = fullTitle_.indexOf(QStringLiteral(" › "));
        if (cut > 0) { title_->setText(fullTitle_.mid(cut + 3)); adjustSize(); }
    }
    // 되돌리기 칸이 아래로 가면(FloatButtons::place) 칩은 호스트 폭을 다 쓸 수 있다: 단계 이름이 그 폭에 들어가면 지키고, 아니면 이름 · 선을 숨긴다(검토 UI 5)
    if (width() > withBtns && width() > hostW) { title_->setVisible(false); rule_->setVisible(false); adjustSize(); }
    const int maxW = width() <= withBtns ? withBtns : hostW;
    if (width() > maxW) {
        const int over = width() - maxW;
        const QFontMetrics fm(hint_->font());
        const int avail = std::max(40, fm.horizontalAdvance(fullHint_) - over);
        hint_->setText(fm.elidedText(fullHint_, Qt::ElideRight, avail));
        adjustSize();
    }
    if (width() != sizeHint().width()) adjustSize();   // 자식 시트가 늦게 입혀져 권장 폭이 바뀌었으면 한 번 더
    move(kMargin, kMargin);
    raise();
}
QString GuideBand::title() const { return title_->text(); }
bool GuideBand::titleShown() const { return title_->isVisible(); }
QString GuideBand::debugWidths() const { return QStringLiteral("hint=%1/%2 band=%3/%4").arg(hint_->width()).arg(hint_->sizeHint().width()).arg(width()).arg(sizeHint().width()); }
QString GuideBand::hint() const { return hint_->text(); }
qint64 GuideBand::iconCacheKey() const { return icon_->pixmap().cacheKey(); }

bool GuideBand::eventFilter(QObject* watched, QEvent* e) {
    if (watched == host_ && e->type() == QEvent::Resize) place();
    return QFrame::eventFilter(watched, e);
}

// 화면을 옮겨 배율이 바뀌면(QEvent::DevicePixelRatioChange, Qt 6.6+) 같은 아이콘 이름이라도 무조건 새 배율로 다시 굽는다 — 옛 장을 늘리면 흐림(B5)
bool GuideBand::event(QEvent* e) {
    if (e->type() == QEvent::DevicePixelRatioChange) bakeIcon();
    return QFrame::event(e);
}

// ---------------------------------------------------------------- FloatButtons
FloatButtons::FloatButtons(QWidget* host, Qt::Orientation o, Qt::Alignment where, int margin)
    : QFrame(host), host_(host), margin_(margin), where_(where), orient_(o) {
    setObjectName(QStringLiteral("floatButtons"));
    setAttribute(Qt::WA_StyledBackground);
    lay_ = new QBoxLayout(o == Qt::Horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom, this);
    lay_->setContentsMargins(0, 0, 0, 0);
    lay_->setSpacing(0);
    host_->installEventFilter(this);
}

QToolButton* FloatButtons::add(const QString& iconName, const QString& tip, QAction* a, const QString& label) {
    auto* b = new QToolButton(this);
    b->setObjectName(label.isEmpty() ? QStringLiteral("floatBtn") : QStringLiteral("floatCell"));
    b->setIcon(kerf::icon(iconName, label.isEmpty() ? 18 : 22, theme::Hand));
    b->setIconSize(label.isEmpty() ? QSize(18, 18) : QSize(22, 22));
    b->setToolTip(tip);
    b->setFocusPolicy(Qt::NoFocus);
    b->setAutoRaise(true);
    if (label.isEmpty()) b->setFixedSize(36, 36);
    else { b->setText(label); b->setToolButtonStyle(Qt::ToolButtonTextUnderIcon); b->setFixedSize(64, 52); }
    if (!buttons_.isEmpty()) b->setProperty("after", true);   // 앞 단추와 사이 1 px 선(QSS)
    if (a) {
        b->setCheckable(a->isCheckable());
        auto sync = [b, a] { b->setEnabled(a->isEnabled()); if (a->isCheckable()) { QSignalBlocker bl(b); b->setChecked(a->isChecked()); } };
        sync();
        QObject::connect(a, &QAction::changed, b, sync);
        QObject::connect(a, &QAction::toggled, b, sync);
        QObject::connect(b, &QToolButton::clicked, a, [b, a] { if (a->isCheckable()) { QSignalBlocker bl(b); b->setChecked(!b->isChecked()); } a->trigger(); });
    }
    lay_->addWidget(b);
    buttons_ << b;
    adjustSize(); place();
    return b;
}

void FloatButtons::followRightOf(QWidget* w, int gap) { follow_ = w; gap_ = gap; if (w) w->installEventFilter(this); place(); }

void FloatButtons::place() {
    adjustSize();
    int x = margin_, y = margin_;
    if (follow_) {
        x = follow_->x() + follow_->width() + gap_; y = follow_->y();
        if (x + width() > host_->width() - margin_) { x = follow_->x(); y = follow_->y() + follow_->height() + 8; }   // 오른쪽에 자리가 없으면 칩 아래
    } else {
        if (where_ & Qt::AlignRight) x = host_->width() - width() - margin_;
        else if (where_ & Qt::AlignHCenter) x = (host_->width() - width()) / 2;
        if (where_ & Qt::AlignBottom) y = host_->height() - height() - margin_;
    }
    move(x, y);
    raise();
}

bool FloatButtons::eventFilter(QObject* watched, QEvent* e) {
    if ((watched == host_ && e->type() == QEvent::Resize) || (watched == follow_ && (e->type() == QEvent::Resize || e->type() == QEvent::Move || e->type() == QEvent::Show)))
        place();
    return QFrame::eventFilter(watched, e);
}
