// 리본 구현 — Strata KaBeginnerRibbon(hgis/src/app/KaBeginnerRibbon.cpp)의 규칙을 moc 없이 옮김. 치수는 디자인 v5 §3.
#include "ribbon.hpp"

#include <QAction>
#include <QEvent>
#include <QFocusEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

namespace {
constexpr int kRowPadLeft = 8, kRowPadRight = 8;                     // 리본 좌우 여백
constexpr int kGroupPadX = 3, kGroupPadTop = 8, kGroupPadBottom = 10;  // 묶음 안 여백 — 타일 50 일 때 8 + 16 + 4 + 80 + 10 = 118
constexpr int kCaptionHeight = 16, kCaptionGap = 4;
constexpr int kChipGap = 2, kGroupGap = 2;
constexpr int kLabelPad = 8;        // 칩 폭 = 글자 폭 + 8(타일 + 8 보다 작지 않게)
constexpr int kMaxLabelWidth = 64;  // 이보다 넓은 글자는 12 px 까지 줄인다(Strata ribbonMaxLabelWidth)
constexpr int kFontPx = 13, kMinFontPx = 12;

int labelWidth(const QToolButton* b) { return QFontMetrics(b->font()).horizontalAdvance(b->text()); }
}  // namespace

Ribbon::Ribbon(const kerf::ChipColors& colors, QWidget* parent) : QWidget(parent), colors_(colors) {
    setObjectName(QStringLiteral("ribbon"));
    setAttribute(Qt::WA_StyledBackground);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    row_ = new QHBoxLayout(this);
    row_->setContentsMargins(kRowPadLeft, 0, kRowPadRight, 0);
    row_->setSpacing(kGroupGap);
    row_->setSizeConstraint(QLayout::SetNoConstraint);
    row_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    row_->addStretch(1);   // 묶음은 왼쪽에 붙고 남는 폭은 여기로(칩이 넓어지지 않게)
}

QFrame* Ribbon::addGroup(const QString& id, const QString& caption) {
    if (groups_.contains(id)) return groups_.value(id).frame;
    Group g;
    g.frame = new QFrame(this);
    g.frame->setObjectName(QStringLiteral("ribbonGroup"));
    g.frame->setAttribute(Qt::WA_StyledBackground);
    g.frame->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    auto* v = new QVBoxLayout(g.frame);
    v->setContentsMargins(kGroupPadX, kGroupPadTop, kGroupPadX, kGroupPadBottom);
    v->setSpacing(kCaptionGap);
    g.caption = new QLabel(caption, g.frame);
    g.caption->setObjectName(QStringLiteral("ribbonGroupCaption"));
    g.caption->setFixedHeight(kCaptionHeight);
    g.caption->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    g.row = new QHBoxLayout;
    g.row->setContentsMargins(0, 0, 0, 0);
    g.row->setSpacing(kChipGap);
    g.row->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);   // 칩이 아닌 위젯(입면 칸)은 타일 높이 안 세로 가운데
    v->addWidget(g.caption, 0, Qt::AlignLeft);
    v->addLayout(g.row);
    // 마지막 묶음의 오른쪽 선은 QSS [last="true"] 로 뺀다
    if (!order_.isEmpty()) { groups_[order_.last()].frame->setProperty("last", false); }
    g.frame->setProperty("last", true);
    row_->insertWidget(std::max(0, row_->count() - (corner_ ? 2 : 1)), g.frame, 0, Qt::AlignLeft | Qt::AlignTop);   // 묶음 이름은 모두 같은 줄(applyLook 이 묶음 높이를 같게 맞춤)
    groups_.insert(id, g);
    order_.append(id);
    for (const QString& k : order_) { QFrame* f = groups_[k].frame; f->style()->unpolish(f); f->style()->polish(f); }
    return g.frame;
}

