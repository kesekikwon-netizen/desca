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
    auto* rule = new QFrame; rule->setObjectName(QStringLiteral("guideRule")); rule->setFixedSize(1, 16);
    hint_ = new QLabel; hint_->setObjectName(QStringLiteral("guideHint"));
    keysBox_ = new QWidget; keysLay_ = new QHBoxLayout(keysBox_); keysLay_->setContentsMargins(0, 0, 0, 0); keysLay_->setSpacing(4);
    h->addWidget(icon_); h->addWidget(title_); h->addWidget(rule); h->addWidget(hint_); h->addWidget(keysBox_);
    host_->installEventFilter(this);
    hide();
}

void GuideBand::setTool(const QString& iconName, const QString& title) {
    icon_->setPixmap(kerf::glyph(iconName, 16, theme::Hand, devicePixelRatioF()));
    title_->setText(title);
    adjustSize(); place();
}

void GuideBand::setHint(const QString& text) { fullHint_ = text; hint_->setText(text); adjustSize(); place(); }

void GuideBand::setKeys(const QStringList& keys) {
    for (QLabel* k : keyLabels_) { keysLay_->removeWidget(k); k->deleteLater(); }
    keyLabels_.clear();
    for (const QString& k : keys) { auto* l = new QLabel(k); l->setObjectName(QStringLiteral("kbd")); keysLay_->addWidget(l); keyLabels_ << l; }
    adjustSize(); place();
}

// 칩은 호스트 폭 안에 들어간다: 문장이 길면 줄임표. 그래야 되돌리기 · 다시가 오른쪽에 붙을 자리가 남는다
void GuideBand::place() {
    const int maxW = std::max(160, host_->width() - 2 * kMargin - reserveRight_);
    hint_->setText(fullHint_);
    adjustSize();
    if (width() > maxW) {
        const int over = width() - maxW;
        const QFontMetrics fm(hint_->font());
        const int avail = std::max(40, fm.horizontalAdvance(fullHint_) - over);
        hint_->setText(fm.elidedText(fullHint_, Qt::ElideRight, avail));
        adjustSize();
    }
    move(kMargin, kMargin);
    raise();
}
QString GuideBand::title() const { return title_->text(); }
QString GuideBand::hint() const { return hint_->text(); }

bool GuideBand::eventFilter(QObject* watched, QEvent* e) {
    if (watched == host_ && e->type() == QEvent::Resize) place();
    return QFrame::eventFilter(watched, e);
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
    if (follow_) { x = follow_->x() + follow_->width() + gap_; y = follow_->y(); }
    else {
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
