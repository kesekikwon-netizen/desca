// 리본 구현 — Strata KaBeginnerRibbon(hgis/src/app/KaBeginnerRibbon.cpp)의 규칙을 moc 없이 옮김. 치수는 디자인 v5 §3.
#include "ribbon.hpp"

#include <QAction>
#include <QEvent>
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
constexpr int kRowPadLeft = 8, kRowPadRight = 12;                     // 리본 좌우 여백
constexpr int kGroupPadX = 4, kGroupPadTop = 8, kGroupPadBottom = 10;  // 묶음 안 여백 — 타일 50 일 때 8 + 16 + 4 + 80 + 10 = 118
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
    g.row->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    v->addWidget(g.caption, 0, Qt::AlignLeft);
    v->addLayout(g.row);
    // 마지막 묶음의 오른쪽 선은 QSS [last="true"] 로 뺀다
    if (!order_.isEmpty()) { groups_[order_.last()].frame->setProperty("last", false); }
    g.frame->setProperty("last", true);
    row_->insertWidget(std::max(0, row_->count() - (corner_ ? 2 : 1)), g.frame, 0, Qt::AlignLeft | Qt::AlignVCenter);
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
    chips_.append({b, kind, iconName});
    drawChip(chips_.last(), looks().at(lookIndex_));
    updateGeometry();
    return b;
}

void Ribbon::addWidget(const QString& groupId, QWidget* w) {
    if (!groups_.contains(groupId) || !w) return;
    groups_[groupId].row->addWidget(w, 0, Qt::AlignLeft | Qt::AlignVCenter);
    updateGeometry();
}

void Ribbon::setCorner(QWidget* w) {
    if (corner_) { row_->removeWidget(corner_); corner_->deleteLater(); }
    corner_ = w;
    if (w) row_->addWidget(w, 0, Qt::AlignRight | Qt::AlignVCenter);
    updateGeometry();
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
    return std::max(L.tile + 8, std::min(labelWidth(b), kMaxLabelWidth) + kLabelPad);
}

int Ribbon::chipHeight(const RibbonLook& L) const { return L.tile + 12 + (L.labels ? lineHeight() : 0); }

int Ribbon::lineHeight() const {
    int line = 0;
    for (const Chip& c : chips_) line = std::max(line, QFontMetrics(c.button->font()).height());
    return line;
}

QIcon Ribbon::chipIconCached(const Chip& c, const RibbonLook& L) {
    const QString key = QStringLiteral("%1|%2|%3").arg(c.icon).arg(L.tile).arg(int(c.kind));
    auto it = iconCache_.find(key);
    if (it == iconCache_.end()) it = iconCache_.insert(key, kerf::chipIcon(c.icon, L.tile, L.glyph, c.kind, colors_));
    return it.value();
}

void Ribbon::drawChip(const Chip& c, const RibbonLook& L) {
    QToolButton* b = c.button;
    b->setToolButtonStyle(L.labels ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
    b->setIcon(chipIconCached(c, L));
    b->setIconSize(QSize(L.tile, L.tile));
    b->setFixedSize(chipWidth(L, b), chipHeight(L));   // 칩 자기 크기 — 남는 창 폭을 칩이 받지 않는다
}

// 크기마다 필요한 폭: 묶음마다 max(이름 폭, 칩들 + 위젯들 나란히) + 묶음 여백 · 선, 오른쪽 끝 묶음, 리본 여백
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
    lookIndex_ = index;
    appliedLine_ = line;
    const RibbonLook L = looks().at(index);
    for (const Chip& c : chips_) drawChip(c, L);
    setFixedHeight(sizeHint().height());
    updateGeometry();
}