QToolButton* Ribbon::addAction(const QString& groupId, QAction* a, kerf::ChipKind kind, const QString& iconName, const QString& label) {
    if (!groups_.contains(groupId) || !a) return nullptr;
    Group& g = groups_[groupId];
    auto* b = new QToolButton(g.frame);
    b->setObjectName(QStringLiteral("ribbonChip"));
    b->setText(label.isEmpty() ? a->iconText() : label);
    b->setCheckable(a->isCheckable());
    b->setFocusPolicy(Qt::TabFocus);
    b->setAutoRaise(true);
    if (kind == kerf::ChipKind::Tool) b->setProperty("tool", true);
    if (kind == kerf::ChipKind::Shown) b->setProperty("shown", true);
    if (kind == kerf::ChipKind::Primary) b->setProperty("primary", true);
    // 동작과 동기화 — setDefaultAction 은 글자 · 아이콘을 동작 것으로 덮으므로 직접 맞춘다(mainwindow_ui.cpp bindButton 과 같음)
    auto sync = [b, a] {
        b->setEnabled(a->isEnabled());
        if (a->isCheckable()) { QSignalBlocker bl(b); b->setChecked(a->isChecked()); }
        b->setToolTip(a->toolTip().isEmpty() ? a->text() : a->toolTip());
    };
    sync();
    QObject::connect(a, &QAction::changed, b, sync);
    QObject::connect(a, &QAction::toggled, b, sync);
    if (a->menu()) { b->setMenu(a->menu()); b->setPopupMode(QToolButton::InstantPopup); }
    else QObject::connect(b, &QToolButton::clicked, a, [b, a] {
        if (a->isCheckable()) { QSignalBlocker bl(b); b->setChecked(!b->isChecked()); }   // 동작 쪽에서 토글한 뒤 sync 가 맞춤
        a->trigger();
    });
    b->installEventFilter(this);
    // 글자 13 px, 너무 넓은 글자만 12 px 까지(앱 스타일시트가 글자 크기를 정하므로 줄일 때는 칩 자기 시트로)
    b->ensurePolished();
    QFont f = b->font(); f.setPixelSize(kFontPx);
    while (f.pixelSize() > kMinFontPx && QFontMetrics(f).horizontalAdvance(b->text()) > kMaxLabelWidth) f.setPixelSize(f.pixelSize() - 1);
    b->setFont(f);
    if (f.pixelSize() < kFontPx) b->setStyleSheet(QStringLiteral("font-size: %1px;").arg(f.pixelSize()));
    g.row->addWidget(b, 0, Qt::AlignLeft | Qt::AlignTop);
    chips_.append({b, kind, iconName, false, a});
    drawChip(chips_.last(), looks().at(lookIndex_));
    updateGeometry();
    return b;
}

// 칩이 아닌 위젯(입면 칸)은 상자에 담아 타일(아이콘) 세로 가운데에 맞춘다 — 위 여백은 applyLook 이 크기마다 다시 준다(검토 UI 2)
void Ribbon::addWidget(const QString& groupId, QWidget* w) {
    if (!groups_.contains(groupId) || !w) return;
    auto* box = new QWidget(groups_[groupId].frame);
    auto* bl = new QVBoxLayout(box); bl->setContentsMargins(0, 0, 0, 0); bl->setSpacing(0); bl->addWidget(w, 0, Qt::AlignTop);
    groups_[groupId].row->addWidget(box, 0, Qt::AlignLeft | Qt::AlignTop);
    widgetBoxes_ << box;
    updateGeometry();
}

void Ribbon::setCorner(QWidget* w) {
    if (corner_) { row_->removeWidget(corner_); corner_->deleteLater(); }
    corner_ = w;
    if (w) row_->addWidget(w, 0, Qt::AlignRight | Qt::AlignVCenter);
    updateGeometry();
}

void Ribbon::setChipOn(QToolButton* b, bool on) {
    for (Chip& c : chips_)
        if (c.button == b && c.forceOn != on) { c.forceOn = on; drawChip(c, look()); return; }
}

void Ribbon::setGroupDim(const QString& id, bool dim) {
    if (!groups_.contains(id)) return;
    QLabel* c = groups_[id].caption;
    if (c->property("dim").toBool() == dim) return;
    c->setProperty("dim", dim);
    c->style()->unpolish(c); c->style()->polish(c);
}

QFrame* Ribbon::group(const QString& id) const { return groups_.contains(id) ? groups_.value(id).frame : nullptr; }

QList<QToolButton*> Ribbon::chips() const {
    QList<QToolButton*> out;
    for (const Chip& c : chips_) out << c.button;
    return out;
}

QList<RibbonLook> Ribbon::looks() {
    static const QList<RibbonLook> table = [] {
        QList<RibbonLook> out;
        for (int tile = 50; tile >= 34; tile -= 2) out.append({tile, qRound(tile * 18 / 32.0), 0, true});
        out.append({32, 18, 0, true});
        out.append({32, 18, 40, false});
        out.append({24, 14, 30, false});
        out.append({20, 12, 26, false});   // 스펙 표: 20 은 아이콘 12(18/32 × 20 = 11 이 아님)
        return out;
    }();
    return table;
}

int Ribbon::chooseLook(const QList<int>& widths, int available) {
    for (int i = 0; i < widths.size(); ++i)
        if (widths.at(i) <= available) return i;
    return std::max(0, int(widths.size()) - 1);
}

RibbonLook Ribbon::look() const { return looks().at(lookIndex_); }

int Ribbon::chipWidth(const RibbonLook& L, const QToolButton* b) const {
    if (!L.labels) return L.chipWidth;
    return std::max(L.tile + 6, std::min(labelWidth(b), kMaxLabelWidth) + kLabelPad);   // 바닥은 타일 + 6(Strata 는 + 8 — 묶음 7 + 입면 위젯이 1920 에 들어가게 2 px 좁힘)
}

int Ribbon::chipHeight(const RibbonLook& L) const { return L.tile + 12 + (L.labels ? lineHeight() : 0); }

int Ribbon::lineHeight() const {
    int line = 0;
    for (const Chip& c : chips_) line = std::max(line, QFontMetrics(c.button->font()).height());
    return std::max(18, line);   // 글자 줄은 18 이상 — 타일 50 일 때 리본 118(스펙 §2)
}

QIcon Ribbon::chipIconCached(const Chip& c, const RibbonLook& L) {
    const bool caret = c.action && c.action->menu();   // 메뉴가 열리는 칩은 타일 오른쪽 아래 ▾(스펙 §3.1 — 글자에 붙이면 폭이 늘어 1920 에서 타일 50 을 잃음)
    const QString key = QStringLiteral("%1|%2|%3|%4|%5").arg(c.icon).arg(L.tile).arg(int(c.kind)).arg(c.forceOn ? 1 : 0).arg(caret ? 1 : 0);
    auto it = iconCache_.find(key);
    if (it != iconCache_.end()) return it.value();
    QIcon src = kerf::chipIcon(c.icon, L.tile, L.glyph, c.forceOn ? kerf::ChipKind::Shown : c.kind, colors_, caret);
    if (!c.forceOn) return *iconCache_.insert(key, src);
    // 체크할 수 없는 칩이라 Off 상태로 그려진다 → On 모양을 Off 자리에 넣는다
    QIcon ic;
    for (auto mode : {QIcon::Normal, QIcon::Active, QIcon::Disabled, QIcon::Selected})
        for (qreal d : {1.0, 1.5, 2.0, 3.0}) ic.addPixmap(src.pixmap(QSize(L.tile, L.tile), d, mode, QIcon::On), mode, QIcon::Off);
    return *iconCache_.insert(key, ic);
}

void Ribbon::drawChip(const Chip& c, const RibbonLook& L) {
    QToolButton* b = c.button;
    b->setToolButtonStyle(L.labels ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
    b->setIcon(chipIconCached(c, L));
    b->setIconSize(QSize(L.tile, L.tile));
    b->setFixedSize(chipWidth(L, b), chipHeight(L));   // 칩 자기 크기 — 남는 창 폭을 칩이 받지 않는다
}

// 크기마다 필요한 폭: 묶음마다 max(이름 폭, 칩들 + 위젯들 나란히) + 묶음 여백 · 선, 오른쪽 끝 묶음, 리본 여백
QString Ribbon::chipIconName(const QAbstractButton* b) const {
    for (const Chip& c : chips_) if (c.button == b) return c.icon;
    return QString();
}

QList<int> Ribbon::lookWidths() const {
    const QList<RibbonLook> all = looks();
    QList<int> widths(all.size(), kRowPadLeft + kRowPadRight + (corner_ ? corner_->sizeHint().width() + row_->spacing() : 0));
    for (const QString& id : order_) {
        const Group& g = groups_.value(id);
        QList<int> inner(all.size(), 0);
        int count = 0;
        for (int i = 0; i < g.row->count(); ++i) {
            QWidget* w = g.row->itemAt(i)->widget();
            if (!w || (w->isHidden() && w->testAttribute(Qt::WA_WState_ExplicitShowHide))) continue;
            ++count;
            const Chip* chip = nullptr;
            for (const Chip& c : chips_) if (c.button == w) { chip = &c; break; }
            for (int k = 0; k < all.size(); ++k) inner[k] += chip ? chipWidth(all.at(k), chip->button) : w->sizeHint().width();
        }
        const int captionW = g.caption->sizeHint().width();
        for (int k = 0; k < all.size(); ++k)
            widths[k] += std::max(captionW, inner.at(k) + std::max(0, count - 1) * g.row->spacing()) + 2 * kGroupPadX + 1 + row_->spacing();
    }
    return widths;
}

QString Ribbon::widthReport() const {
    const RibbonLook L = looks().first();
    QStringList out;
    for (const QString& id : order_) {
        const Group& g = groups_.value(id);
        int chipsW = 0, widgetsW = 0;
        for (int i = 0; i < g.row->count(); ++i) {
            QWidget* w = g.row->itemAt(i)->widget();
            if (!w) continue;
            const Chip* chip = nullptr;
            for (const Chip& ch : chips_) if (ch.button == w) { chip = &ch; break; }
            if (chip) chipsW += chipWidth(L, chip->button); else widgetsW += w->sizeHint().width();
        }
        out << QStringLiteral("%1:%2/%3/%4").arg(id).arg(g.caption->sizeHint().width()).arg(chipsW).arg(widgetsW);
    }
    out << QStringLiteral("corner:%1").arg(corner_ ? corner_->sizeHint().width() : 0);
    return out.join(QLatin1Char(' '));
}

QSize Ribbon::sizeHint() const {
    const RibbonLook L = look();
    return QSize(lookWidths().value(lookIndex_), kGroupPadTop + kCaptionHeight + kCaptionGap + chipHeight(L) + kGroupPadBottom);
}

QSize Ribbon::minimumSizeHint() const {
    const RibbonLook L = looks().last();
    return QSize(lookWidths().last(), kGroupPadTop + kCaptionHeight + kCaptionGap + chipHeight(L) + kGroupPadBottom);
}

void Ribbon::resizeEvent(QResizeEvent* e) { QWidget::resizeEvent(e); updateLook(); }
void Ribbon::showEvent(QShowEvent* e) { QWidget::showEvent(e); updateLook(); }

bool Ribbon::eventFilter(QObject* watched, QEvent* e) {
    if (e && (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut)) {   // 초점 테는 Tab 으로 왔을 때만(QSS [kbdFocus="true"]:focus — 검토 UI 5)
        for (const Chip& c : chips_)
            if (c.button == watched) {
                const Qt::FocusReason r = static_cast<const QFocusEvent*>(e)->reason();
                if (r == Qt::ActiveWindowFocusReason || r == Qt::PopupFocusReason) break;   // 창 전환 · 팝업은 이전 값 유지(검토 M8)
                const bool kbd = e->type() == QEvent::FocusIn && (r == Qt::TabFocusReason || r == Qt::BacktabFocusReason);
                if (c.button->property("kbdFocus").toBool() != kbd) { c.button->setProperty("kbdFocus", kbd); c.button->style()->unpolish(c.button); c.button->style()->polish(c.button); }
                break;
            }
    }
    if (e && e->type() == QEvent::KeyPress) {
        const auto* k = static_cast<const QKeyEvent*>(e);
        if (k->key() == Qt::Key_Return || k->key() == Qt::Key_Enter) {
            for (const Chip& c : chips_)
                if (c.button == watched && c.button->isEnabled()) { c.button->animateClick(); return true; }
        }
    }
    return QWidget::eventFilter(watched, e);
}

void Ribbon::updateLook() {
    if (updating_) return;
    updating_ = true;
    applyLook(chooseLook(lookWidths(), width()));
    updating_ = false;
}

void Ribbon::applyLook(int index, bool force) {
    const int line = lineHeight();
    if (index == lookIndex_ && line == appliedLine_ && !force) return;
    if (index != lookIndex_) iconCache_.clear();   // 지금 크기 것만 남긴다(검토 I7: 12단계 × 칩 21 × 32장이 다 쌓이면 수백 MB)
    lookIndex_ = index;
    appliedLine_ = line;
    const RibbonLook L = looks().at(index);
    for (const Chip& c : chips_) drawChip(c, L);
    const int gh = kGroupPadTop + kCaptionHeight + kCaptionGap + chipHeight(L) + kGroupPadBottom;   // 묶음 높이를 같게 — 입면 묶음 이름이 내려가지 않음(검토 UI 2)
    for (const QString& id : order_) groups_[id].frame->setFixedHeight(gh);
    for (QWidget* box : widgetBoxes_) {   // 위젯 가운데 = 타일 가운데(칩 위 여백 6 + 타일/2)
        QWidget* inner = box->layout()->count() ? box->layout()->itemAt(0)->widget() : nullptr;
        const int ih = inner ? inner->sizeHint().height() : 0;
        box->layout()->setContentsMargins(0, std::max(0, 6 + L.tile / 2 - ih / 2), 0, 0);
        box->setFixedHeight(chipHeight(L));
    }
    setFixedHeight(sizeHint().height());
    updateGeometry();
}
