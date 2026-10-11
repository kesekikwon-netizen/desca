// 1.2 화면 구성: 리본·지금 도구 줄·알림 띠·보기 머리·정보 띠·상태줄·시작 화면·단면 목록, 되돌리기, 최근 파일, 모델별 상태.
#include "mainwindow.hpp"
#include "ribbon.hpp"
#include "guideband.hpp"
#include <QWidgetAction>

#include <QApplication>
#include <QClipboard>
#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QPainterPath>
#include <QRegularExpression>
#include <QFocusEvent>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QSet>
#include <QStyleHints>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QProgressBar>
#include <QPushButton>
#include <QScreen>
#include <QScopeGuard>
#include <QSettings>
#include <QShortcut>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTabBar>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>
#include <cmath>

using namespace asec;
extern const char* const kVersion;

static QString qs8(const std::string& s) { return QString::fromUtf8(s.c_str()); }
QString MainWindow::modelKey(const QString& path) {
    QByteArray h = QCryptographicHash::hash(QFileInfo(path).absoluteFilePath().toLower().toUtf8(), QCryptographicHash::Sha1).toHex().left(16);
    return QStringLiteral("model/") + QString::fromLatin1(h) + "/";
}

namespace {

// 단추 ↔ 동작 묶기(setDefaultAction 은 동작이 바뀔 때마다 글자·아이콘을 동작 것으로 덮으므로 직접 동기화)
void bindButton(QToolButton* b, QAction* a) {
    b->setCheckable(a->isCheckable());
    auto sync = [b, a] {
        b->setEnabled(a->isEnabled());
        if (a->isCheckable()) { QSignalBlocker bl(b); b->setChecked(a->isChecked()); }
        b->setToolTip(a->toolTip());
    };
    sync();
    QObject::connect(a, &QAction::changed, b, sync);
    QObject::connect(a, &QAction::toggled, b, sync);
    if (a->menu()) { b->setMenu(a->menu()); b->setPopupMode(QToolButton::InstantPopup); }
    else QObject::connect(b, &QToolButton::clicked, a, [b, a] {
        if (a->isCheckable()) { QSignalBlocker bl(b); b->setChecked(!b->isChecked()); }   // 동작 쪽에서 토글 후 sync 가 맞춤
        a->trigger();
    });
}

// 작은 줄: 아이콘 + 글자 + 오른쪽 키 칩
QWidget* smallRow(QAction* a, const QString& text, const QString& key) {
    auto* w = new QWidget; auto* h = new QHBoxLayout(w); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(4);
    auto* b = new QToolButton;
    b->setIcon(a->icon());
    b->setText(text);
    b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    b->setIconSize(QSize(16, 16));
    b->setFocusPolicy(Qt::NoFocus);
    b->setAutoRaise(true);
    b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    bindButton(b, a);
    h->addWidget(b, 1);
    if (!key.isEmpty()) { auto* k = new QLabel(key); k->setObjectName("kbd"); h->addWidget(k); }
    QObject::connect(a, &QAction::changed, w, [w, a] { w->setEnabled(a->isEnabled()); });
    return w;
}

QToolButton* iconBtn(QAction* a) {
    auto* b = new QToolButton; b->setDefaultAction(a); b->setToolButtonStyle(Qt::ToolButtonIconOnly); b->setIconSize(QSize(16, 16));
    b->setAutoRaise(true); b->setFocusPolicy(Qt::NoFocus);
    return b;
}

QLabel* kbd(const QString& t) { auto* l = new QLabel(t); l->setObjectName("kbd"); return l; }
QLabel* lab(const QString& t, const char* obj = nullptr) { auto* l = new QLabel(t); if (obj) l->setObjectName(obj); return l; }

// --ui-audit 도움 판정(1280 좁은 창 spec §3-A): bar 의 보이는 직계 자식이 (a) 서로 겹침 (b) bar 밖으로 나감 (c) 자기 최소 폭보다 짓눌림 — 그 수의 합
int overlapCount(QWidget* bar) {
    if (!bar) return 0;
    QList<QWidget*> kids;
    for (QObject* o : bar->children())
        if (o->isWidgetType()) { auto* w = static_cast<QWidget*>(o); if (w->isVisibleTo(bar)) kids << w; }
    int n = 0;
    for (int i = 0; i < kids.size(); ++i) {
        QWidget* w = kids[i];
        const QRect g = w->geometry();
        for (int j = i + 1; j < kids.size(); ++j) if (g.intersects(kids[j]->geometry())) ++n;
        if (!bar->rect().contains(g)) ++n;
        const int need = w->inherits("QLabel") ? w->sizeHint().width() : (w->minimumWidth() > 0 ? w->minimumWidth() : w->minimumSizeHint().width());
        if (w->width() < need) ++n;
    }
    return n;
}
// root 아래 보이는 QLabel 중 글이 잘린 수: ① 줄바꿈 없이 폭 < sizeHint(짓눌림) ② 전체 글(동적 속성 fullText)이 있는데 보이는 글이 비었거나 「…」만 남았거나
// 폭이 「…」 한 자보다 좁음(줄임표 라벨은 sizeHint 가 줄인 글을 따라 ①에 안 걸린다 — rework 2 code S1: 「<모델> 기준」이 폭 0 으로 사라진 것을 놓쳤다).
// tipMissing = 줄인 글(fullText 와 다름)인데 툴팁이 전체 글이 아닌 수. names = 잘린 라벨의 글(앞 16자, 최대 6개 — 진단용)
int clippedLabels(QWidget* root, int* tipMissing, QStringList* names = nullptr) {
    int n = 0;
    if (!root) return n;
    const QString ell(QChar(0x2026));
    for (QLabel* l : root->findChildren<QLabel*>()) {
        if (!l->isVisibleTo(root)) continue;
        const QVariant full = l->property("fullText");
        const QString fullS = full.isValid() ? full.toString() : QString();
        const QString shown = l->text().trimmed();
        const bool squeezed = !l->wordWrap() && l->width() < l->sizeHint().width();
        const bool gone = !fullS.trimmed().isEmpty() && (shown.isEmpty() || shown == ell || l->width() < l->fontMetrics().horizontalAdvance(ell));
        if (squeezed || gone) { ++n; if (names && names->size() < 6) *names << (fullS.isEmpty() ? l->text() : fullS).left(16); }
        if (tipMissing && full.isValid() && l->text() != fullS && l->toolTip() != fullS) ++*tipMissing;
    }
    return n;
}
// 라벨 글을 width(px)에 맞게 줄임표로(QFontMetrics::elidedText — 넓으면 "…", 아니면 원래 글). 동적 속성 fullText = 전체 글(full 이 비면 text),
// 줄였으면 툴팁 = 전체 글, 아니면 빈 툴팁 — 감사 clippedLabels 의 elide-tip-missing 이 읽는다. 머리 제목 · 정보 줄 · 홈 한 줄 글이 같이 쓴다
// 정보 줄 「잘린 선 n줄 · 빈 구간 …」의 빈 구간 꼬리(첫 「 · 」 뒤, 없으면 전체) — 좁은 창에서도 지켜야 하는 부분(스펙 §4 · Q2 A). fitInfoStrip 과 감사 gap-kept 가 같이 쓴다
QString stripGapTail(const QString& cutShort) {
    const int i = cutShort.indexOf(QStringLiteral(" · "));
    return i < 0 ? cutShort : cutShort.mid(i + 3);
}
void setElidedText(QLabel* l, const QString& text, Qt::TextElideMode mode, int width, const QString& full = QString()) {
    const QString whole = full.isEmpty() ? text : full;
    const QString shown = l->fontMetrics().elidedText(text, mode, std::max(0, width));
    l->setProperty("fullText", whole);
    l->setText(shown);
    l->setToolTip(shown == whole ? QString() : whole);
}
// 홈의 한 줄 글(스펙 §7): 가로 Ignored(창 최소 폭을 밀지 않음) + 동적 속성 elideMode — 앱 전체 필터가 Resize 때 contentsRect 폭에 맞게 줄임표 · 툴팁(경로 · 이름 = 가운데, 문장 = 끝)
QLabel* elideLab(QLabel* l, Qt::TextElideMode mode) {
    l->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    l->setProperty("elideMode", int(mode));
    l->setProperty("fullText", l->text());
    return l;
}
// 이어서 작업 칸 줄(스펙 §7): 폭이 넉넉하면 옛 배치 그대로 폴더 2 : 나머지 1(rework 3 ⓐ — 1920 이상 모양 그대로). 그 비율에서 칸 이름이나
// 칸 1–5 값이 줄임표가 될 때만(실제 글 폭으로 판단 — 창 폭 상수가 아님) 좁은 창 규칙: 칸 이름은 줄이지 않고, 칸 1–5(마지막 열림 · 수평 좌표계 · 높이 · 단면 · 도면)는 내용 폭,
// 폴더 칸만 남는 폭을 받아 늘고 줄며 가운데 줄임표. 폴더 칸이 「C:…」도 못 담으면 칸 안 여백을 12 → 8 → 4 로 줄이고(값은 그대로),
// 그래도 모자라면 좌표계 · 높이 값만 앞쪽부터 줄인다(번호 끝자리를 지킴 — 「…5186」, rework 2 UI B1 · correctness C4). ccCells 는 가로 Ignored 라 이 폭들이 창 최소 폭을 밀지 않는다
void fitHomeCells(QWidget* cells) {
    struct C { QWidget* w; QLabel* cap; QLabel* val; };
    QList<C> cs;
    for (QObject* o : cells->children()) {
        if (!o->isWidgetType()) continue;
        auto* w = static_cast<QWidget*>(o);
        QLayout* cl = w->layout();
        if (!cl || cl->count() < 2 || !cl->itemAt(0)->widget() || !cl->itemAt(1)->widget() || !cl->itemAt(0)->widget()->inherits("QLabel") || !cl->itemAt(1)->widget()->inherits("QLabel")) continue;
        cs.push_back({w, static_cast<QLabel*>(cl->itemAt(0)->widget()), static_cast<QLabel*>(cl->itemAt(1)->widget())});
    }
    if (cs.size() < 2) return;
    auto textW = [](QLabel* l) { l->ensurePolished(); const QVariant f = l->property("fullText"); return l->fontMetrics().horizontalAdvance(f.isValid() ? f.toString() : l->text()); };
    const QString ell(QChar(0x2026));
    const int avail = cells->width();
    QVector<int> width(cs.size());
    {   // 옛 비율(폴더 2 : 나머지 1) — 칸 이름 전부와 칸 1–5 값이 여백 12 로 다 들어가고 폴더가 「C:…」를 담으면 그대로 쓴다(+1 px: 정수 글 폭 반올림 보험)
        int wsum = 0; for (int i = 0; i < cs.size(); ++i) wsum += i == 0 ? 2 : 1;
        int cum = 0, prevX = 0; bool fits = true;
        for (int i = 0; i < cs.size(); ++i) {
            cum += i == 0 ? 2 : 1;
            const int x = int(std::lround(double(avail) * cum / wsum));
            width[i] = x - prevX; prevX = x;
            const int need = i == 0 ? std::max(textW(cs[0].cap), cs[0].val->fontMetrics().horizontalAdvance(QStringLiteral("C:") + ell)) : std::max(textW(cs[i].cap), textW(cs[i].val));
            if (need + 2 * 12 + 1 > width[i]) fits = false;
        }
        if (fits) {
            for (int i = 0; i < cs.size(); ++i) { cs[i].w->layout()->setContentsMargins(12, 6, 12, 6); cs[i].w->setFixedWidth(width[i]); }
            return;
        }
    }
    int usePad = 4;
    bool full = false;
    for (int pad : {12, 8, 4}) {   // 칸 1–5 를 내용 폭 그대로 두고 폴더가 「C:…」를 담는 가장 큰 여백
        int sum = 0;
        for (int i = 1; i < cs.size(); ++i) { width[i] = std::max(textW(cs[i].cap), textW(cs[i].val)) + 2 * pad; sum += width[i]; }
        const int folderMin = std::max(textW(cs[0].cap), cs[0].val->fontMetrics().horizontalAdvance(QStringLiteral("C:") + ell)) + 2 * pad;
        usePad = pad;
        if (avail - sum >= folderMin) { full = true; break; }
    }
    if (!full) {   // 여백 4 로도 모자람: 좌표계 · 높이(elideCode) 값만 「…」 + 끝 4자까지 줄인다
        const int folderMin = std::max(textW(cs[0].cap), cs[0].val->fontMetrics().horizontalAdvance(QStringLiteral("C:") + ell)) + 2 * usePad;
        int sum = 0; for (int i = 1; i < cs.size(); ++i) sum += width[i];
        int deficit = folderMin - (avail - sum);
        for (int i = 1; i < cs.size() && deficit > 0; ++i) {
            if (!cs[i].val->property("elideCode").toBool()) continue;
            const QVariant f = cs[i].val->property("fullText");
            const QString s = f.isValid() ? f.toString() : cs[i].val->text();
            const int minW = std::max(textW(cs[i].cap), cs[i].val->fontMetrics().horizontalAdvance(ell + s.right(4))) + 2 * usePad;
            const int cut = std::min(deficit, std::max(0, width[i] - minW));
            width[i] -= cut; deficit -= cut;
        }
    }
    int used = 0;
    for (int i = 1; i < cs.size(); ++i) used += width[i];
    width[0] = std::max(0, avail - used);
    for (int i = 0; i < cs.size(); ++i) { cs[i].w->layout()->setContentsMargins(usePad, 6, usePad, 6); cs[i].w->setFixedWidth(width[i]); }
}
// 홈 「처음 쓰는 키」 줄(스펙 §7): 폭이 모자라면 뒤 키 묶음부터 숨긴다(Ctrl+Z → Ctrl+P → O → H → 1–5 → S). 「처음 쓰는 키」 머리와 「모든 키 F1」은 남긴다
void fitHomeKeys(QWidget* keys) {
    QLayout* lay = keys->layout();
    if (!lay) return;
    QList<QWidget*> groups;
    for (int i = 0; i < lay->count(); ++i) if (QWidget* w = lay->itemAt(i)->widget()) if (w->property("keyGroup").toBool()) groups << w;
    for (QWidget* g : groups) g->setVisible(true);
    lay->activate();
    for (int i = groups.size() - 1; i >= 0 && lay->minimumSize().width() > keys->width(); --i) { groups[i]->setVisible(false); lay->activate(); }
}

QWidget* vbox(std::initializer_list<QWidget*> ws, int spacing = 3) {
    auto* w = new QWidget; auto* l = new QVBoxLayout(w); l->setContentsMargins(0, 0, 0, 0); l->setSpacing(spacing);
    for (auto* x : ws) l->addWidget(x);
    l->addStretch();
    return w;
}

// 되돌리기 명령: 바뀌기 전·후 스냅숏. 같은 id(두께 칸)를 1.5 초 안에 연달아 바꾸면 하나로 합침
struct StateCmd : QUndoCommand {
    MainWindow* w; MainWindow::ViewState a, b; int mid; bool first = true; qint64 t;
    StateCmd(MainWindow* w_, const MainWindow::ViewState& a_, const MainWindow::ViewState& b_, const QString& text, int id_)
        : w(w_), a(a_), b(b_), mid(id_), t(QDateTime::currentMSecsSinceEpoch()) { setText(text); }
    void undo() override { w->applyState(a); }
    void redo() override { if (first) { first = false; return; } w->applyState(b); }
    int id() const override { return mid; }
    bool mergeWith(const QUndoCommand* o) override {
        auto* c = static_cast<const StateCmd*>(o);
        if (c->mid != mid || c->t - t > 1500) return false;
        b = c->b; t = c->t;
        return true;
    }
};
}  // namespace

bool MainWindow::ViewState::operator==(const ViewState& o) const {
    auto eqL = [](const SectionLine& x, const SectionLine& y) { return x.a.x == y.a.x && x.a.y == y.a.y && x.b.x == y.b.x && x.b.y == y.b.y; };
    return hasLine == o.hasLine && (!hasLine || eqL(line, o.line)) && front == o.front && back == o.back && image == o.image && profile == o.profile &&
           levels == o.levels && ticks == o.ticks && fade == o.fade && smooth == o.smooth && current == o.current;
}

QAction* MainWindow::makeAction(const QString& key, const QString& ko, const QString& en, theme::Ico ico, const QString& shortcut, bool checkable) {
    auto* a = new QAction(theme::icon(ico), ko, this);
    a->setProperty("iconName", QString::fromLatin1(theme::iconName(ico)));   // --ui-audit 「같은 아이콘 · 다른 일」 검사용
    a->setCheckable(checkable);
    if (!shortcut.isEmpty()) { a->setShortcut(QKeySequence(shortcut)); a->setShortcutContext(Qt::ApplicationShortcut); }
    a->setToolTip(shortcut.isEmpty() ? QStringLiteral("%1  (%2)").arg(ko, en) : QStringLiteral("%1  (%2)  [%3]").arg(ko, en, shortcut));
    a->setIconText(ko);
    act_[key] = a;
    addAction(a);
    return a;
}

// ---------------------------------------------------------------- 리본(디자인 v5 §3: 탭 없음 · 묶음 7 · 타일 칩 — 오른쪽 끝 단추 칸 없음, T5)
QWidget* MainWindow::buildRibbon() {
    using K = kerf::ChipKind;
    ribbon_ = new Ribbon(theme::chipColors());
    auto add = [this](const char* g, const char* key, K kind, const char* icon, const QString& label) {
        return ribbon_->addAction(QString::fromLatin1(g), action(key), kind, QString::fromLatin1(icon), label);
    };
    auto rowOf = [](std::initializer_list<QWidget*> ws) {
        auto* w = new QWidget; auto* l = new QHBoxLayout(w); l->setContentsMargins(0, 0, 0, 0); l->setSpacing(2);
        for (auto* x : ws) l->addWidget(x, 0, Qt::AlignVCenter);
        return w;
    };
    // ---- 모델: 열기 · 최근 ▾ · 닫기
    ribbon_->addGroup(QStringLiteral("model"), QStringLiteral("모델"));
    recentMenu_ = new QMenu(this);
    action("recent")->setMenu(recentMenu_);
    add("model", "open", K::Normal, "folder", QStringLiteral("열기"));
    add("model", "recent", K::Normal, "clock", QStringLiteral("최근"));
    { auto* pm = new QMenu(this); pm->addAction(action("projsave")); pm->addAction(action("projsaveas")); pm->addSeparator(); pm->addAction(action("projopen")); action("project")->setMenu(pm); }
    add("model", "project", K::Normal, "save", QStringLiteral("프로젝트"));   // 아이콘은 목록 파일(sections-list)과 달라야 함(--ui-audit dup-icons)   // 작업 상태 파일(.trench.json) — 자동 저장은 늘, 여기는 손으로
    add("model", "close", K::Normal, "x", QStringLiteral("닫기"));
    // ---- 단면: 긋기(도구) · 새 단면 · 반전 · 이동 ▾ · 잘린 돌(도구, F단계) · 윤곽(도구, G단계)
    ribbon_->addGroup(QStringLiteral("sec"), QStringLiteral("단면"));
    add("sec", "draw", K::Tool, "section-line", QStringLiteral("긋기"));
    add("sec", "addsec", K::Normal, "plus", QStringLiteral("새 단면"));
    add("sec", "flip", K::Normal, "flip", QStringLiteral("반전"));
    add("sec", "move", K::Normal, "move", QStringLiteral("이동"));
    add("sec", "hatch", K::Tool, "hatch", QStringLiteral("잘린 돌"));
    add("sec", "outline", K::Tool, "outline", QStringLiteral("윤곽"));
    // ---- 입면: 뒤 깊이 칩(숫자키 1–5) · 뒤 m · 앞 m
    ribbon_->addGroup(QStringLiteral("elev"), QStringLiteral("입면"));
    front_ = new QDoubleSpinBox; back_ = new QDoubleSpinBox;
    front_->setObjectName(QStringLiteral("frontSpin")); back_->setObjectName(QStringLiteral("backSpin"));   // 명령줄 점검(--sheet-back)용 이름
    for (auto* sp : {front_, back_}) { sp->setRange(0, kMaxBandDepth); sp->setDecimals(2); sp->setSingleStep(0.05); sp->setSuffix(" m"); sp->setFixedWidth(56); sp->setFixedHeight(24); sp->setAlignment(Qt::AlignRight); }
    front_->setToolTip(QStringLiteral("단면선 앞쪽(보는 사람 쪽) 두께, 0–5 m"));
    back_->setToolTip(QStringLiteral("단면선 뒤쪽(보는 방향) 깊이 — 입면 영상(배경)에 보이는 깊이, 0–5 m, 기본 3 m(입면도용). 숫자키 1–5 = 0.5 / 1 / 2 / 3 / 5 m"));
    auto* chips = new QWidget; { auto* h = new QHBoxLayout(chips); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(2);
        const double vals[5] = {0.5, 1, 2, 3, 5};   // 칩 = 숫자키 1–5. 3 m 가 기본(입면도)
        for (int i = 0; i < 5; ++i) {
            auto* c = new QToolButton; c->setObjectName("chip"); c->setCheckable(true); c->setFocusPolicy(Qt::NoFocus);
            c->setText(QString::number(vals[i], 'g', 2)); c->setFixedSize(32, 24); c->setProperty("tight", true);
            c->setToolTip(QStringLiteral("뒤 깊이 %1 m (숫자키 %2)%3").arg(vals[i], 0, 'g', 2).arg(i + 1).arg(i == 3 ? QStringLiteral(" — 기본, 입면도용") : QString()));
            if (i == 0) c->setProperty("pos", "first");
            if (i == 4) c->setProperty("pos", "last");
            double v2 = vals[i];
            QObject::connect(c, &QToolButton::clicked, this, [this, v2] { setBackDepth(v2); });
            depthChip_[i] = c; h->addWidget(c);
        } }
    depthBox_ = new QWidget; { auto* h = new QHBoxLayout(depthBox_); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(4);
        h->addWidget(chips);
        h->addWidget(lab(QStringLiteral("뒤"), "hint")); h->addWidget(back_);
        h->addWidget(lab(QStringLiteral("앞"), "hint")); h->addWidget(front_); }
    depthBox_->setToolTip(QStringLiteral("입면 배경(뒤 깊이) · 숫자키 1–5, 앞 두께"));
    // 단면선 굵기 칩 셋(아래 줄 · 더보기 공용 모양). 어느 쪽을 눌러도 setSectionLineWidth 가 모든 칩 · 설정 · 그리기를 맞춘다
    auto lineWidthChips = [this](bool compact) {
        auto* w = new QWidget; auto* h = new QHBoxLayout(w); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(compact ? 2 : 0);
        const double ws[3] = {1.5, 2.0, 3.0};
        const double cur = QSettings().value("view/sectionLineWidth", 2.0).toDouble();
        for (int i = 0; i < 3; ++i) {
            auto* c = new QToolButton; c->setObjectName("chip"); c->setCheckable(true); c->setFocusPolicy(Qt::NoFocus);
            c->setText(compact ? QString::number(ws[i], 'g', 2) : QStringLiteral("%1 px").arg(ws[i], 0, 'g', 2));
            if (compact) { c->setFixedSize(28, 22); c->setProperty("tight", true); }   // 28: 아래 줄이 위 줄(깊이) 폭 안에 들어가게
            c->setToolTip(QStringLiteral("단면선 굵기 %1 px (화면만 — 인쇄는 0.35 mm)").arg(ws[i], 0, 'g', 2));
            c->setProperty("lineWidth", ws[i]); c->setChecked(std::fabs(cur - ws[i]) < 0.01);
            if (i == 0) c->setProperty("pos", "first");
            if (i == 2) c->setProperty("pos", "last");
            const double wv = ws[i];
            QObject::connect(c, &QToolButton::clicked, this, [this, wv] { setSectionLineWidth(wv); });
            lineWidthChips_.push_back(c); h->addWidget(c);
        }
        return w;
    };
    // 입면 아래 줄(사용자 지시 2026-10-10 「더보기 안의 영상 불투명도 · 단면선 굵기를 입면 밑으로」): 위 줄(깊이)은 타일 가운데 그대로, 그 밑에 단다.
    // 슬라이더는 가로 Ignored(최소 56) — 아래 줄이 위 줄보다 넓어지지 않아 리본 폭 계산(1920 타일 50 · 1280 타일 20)이 그대로
    elevRow2_ = new QWidget; elevRow2_->setObjectName("elevRow2");
    { auto* h = new QHBoxLayout(elevRow2_); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(4);
        opacity_ = new QSlider(Qt::Horizontal); opacity_->setObjectName("opacityRow"); opacity_->setRange(10, 100); opacity_->setValue(100);
        opacity_->setMinimumWidth(56); opacity_->setFixedHeight(22); opacity_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        opacity_->setToolTip(QStringLiteral("입면 영상 불투명도 10–100 % (화면만 — 도면 · 내보내기는 100 %)"));
        h->addWidget(lab(QStringLiteral("불투명도"), "hint")); h->addWidget(opacity_, 1); h->addSpacing(2);
        h->addWidget(lab(QStringLiteral("선 굵기"), "hint")); h->addWidget(lineWidthChips(true));
        h->addSpacing(2);
        // 평활(T6 · 사용자 결정 「평활은 입면 아래 줄, 선 굵기 옆 켜기 · 끄기」): 같은 동작 「smooth」 — 줄에서 눌러도 입면 ▾ 안에서 눌러도 하나
        auto* sm = new QToolButton; sm->setObjectName("chip"); sm->setProperty("smoothChip", true); sm->setCheckable(true); sm->setFocusPolicy(Qt::NoFocus);
        sm->setText(QStringLiteral("평활")); sm->setFixedSize(40, 22); sm->setProperty("tight", true); sm->setChecked(action("smooth")->isChecked());
        sm->setToolTip(QStringLiteral("평활 — 잘린 선을 매끈하게 다듬습니다(켜면 단면을 다시 계산 · 내보내기도 같음)"));
        QObject::connect(sm, &QToolButton::clicked, action("smooth"), &QAction::toggle);
        QObject::connect(action("smooth"), &QAction::toggled, sm, [sm](bool on) { QSignalBlocker bk(sm); sm->setChecked(on); });
        QObject::connect(action("smooth"), &QAction::changed, sm, [this, sm] { sm->setEnabled(action("smooth")->isEnabled()); });
        h->addWidget(sm); }
    auto* elevBlock = new QWidget; elevBlock->setObjectName("elevBlock");
    elevBlock->setProperty("ribbonCenterHeight", depthBox_->sizeHint().height());   // 위 줄만 타일 가운데(Ribbon::addWidget)
    { auto* v = new QVBoxLayout(elevBlock); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(3); v->addWidget(depthBox_); v->addWidget(elevRow2_); }
    ribbon_->addWidget(QStringLiteral("elev"), elevBlock);
    // 입면 ▾ 칩(T6 · 더보기 삭제): 리본이 낮아(글자 숨김 크기) 아래 줄이 숨을 때만 보이고, 누르면 아래 줄의 같은 셋(영상 불투명도 · 단면선 굵기 · 평활)이 열린다 — 같은 값 · 같은 동작
    QToolButton* elevMenuChip = nullptr;
    { auto* mn = new QMenu(this);
        opacityMenu_ = new QSlider(Qt::Horizontal); opacityMenu_->setObjectName("opacityMenu"); opacityMenu_->setRange(10, 100); opacityMenu_->setValue(opacity_->value()); opacityMenu_->setFixedWidth(110);
        QObject::connect(opacityMenu_, &QSlider::valueChanged, opacity_, [this](int v) { opacity_->setValue(v); });
        QObject::connect(opacity_, &QSlider::valueChanged, opacityMenu_, [this](int v) { QSignalBlocker bk(opacityMenu_); opacityMenu_->setValue(v); });
        auto* opw = rowOf({lab(QStringLiteral("불투명도"), "hint"), opacityMenu_}); opw->setContentsMargins(12, 4, 12, 4);
        auto* wa = new QWidgetAction(mn); wa->setDefaultWidget(opw); mn->addAction(wa); opacityMenuAct_ = wa;
        lineWidthMenu_ = lineWidthChips(false); lineWidthMenu_->setObjectName("lineWidthMenu");
        auto* lwBox = rowOf({lab(QStringLiteral("선 굵기"), "hint"), lineWidthMenu_}); lwBox->setContentsMargins(12, 4, 12, 4);
        auto* wa2 = new QWidgetAction(mn); wa2->setDefaultWidget(lwBox); mn->addAction(wa2); lineWidthMenuAct_ = wa2;
        mn->addAction(action("smooth"));
        action("elevmenu")->setMenu(mn); }
    elevMenuChip = add("elev", "elevmenu", K::Normal, "sliders-horizontal", QStringLiteral("입면"));
    elevMenuChip->setProperty("narrowOnly", true); elevMenuChip->hide();
    // 레벨선 간격 글(옛 가짜 입력칸)은 리본에서 뺐다 — 갱신 코드가 쓰므로 위젯만 숨겨 둔다(범례는 정보 줄)
    lvLine_ = lab(QStringLiteral("0.10 m"), "mono"); lvLabel_ = lab(QStringLiteral("0.50 m"), "mono");
    for (auto* l : {lvLine_, lvLabel_}) { l->setParent(ribbon_); l->hide(); }
    // ---- 보기: 목록 · 평면 · 단면 · 고대비(보기 켜짐 = 귀리색 타일)
    ribbon_->addGroup(QStringLiteral("view"), QStringLiteral("보기"));
    add("view", "listpanel", K::Shown, "list", QStringLiteral("목록"));
    add("view", "view1", K::Shown, "map", QStringLiteral("평면"));
    add("view", "view2", K::Shown, "profile", QStringLiteral("단면"));
    add("view", "contrast", K::Shown, "contrast", QStringLiteral("고대비"));
    add("view", "keys", K::Normal, "keyboard", QStringLiteral("단축키"));   // 옛 「기타」 묶음은 이 칩 하나뿐이라 보기에 합침(T6)
    // ---- 자료: 숫자 · 좌표 자료만(그림은 「도면」)
    ribbon_->addGroup(QStringLiteral("data"), QStringLiteral("자료"));
    add("data", "csv", K::Normal, "csv", QStringLiteral("CSV"));
    add("data", "plan", K::Normal, "geotiff", QStringLiteral("GeoTIFF"));
    { auto* pm = new QMenu(this); pm->addAction(action("xyz")); pm->addAction(action("las")); action("points")->setMenu(pm); }   // 점군은 칩 하나, XYZ · LAS 는 그 메뉴 안(T6)
    add("data", "points", K::Normal, "points-xyz", QStringLiteral("점군"));
    { auto* mn = new QMenu(this); mn->addAction(action("secjson")); mn->addAction(action("secimport")); action("seclist")->setMenu(mn); }
    add("data", "seclist", K::Normal, "sections-list", QStringLiteral("목록 파일"));
    // ---- 내보내기: 도면(흙색 주 단추 — 단면 탭에서 한 화면의 유일한 흙색)
    ribbon_->addGroup(QStringLiteral("out"), QStringLiteral("내보내기"));
    sheetChip_ = add("out", "sheet", K::Primary, "sheet", QStringLiteral("도면"));
    // 입면 아래 줄은 칩 높이 안에 두 줄이 들어갈 때만(글자 보이는 크기 — 1920 · 2560 …). 안 들어가면(글자 숨김 크기 — 1280 …) 숨기고 더보기 안의 같은 칸을 보인다
    ribbon_->setLookHook([this, elevBlock, elevMenuChip](const RibbonLook& L, int chipH) {
        const int topH = depthBox_->sizeHint().height();
        const bool two = std::max(0, Ribbon::tileCenterY(L) - topH / 2) + topH + elevBlock->layout()->spacing() + elevRow2_->sizeHint().height() <= chipH;
        elevRow2_->setVisible(two);
        opacityMenuAct_->setVisible(!two); lineWidthMenuAct_->setVisible(!two);
        elevMenuChip->setVisible(!two);
    });
    return ribbon_;
}

// 리본 문맥(디자인 v5 §3.1): 홈 = 모델 묶음만 살아 있고 나머지 흐림, 단면 = 전부, 도면 = 단면 · 입면 꺼짐 + 「도면」 칩은 보기 켜짐 모양
void MainWindow::setRibbonContext(int ctx) {
    if (!ribbon_) return;
    if (ctx != 1 && plan_ && plan_->drawMode()) plan_->setDrawMode(false);   // 단면 문맥을 떠나면(홈 · 도면) 도구를 내려놓는다 — 한 곳(검토 I2 · I3)
    ribbonCtx_ = ctx;
    const bool home = ctx == 0, sheet = ctx == 2;
    for (const char* g : {"sec", "elev", "view", "data", "out"}) ribbon_->setGroupDim(QString::fromLatin1(g), home);
    if (sheetChip_) ribbon_->setChipOn(sheetChip_, sheet);
    applyProjectHeader();
    updateEnabled();   // 켜고 끄기는 updateEnabled 한 곳(검토 I1)
}

void MainWindow::showHomeTab() { showStart(true); }

void MainWindow::closeSheetTab() {
    discardSheetTab();
    showWorkTab();
}

// 「도면」 탭 · 조판 위젯 · 저장 함수를 버린다(검토 I4): 모델을 닫거나 다른 모델을 열어도 옛 모델의 도면이 남아 다른 이름으로 저장되지 않게
void MainWindow::discardSheetTab() {
    sheetRefresh_ = nullptr;
    if (viewTabs_ && viewTabs_->count() > 2) { QSignalBlocker b(viewTabs_); viewTabs_->removeTab(2); }
    if (sheetHost_) { if (body_) body_->removeWidget(sheetHost_); sheetHost_->deleteLater(); sheetHost_ = nullptr; }
    embeddedSave_ = nullptr;
}

// 키보드로 온 초점에만 테를 그린다(QSS [kbdFocus="true"]:focus) — 리본 칩뿐 아니라 모든 단추(검토 UI 6). 창 전환 · 팝업은 이전 값을 지킨다(검토 M8)
bool MainWindow::eventFilter(QObject* o, QEvent* e) {
    if (e && e->type() == QEvent::Resize && o && o->isWidgetType()) {   // roundMask 속성이 있는 틀은 모서리를 둥글게 깎는다(자식 캔버스 포함 — 사용자 지시 2026-10-09 둥글기)
        if (const int rr = o->property("roundMask").toInt(); rr > 0) {
            auto* w = static_cast<QWidget*>(o);
            QPainterPath pp; pp.addRoundedRect(QRectF(w->rect()), rr, rr);
            w->setMask(QRegion(pp.toFillPolygon().toPolygon()));
        }
        if (o == secTitleBar_) fitSectionHeader();   // 좁은 창: 단면 머리를 폭에 맞게 줄인다(스펙 §4 — 필터는 layout 의 doResize 보다 먼저 받는다)
        if (o == infoStrip_) fitInfoStrip();         // 정보 줄도 같은 때
        if (const QVariant em = o->property("elideMode"); em.isValid() && o->inherits("QLabel")) {   // 홈 한 줄 글: 폭에 맞게 줄임표 + 툴팁(스펙 §7)
            auto* l = static_cast<QLabel*>(o);
            setElidedText(l, l->property("fullText").toString(), Qt::TextElideMode(em.toInt()), l->contentsRect().width());
        }
        if (o->objectName() == QLatin1String("homeKeys")) fitHomeKeys(static_cast<QWidget*>(o));   // 키 줄은 뒤 키부터 숨김
        if (o->objectName() == QLatin1String("ccCells")) fitHomeCells(static_cast<QWidget*>(o));   // 이어서 작업 칸: 칸 1–5 내용 폭 · 폴더만 늘고 줆
    }
    if (e && (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut) && o && o->isWidgetType() && o->inherits("QAbstractButton")) {
        auto* b = static_cast<QWidget*>(o);
        const Qt::FocusReason r = static_cast<const QFocusEvent*>(e)->reason();
        if (r != Qt::ActiveWindowFocusReason && r != Qt::PopupFocusReason) {
            const bool kbd = e->type() == QEvent::FocusIn && (r == Qt::TabFocusReason || r == Qt::BacktabFocusReason);
            if (b->property("kbdFocus").toBool() != kbd) { b->setProperty("kbdFocus", kbd); b->style()->unpolish(b); b->style()->polish(b); }
        }
    }
    return QMainWindow::eventFilter(o, e);
}

// ---------------------------------------------------------------- 지금 도구 줄(그리는 중에만)
// 그리는 동안 떠 있는 안내(v5 C4 · 스펙 §5): 「단면선 긋기 › A 찾는 중 | 평면에서 시작점 A를 클릭하세요 | Esc」
void MainWindow::updateCtx(int stage, const SectionLine& l) {
    if (!guide_) return;
    if (stage == 0) {
        guide_->setTool(QStringLiteral("section-line"), QStringLiteral("단면선 긋기 › A 찾는 중"));
        guide_->setHint(QStringLiteral("평면에서 시작점 A를 클릭하세요"));
        guide_->setKeys({QStringLiteral("Esc")});
        readoutDrawing(0, l);   // 목록 아래 수치도 「그리는 중」(화판 2)
        return;
    }
    const double L = SectionFrame(l).L;
    guide_->setTool(QStringLiteral("section-line"), QStringLiteral("단면선 긋기 › A′ 찾는 중"));
    guide_->setHint(QStringLiteral("끝점 A′를 클릭하세요 · Shift = 축 맞춤"));   // 스펙 §5 — 길이는 평면 A′ 이름표 「A′ · 7.43 m」가 든다(T5: 「숫자 = 길이」 · Enter 는 좌표 입력과 함께 지움)
    guide_->setKeys({QStringLiteral("Esc")});
    readoutDrawing(1, l);
}

// ---------------------------------------------------------------- 알림 띠(노란 띠 대신)
QFrame* MainWindow::buildNotice() {
    auto* f = new QFrame; f->setObjectName("notice"); f->setProperty("level", "caution"); f->setAttribute(Qt::WA_StyledBackground);
    auto* h = new QHBoxLayout(f); h->setContentsMargins(12, 5, 8, 5); h->setSpacing(8);
    auto* ic = new QLabel(QString::fromUtf8("▲")); ic->setObjectName("noticeIcon");
    h->addWidget(ic);
    noticeText_ = new QLabel; noticeText_->setTextFormat(Qt::RichText); noticeText_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    h->addWidget(noticeText_, 1);
    auto* more = new QPushButton(QStringLiteral("자세히")); more->setObjectName("quiet"); more->setStyleSheet("QPushButton{padding:3px 10px;}");
    auto* x = new QToolButton; x->setIcon(theme::icon(theme::Ico::Close, 16)); x->setAutoRaise(true); x->setToolTip(QStringLiteral("이 알림 닫기(좌표계 표시는 상태줄 오른쪽에 계속)"));
    for (QWidget* b : {static_cast<QWidget*>(more), static_cast<QWidget*>(x)}) { b->setFocusPolicy(Qt::NoFocus); h->addWidget(b); }
    QObject::connect(more, &QPushButton::clicked, this, [this] { dlgSrsDetails(); });
    QObject::connect(x, &QToolButton::clicked, f, [f] { f->setVisible(false); });
    f->setVisible(false);
    return f;
}

// ---------------------------------------------------------------- 보기 머리
QWidget* MainWindow::buildPlanFrame() {
    auto* fr = new QWidget; fr->setObjectName("viewFrame"); fr->setProperty("roundMask", 8); fr->setAttribute(Qt::WA_StyledBackground);
    auto* v = new QVBoxLayout(fr); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(0);
    planTitle_ = new QWidget; planTitle_->setObjectName("viewTitle"); planTitle_->setAttribute(Qt::WA_StyledBackground); planTitle_->setFixedHeight(32); planTitle_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);   // 머리 글이 화면 최소 폭을 밀어내지 않게(좁은 창 · 글꼴 없는 offscreen)
    auto* h = new QHBoxLayout(planTitle_); h->setContentsMargins(10, 0, 6, 0); h->setSpacing(2);
    h->addWidget(lab(QStringLiteral("평면"), "viewTitleText"));
    h->addSpacing(6); h->addWidget(lab(QStringLiteral("위에서"), "hint"));
    h->addStretch();
    auto mk = [&](theme::Ico ic, const QString& tip, std::function<void()> fn) {
        auto* a = new QAction(theme::icon(ic, 16), tip, this); a->setProperty("iconName", QString::fromLatin1(theme::iconName(ic)));
        QObject::connect(a, &QAction::triggered, this, fn);
        h->addWidget(iconBtn(a));
    };
    h->addWidget(iconBtn(action("top")));   // v4: 위에서(T) — 제자리
    mk(theme::Ico::Fit, QStringLiteral("맞춤 (F)"), [this] { plan_->fitAll(); });
    mk(theme::Ico::ZoomIn, QStringLiteral("확대"), [this] { plan_->zoomBy(1 / 1.4); });
    mk(theme::Ico::ZoomOut, QStringLiteral("축소"), [this] { plan_->zoomBy(1.4); });
    mk(theme::Ico::Max, QStringLiteral("이 보기만 크게 / 나란히"), [this] { bool solo = action("view2")->isChecked(); action("view1")->setChecked(true); action("view2")->setChecked(!solo); });
    v->addWidget(planTitle_);
    // 평면 캔버스를 품는 호스트: 떠 있는 안내 · 되돌리기 · 다시가 이 위에 뜬다(QOpenGLWidget 형제 위젯)
    planHost_ = new QWidget; planHost_->setObjectName("planHost");
    { auto* hl = new QVBoxLayout(planHost_); hl->setContentsMargins(0, 0, 0, 0); hl->setSpacing(0); hl->addWidget(plan_, 1); }
    guide_ = new GuideBand(planHost_);
    undoBtns_ = new FloatButtons(planHost_, Qt::Horizontal, Qt::AlignLeft | Qt::AlignTop);
    undoBtns_->add(QStringLiteral("undo-2"), QStringLiteral("되돌리기 (Ctrl+Z)"), action("undo"));
    undoBtns_->add(QStringLiteral("redo-2"), QStringLiteral("다시 (Ctrl+Y)"), action("redo"));
    undoBtns_->followRightOf(guide_, 12);
    undoBtns_->hide();
    v->addWidget(planHost_, 1);
    return fr;
}

QWidget* MainWindow::buildSectionFrame() {
    auto* fr = new QWidget; fr->setObjectName("viewFrame"); fr->setProperty("roundMask", 8); fr->setAttribute(Qt::WA_StyledBackground);
    auto* v = new QVBoxLayout(fr); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(0);
    secTitleBar_ = new QWidget; secTitleBar_->setObjectName("viewTitle"); secTitleBar_->setAttribute(Qt::WA_StyledBackground); secTitleBar_->setFixedHeight(32); secTitleBar_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    auto* h = new QHBoxLayout(secTitleBar_); h->setContentsMargins(10, 0, 6, 0); h->setSpacing(2);
    secTitle_ = lab(QStringLiteral("단면"), "viewTitleText"); h->addWidget(secTitle_);   // 평면 머리와 같은 13/700(검토 UI 1 · 7)
    h->addSpacing(8);
    secFacing_ = lab(QString(), "facing"); h->addWidget(secFacing_);
    h->addStretch();
    // v4: 그릴 것 켜기·끄기 넷은 영향을 받는 이 머리에만(리본에서 뺐다). 같은 동작이라 키(I · L · V)와 상태를 같이 씀
    {
        auto* box = secToggles_ = new QWidget; box->setObjectName("viewToggles"); box->setAttribute(Qt::WA_StyledBackground);
        auto* bl = new QHBoxLayout(box); bl->setContentsMargins(2, 2, 2, 2); bl->setSpacing(2);
        const std::pair<const char*, theme::Ico> tg[] = {{"image", theme::Ico::Image}, {"line", theme::Ico::Line}, {"levels", theme::Ico::Levels}, {"ticks", theme::Ico::Ticks}};
        for (const auto& [key, ico] : tg) {
            auto* b = new QToolButton; b->setObjectName("viewToggle"); b->setIcon(theme::icon(ico, 16)); b->setIconSize(QSize(16, 16));
            b->setFocusPolicy(Qt::NoFocus); b->setFixedSize(28, 24);
            bindButton(b, action(key));
            b->setAccessibleName(action(key)->text());
            bl->addWidget(b);
        }
        // 깊이 음영은 글자 있는 체크 칸(사용자 지시 2026-10-10 「깊이 음영 체크를 만들라」 — 아이콘만으로는 못 찾았다). 같은 QAction 이라 메뉴 · 되돌리기 · 설정과 함께 움직인다
        {
            QAction* fa = action("fade");
            auto* cb = new QCheckBox(fa->text()); cb->setObjectName("depthFadeCheck");
            cb->setIcon(theme::icon(theme::Ico::Band, 16)); cb->setIconSize(QSize(16, 16)); cb->setProperty("iconName", QString::fromLatin1(theme::iconName(theme::Ico::Band)));
            cb->setFocusPolicy(Qt::NoFocus); cb->setFixedHeight(24);
            auto sync = [cb, fa] { cb->setEnabled(fa->isEnabled()); cb->setToolTip(fa->toolTip()); QSignalBlocker bk(cb); cb->setChecked(fa->isChecked()); };
            sync();
            QObject::connect(fa, &QAction::changed, cb, sync);
            QObject::connect(fa, &QAction::toggled, cb, sync);
            QObject::connect(cb, &QCheckBox::toggled, fa, [fa](bool on) { if (fa->isChecked() != on) fa->setChecked(on); });
            bl->addWidget(cb);
        }
        h->addWidget(box);
        // 좁은 창(스펙 §4 4단계): 상자 대신 「그릴 것 ▾」 메뉴 단추 하나 — 같은 QAction 넷이라 체크 상태 · 키 I · L · V 가 그대로(깊이 음영은 키가 없어 마우스로 닿아야 함)
        secTogglesMenu_ = new QToolButton; secTogglesMenu_->setObjectName("secTogglesMenu"); secTogglesMenu_->setFocusPolicy(Qt::NoFocus); secTogglesMenu_->setAutoRaise(true);
        secTogglesMenu_->setIcon(kerf::icon(QStringLiteral("layers"), 16, theme::Hand)); secTogglesMenu_->setIconSize(QSize(16, 16)); secTogglesMenu_->setProperty("iconName", QStringLiteral("layers"));
        secTogglesMenu_->setIcon(kerf::iconWithCaret(QStringLiteral("layers"), 16, theme::Hand, theme::Card)); secTogglesMenu_->setProperty("caretBaked", true);   // 메뉴 단추라는 표시 ▾ 를 아이콘에(스펙 §4 「그릴 것 ▾」 — 폭을 늘리지 않음, rework 2 UI S1)
        secTogglesMenu_->setAccessibleName(QStringLiteral("그릴 것"));
        secTogglesMenu_->setPopupMode(QToolButton::InstantPopup);
        secTogglesMenu_->setToolTip(QStringLiteral("그릴 것 켜기 · 끄기 — 입면 영상 · 잘린 선 · 레벨선 · 깊이 음영 · 거리 눈금 (I · L · V · D)"));
        { auto* m = new QMenu(secTogglesMenu_); for (const char* k : {"image", "line", "levels", "fade", "ticks"}) m->addAction(action(k)); secTogglesMenu_->setMenu(m); }
        secTogglesMenu_->hide();
        h->addWidget(secTogglesMenu_); h->addSpacing(6);
    }
    // 세로:가로 — 누르면 화면 세로 과장(×1·×2·×5·×10, X 키로 돌아가며). 기복이 작으면 추천을 함께 보여 줌
    vexBtn_ = new QToolButton; vexBtn_->setObjectName("vexBtn"); vexBtn_->setFocusPolicy(Qt::NoFocus);
    vexBtn_->setAutoRaise(true); vexBtn_->setPopupMode(QToolButton::InstantPopup);
    vexIcon_ = theme::icon(theme::Ico::Vex, 16); vexCaretIcon_ = kerf::iconWithCaret(QString::fromLatin1(theme::iconName(theme::Ico::Vex)), 16, theme::Hand, theme::Card);
    vexBtn_->setIcon(vexIcon_); vexBtn_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // v4 D11
    {
        auto* m = new QMenu(vexBtn_);
        for (int k : {1, 2, 5, 10}) {
            QAction* a = m->addAction(k == 1 ? QStringLiteral("세로:가로 1:1 (실제 비율)") : QStringLiteral("세로 ×%1 과장 (화면만)").arg(k));
            QObject::connect(a, &QAction::triggered, this, [this, k] { setVex(k); });
        }
        vexBtn_->setMenu(m);
    }
    h->addWidget(vexBtn_); h->addSpacing(4);
    h->addSpacing(6);
    auto mk = [&](theme::Ico ic, const QString& tip, std::function<void()> fn) {
        auto* a = new QAction(theme::icon(ic, 16), tip, this); a->setProperty("iconName", QString::fromLatin1(theme::iconName(ic)));
        QObject::connect(a, &QAction::triggered, this, fn);
        auto* b = iconBtn(a); h->addWidget(b);
        return b;
    };
    mk(theme::Ico::Fit, QStringLiteral("맞춤"), [this] { section_->fit(); section_->update(); });
    mk(theme::Ico::ZoomIn, QStringLiteral("확대"), [this] { section_->zoomBy(1.4); });
    mk(theme::Ico::ZoomOut, QStringLiteral("축소"), [this] { section_->zoomBy(1 / 1.4); });
    secMaxBtn_ = mk(theme::Ico::Max, QStringLiteral("이 보기만 크게 / 나란히"), [this] { bool solo = action("view1")->isChecked(); action("view2")->setChecked(true); action("view1")->setChecked(!solo); });
    v->addWidget(secTitleBar_);
    auto* strip = infoStrip_ = new QWidget; strip->setObjectName("infoStrip"); strip->setAttribute(Qt::WA_StyledBackground); strip->setFixedHeight(26); strip->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    auto* sh = new QHBoxLayout(strip); sh->setContentsMargins(8, 0, 8, 0); sh->setSpacing(16);
    stripLen_ = new QLabel; stripDepth_ = new QLabel; stripScale_ = new QLabel; stripState_ = new QLabel; stripLevels_ = new QLabel;
    // v4: 정보 줄 = 계산 상태 + 범례만. 길이 · 앞뒤는 오른쪽 판, 화면 축척은 단면 머리 — 글 위젯은 갱신 코드가 쓰므로 숨겨 둠
    for (auto* l : {stripLen_, stripDepth_, stripScale_}) { l->setTextFormat(Qt::RichText); l->setParent(strip); l->hide(); }
    stripState_->setTextFormat(Qt::RichText); sh->addWidget(stripState_);
    sh->addStretch();
    stripCut_ = new QLabel(QStringLiteral("잘린 면"));
    stripBand_ = new QLabel;
    stripCut_->setObjectName("hint");
    stripBand_->setObjectName("hint");
    sh->addWidget(stripCut_);
    sh->addWidget(stripBand_);
    stripLevels_->setText(QStringLiteral("레벨선 10 cm · 숫자 50 cm"));
    stripLevels_->setObjectName("hint");
    sh->addWidget(stripLevels_);
    v->addWidget(strip);
    v->addWidget(section_, 1);
    return fr;
}

void MainWindow::setActiveView(int vw) {
    for (auto* t : {planTitle_, secTitleBar_}) {
        if (!t) continue;
        t->setProperty("active", (t == planTitle_) == (vw == 0));
        t->style()->unpolish(t); t->style()->polish(t);
    }
}

void MainWindow::setVex(double v) {
    section_->setVerticalExaggeration(v);
    updateVexUi();
    if (v > 1.5) showStatus(QStringLiteral("세로 ×%1 과장 — 화면 보기만 바뀝니다. 도면·DXF·영상 내보내기는 언제나 1:1 (X: 다음 배율)").arg(v, 0, 'g', 3));
    else showStatus(QStringLiteral("세로:가로 1:1 (실제 비율)"));
}

void MainWindow::updateVexUi() {
    if (!vexBtn_) return;
    const double v = section_->verticalExaggeration();
    const bool hasSec = section_->hasResult();
    QString t = v > 1.5 ? QStringLiteral("세로 <b>×%1</b> 과장").arg(v, 0, 'g', 3) : QStringLiteral("세로:가로 <b>1:1</b>");
    QString plain = QStringLiteral("세로 ×%1 ▾").arg(v > 1.5 ? QString::number(v, 'g', 3) : QStringLiteral("1"));   // 화판 secHead — 권장은 툴팁에(검토 UI 1)
    QString state = v > 1.5 ? "on" : "off";
    QString tip = QStringLiteral("단면 화면의 세로 과장(×1·×2·×5·×10) — 기복이 작은 면(얕은 수혈 윤곽 등)을 보기 쉽게. 화면만 바뀌고 도면·내보내기는 언제나 1:1 [X]");
    if (hasSec && v < 1.5 && vexSuggest_ > 1) {
        state = "hint";
        tip = QStringLiteral("이 단면의 기복은 %1 cm라 1:1 화면에서는 거의 평평하게 보입니다 — 세로 ×%2 권장. ").arg(vexRelief_ * 100, 0, 'f', 0).arg(vexSuggest_) + tip;
    }
    (void)t;
    vexFull_ = plain;
    vexBtn_->setText(plain);
    vexBtn_->setToolTip(tip);
    vexBtn_->setProperty("state", state);
    vexBtn_->style()->unpolish(vexBtn_); vexBtn_->style()->polish(vexBtn_);
    if (!headerBatch_) fitSectionHeader();   // 「세로 ×10 ▾」처럼 글이 길어지면 머리를 다시 맞춘다(Review Focus 2) — updateHeader 안에서는 그 끝에서 한 번
}

// 단면 머리(스펙 §4 · 사용자 답 Q1 A): 머리 폭에 들어가는 가장 작은 단계를 고른다 — ① 「북쪽을 봄」 숨김 ② 「크게」 숨김 ③ 「세로 ×1 ▾」 아이콘만
// ④ 켜기 · 끄기 상자 → 「그릴 것 ▾」 메뉴 단추 ⑤ 제목 줄임표(끝, 툴팁 전체). 제목 · 축척 칸 · 맞춤 · 확대 · 축소는 늘 보인다.
// 크기 판단은 단계를 적용한 뒤 레이아웃 최소 폭 ≤ 머리 폭(숨긴 자식은 최소 폭에서 빠진다 — QWidgetItem::isEmpty). 앱 전체 필터의 Resize · updateHeader · updateVexUi 가 부른다
void MainWindow::fitSectionHeader() {
    if (!secTitleBar_ || !secTitle_ || !secTitleBar_->layout() || fittingHeader_) return;
    fittingHeader_ = true;
    QLayout* lay = secTitleBar_->layout();
    const int avail = secTitleBar_->width();
    {   // 입력이 지난번과 같으면 단계를 다시 훑지 않는다(확대 애니메이션 프레임마다 보였다 숨기던 것 — rework 2 code S2)
        const QString key = QStringLiteral("%1|%2|%3|%4|%5").arg(avail).arg(secTitleFull_, secFacing_ ? secFacing_->text() : QString(), vexFull_, vexBtn_ ? vexBtn_->property("state").toString() : QString());
        if (key == secHeaderKey_) { fittingHeader_ = false; return; }
        secHeaderKey_ = key;
    }
    auto apply = [&](int step) {
        if (secFacing_) secFacing_->setVisible(step < 1);
        if (secMaxBtn_) secMaxBtn_->setVisible(step < 2);
        if (vexBtn_) {   // ③ 아이콘만 — 메뉴라는 표시는 아이콘에 구운 ▾(QSS 가 메뉴 표시를 지우고, 글 ▾ 는 1280 머리에 들어가지 않음 — rework 2 UI S1)
            vexBtn_->setToolButtonStyle(step < 3 ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
            vexBtn_->setIcon(step < 3 ? vexIcon_ : vexCaretIcon_);
            vexBtn_->setProperty("caretBaked", step >= 3);
        }
        if (secToggles_) secToggles_->setVisible(step < 4);
        if (secTogglesMenu_) secTogglesMenu_->setVisible(step >= 4);
    };
    ++secHeaderPasses_;   // 감사 refit: updateHeader 한 번에 몇 번 다시 맞추는지
    secTitle_->setProperty("fullText", secTitleFull_); secTitle_->setText(secTitleFull_); secTitle_->setToolTip(QString());
    int step = 0;
    for (; step < 5; ++step) { apply(step); lay->activate(); if (lay->minimumSize().width() <= avail) break; }
    if (step == 5) {   // ⑤ 남는 폭에 제목을 맞춘다 — 글 폭과 라벨 sizeHint 의 차(여백)는 되풀이해 흡수
        apply(4);
        int tw = secTitle_->fontMetrics().horizontalAdvance(secTitleFull_);
        const int floorW = secTitle_->fontMetrics().horizontalAdvance(secTitleFull_.left(1) + QChar(0x2026));   // 바닥 = 첫 글자 + 「…」(「…」만 남지 않게 — 감사 clippedLabels)
        for (int pass = 0; pass < 4; ++pass) {
            lay->activate();
            const int over = lay->minimumSize().width() - avail;
            if (over <= 0) break;
            tw = std::max(floorW, tw - over);
            setElidedText(secTitle_, secTitleFull_, Qt::ElideRight, tw);
        }
    }
    secHeaderStep_ = step;
    lay->activate();
    fittingHeader_ = false;
}

void MainWindow::setSecTitleText(const QString& full) {
    secTitleFull_ = full;
    if (!headerBatch_) fitSectionHeader();   // updateHeader 안에서는 그 끝에서 한 번(방위 글을 바꾼 뒤)
}

// 정보 줄(스펙 §4 · 사용자 답 Q2 A): 폭이 모자라면 ① 「레벨선 10 cm · 숫자 50 cm」 숨김 ② 「입면 뒤 0–3.00 m」 숨김 ③ 「잘린 선 …」을 점 수 뺀 짧은 글(툴팁 전체)
// ④ 남은 글(계산 상태 · 잘린 선)을 넘치는 만큼 비례로 줄임표(바닥 40 px) + 툴팁. 계산 상태는 숨기지 않는다. 줄이지 않을 때 상태 글은 서식(RichText) 그대로(1920 모양 변화 없음)
void MainWindow::fitInfoStrip() {
    if (!infoStrip_ || !infoStrip_->layout() || !stripState_ || !stripCut_ || fittingStrip_) return;
    fittingStrip_ = true;
    QLayout* lay = infoStrip_->layout();
    const int avail = infoStrip_->width();
    static const QRegularExpression tags(QStringLiteral("<[^>]*>"));
    const QString statePlain = QString(stripStateRich_).remove(tags);
    auto apply = [&](int step) {
        if (stripLevels_) stripLevels_->setVisible(step < 1);
        if (stripBand_) stripBand_->setVisible(step < 2);
        stripState_->setTextFormat(Qt::RichText); stripState_->setText(stripStateRich_); stripState_->setProperty("fullText", QVariant()); stripState_->setToolTip(QString());
        if (step < 3) { stripCut_->setText(stripCutFull_); stripCut_->setProperty("fullText", QVariant()); stripCut_->setToolTip(QString()); }
        else { stripCut_->setText(stripCutShort_); stripCut_->setProperty("fullText", stripCutFull_); stripCut_->setToolTip(stripCutShort_ == stripCutFull_ ? QString() : stripCutFull_); }
    };
    int step = 0;
    for (; step < 4; ++step) { apply(step); lay->activate(); if (lay->minimumSize().width() <= avail) break; }
    if (step == 4) {   // ④ 빈 구간 꼬리(「빈 구간 없음」 · 「빈 구간 k곳 x m」)는 늘 그대로 — 「잘린 선 n줄」 앞부분을 먼저 줄이고(없애고), 다음 계산 상태를 바닥 40 px 까지(rework 2 correctness C1)
        apply(3);
        stripState_->setTextFormat(Qt::PlainText);   // 줄인 글은 서식 없이(sizeHint 가 글 폭과 같게)
        const QString tail = stripGapTail(stripCutShort_), sep = QStringLiteral(" · ");
        const QString head = tail == stripCutShort_ ? QString() : stripCutShort_.left(stripCutShort_.size() - tail.size() - sep.size());
        const QFontMetrics fmS = stripState_->fontMetrics(), fmC = stripCut_->fontMetrics();
        const int ellC = fmC.horizontalAdvance(QChar(0x2026)), minS = fmS.horizontalAdvance(QStringLiteral("… ") + statePlain.left(1));
        int tS = fmS.horizontalAdvance(statePlain), tH = fmC.horizontalAdvance(head), tT = fmC.horizontalAdvance(tail);
        for (int pass = 0; pass < 8; ++pass) {
            setElidedText(stripState_, statePlain, Qt::ElideRight, tS);
            const QString h = head.isEmpty() || tH < 2 * ellC ? QString() : fmC.elidedText(head, Qt::ElideRight, tH);
            const QString t = fmC.elidedText(tail, Qt::ElideRight, tT);
            const QString shown = h.isEmpty() || h == QString(QChar(0x2026)) ? t : h + sep + t;
            stripCut_->setText(shown); stripCut_->setProperty("fullText", stripCutFull_); stripCut_->setToolTip(shown == stripCutFull_ ? QString() : stripCutFull_);
            lay->activate();
            int over = lay->minimumSize().width() - avail;
            if (over <= 0) break;
            int d = std::min(over, tH); tH -= d; over -= d;                         // 1) 「잘린 선 n줄」
            d = std::min(over, std::max(0, tS - 40)); tS -= d; over -= d;          // 2) 계산 상태 바닥 40
            d = std::min(over, std::max(0, tS - minS)); tS -= d; over -= d;        // 3) 계산 상태 「… 첫 글자」까지
            if (over > 0) tT = std::max(ellC, tT - over);                          // 4) 마지막 수단 — 그래도 넘치면 꼬리(감사 gap-kept 가 잡는다)
        }
    }
    stripStep_ = step;
    lay->activate();
    fittingStrip_ = false;
}

void MainWindow::setStripTexts(const QString& stateRich, const QString& cutFull, const QString& cutShort) {
    stripStateRich_ = stateRich; stripCutFull_ = cutFull; stripCutShort_ = cutShort;
    fitInfoStrip();
}

QString MainWindow::windowTitleCheck() const {
    const QString t = windowTitle(), dn = QGuiApplication::applicationDisplayName();
    // Qt 는 제목이 표시 이름으로 끝나지 않으면 「 - 표시 이름」을 붙인다(Windows·X11) → 이름이 두 번 보이던 원인
    const bool dup = !dn.isEmpty() && !t.endsWith(dn);
    return QStringLiteral("title=\"%1\" displayName=\"%2\" appended=%3").arg(t, dn).arg(dup ? 1 : 0);
}

void MainWindow::updateHeader() {
    if (!secTitle_) return;
    ++headerBatch_;   // 머리 맞춤은 이 함수가 끝날 때 한 번 — 제목 · 세로 글 · 방위 글을 다 바꾼 뒤(어느 return 길이든, qScopeGuard)
    const auto fitHeaderOnce = qScopeGuard([this] { if (--headerBatch_ == 0) fitSectionHeader(); });
    updateVexUi();
    const bool has = plan_ && plan_->hasLine();
    const QString monoB = QStringLiteral("<b style='font-family:Consolas,\"DejaVu Sans Mono\",monospace'>%1</b>");
    setSecTitleText(has ? QStringLiteral("%1 단면").arg(sectionName()) : QStringLiteral("단면"));
    if (plan_->drawMode()) {   // 그리는 중 판 · 정보 줄은 updateCtx 가 주인(검토 I2) — 옛 단면 수치가 새 선 것처럼 읽히지 않게 잘린 선 · 입면 글도 비움
        if (stripBand_) stripBand_->clear();
        setStripTexts(QStringLiteral("단면선을 긋는 중 — 단면은 A′를 찍은 뒤 계산됩니다"), QString(), QString());
        return;
    }
    QString stateRich;   // 계산 상태(서식 글) — 정보 줄 글은 맨 끝 setStripTexts 한 길로(좁으면 fitInfoStrip 이 줄인다)
    if (has) {
        const SectionLine& l = plan_->line();
        secFacing_->setText(qs8(facingKo(sectionAzimuthDeg(l))));
        stripLen_->setText(QStringLiteral("길이 ") + monoB.arg(QStringLiteral("%1 m").arg(SectionFrame(l).L, 0, 'f', 2)));
    } else { secFacing_->clear(); stripLen_->setText(QStringLiteral("길이 —")); }
    stripDepth_->setText(QStringLiteral("앞 %1 / 뒤 %2").arg(monoB.arg(QString::number(front_->value(), 'f', 2)), monoB.arg(QStringLiteral("%1 m").arg(back_->value(), 0, 'f', 2))));
    if (section_->hasResult()) {
        double d = section_->screenDenom();
        QString ds = (d >= 100 || std::fabs(d - std::round(d)) < 0.05) ? QString::number(std::round(d)) : QString::number(d, 'f', 1);
        stripScale_->setText(QStringLiteral("화면 축척 ") + monoB.arg(QStringLiteral("1:") + ds));
        stateRich = last_.previewLod || !lastFinal_ ? (last_.cutFromLeaf ? QStringLiteral("<span style='color:#7A5A00'>단면선 정확 · 배경 미리보기 → 최종 계산 중…</span>") : QStringLiteral("<span style='color:#7A5A00'>미리보기(거친) → 최종 계산 중…</span>"))
                                                   : QStringLiteral("<span style='color:#3F6B31'>✓</span> 최종(잎)");
        LevelPlan lp = sectionLevelPlan(section_->xf().ppmZ(), 1.0, false);
        lvLine_->setText(QStringLiteral("%1 m").arg(lp.lineCm / 100.0, 0, 'f', 2));
        lvLabel_->setText(QStringLiteral("%1 m").arg(lp.labelCm / 100.0, 0, 'f', 2));
        if (scaleCombo_) { QSignalBlocker b(scaleCombo_); scaleCombo_->setEditText(QStringLiteral("1:") + ds); }
    } else {
        stripScale_->setText(QStringLiteral("화면 축척 —"));
        if (scaleCombo_) { QSignalBlocker b(scaleCombo_); scaleCombo_->setEditText(QStringLiteral("—")); }
    }
    if (secCount_) secCount_->setText(QString::number(sections_.size()));
    refreshSteps();
    if (stripBand_) stripBand_->setText(has ? QStringLiteral("입면 뒤 0–%1 m").arg(back_->value(), 0, 'f', 2) : QString());
    {   // v4 단계 5 · 화판 2: 「잘린 선 n줄 · m점 · 빈 구간 없음 / k곳 x m」 — 빈 구간은 core profileGaps. 짧은 글(좁은 창 3단계)은 점 수만 뺀다 — 빈 구간 정보는 지킴
        QString cutFull = QStringLiteral("잘린 선 —"), cutShort = cutFull;
        if (has && section_->hasResult()) {
            const auto& r = section_->result();
            size_t nv = 0; for (auto& pl : r.profile) nv += pl.size();
            const std::vector<SGap> gaps = profileGaps(r.profile, SectionFrame(r.line).L);   // 결과의 선 길이(끄는 중 · 늦게 온 결과에도 맞음, 검토 M1)
            double gl = 0; for (const SGap& g : gaps) gl += g.s1 - g.s0;
            const QString gapTxt = gaps.empty() ? QStringLiteral("빈 구간 없음") : QStringLiteral("빈 구간 %1곳 %2 m").arg(gaps.size()).arg(gl, 0, 'f', 2);
            cutFull = QStringLiteral("잘린 선 %1줄 · %2점 · %3").arg(r.profile.size()).arg(nv).arg(gapTxt);
            cutShort = QStringLiteral("잘린 선 %1줄 · %2").arg(r.profile.size()).arg(gapTxt);
        }
        setStripTexts(stateRich, cutFull, cutShort);
    }
    updateReadout();
}

// ---------------------------------------------------------------- 단면 목록 아래 읽기 전용 수치(T7 · 오른쪽 판 대신)
// 194879.139 → 「194 879.139」(천 단위 띄어 쓰기 — 캔버스 「최종 디자인」 Work 화판)
static QString groupedFixed(double v, int dec) {
    QString s = QString::number(std::fabs(v), 'f', dec);
    const int dot = s.indexOf(QLatin1Char('.')), ip = dot < 0 ? int(s.size()) : dot;
    for (int i = ip - 3; i > 0; i -= 3) s.insert(i, QLatin1Char(' '));
    return (v < 0 ? QStringLiteral("−") : QString()) + s;
}

void MainWindow::setRo(int row, const QString& cap, const QString& val) {
    if (row < 0 || row >= RoCount || !roVal_[row]) return;
    const bool on = !val.isEmpty();
    roCap_[row]->setText(cap); roVal_[row]->setText(val);
    roCap_[row]->setVisible(on); roVal_[row]->setVisible(on);
}

void MainWindow::updateReadout() {
    if (!roName_) return;
    const bool has = plan_ && plan_->hasLine();
    const int n = int(sections_.size());
    roHead_->setText(current_ >= 0 && current_ < n ? QStringLiteral("<b>선택한 단면</b> &nbsp;%1 / %2").arg(current_ + 1).arg(n) : QStringLiteral("<b>선택한 단면</b>"));
    roName_->setText(has ? sectionName() : QStringLiteral("고른 단면이 없습니다"));
    QString note;
    if (current_ >= 0 && current_ < n) note = qs8(sections_[size_t(current_)].note);
    roNote_->setText(has ? (note.isEmpty() ? secFacing_->text() : note + QStringLiteral(" · ") + secFacing_->text()) : QStringLiteral("단면선을 그으면 여기 수치가 모입니다"));
    for (int i = 0; i < RoCount; ++i) setRo(i, QString(), QString());
    if (!has) return;
    const SectionLine& ln = plan_->line();
    const SectionFrame fr(ln);
    const Vec3 org = src_ ? src_->srs.origin : Vec3();
    setRo(RoLen, QStringLiteral("길이"), QStringLiteral("%1 m").arg(fr.L, 0, 'f', 2));
    setRo(RoBearing, QStringLiteral("방위"), qs8(formatAzimuth(sectionAzimuthDeg(ln))));
    setRo(RoBand, QStringLiteral("앞 / 뒤"), QStringLiteral("%1 / %2 m").arg(ln.front, 0, 'f', 2).arg(ln.back, 0, 'f', 2));
    if (section_->hasResult()) {
        const auto& r = section_->result();
        // 잘린 선(빨강)에서 잼: 지표 = A·A′ 끝의 높이, 바닥 = 가장 낮은 점, 깊이 = 높은 쪽 지표 − 바닥
        double zLo = 1e300, zHi = -1e300, sA = 1e300, sB = -1e300, zA = 0, zB = 0;
        for (const auto& pl : r.profile)
            for (const auto& q : pl) {
                zLo = std::min(zLo, q.y); zHi = std::max(zHi, q.y);
                if (q.x < sA) { sA = q.x; zA = q.y; }
                if (q.x > sB) { sB = q.x; zB = q.y; }
            }
        if (zLo > zHi) { zLo = r.zMin; zHi = r.zMax; zA = zB = r.zMax; }
        const double top = std::max(zA, zB);
        size_t nv = 0; for (auto& pl : r.profile) nv += pl.size();
        setRo(RoDepth, QStringLiteral("깊이"), QStringLiteral("%1 m").arg(std::max(0.0, top - zLo), 0, 'f', 2));
        setRo(RoTop, QStringLiteral("지표 EL."), QStringLiteral("%1 – %2 m").arg(std::min(zA, zB) + org.z, 0, 'f', 2).arg(std::max(zA, zB) + org.z, 0, 'f', 2));
        setRo(RoFloor, QStringLiteral("바닥 EL."), QStringLiteral("%1 m").arg(zLo + org.z, 0, 'f', 2));
        setRo(RoCut, QStringLiteral("잘린 선"), QStringLiteral("%1줄 · %2점").arg(r.profile.size()).arg(groupedFixed(double(nv), 0)));
    }
    { QString la, lb; PlanView::splitLetters(sectionName(), la, lb);   // 단면 이름의 글자(B–B′ → B · B′)
      setRo(RoA, la, groupedFixed(ln.a.x + org.x, 3) + QLatin1Char('\n') + groupedFixed(ln.a.y + org.y, 3));
      setRo(RoB, lb, groupedFixed(ln.b.x + org.x, 3) + QLatin1Char('\n') + groupedFixed(ln.b.y + org.y, 3)); }
    if (roVal_[RoDepth]) roVal_[RoDepth]->setToolTip(QStringLiteral("지표(높은 쪽 끝)에서 바닥(가장 낮은 점)까지"));
    setRo(RoSrs, QStringLiteral("수평"), qs8(srsReport_.desc.horizontalShortKo()));
}

// 그리는 중(0 = A 대기, 1 = A′ 대기): 같은 자리에 길이 · 방위 · 보는 쪽 · 앞/뒤 · A · 커서(A′ 자리)
void MainWindow::readoutDrawing(int stage, const SectionLine& l) {
    if (!roName_) return;
    roHead_->setText(QStringLiteral("<b>선택한 단면</b>"));
    roName_->setText(QStringLiteral("새 단면 · 그리는 중"));
    for (int i = 0; i < RoCount; ++i) setRo(i, QString(), QString());
    if (stage == 0) {
        roNote_->setText(QStringLiteral("평면에서 시작점 A를 찍으세요. 길이 · 방위 · 커서는 A′를 찾는 동안 여기 보입니다"));
        return;
    }
    roNote_->setText(QStringLiteral("A 찍음 · 끝점 A′를 찍으세요"));   // 화판 2
    const double L = SectionFrame(l).L;
    if (L <= 1e-6) return;
    const Vec3 org = src_ ? src_->srs.origin : Vec3();
    setRo(RoLen, QStringLiteral("길이"), QStringLiteral("%1 m").arg(L, 0, 'f', 2));
    setRo(RoBearing, QStringLiteral("방위"), qs8(formatAzimuth(sectionAzimuthDeg(l))));
    setRo(RoFacing, QStringLiteral("보는 쪽"), qs8(facingKo(sectionAzimuthDeg(l))));
    setRo(RoBand, QStringLiteral("앞 / 뒤"), QStringLiteral("%1 / %2 m").arg(front_->value(), 0, 'f', 2).arg(back_->value(), 0, 'f', 2));
    { QString la, lb; PlanView::splitLetters(sectionName(), la, lb);
      setRo(RoA, la, groupedFixed(l.a.x + org.x, 3) + QLatin1Char('\n') + groupedFixed(l.a.y + org.y, 3)); }
    setRo(RoB, QStringLiteral("커서"), groupedFixed(l.b.x + org.x, 3) + QLatin1Char('\n') + groupedFixed(l.b.y + org.y, 3));
    setRo(RoSrs, QStringLiteral("수평"), qs8(srsReport_.desc.horizontalShortKo()));
}

// ---------------------------------------------------------------- 상태줄
// 상태줄(v5 C6 · 스펙 §6 · Strata 와 같은 셀): 왼쪽 다음 할 일 한 문장 | X | Y | Z | 수평 배지 · 높이 배지
QWidget* MainWindow::buildCoordBar() {
    auto* w = new QWidget; w->setObjectName("statusBar"); w->setAttribute(Qt::WA_StyledBackground); w->setFixedHeight(32);
    auto* h = new QHBoxLayout(w); h->setContentsMargins(16, 0, 12, 0); h->setSpacing(0);
    msg_ = new QLabel; msg_->setObjectName("statusMsg");
    msg_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    h->addWidget(msg_, 1);
    auto cellStart = [&] { auto* r = new QFrame; r->setObjectName("statusRule"); r->setFixedSize(1, 16); h->addWidget(r, 0, Qt::AlignVCenter); h->addSpacing(12); };
    auto cellEnd = [&] { h->addSpacing(12); };
    auto field = [&](const char* k, int wpx) {
        auto* l = new QLabel(QString::fromUtf8(k)); l->setObjectName("coordKey"); h->addWidget(l);
        auto* e = new QLineEdit; e->setObjectName("coord"); e->setReadOnly(true); e->setFixedWidth(wpx); e->setFocusPolicy(Qt::ClickFocus); e->setAlignment(Qt::AlignRight);
        h->addWidget(e);
        return e;
    };
    cellStart(); cx_ = field("X", 96); cellEnd();
    cellStart(); cy_ = field("Y", 96); cellEnd();
    cellStart(); cz_ = field("Z", 64);
    zSrc_ = new QLabel; zSrc_->setObjectName("zSrc"); zSrc_->setMinimumWidth(48);
    h->addWidget(zSrc_); cellEnd();
    scaleCombo_ = new QComboBox; scaleCombo_->setEditable(true); scaleCombo_->setFixedWidth(84); scaleCombo_->setFocusPolicy(Qt::ClickFocus);
    for (int d : {10, 20, 40, 50, 100, 200}) scaleCombo_->addItem(QStringLiteral("1:%1").arg(d), d);
    scaleCombo_->addItem(QStringLiteral("맞춤"), 0);
    scaleCombo_->setToolTip(QStringLiteral("단면 보기를 이 축척(화면 96 dpi 기준)으로 맞춤. 1:20 / 1:40 에서 레벨선 10 cm · 숫자 50 cm"));
    QObject::connect(scaleCombo_, &QComboBox::activated, this, [this](int i) {
        int d = scaleCombo_->itemData(i).toInt();
        if (d <= 0) { section_->fit(); section_->update(); } else section_->setScreenDenom(d);
        updateHeader();
    });
    // v4 단계 4: 단면 화면 축척 칸은 단면 머리 하나(상태줄에서 뺌). 단면 머리는 이 줄보다 먼저 만들어지므로 「세로 ×1」 바로 뒤에 끼운다
    if (secTitleBar_ && vexBtn_) {
        auto* sl = static_cast<QHBoxLayout*>(secTitleBar_->layout());
        sl->insertWidget(sl->indexOf(vexBtn_) + 1, scaleCombo_);
    } else {
        cellStart(); h->addWidget(lab(QStringLiteral("축척"), "coordKey")); h->addWidget(scaleCombo_); cellEnd();
    }
    info_ = new QLabel; info_->setObjectName("statusInfo"); info_->setVisible(false);
    h->addWidget(info_);
    progress_ = new QProgressBar; progress_->setTextVisible(false); progress_->setRange(0, 1000); progress_->setVisible(false); progress_->setFixedWidth(120); h->addWidget(progress_);
    // 수평 좌표계는 테두리 없는 작은 글 하나(T8 · 사용자 결정 2026-10-10 「좌표계는 표시만 작게」 · 높이는 앱 어디에도 안 씀 — 캔버스 Work 화판): 「수평 EPSG:5187 · 동부원점(GRS80)」
    cellStart();
    srsLabel_ = new QLabel(QStringLiteral("수평 —")); srsLabel_->setObjectName("statusSrs"); srsLabel_->setProperty("state", "none"); srsLabel_->setContentsMargins(0, 0, 16, 0);
    h->addWidget(srsLabel_);
    return w;
}

// 좌표계 상세(상태줄 「수평」 배지 · 알림 띠 「자세히」)
void MainWindow::dlgSrsDetails() {
    QString t = QStringLiteral("<b>%1</b><br><br>").arg(qs8(srsReport_.desc.horizontalLabelKo()).toHtmlEscaped());
    for (auto& w : srsReport_.warnings) t += QStringLiteral("▲ ") + qs8(w).toHtmlEscaped() + "<br>";
    t += "<br><small>" + qs8(srsReport_.horizontalTooltipKo).toHtmlEscaped().replace("\n", "<br>") + "</small>";
    QMessageBox mb(this); mb.setWindowTitle(QStringLiteral("좌표계 확인")); mb.setTextFormat(Qt::RichText); mb.setText(t); mb.exec();
}

// ---------------------------------------------------------------- 단면 목록(P1-1)
QWidget* MainWindow::buildSidePanel() {
    auto* w = new QWidget; w->setObjectName("sidePanel"); w->setAttribute(Qt::WA_StyledBackground);
    w->setFixedWidth(348);   // 스펙 §2.1: 판 폭은 고정(348), 가운데 화면만 늘어난다 — 창을 줄였다 늘려도 판이 줄지 않게
    auto* outer = new QVBoxLayout(w); outer->setContentsMargins(0, 0, 0, 0); outer->setSpacing(0);
    auto* headW = new QWidget; headW->setObjectName("viewTitle"); headW->setAttribute(Qt::WA_StyledBackground); headW->setFixedHeight(32);   // 네 판의 머리 높이를 같게(검토 UI 2)
    auto* head = new QHBoxLayout(headW); head->setContentsMargins(16, 0, 16, 0); head->setSpacing(6);
    head->addWidget(lab(QStringLiteral("단면 목록"), "viewTitleText"));
    secCount_ = lab(QStringLiteral("0"), "monoFaint"); head->addWidget(secCount_);
    head->addStretch();   // v4: 「＋ 새 단면」은 리본 · N 키와 같아 목록 머리에서 뺐다
    outer->addWidget(headW);
    auto* bodyW = new QWidget; auto* v = new QVBoxLayout(bodyW); v->setContentsMargins(16, 6, 16, 12); v->setSpacing(6);
    outer->addWidget(bodyW, 1);
    secList_ = new QListWidget; secList_->setObjectName("sectionList"); secList_->setFocusPolicy(Qt::TabFocus);   // 키보드로도 닿게(검토 UI 6)
    secList_->setContextMenuPolicy(Qt::CustomContextMenu); secList_->setSpacing(2);
    QObject::connect(secList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) { selectSection(secList_->row(it)); });
    QObject::connect(secList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it) { renameSection(secList_->row(it)); });
    QObject::connect(secList_, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pt) {
        auto* it = secList_->itemAt(pt); if (!it) return;
        int i = secList_->row(it);
        QMenu m(this);
        // v4 메뉴: 아이콘 + 글자. 「이 단면의 도면」은 판에서 옮겨 온 것
        m.addAction(theme::icon(theme::Ico::Sheet, 16), QStringLiteral("이 단면의 도면\tCtrl+P"), this, [this, i] { selectSection(i); dlgSheet(); });
        m.addAction(theme::icon(theme::Ico::Flip, 16), QStringLiteral("방향 반전\tR"), this, [this, i] { selectSection(i); action("flip")->trigger(); });
        m.addAction(theme::icon(theme::Ico::Draw, 16), QStringLiteral("이름 · 메모…\tF2"), this, [this, i] { renameSection(i); });
        m.addSeparator();
        m.addAction(theme::icon(theme::Ico::Clear, 16, theme::Block), QStringLiteral("지우기…\tDel"), this, [this, i] { deleteSection(i); });
        m.exec(secList_->viewport()->mapToGlobal(pt));
    });
    v->addWidget(secList_, 1);
    // 고른 단면의 수치(읽기 전용 — T7: 오른쪽 판을 없애고 목록 아래로). 계산한 값이라 여기서 고칠 수 없다
    readout_ = new QFrame; readout_->setObjectName("listReadout"); readout_->setAttribute(Qt::WA_StyledBackground);
    { auto* rv = new QVBoxLayout(readout_); rv->setContentsMargins(0, 10, 0, 0); rv->setSpacing(6);
        roHead_ = lab(QString(), "roHead"); roHead_->setTextFormat(Qt::RichText); rv->addWidget(roHead_);
        auto* nr = new QHBoxLayout; nr->setSpacing(10);
        roName_ = lab(QString(), "roName"); roNote_ = lab(QString(), "hint"); roNote_->setWordWrap(true);
        nr->addWidget(roName_, 0, Qt::AlignBottom); nr->addWidget(roNote_, 1, Qt::AlignBottom); rv->addLayout(nr);
        auto* g = new QGridLayout; g->setContentsMargins(0, 0, 0, 0); g->setHorizontalSpacing(12); g->setVerticalSpacing(3); g->setColumnStretch(1, 1);
        for (int i = 0; i < RoCount; ++i) {
            roCap_[i] = lab(QString(), "roCap"); roVal_[i] = lab(QString(), "roVal");
            roCap_[i]->setAlignment(Qt::AlignLeft | Qt::AlignTop); roVal_[i]->setAlignment(Qt::AlignRight | Qt::AlignTop);
            roVal_[i]->setTextInteractionFlags(Qt::TextSelectableByMouse); roVal_[i]->setCursor(Qt::IBeamCursor);
            roCap_[i]->hide(); roVal_[i]->hide();
            g->addWidget(roCap_[i], i, 0, Qt::AlignTop); g->addWidget(roVal_[i], i, 1);
        }
        rv->addLayout(g);
        auto* ronote = lab(QStringLiteral("계산한 값이라 여기서 고칠 수 없습니다. 단면선을 옮기면 바로 바뀝니다."), "faint"); ronote->setWordWrap(true); rv->addWidget(ronote); }
    v->addWidget(readout_);
    auto* foot = lab(QStringLiteral("누르면 그 단면으로 · 두 번 누르면 이름 바꾸기\n목록·단면선·입면·조판 설정은 projects 폴더의 프로젝트 파일(.trench.json)에 자동 저장"), "faint");
    foot->setWordWrap(true);
    v->addWidget(foot);
    return w;
}

QString MainWindow::sectionName() const {
    if (current_ >= 0 && current_ < int(sections_.size())) return qs8(sections_[size_t(current_)].name);
    return QStringLiteral("A–A′");
}

QImage MainWindow::makeThumb() const {
    if (!section_->hasResult()) return {};
    SectionDoc d = section_->doc();
    QImage t(128, 60, QImage::Format_ARGB32_Premultiplied); t.fill(QColor("#FFFFFF"));
    QPainter p(&t); p.setRenderHint(QPainter::Antialiasing);
    const auto& r = d.r;
    double s0 = 0, s1 = SectionFrame(r.line).L, z0 = r.zMin, z1 = r.zMax;
    if (z1 <= z0) { z0 = 0; z1 = 1; }
    double pad = 4, sc = std::min((t.width() - 2 * pad) / std::max(1e-6, s1 - s0), (t.height() - 2 * pad) / std::max(1e-6, z1 - z0));
    double ox = (t.width() - (s1 - s0) * sc) / 2, oy = (t.height() - (z1 - z0) * sc) / 2;
    if (!d.img.isNull() && d.imgRes > 0) {
        QRectF dst(ox + (d.imgS0 - s0) * sc, oy + (z1 - d.imgZ1) * sc, d.img.width() * d.imgRes * sc, d.img.height() * d.imgRes * sc);
        p.drawImage(dst, d.img);
    }
    p.setPen(QPen(QColor(255, 0, 0), 2.0));
    for (auto& pl : r.profile) {
        QPolygonF poly;
        for (auto& q : pl) poly << QPointF(ox + (q.x - s0) * sc, oy + (z1 - q.y) * sc);
        p.drawPolyline(poly);
    }
    p.end();
    return t;
}

QWidget* MainWindow::sectionRowWidget(int i) const {
    const auto& s = sections_[size_t(i)];
    auto* row = new QWidget; row->setObjectName("secRow");
    auto* h = new QHBoxLayout(row); h->setContentsMargins(6, 5, 6, 5); h->setSpacing(8);
    auto* th = new QLabel; th->setFixedSize(64, 30); th->setStyleSheet("QLabel{background:#FFFFFF;border:1px solid #DEDCD1;border-radius:6px;}");
    if (auto f = thumbs_.find(i); f != thumbs_.end() && !f->second.isNull())
        th->setPixmap(QPixmap::fromImage(f->second.scaled(62, 28, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    h->addWidget(th);
    double L = std::hypot(s.bx - s.ax, s.by - s.ay);
    auto* txt = vbox({lab(qs8(s.name), "secName"),
                      lab(QStringLiteral("%1 m · 뒤 %2 m").arg(L, 0, 'f', 2).arg(s.back, 0, 'f', 2), "mono")}, 1);
    if (!s.note.empty()) static_cast<QVBoxLayout*>(txt->layout())->insertWidget(2, lab(qs8(s.note), "faint"));
    h->addWidget(txt, 1);
    row->setProperty("current", i == current_);
    return row;
}

void MainWindow::refreshSectionList() {
    if (!secList_) return;
    {
        QSignalBlocker b(secList_);
        secList_->clear();
        for (size_t i = 0; i < sections_.size(); ++i) {
            auto* it = new QListWidgetItem(secList_);
            it->setSizeHint(QSize(10, sections_[i].note.empty() ? 46 : 60));
            secList_->setItemWidget(it, sectionRowWidget(int(i)));
            if (int(i) == current_) it->setSelected(true);
        }
        if (secCount_) secCount_->setText(QString::number(sections_.size()));
    }
    // 평면: 지금 단면의 글자 + 나머지 단면들을 가늘게 함께(새 단면을 그려도 앞 단면이 남는다 — 사용자 결정 2026-10-11)
    if (plan_) {
        const Vec3 o = srs().origin;
        std::vector<PlanView::OtherSection> others;
        for (size_t i = 0; i < sections_.size(); ++i) {
            if (int(i) == current_) continue;
            const SavedSection& s = sections_[i];
            PlanView::OtherSection os;
            os.line.a = {s.ax - o.x, s.ay - o.y}; os.line.b = {s.bx - o.x, s.by - o.y}; os.line.front = s.front; os.line.back = s.back;
            os.name = qs8(s.name);
            others.push_back(std::move(os));
        }
        plan_->setOtherSections(std::move(others));
        plan_->setLineName(current_ >= 0 && current_ < int(sections_.size()) ? qs8(sections_[size_t(current_)].name) : QString());
    }
    if (section_) section_->setStyle(secStyle());   // 단면 보기 축 글자도 이름대로
    refreshSheetList();
}

void MainWindow::syncCurrentSection() {
    if (!src_ || !plan_->hasLine() || applyingState_) return;
    const SectionLine& l = plan_->line();
    const Vec3 o = srs().origin;
    if (current_ < 0 || current_ >= int(sections_.size())) {
        SavedSection s; s.name = sectionLetterName(int(sections_.size()));
        sections_.push_back(s); current_ = int(sections_.size()) - 1;
    }
    SavedSection& s = sections_[size_t(current_)];
    s.ax = l.a.x + o.x; s.ay = l.a.y + o.y; s.bx = l.b.x + o.x; s.by = l.b.y + o.y;
    s.front = front_->value(); s.back = back_->value(); s.backUserSet = backUserSet_;
    if (!thumbs_.count(current_) && section_->hasResult() && lastFinal_) thumbs_[current_] = makeThumb();
    refreshSectionList();
}

void MainWindow::selectSection(int i) {
    if (i < 0 || i >= int(sections_.size()) || !src_) return;
    if (i == current_ && plan_->hasLine()) return;
    current_ = i;
    const SavedSection& s = sections_[size_t(i)];
    const Vec3 o = srs().origin;
    SectionLine l; l.a = {s.ax - o.x, s.ay - o.y}; l.b = {s.bx - o.x, s.by - o.y}; l.front = s.front; l.back = s.back;
    applyingState_ = true;
    { QSignalBlocker b1(front_), b2(back_); front_->setValue(s.front); back_->setValue(s.back); }
    backUserSet_ = s.backUserSet;
    plan_->setBand(s.front, s.back);
    plan_->setLine(l, true);
    applyingState_ = false;
    fitNextResult_ = true;
    requestSection(true);
    updateDepthChips();
    refreshSectionList();
    commitState(QStringLiteral("단면 %1 보기").arg(qs8(s.name)));
    updateHeader();
}

void MainWindow::addSection() {
    if (!src_) return;
    syncCurrentSection();
    std::vector<std::string> used; for (auto& s : sections_) used.push_back(s.name);
    int k = 0; while (std::find(used.begin(), used.end(), sectionLetterName(k)) != used.end()) ++k;
    SavedSection s; s.name = sectionLetterName(k); s.front = front_->value();
    s.back = backUserSet_ ? back_->value() : kDefaultBackDepth; s.backUserSet = backUserSet_;   // 새 단면: 기본 3 m(사용자가 고른 값이 있으면 그 값)
    sections_.push_back(s); current_ = int(sections_.size()) - 1;
    applyingState_ = true;
    plan_->setLine(SectionLine(), false); section_->clear(); last_ = SectionOutput();
    applyingState_ = false;
    refreshSectionList();
    plan_->setDrawMode(true);
    showStatus(QStringLiteral("새 단면 %1: 평면에서 시작점을 클릭하세요").arg(qs8(s.name)));
    updateHeader(); updateEnabled();
}

void MainWindow::renameSection(int i) {
    if (i < 0 || i >= int(sections_.size())) return;
    SavedSection& s = sections_[size_t(i)];
    QDialog d(this); d.setWindowTitle(QStringLiteral("단면 이름 · 메모"));
    auto* f = new QFormLayout(&d);
    auto* name = new QLineEdit(qs8(s.name)); auto* note = new QLineEdit(qs8(s.note));
    note->setPlaceholderText(QStringLiteral("예: 2호 주거지 북벽"));
    f->addRow(QStringLiteral("이름"), name); f->addRow(QStringLiteral("메모"), note);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(bb, &QDialogButtonBox::accepted, &d, &QDialog::accept); QObject::connect(bb, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    f->addRow(bb);
    if (d.exec() != QDialog::Accepted) return;
    if (!name->text().trimmed().isEmpty()) s.name = name->text().trimmed().toStdString();
    s.note = note->text().trimmed().toStdString();
    refreshSectionList(); updateHeader(); saveModelState();
}

void MainWindow::deleteSection(int i) {
    if (i < 0 || i >= int(sections_.size())) return;
    if (QMessageBox::question(this, QStringLiteral("단면 지우기"), QStringLiteral("목록에서 %1 을(를) 지울까요? (모델 파일은 그대로)").arg(qs8(sections_[size_t(i)].name))) != QMessageBox::Yes) return;
    sections_.erase(sections_.begin() + i);
    std::map<int, QImage> nt;
    for (auto& [k, im] : thumbs_) if (k < i) nt[k] = im; else if (k > i) nt[k - 1] = im;
    thumbs_ = nt;
    if (current_ == i) current_ = -1; else if (current_ > i) --current_;
    refreshSectionList();
    if (current_ < 0 && !sections_.empty()) selectSection(0);
    saveModelState(); updateHeader();
}

bool MainWindow::exportSectionsJson(const QString& path, QString* msg) {
    syncCurrentSection();
    std::string js = sectionsToJson(sections_, QFileInfo(path_).fileName().toStdString(), srsReport_.desc.shortAscii(), current_);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) { if (msg) *msg = QStringLiteral("저장할 수 없습니다: ") + path; return false; }
    f.write(QByteArray::fromStdString(js));
    if (msg) *msg = QStringLiteral("단면 %1개 저장: %2").arg(sections_.size()).arg(QDir::toNativeSeparators(path));
    return true;
}

void MainWindow::importSectionsJson() {
    if (!src_) return;
    QString start = QFileInfo(path_).absolutePath();
    QString fn = QFileDialog::getOpenFileName(this, QStringLiteral("단면 목록 가져오기"), start, QStringLiteral("단면 목록 (*.sections.json *.json)"));
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (!f.open(QIODevice::ReadOnly)) { QMessageBox::warning(this, windowTitle(), QStringLiteral("열 수 없습니다")); return; }
    std::vector<SavedSection> v; int cur = 0; std::string err;
    if (!sectionsFromJson(f.readAll().toStdString(), v, &cur, &err)) { QMessageBox::warning(this, windowTitle(), QStringLiteral("단면 목록을 읽지 못했습니다: ") + qs8(err)); return; }
    sections_ = v; thumbs_.clear(); current_ = -1;
    refreshSectionList();
    if (!sections_.empty()) selectSection(std::clamp(cur, 0, int(sections_.size()) - 1));
    saveModelState();
    showStatus(QStringLiteral("단면 %1개 가져옴").arg(sections_.size()));
}

// ---------------------------------------------------------------- 되돌리기
MainWindow::ViewState MainWindow::captureState() const {
    ViewState s;
    s.hasLine = plan_->hasLine(); if (s.hasLine) s.line = plan_->line();
    s.front = front_->value(); s.back = back_->value();
    s.image = action("image")->isChecked(); s.profile = action("line")->isChecked(); s.levels = action("levels")->isChecked(); s.ticks = action("ticks")->isChecked();
    s.fade = action("fade")->isChecked(); s.smooth = action("smooth")->isChecked();
    s.current = current_;
    return s;
}

void MainWindow::applyState(const ViewState& s) {
    applyingState_ = true;
    {
        QSignalBlocker b1(front_), b2(back_);
        front_->setValue(s.front); back_->setValue(s.back);
    }
    plan_->setBand(s.front, s.back);
    const std::pair<const char*, bool> toggles[] = {{"image", s.image}, {"line", s.profile}, {"levels", s.levels}, {"ticks", s.ticks}, {"fade", s.fade}, {"smooth", s.smooth}};
    for (auto& [k, v] : toggles) action(k)->setChecked(v);   // applyingState_ 가 되돌리기 기록을 막음(단추 동기화는 신호로)
    section_->setStyle(secStyle());
    current_ = (s.current >= 0 && s.current < int(sections_.size())) ? s.current : current_;
    if (s.hasLine) { SectionLine l = s.line; l.front = s.front; l.back = s.back; plan_->setLine(l, true); }
    else { plan_->setLine(SectionLine(), false); section_->clear(); last_ = SectionOutput(); }
    lastState_ = s;
    applyingState_ = false;
    if (s.hasLine) { requestSection(true); syncCurrentSection(); }
    updateDepthChips(); updateEnabled(); updateHeader(); refreshSectionList();
}

void MainWindow::commitState(const QString& text, int mergeId) {
    if (applyingState_ || !undo_ || !src_) return;
    ViewState now = captureState();
    if (now == lastState_) return;
    undo_->push(new StateCmd(this, lastState_, now, text, mergeId));
    lastState_ = now;
    saveModelState();
}

bool MainWindow::undoTest(QString* log) {
    auto say = [&](const QString& s) { if (log) *log += s + "\n"; };
    if (!src_ || !plan_->hasLine()) { say("undo-test: no line"); return false; }
    syncCurrentSection();
    undo_->clear(); lastState_ = captureState();
    ViewState s0 = captureState();
    back_->setValue(back_->value() > 2 ? 1.0 : 3.0);      // 1 두께(병합 id 1)
    action("flip")->trigger();                              // 2 반전
    action("levels")->trigger();                            // 3 레벨선 끔/켬
    ViewState s3 = captureState();
    int n = undo_->count();
    say(QStringLiteral("undo-test: commands=%1 (%2)").arg(n).arg(n == 3 ? "ok" : "FAIL"));
    for (int i = 0; i < 3; ++i) undo_->undo();
    bool u = captureState() == s0;
    for (int i = 0; i < 3; ++i) undo_->redo();
    bool r = captureState() == s3;
    for (int i = 0; i < 3; ++i) undo_->undo();
    bool u2 = captureState() == s0;
    say(QStringLiteral("undo-test: undo3=%1 redo3=%2 undo3again=%3").arg(u ? "ok" : "FAIL", r ? "ok" : "FAIL", u2 ? "ok" : "FAIL"));
    return n == 3 && u && r && u2;
}

// ---------------------------------------------------------------- 최근 파일 · 모델별 상태
QStringList MainWindow::recentFiles() const { return QSettings().value("recent/files").toStringList(); }

void MainWindow::addRecent(const QString& path) {
    std::vector<std::string> v; for (auto& s : recentFiles()) v.push_back(s.toStdString());
    v = pushRecent(v, QFileInfo(path).absoluteFilePath().toStdString(), 8);
    QStringList out; for (auto& s : v) out << qs8(s);
    QSettings().setValue("recent/files", out);
    rebuildRecentMenu();
}

void MainWindow::rebuildRecentMenu() {
    if (!recentMenu_) return;
    recentMenu_->clear();
    const QStringList rf = recentFiles();
    if (rf.isEmpty()) { recentMenu_->addAction(QStringLiteral("(최근 연 파일 없음)"))->setEnabled(false); return; }
    int i = 0;
    for (const QString& f : rf) {
        bool ok = QFileInfo::exists(f);
        QString t = QStringLiteral("&%1  %2%3").arg(++i).arg(QFileInfo(f).fileName(), ok ? QString() : QStringLiteral("   ● 원본 없음"));
        auto* a = recentMenu_->addAction(t, this, [this, f] { openFile(f); });
        a->setToolTip(QDir::toNativeSeparators(f)); a->setEnabled(ok);
    }
    recentMenu_->addSeparator();
    recentMenu_->addAction(QStringLiteral("목록 비우기"), this, [this] { QSettings().remove("recent/files"); rebuildRecentMenu(); if (body_ && body_->currentIndex() == 0) rebuildStartPage(); });
}

void MainWindow::saveModelState() {
    if (!src_ || path_.isEmpty()) return;
    syncCurrentSection();
    QSettings st; const QString k = modelKey(path_);
    const Vec3 o = srs().origin;
    st.setValue(k + "path", QFileInfo(path_).absoluteFilePath());
    st.setValue(k + "front", front_->value()); st.setValue(k + "back", back_->value()); st.setValue(k + "backUserSet", backUserSet_);
    st.setValue(k + "hasLine", plan_->hasLine());
    if (plan_->hasLine()) {
        const SectionLine& l = plan_->line();
        st.setValue(k + "line", QStringLiteral("%1 %2 %3 %4").arg(l.a.x + o.x, 0, 'f', 4).arg(l.a.y + o.y, 0, 'f', 4).arg(l.b.x + o.x, 0, 'f', 4).arg(l.b.y + o.y, 0, 'f', 4));
    }
    st.setValue(k + "camera", QStringLiteral("%1 %2 %3").arg(plan_->cameraX() + o.x, 0, 'f', 3).arg(plan_->cameraY() + o.y, 0, 'f', 3).arg(plan_->metersPerPixel(), 0, 'g', 6));
    st.setValue(k + "sections", qs8(sectionsToJson(sections_, QFileInfo(path_).fileName().toStdString(), srsReport_.desc.shortAscii(), current_)));
    st.setValue(k + "count", int(sections_.size()));
    QStringList names; for (auto& s : sections_) names << qs8(s.name);
    st.setValue(k + "names", names.join(", "));
    for (int i = 0; i < 3; ++i) st.remove(k + QStringLiteral("thumb%1").arg(i));
    int saved = 0;
    for (int i = 0; i < int(sections_.size()) && saved < 3; ++i) {
        auto it = thumbs_.find(i);
        if (it == thumbs_.end() || it->second.isNull()) continue;
        QByteArray ba;
        QBuffer buf(&ba);
        buf.open(QIODevice::WriteOnly);
        it->second.save(&buf, "PNG");
        st.setValue(k + QStringLiteral("thumb%1").arg(saved), ba);
        st.setValue(k + QStringLiteral("thumbName%1").arg(saved), i < int(sections_.size()) ? qs8(sections_[size_t(i)].name) : QString());
        ++saved;
    }
    st.setValue(k + "srs", srsReport_.desc.horizontalEpsg ? QStringLiteral("EPSG:%1").arg(srsReport_.desc.horizontalEpsg) : qs8(srsReport_.desc.shortAscii()));
    st.setValue(k + "lastOpened", QDateTime::currentDateTime().toString(Qt::ISODate));
    writeProject(autoProjectPath(), nullptr);   // 프로젝트 파일(앱 폴더 projects\) — 다른 PC·포터블로도 작업이 따라간다(2026-10-11)
}

void MainWindow::restoreModelState() {
    sections_.clear(); thumbs_.clear(); current_ = -1;
    projectPath_.clear();
    // 프로젝트 파일이 있으면 그걸로(열기 대화상자·끌어놓기로 연 .trench.json → 없으면 projects\ 의 자동 저장본). 없으면 설정(레지스트리)
    std::shared_ptr<ProjectDoc> pj = pendingProject_; QString pjPath = pendingProjectPath_;
    pendingProject_.reset(); pendingProjectPath_.clear();
    if (!pj) {
        ProjectDoc d; QString e;
        const QString ap = autoProjectPath();
        if (!ap.isEmpty() && QFileInfo::exists(ap) && readProjectFile(ap, d, &e)) {
            // 같은 이름의 다른 모델(다른 폴더)이면 그 파일은 남의 것 — 설정으로 간다
            if (QFileInfo(qs8(d.modelPath)).fileName().compare(QFileInfo(path_).fileName(), Qt::CaseInsensitive) == 0) { pj = std::make_shared<ProjectDoc>(d); pjPath = ap; }
        }
    }
    if (pj) {
        applyProject(*pj);
        projectPath_ = pjPath;
    } else restoreFromSettings();
    if (undo_) undo_->clear();
    lastState_ = captureState();
    updateDepthChips(); refreshSectionList(); updateHeader();
}

void MainWindow::restoreFromSettings() {
    QSettings st; const QString k = modelKey(path_);
    const Vec3 o = srs().origin;
    if (st.contains(k + "sections")) {
        int cur = -1; std::string err;
        sectionsFromJson(st.value(k + "sections").toString().toStdString(), sections_, &cur, &err);
        current_ = (cur >= 0 && cur < int(sections_.size())) ? cur : (sections_.empty() ? -1 : 0);
        if (pendingSection_ >= 0 && pendingSection_ < int(sections_.size())) current_ = pendingSection_;   // 홈 썸네일로 연 단면
        pendingSection_ = -1;
    }
    if (st.contains(k + "front")) {
        QSignalBlocker b1(front_), b2(back_);
        const bool us = st.value(k + "backUserSet", false).toBool();
        front_->setValue(st.value(k + "front").toDouble());
        back_->setValue(resolveBackDepth(true, st.value(k + "back").toDouble(), us));   // 옛 0.5(기본값) → 3 m
        backUserSet_ = us;
        plan_->setBand(front_->value(), back_->value());
    }
    if (st.value(k + "hasLine").toBool()) {
        QStringList p = st.value(k + "line").toString().split(' ', Qt::SkipEmptyParts);
        if (p.size() == 4) {
            SectionLine l; l.a = {p[0].toDouble() - o.x, p[1].toDouble() - o.y}; l.b = {p[2].toDouble() - o.x, p[3].toDouble() - o.y};
            l.front = front_->value(); l.back = back_->value();
            applyingState_ = true; plan_->setLine(l, true); applyingState_ = false;
            fitNextResult_ = true;
            requestSection(true);
        }
    }
    QStringList cam = st.value(k + "camera").toString().split(' ', Qt::SkipEmptyParts);
    if (cam.size() == 3 && cam[2].toDouble() > 0) {
        double cx = cam[0].toDouble() - o.x, cy = cam[1].toDouble() - o.y, mpp = cam[2].toDouble();
        QTimer::singleShot(50, this, [this, cx, cy, mpp] { if (src_) plan_->setCamera(cx, cy, mpp); });
    }
}

// ---------------------------------------------------------------- 시작 화면
QWidget* MainWindow::buildStartPage() {
    auto* w = new QWidget; w->setObjectName("startPage"); w->setAttribute(Qt::WA_StyledBackground);
    new QVBoxLayout(w);
    return w;
}

// 레이아웃을 재귀로 비운다(QLayout::takeAt 문서의 안전한 비우기 + 하위 레이아웃 재귀): 위젯은 숨기고 deleteLater, 하위 레이아웃은 먼저 비운 뒤 항목으로 delete.
// 전에는 최상위 항목(격자 레이아웃 하나)만 delete 해 격자 안 위젯이 startPage_ 의 자식으로 남아 (0,0)에 겹쳐 그려졌다(AC14 — 사용자 버그 보고 2026-10-10)
void MainWindow::clearLayout(QLayout* lay) {
    if (!lay) return;
    while (QLayoutItem* it = lay->takeAt(0)) {
        if (QWidget* w = it->widget()) { w->hide(); w->deleteLater(); }
        else if (QLayout* sub = it->layout()) clearLayout(sub);
        delete it;
    }
}

// 홈(디자인 v5 C3 · 스펙 §7 · Strata 홈과 같은 뼈대): 이름 · 설명 · 단추 둘 | 이어서 작업 카드 | 최근 모델 삼선표 + 찾기 | 작업 순서 세 걸음 | 처음 쓰는 키
void MainWindow::rebuildStartPage() {
    if (!startPage_) return;
    auto* lay = static_cast<QVBoxLayout*>(startPage_->layout());
    clearLayout(lay);
    lay->setContentsMargins(48, 24, 48, 0); lay->setSpacing(0);
    const QStringList rf = recentFiles();
    QSettings st;
    auto meta = [&](const QString& f, const QString& key) { return st.value(modelKey(f) + key); };
    auto ago = [](const QString& iso) {
        QDateTime t = QDateTime::fromString(iso, Qt::ISODate);
        if (!t.isValid()) return QStringLiteral("—");
        qint64 s = t.secsTo(QDateTime::currentDateTime());
        if (s < 3600) return QStringLiteral("%1분 전").arg(std::max<qint64>(1, s / 60));
        if (s < 86400) return QStringLiteral("오늘 %1").arg(t.toString("HH:mm"));
        if (s < 2 * 86400) return QStringLiteral("어제 %1").arg(t.toString("HH:mm"));
        return t.toString("yyyy-MM-dd");
    };
    auto dash = [](const QString& s) { return s.isEmpty() ? QStringLiteral("—") : s; };
    auto* g = new QGridLayout; g->setContentsMargins(0, 0, 0, 0); g->setHorizontalSpacing(48); g->setVerticalSpacing(24);
    g->setColumnMinimumWidth(0, 420); g->setColumnStretch(1, 1); g->setColumnMinimumWidth(2, 280);   // 카드 오른쪽 칸(280)과 같은 폭
    g->setRowStretch(1, 1);

    // ---- 왼쪽 위: 앱 아이콘 56 · 「Trench | 발굴 평·단면」 · 설명 · 「모델 열기 Ctrl+O」 「최근 ▾」 · 끌어 놓기 안내
    auto* hero = new QWidget; { auto* v = new QVBoxLayout(hero); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(12);   // 카드 위끝과 같은 줄(검토 UI 2)
        auto* nm = new QHBoxLayout; nm->setSpacing(14);
        auto* ic = new QLabel; ic->setPixmap(kerf::appPixmap(56, devicePixelRatioF())); ic->setFixedSize(56, 56); ic->setAccessibleName(QStringLiteral("Trench"));   // 앱 아이콘 = 「T는 트렌치의 단면」(T2) — 이 화면의 dpr 로 굽는다(홈은 보일 때마다 다시 그림)
        nm->addWidget(ic); nm->addWidget(lab(QStringLiteral("Trench"), "heroName"));
        auto* sep = new QFrame; sep->setObjectName("homeSep"); sep->setFixedSize(1, 24);   // 이름 | 부제 가름선 — 라벨 테두리 대신(라벨 테두리는 sizeHint 에 여백이 더해져 줄임표 글이 짓눌려 보임)
        auto* sub = elideLab(lab(QStringLiteral("발굴 평·단면"), "homeSub"), Qt::ElideRight);   // 이름 글꼴이 넓어도(Batang 없는 PC · offscreen) 왼쪽 칸 420 을 넘기지 않게 부제가 줄임(T1 — 「Trench」 가 「Kerf」 보다 길다)
        nm->addWidget(sep, 0, Qt::AlignVCenter); nm->addWidget(sub, 1, Qt::AlignVCenter);
        v->addLayout(nm);
        auto* desc = lab(QStringLiteral("3MX · 3SM · OBJ 실사 메시에서 평면도 · 단면도를 잘라 보고서 도면 밑그림(SVG · DXF)까지 만듭니다."), "homeLead"); desc->setWordWrap(true);
        v->addWidget(desc);
        auto* bt = new QHBoxLayout; bt->setSpacing(8);
        auto* ob = new QPushButton(kerf::icon(QStringLiteral("folder"), 16, theme::Hand), QStringLiteral("모델 열기   Ctrl+O")); ob->setObjectName(rf.isEmpty() ? "primary" : "homeBtn"); ob->setFixedHeight(40);   // 최근 모델이 없으면 이것이 흙색 주 단추 하나(검토 UI 1)
        QObject::connect(ob, &QPushButton::clicked, this, [this] { chooseOpen(); });
        auto* rb = new QToolButton; rb->setObjectName("homeBtn"); rb->setText(QStringLiteral("최근 ▾")); rb->setIcon(kerf::icon(QStringLiteral("clock"), 16, theme::Hand)); rb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        rb->setFixedHeight(40); rb->setMenu(recentMenu_); rb->setPopupMode(QToolButton::InstantPopup); rb->setEnabled(!rf.isEmpty()); rb->setFocusPolicy(Qt::NoFocus);
        bt->addWidget(ob); bt->addWidget(rb); bt->addStretch();
        v->addLayout(bt);
        auto* drop = elideLab(lab(QStringLiteral("파일을 창 어디에나 끌어다 놓아도 열립니다 · *.3mx(권장) · *.3sm · *.obj"), "lab"), Qt::ElideRight);   // 한 줄 글이 창 최소 폭을 밀어내지 않게(B5) + 좁으면 줄임표(#3c)
        v->addWidget(drop);
        v->addStretch(); }
    g->addWidget(hero, 0, 0);

    // ---- 이어서 작업 카드(먹색 테): 이름 명조 40 · 보조 줄 · 칸 6 · 오른쪽 흙색 「이어서 열기 →」 + 단면 썸네일
    const QString f0 = rf.isEmpty() ? QString() : rf.first();
    const bool ok0 = !f0.isEmpty() && QFileInfo::exists(f0);
    const int cnt = f0.isEmpty() ? 0 : meta(f0, "count").toInt();
    auto* card = new QFrame; card->setObjectName("continueCard"); card->setAttribute(Qt::WA_StyledBackground);
    { auto* h = new QHBoxLayout(card); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(0);
        auto* body = new QWidget; auto* bv = new QVBoxLayout(body); bv->setContentsMargins(0, 0, 0, 0); bv->setSpacing(0);
        auto* top = new QWidget; { auto* tv = new QVBoxLayout(top); tv->setContentsMargins(20, 14, 20, 12); tv->setSpacing(2);
            auto* cap = new QHBoxLayout; cap->addWidget(lab(QStringLiteral("이어서 작업"), "cap")); cap->addStretch(); cap->addWidget(lab(f0.isEmpty() ? QString() : (ok0 ? QStringLiteral("저장됨 · 이 PC") : QStringLiteral("원본 없음")), "cap"));
            tv->addLayout(cap);
            tv->addWidget(elideLab(lab(f0.isEmpty() ? QStringLiteral("아직 연 모델이 없습니다") : QFileInfo(f0).completeBaseName(), f0.isEmpty() ? "ccEmpty" : "heroName"), f0.isEmpty() ? Qt::ElideRight : Qt::ElideMiddle));   // 빈 상태는 20 Muted(검토 UI 1). 긴 모델 이름은 가운데 줄임표(B5 review-code blocking 1)
            auto* ccSub = elideLab(lab(f0.isEmpty() ? QStringLiteral("모델을 열면 마지막 단면선 · 뒤 깊이 · 화면이 여기 남습니다")
                                                    : (cnt ? QStringLiteral("마지막 단면 %1 · 뒤 %2 m 그대로 열립니다").arg(meta(f0, "names").toString().section(QStringLiteral(" · "), 0, 0), QString::number(meta(f0, "back").toDouble(), 'f', 2))
                                                           : QStringLiteral("단면선을 그으면 여기에 남습니다")), "lab"), Qt::ElideRight);   // 한 줄 글이 창 최소 폭을 밀어내지 않게(B5) + 줄임표
            tv->addWidget(ccSub); }
        bv->addWidget(top);
        bv->addStretch(1);   // 칸 줄은 카드 바닥에(스펙 §7: 칸 높이 48)
        auto* cells = new QWidget; cells->setObjectName("ccCells"); cells->setAttribute(Qt::WA_StyledBackground); cells->setFixedHeight(56); cells->setVisible(!f0.isEmpty());
        cells->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);   // 칸 내용 폭이 창 최소 폭을 밀지 않게(1280 · 긴 이름)
        { auto* ch = new QHBoxLayout(cells); ch->setContentsMargins(0, 0, 0, 0); ch->setSpacing(0);
            const QString folder = f0.isEmpty() ? QStringLiteral("—") : QFileInfo(f0).absolutePath().split(QLatin1Char('/'), Qt::SkipEmptyParts).join(QStringLiteral(" › "));   // 화판: 역슬래시는 한글 글꼴에서 ₩ 로 보임
            const QString sheets = meta(f0, "sheets").toString();
            const QStringList ks = {QStringLiteral("폴더"), QStringLiteral("마지막 열림"), QStringLiteral("수평 좌표계"), QStringLiteral("단면"), QStringLiteral("도면")};
            const QStringList vs = {folder, f0.isEmpty() ? QStringLiteral("—") : ago(meta(f0, "lastOpened").toString()), dash(meta(f0, "srs").toString()),
                                    f0.isEmpty() ? QStringLiteral("—") : QStringLiteral("%1개").arg(cnt), sheets.isEmpty() ? QStringLiteral("—") : sheets};
            for (int i = 0; i < 5; ++i) {
                auto* c = new QWidget; c->setObjectName(i ? "ccCell" : "ccCellFirst"); c->setAttribute(Qt::WA_StyledBackground);
                auto* cv = new QVBoxLayout(c); cv->setContentsMargins(12, 6, 12, 6); cv->setSpacing(0);
                cv->addWidget(lab(ks[i], "cap"));   // 칸 이름은 줄이지 않는다(스펙 §7 — 줄임표는 칸 값만, rework 2 UI B1)
                auto* vl = elideLab(lab(vs[i]), i == 0 ? Qt::ElideMiddle : i == 2 ? Qt::ElideLeft : Qt::ElideRight);   // 칸 폭은 fitHomeCells 가 정한다 — 폴더(경로)만 늘고 줄며 가운데 줄임표, 좌표계 · 높이는 줄여야 할 때 앞쪽부터(번호 끝자리 지킴)
                if (i == 2) vl->setProperty("elideCode", true);
                cv->addWidget(vl);
                ch->addWidget(c);
            } }
        bv->addWidget(cells);
        h->addWidget(body, 1);
        auto* side = new QWidget; side->setObjectName("ccSide"); side->setAttribute(Qt::WA_StyledBackground); side->setFixedWidth(280);
        { auto* sv = new QVBoxLayout(side); sv->setContentsMargins(20, 16, 20, 16); sv->setSpacing(10);
            auto* go = new QPushButton(ok0 ? QStringLiteral("이어서 열기  →") : (f0.isEmpty() ? QStringLiteral("모델 열기…") : QStringLiteral("원본 없음"))); go->setObjectName("primary"); go->setFixedHeight(40);
            go->setEnabled(ok0 || f0.isEmpty()); go->setVisible(!f0.isEmpty());   // 빈 상태의 주 단추는 왼쪽 「모델 열기」 하나
            go->setToolTip(QStringLiteral("마지막 단면선·두께·화면을 그대로 되살립니다 (Ctrl+Shift+O)"));
            QObject::connect(go, &QPushButton::clicked, this, [this, f0] { if (f0.isEmpty()) chooseOpen(); else openFile(f0); });
            sv->addWidget(go);
            QVector<QPair<QImage, QString>> thumbs;
            for (int i = 0; i < 3 && !f0.isEmpty(); ++i) {
                QByteArray ba = meta(f0, QStringLiteral("thumb%1").arg(i)).toByteArray();
                if (ba.isEmpty()) continue;
                QImage im; im.loadFromData(ba, "PNG");
                if (!im.isNull()) thumbs.push_back({im, meta(f0, QStringLiteral("thumbName%1").arg(i)).toString()});
            }
            if (!thumbs.isEmpty()) {
                sv->addWidget(elideLab(lab(QStringLiteral("썸네일을 누르면 그 단면으로 엽니다"), "lab"), Qt::ElideRight));
                auto* tr = new QHBoxLayout; tr->setSpacing(8);
                for (int i = 0; i < thumbs.size(); ++i) {
                    auto* col = new QWidget; auto* cv = new QVBoxLayout(col); cv->setContentsMargins(0, 0, 0, 0); cv->setSpacing(2);
                    auto* pic = new QToolButton; pic->setObjectName("thumb"); pic->setFocusPolicy(Qt::NoFocus); pic->setIcon(QPixmap::fromImage(thumbs[i].first)); pic->setIconSize(QSize(72, 40)); pic->setFixedSize(76, 44);
                    pic->setToolTip(thumbs[i].second);
                    QObject::connect(pic, &QToolButton::clicked, this, [this, f0, i] { pendingSection_ = i; openFile(f0); });
                    cv->addWidget(pic); auto* nl = elideLab(lab(thumbs[i].second.isEmpty() ? QStringLiteral("단면") : thumbs[i].second, "thumbName"), Qt::ElideMiddle); nl->setAlignment(Qt::AlignCenter); cv->addWidget(nl);
                    tr->addWidget(col);
                }
                tr->addStretch(); sv->addLayout(tr);
            }
            sv->addStretch(); }
        h->addWidget(side); }
    g->addWidget(card, 0, 1, 1, 2);

    // ---- 최근 모델: 머리 「최근 모델 n」 + 찾기, 삼선표(모델 · 상태 · 수평 · 단면 · 마지막 열림), 도움말 한 줄
    auto* recent = new QWidget; { auto* rv = new QVBoxLayout(recent); rv->setContentsMargins(0, 0, 0, 0); rv->setSpacing(8);
        auto* hd = new QHBoxLayout; hd->setSpacing(8);
        hd->addWidget(lab(QStringLiteral("최근 모델"), "sectionHead")); hd->addWidget(lab(QString::number(rf.size()), "cap")); hd->addStretch();
        auto* find = new QLineEdit; find->setObjectName("homeFind"); find->setPlaceholderText(QStringLiteral("이름 · 폴더로 찾기")); find->setClearButtonEnabled(true); find->setFixedSize(260, 32);
        find->addAction(kerf::icon(QStringLiteral("search"), 14, theme::Faint), QLineEdit::LeadingPosition); find->setEnabled(!rf.isEmpty());
        hd->addWidget(find);
        rv->addLayout(hd);
        auto* tbl = new QTableWidget(std::max<int>(1, int(rf.size())), 5); tbl->setObjectName("recentTable");
        tbl->setHorizontalHeaderLabels({QStringLiteral("모델"), QStringLiteral("상태"), QStringLiteral("수평"), QStringLiteral("단면"), QStringLiteral("마지막 열림")});
        tbl->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tbl->verticalHeader()->setVisible(false); tbl->setEditTriggers(QAbstractItemView::NoEditTriggers); tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
        tbl->setSelectionMode(QAbstractItemView::SingleSelection); tbl->setShowGrid(false); tbl->setFocusPolicy(Qt::TabFocus);
        tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int c = 1; c < 5; ++c) tbl->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
        tbl->horizontalHeader()->setFixedHeight(28);
        tbl->verticalHeader()->setDefaultSectionSize(48);
        if (rf.isEmpty()) { tbl->setItem(0, 0, new QTableWidgetItem(QStringLiteral("열어 본 모델이 여기에 쌓입니다"))); tbl->setSpan(0, 0, 1, 5); }
        for (int i = 0; i < rf.size(); ++i) {
            const QString& f = rf[i];
            const bool ok = QFileInfo::exists(f);
            auto* it0 = new QTableWidgetItem; it0->setToolTip(QDir::toNativeSeparators(f)); tbl->setItem(i, 0, it0);
            { auto* cell = new QWidget; cell->setAttribute(Qt::WA_TransparentForMouseEvents);
                auto* cv = new QVBoxLayout(cell); cv->setContentsMargins(8, 4, 8, 4); cv->setSpacing(0);
                auto* nm = elideLab(lab(QFileInfo(f).fileName(), ok ? "secName" : "faintName"), Qt::ElideMiddle);
                auto* pth = elideLab(lab(QDir::toNativeSeparators(QFileInfo(f).absolutePath()), "monoFaint"), Qt::ElideMiddle);
                cv->addWidget(nm); cv->addWidget(pth);
                tbl->setCellWidget(i, 0, cell); }
            tbl->setItem(i, 1, new QTableWidgetItem(ok ? QStringLiteral("열 수 있음") : QStringLiteral("● 원본 없음")));
            if (!ok) tbl->item(i, 1)->setForeground(theme::Block);
            tbl->setItem(i, 2, new QTableWidgetItem(dash(meta(f, "srs").toString())));
            auto* ci = new QTableWidgetItem(QString::number(meta(f, "count").toInt())); ci->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter); tbl->setItem(i, 3, ci);
            auto* ai = new QTableWidgetItem(ago(meta(f, "lastOpened").toString())); ai->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter); tbl->setItem(i, 4, ai);
        }
        QObject::connect(tbl, &QTableWidget::cellDoubleClicked, this, [this, rf](int r, int) { if (r < rf.size() && QFileInfo::exists(rf[r])) openFile(rf[r]); });
        QObject::connect(find, &QLineEdit::textChanged, tbl, [tbl, rf](const QString& t) {
            for (int r = 0; r < rf.size(); ++r) tbl->setRowHidden(r, !t.trimmed().isEmpty() && !rf[r].contains(t.trimmed(), Qt::CaseInsensitive));
        });
        tbl->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(tbl, &QTableWidget::customContextMenuRequested, this, [this, tbl, rf](const QPoint& p) {
            const int r = tbl->rowAt(p.y()); if (r < 0 || r >= rf.size()) return;
            QMenu m(this);
            m.addAction(QStringLiteral("열기"), this, [this, rf, r] { openFile(rf[r]); });
            m.addAction(QStringLiteral("폴더 열기"), this, [rf, r] { QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(rf[r]).absolutePath())); });
            m.addAction(QStringLiteral("목록에서 지우기(파일은 그대로)"), this, [this, rf, r] { QStringList l = rf; l.removeAt(r); QSettings().setValue("recent/files", l); rebuildRecentMenu(); rebuildStartPage(); });
            m.exec(tbl->viewport()->mapToGlobal(p));
        });
        tbl->setFixedHeight(28 + 48 * std::min(8, std::max(1, int(rf.size()))) + 2);   // 줄 수만큼(최대 8줄, 넘으면 스크롤) — 아래 선이 마지막 줄에 붙는다(검토 UI 2)
        rv->addWidget(tbl, 0);
        auto* help = elideLab(lab(QStringLiteral("두 번 누르면 엶 · 오른쪽 클릭 = 폴더 열기 · 목록에서 지우기(모델 파일은 그대로)"), "lab"), Qt::ElideRight);   // 한 줄 글이 창 최소 폭을 밀어내지 않게(B5) + 줄임표
        rv->addWidget(help);
        rv->addStretch(1); }
    g->addWidget(recent, 1, 0, 1, 2);

    // ---- 작업 순서 세 걸음(카드 없음): ✓ 끝 · ● 다음 · 숫자 나중
    auto* steps = new QWidget; { auto* sv = new QVBoxLayout(steps); sv->setContentsMargins(0, 0, 0, 0); sv->setSpacing(14);
        auto* hd = new QHBoxLayout; hd->addStrut(32); hd->addWidget(lab(QStringLiteral("작업 순서"), "sectionHead"));   // 「최근 모델」 줄과 같은 높이
        {   // 「<모델> 기준」: 늘림 칸 대신 이 글이 남는 폭을 다 받고 오른쪽 정렬 — 앞에 늘림 칸이 있으면 Ignored 라벨이 폭 0 으로 사라졌다(rework 2 code B1). 긴 이름은 가운데 줄임표(창 최소 폭을 밀지 않음)
            auto* basis = elideLab(lab(f0.isEmpty() ? QString() : QStringLiteral("%1 기준").arg(QFileInfo(f0).completeBaseName()), "cap"), Qt::ElideMiddle);
            basis->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            hd->addWidget(basis, 1);
        }
        sv->addLayout(hd);
        int done = 0;
        if (ok0) done = (meta(f0, "hasLine").toBool() || meta(f0, "count").toInt() > 0) ? 2 : 1;
        if (ok0 && !meta(f0, "sheets").toString().isEmpty()) done = 3;
        const char* st3[3][2] = {{"모델 열기", "3MX 폴더의 .3mx 파일을 열면 평면이 위에서 본 모양으로 나옵니다."},
                                 {"단면선 긋기", "평면에서 A, A′를 찍으면 오른쪽에 단면이 바로 나옵니다. 잘린 돌은 H로 빗금."},
                                 {"도면 · 저장", "「도면」 탭에서 축척 · 용지를 정하고 SVG로 저장해 일러스트레이터에서 엽니다."}};
        for (int i = 0; i < 3; ++i) {
            auto* row = new QHBoxLayout; row->setSpacing(12);
            auto* mark = new QLabel; mark->setObjectName("stepMark"); mark->setFixedSize(22, 22); mark->setAlignment(Qt::AlignCenter);
            const bool isDone = i < done, isNext = i == done;
            mark->setProperty("state", isDone ? "ok" : isNext ? "now" : "later");
            if (isDone) mark->setPixmap(kerf::glyph(QStringLiteral("check"), 12, theme::Card, devicePixelRatioF())); else mark->setText(QString::number(i + 1));   // 다음 단계도 숫자(「● 다음」 표시가 따로 있음, 스펙 §7)
            row->addWidget(mark, 0, Qt::AlignTop);
            auto* tt = new QHBoxLayout; tt->setSpacing(6);
            tt->addWidget(lab(QString::fromUtf8(st3[i][0]), isNext ? "secName" : nullptr));
            if (isNext) tt->addWidget(lab(QStringLiteral("● 다음"), "nextMark"));
            tt->addStretch();
            auto* ds = lab(QString::fromUtf8(st3[i][1]), "lab"); ds->setWordWrap(true);
            auto* col = new QWidget; auto* cv = new QVBoxLayout(col); cv->setContentsMargins(0, 0, 0, 0); cv->setSpacing(2); cv->addLayout(tt); cv->addWidget(ds);
            row->addWidget(col, 1);
            sv->addLayout(row);
        }
        sv->addStretch(); }
    g->addWidget(steps, 1, 2);

    // ---- 처음 쓰는 키(선 위): S · 1–5 · H · O · Ctrl+P · Ctrl+Z + 오른쪽 「모든 키 F1」
    auto* keys = new QWidget; keys->setObjectName("homeKeys"); keys->setAttribute(Qt::WA_StyledBackground);
    keys->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);   // 키 줄이 창 최소 폭을 밀어내지 않게(B5): 홈이 보이는 채로 뜨면 이 줄(1495)이 창을 1591 로 넓혔다(실측, --size 1280x800)
    { auto* kh = new QHBoxLayout(keys); kh->setContentsMargins(0, 12, 0, 16); kh->setSpacing(32);
        kh->addWidget(lab(QStringLiteral("처음 쓰는 키"), "sectionHead"));
        const char* kk[][2] = {{"S", "단면선 긋기"}, {"1–5", "뒤 깊이 0.5 · 1 · 2 · 3 · 5 m"}, {"H", "잘린 돌 칠하기"}, {"O", "윤곽 따기"}, {"Ctrl+P", "도면"}, {"Ctrl+Z", "되돌리기 200단계"}};
        for (auto& k : kk) {   // 키 묶음은 위젯 하나 — 폭이 모자라면 fitHomeKeys 가 뒤에서부터 숨긴다(스펙 §7)
            auto* grp = new QWidget; grp->setProperty("keyGroup", true);
            auto* one = new QHBoxLayout(grp); one->setContentsMargins(0, 0, 0, 0); one->setSpacing(8); one->addWidget(kbd(QString::fromUtf8(k[0]))); one->addWidget(lab(QString::fromUtf8(k[1]), "lab"));
            kh->addWidget(grp);
        }
        kh->addStretch();
        auto* all = new QPushButton(QStringLiteral("모든 키  F1")); all->setObjectName("quietLink"); all->setFlat(true); all->setFocusPolicy(Qt::NoFocus);
        QObject::connect(all, &QPushButton::clicked, this, [this] { dlgKeys(); });
        kh->addWidget(all); }
    g->addWidget(keys, 2, 0, 1, 3);
    lay->addLayout(g, 1);
}

// 홈의 「작업 순서」는 rebuildStartPage 가 그린다 — 홈이 보이는 동안 상태가 바뀌면 다시 그린다
void MainWindow::refreshSteps() {
    if (body_ && body_->currentIndex() == 0 && startPage_ && startPage_->isVisible()) rebuildStartPage();
}

void MainWindow::showStart(bool on) {
    if (!body_) return;
    if (on) rebuildStartPage();
    body_->setCurrentIndex(on ? 0 : 1);
    if (guide_ && on) { guide_->hide(); undoBtns_->hide(); }
    if (viewTabs_) { QSignalBlocker b(viewTabs_); viewTabs_->setTabEnabled(1, bool(src_)); viewTabs_->setCurrentIndex(on ? 0 : 1); }
    setRibbonContext(on ? 0 : 1);
    showStatus(on ? (src_ ? QStringLiteral("%1 열림 — 「단면」 탭에서 이어서 작업합니다").arg(QFileInfo(path_).completeBaseName()) : QStringLiteral("먼저 모델을 열거나, 파일을 창에 끌어다 놓으세요.")) : QString());
    // 홈은 위에서 한 번 그렸다 — refreshSteps() 로 한 번 더 그리지 않는다(AC14: 두 번 그리기가 옛 위젯 잔류를 드러냈다. 홈이 보이는 동안의 갱신은 updateHeader → refreshSteps)
}

// ---------------------------------------------------------------- 작은 동작
void MainWindow::setBackDepth(double v) {
    if (!back_) return;
    backUserSet_ = true;
    QSettings().setValue("section/backUserSet", true);
    back_->setValue(std::clamp(v, 0.0, kMaxBandDepth));
    updateDepthChips();
    showStatus(QStringLiteral("뒤 깊이 %1 m — 입면 영상에 단면선 뒤 %1 m 까지 보입니다").arg(back_->value(), 0, 'g', 3));
}

void MainWindow::updateDepthChips() {
    const double vals[5] = {0.5, 1, 2, 3, 5};
    for (int i = 0; i < 5; ++i) if (depthChip_[i]) depthChip_[i]->setChecked(std::fabs(back_->value() - vals[i]) < 1e-6);
}

void MainWindow::shiftLine(double d) {
    if (!src_ || !plan_->hasLine()) return;
    SectionLine l = plan_->line();
    SectionFrame fr(l);
    Vec2 off = fr.n * d;
    l.a = l.a + off; l.b = l.b + off;
    plan_->setLine(l, true);
    requestSection(true);
    syncCurrentSection();
    commitState(QStringLiteral("평행 이동"), 2);
    showStatus(QStringLiteral("단면선을 %1 %2 m 옮김 ([ ] 0.1 m · { } 1 m)").arg(d > 0 ? QStringLiteral("보는 쪽으로") : QStringLiteral("반대쪽으로")).arg(std::fabs(d), 0, 'g', 3));
    updateHeader();
}

// 단면선 굵기(화면만): 칩 전부(입면 아래 줄 · 더보기) · 설정 · 단면 그리기를 한 번에
void MainWindow::setSectionLineWidth(double px) {
    QSettings().setValue("view/sectionLineWidth", px);
    for (QToolButton* c : lineWidthChips_) c->setChecked(std::fabs(c->property("lineWidth").toDouble() - px) < 0.01);
    if (section_) section_->setStyle(secStyle());
    notifySheetDoc();
}

void MainWindow::applyHighContrast(bool on) {
    highContrast_ = on;
    QSettings().setValue("ui/highContrast", on);
    if (plan_) plan_->setCursorHighContrast(on);
    qApp->setStyleSheet(theme::styleSheet(on));
    section_->setStyle(secStyle());
    secHeaderKey_.clear();   // 시트가 바뀌면 글 폭이 달라질 수 있다 — 머리를 다시 맞춘다
    updateHeader();
}

// ---------------------------------------------------------------- F1 창(키 표 = 실제로 연결된 키 — T9 · 감사 keys-table-match)
// 표의 한 줄 = 보이는 키 글 + 하는 일 + 그 줄이 말하는 실제 단축키들. 표와 코드가 어긋나면 감사가 잡는다(없는 키를 적거나, 있는 키를 빼먹으면 실패)
struct KeyRow { QString key, what; QStringList seqs; };
static const QList<KeyRow>& keyRows() {
    static const QList<KeyRow> rows = {
        {QStringLiteral("Ctrl+O"), QStringLiteral("모델 열기"), {QStringLiteral("Ctrl+O")}},
        {QStringLiteral("Ctrl+Shift+O"), QStringLiteral("최근 모델 다시 열기(마지막 단면선·화면 그대로)"), {QStringLiteral("Ctrl+Shift+O")}},
        {QStringLiteral("S"), QStringLiteral("단면선 긋기 — A 클릭, A′ 클릭"), {QStringLiteral("S")}},
        {QStringLiteral("Shift (긋는 중)"), QStringLiteral("동서·남북으로 고정"), {}},
        {QStringLiteral("Esc"), QStringLiteral("그리기 취소"), {QStringLiteral("Esc")}},
        {QStringLiteral("1 – 5"), QStringLiteral("뒤 깊이 0.5 · 1 · 2 · 3 · 5 m (기본 3 m — 입면도용 배경)"), {QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4"), QStringLiteral("5")}},
        {QStringLiteral("[  ]"), QStringLiteral("단면선 평행 이동 −0.1 · +0.1 m (보는 쪽 +)"), {QStringLiteral("["), QStringLiteral("]")}},
        {QStringLiteral("{  }"), QStringLiteral("단면선 평행 이동 −1 · +1 m"), {QStringLiteral("{"), QStringLiteral("}")}},
        {QStringLiteral("X"), QStringLiteral("세로 과장 ×1 → ×2 → ×5 → ×10 (화면만 · 도면은 1:1)"), {QStringLiteral("X")}},
        {QStringLiteral("R"), QStringLiteral("방향 반전(A↔A′)"), {QStringLiteral("R")}},
        {QStringLiteral("N"), QStringLiteral("새 단면"), {QStringLiteral("N")}},
        {QStringLiteral("I · L · V · D"), QStringLiteral("입면 영상 · 잘린 선 · 레벨선 · 거리 눈금 켜기 · 끄기"), {QStringLiteral("I"), QStringLiteral("L"), QStringLiteral("V"), QStringLiteral("D")}},
        {QStringLiteral("F · T"), QStringLiteral("맞춤 · 위에서 보기"), {QStringLiteral("F"), QStringLiteral("T")}},
        {QStringLiteral("+  −"), QStringLiteral("확대 · 축소"), {QStringLiteral("+"), QStringLiteral("-")}},
        {QStringLiteral("Ctrl+1 · Ctrl+2"), QStringLiteral("평면 · 단면 보이기 켜기 · 끄기"), {QStringLiteral("Ctrl+1"), QStringLiteral("Ctrl+2")}},
        {QStringLiteral("F11"), QStringLiteral("전체화면 · 원래 크기"), {QStringLiteral("F11")}},
        {QStringLiteral("Ctrl+P"), QStringLiteral("도면 — 평면도 · 단면도 조판"), {QStringLiteral("Ctrl+P")}},
        {QStringLiteral("Ctrl+D · Ctrl+E"), QStringLiteral("단면 DXF · 단면 영상(PNG · TIFF · GeoTIFF)"), {QStringLiteral("Ctrl+D"), QStringLiteral("Ctrl+E")}},
        {QStringLiteral("F2"), QStringLiteral("고른 단면 이름 · 메모 바꾸기"), {QStringLiteral("F2")}},
        {QStringLiteral("Del"), QStringLiteral("고른 단면 지우기(확인 창이 뜸 · 모델 파일은 그대로)"), {QStringLiteral("Del")}},
        {QStringLiteral("Ctrl+Z · Ctrl+Y"), QStringLiteral("되돌리기 · 다시"), {QStringLiteral("Ctrl+Z"), QStringLiteral("Ctrl+Y"), QStringLiteral("Ctrl+Shift+Z")}},
        {QStringLiteral("Ctrl+Q"), QStringLiteral("끝내기"), {QStringLiteral("Ctrl+Q")}},
        {QStringLiteral("F1"), QStringLiteral("이 표"), {QStringLiteral("F1")}},
    };
    return rows;
}

QDialog* MainWindow::keysDialog() {
    auto* d = new QDialog(this); d->setWindowTitle(QStringLiteral("단축키"));
    auto* v = new QVBoxLayout(d); v->setContentsMargins(20, 16, 20, 14); v->setSpacing(10);
    const QList<KeyRow>& rows = keyRows();
    const int n = int(rows.size()), half = (n + 1) / 2;
    auto* t = new QTableWidget(half, 4); t->setObjectName("keysTable");   // 두 열(키 · 하는 일 | 키 · 하는 일)
    t->setHorizontalHeaderLabels({QStringLiteral("키"), QStringLiteral("하는 일"), QStringLiteral("키"), QStringLiteral("하는 일")});
    t->verticalHeader()->setVisible(false); t->setEditTriggers(QAbstractItemView::NoEditTriggers); t->setSelectionMode(QAbstractItemView::NoSelection); t->setShowGrid(false);
    t->setFocusPolicy(Qt::NoFocus); t->setWordWrap(true);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    t->horizontalHeader()->setFixedHeight(28);
    t->verticalHeader()->setDefaultSectionSize(30);
    for (int i = 0; i < n; ++i) {
        const int r = i % half, c = (i / half) * 2;
        auto* k = new QTableWidgetItem(rows[i].key); k->setFont(theme::monoFont(12));
        t->setItem(r, c, k); t->setItem(r, c + 1, new QTableWidgetItem(rows[i].what));
    }
    t->setColumnWidth(0, 140); t->setColumnWidth(1, 350); t->setColumnWidth(2, 140); t->setColumnWidth(3, 350);
    t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); t->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    t->setFixedHeight(28 + 30 * half + 8);
    v->addWidget(t);
    // 프로그램 정보는 여기 한 줄(T6 · 더보기 삭제)
    auto* about = new QPushButton(QStringLiteral("Trench %1 · 프로그램 정보").arg(QString::fromUtf8(kVersion))); about->setObjectName("aboutLink"); about->setFlat(true); about->setCursor(Qt::PointingHandCursor);
    QObject::connect(about, &QPushButton::clicked, this, [this] { dlgAbout(); });
    v->addWidget(about, 0, Qt::AlignLeft);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close);
    bb->button(QDialogButtonBox::Close)->setText(QStringLiteral("닫기  Esc"));
    QObject::connect(bb, &QDialogButtonBox::rejected, d, &QDialog::reject);
    v->addWidget(bb);
    d->setFixedWidth(20 + 140 + 350 + 140 + 350 + 20 + 8);
    return d;
}

void MainWindow::dlgKeys() {
    QDialog* d = keysDialog();
    d->exec();
    d->deleteLater();
}

// ---------------------------------------------------------------- 생성자
MainWindow::MainWindow() {
    qApp->installEventFilter(this);   // 모든 단추의 키보드 초점 테(검토 UI 6)
    setWindowTitle(QStringLiteral("Trench %1").arg(QString::fromUtf8(kVersion)));
    setAcceptDrops(true);
    highContrast_ = QSettings().value("ui/highContrast", false).toBool();
    using I = theme::Ico;
    undo_ = new QUndoStack(this);
    undo_->setUndoLimit(200);
    makeAction("open", QStringLiteral("열기"), "Open", I::Open, "Ctrl+O");
    makeAction("recent", QStringLiteral("최근"), "Recent", I::Recent, "Ctrl+Shift+O");
    makeAction("close", QStringLiteral("닫기"), "Close", I::Close);
    makeAction("about", QStringLiteral("프로그램 정보"), "About", I::Info);
    makeAction("keys", QStringLiteral("단축키"), "Shortcuts", I::Keys, "F1");
    makeAction("quit", QStringLiteral("끝내기"), "Exit", I::Close, "Ctrl+Q");
    makeAction("draw", QStringLiteral("단면선 긋기"), "Draw Section", I::Draw, "S", true);
    makeAction("flip", QStringLiteral("방향 반전"), "Flip Direction", I::Flip, "R");
    makeAction("move", QStringLiteral("평행 이동"), "Offset", I::Move);
    makeAction("addsec", QStringLiteral("새 단면"), "New Section", I::Add, "N");
    makeAction("clear", QStringLiteral("단면선 지우기"), "Clear Line", I::Clear);
    makeAction("fit", QStringLiteral("맞춤"), "Fit View", I::Fit, "F");
    makeAction("top", QStringLiteral("위에서"), "Top", I::Orbit, "T");   // 「평면」 칩(map)과 다른 아이콘(검토 UI 6)
    makeAction("zoomin", QStringLiteral("확대"), "Zoom In", I::ZoomIn, "+");
    makeAction("zoomout", QStringLiteral("축소"), "Zoom Out", I::ZoomOut, "-");
    makeAction("image", QStringLiteral("입면 영상"), "Image", I::Image, "I", true)->setChecked(true);
    makeAction("line", QStringLiteral("잘린 선"), "Cut Line", I::Line, "L", true)->setChecked(true);
    makeAction("levels", QStringLiteral("레벨선"), "Level Lines", I::Levels, "V", true)->setChecked(true);
    makeAction("smooth", QStringLiteral("평활"), "Smooth", I::Smooth, QString(), true);
    makeAction("fade", QStringLiteral("깊이 음영"), "Depth Shading", I::Band, QString(), true);
    makeAction("ticks", QStringLiteral("거리 눈금"), "Distance Ticks", I::Ticks, "D", true)->setChecked(QSettings().value("section/ticks", true).toBool());
    action("ticks")->setToolTip(QStringLiteral("거리 눈금 (D) — 입면 위 세로 눈금(1 m 마다 · 5 m 굵게, 아래 거리 숫자와 같은 자리) — 토층을 그릴 자리를 m 로 읽습니다(도면 · 내보내기도 같음)"));
    action("fade")->setToolTip(QStringLiteral("깊이 음영 (Depth Shading) — 입면 영상에서 단면선보다 뒤에 있는 면일수록 옅게 그려 빨간 단면선이 잘 보이게 합니다(내보내기도 같음)"));
    makeAction("info", QStringLiteral("단면 정보"), "Section Info", I::Info);
    makeAction("view1", QStringLiteral("평면"), "Plan", I::View1, "Ctrl+1", true)->setChecked(true);
    makeAction("view2", QStringLiteral("단면"), "Section", I::View2, "Ctrl+2", true)->setChecked(true);
    makeAction("full", QStringLiteral("전체화면"), "Full Screen", I::Max, "F11");
    makeAction("listpanel", QStringLiteral("단면 목록"), "Section List", I::Csv, QString(), true)->setChecked(QSettings().value("ui/sectionList", true).toBool());
    makeAction("sheet", QStringLiteral("도면"), "Sheet", I::Sheet, "Ctrl+P");
    makeAction("plansheet", QStringLiteral("평면도"), "Plan Sheet", I::Plan);
    makeAction("sectionsheet", QStringLiteral("단면도"), "Section Sheet", I::Sheet);
    makeAction("dxf", QStringLiteral("단면 DXF"), "Section DXF", I::Dxf, "Ctrl+D");
    makeAction("secimg", QStringLiteral("단면 영상"), "PNG/TIFF/GeoTIFF", I::Picture, "Ctrl+E");
    makeAction("plan", QStringLiteral("평면 GeoTIFF"), "Plan GeoTIFF", I::Geo);
    makeAction("xyz", QStringLiteral("점군 XYZ"), "Points XYZ", I::Xyz);
    makeAction("las", QStringLiteral("점군 LAS"), "Points LAS", I::Las);
    makeAction("csv", QStringLiteral("단면선 CSV"), "Profile CSV", I::Csv);
    makeAction("secjson", QStringLiteral("단면 목록 내보내기"), "Export Sections", I::Csv);
    makeAction("project", QStringLiteral("프로젝트"), "Project", I::Sheet);
    makeAction("projsave", QStringLiteral("저장 (projects 폴더에)"), "Save Project", I::Sheet);
    makeAction("projsaveas", QStringLiteral("다른 이름으로 저장…"), "Save Project As", I::Sheet);
    makeAction("projopen", QStringLiteral("프로젝트 열기…"), "Open Project", I::Open);
    makeAction("secimport", QStringLiteral("단면 목록 가져오기"), "Import Sections", I::Open);
    makeAction("seclist", QStringLiteral("단면 목록"), "Section List", I::Csv);                 // v5 리본 「자료 › 목록 ▾」(내보내기 · 가져오기)
    makeAction("elevmenu", QStringLiteral("입면"), "Elevation", I::Sliders);                   // 리본이 낮을 때만 보이는 「입면 ▾」(불투명도 · 굵기 · 평활)
    makeAction("points", QStringLiteral("점군"), "Points", I::Xyz);                            // 리본 「자료 › 점군 ▾」(XYZ · LAS)
    makeAction("hatch", QStringLiteral("잘린 돌 칠하기"), "Hatch Cut Stone", I::Band, "H", true)->setEnabled(false);   // A1 UI(F단계)에서 켠다
    makeAction("outline", QStringLiteral("윤곽 따기"), "Trace Outline", I::Draw, "O", true)->setEnabled(false);          // A3 UI(G단계)에서 켠다
    makeAction("contrast", QStringLiteral("고대비"), "High Contrast", I::Band, QString(), true)->setChecked(highContrast_);
    makeAction("esc", QStringLiteral("그리기 취소"), "Cancel", I::Close, "Esc")->setEnabled(false);
    makeAction("undo", QStringLiteral("되돌리기"), "Undo", I::Undo, "Ctrl+Z")->setEnabled(false);
    makeAction("redo", QStringLiteral("다시"), "Redo", I::Redo, "Ctrl+Y")->setEnabled(false);
    action("redo")->setShortcuts({QKeySequence("Ctrl+Y"), QKeySequence("Ctrl+Shift+Z")});
    action("smooth")->setChecked(QSettings().value("section/smooth", false).toBool());
    action("fade")->setChecked(QSettings().value("section/depthFade", false).toBool());   // 기본 끔(사용자 답 2026-10-10 — 켜 두면 입면이 하얗게 바래 보였다)
    {
        auto* m = new QMenu(this);
        m->addAction(QStringLiteral("보는 쪽으로 0.1 m   ]"), this, [this] { shiftLine(0.1); });
        m->addAction(QStringLiteral("반대쪽으로 0.1 m   ["), this, [this] { shiftLine(-0.1); });
        m->addAction(QStringLiteral("보는 쪽으로 1 m   }"), this, [this] { shiftLine(1.0); });
        m->addAction(QStringLiteral("반대쪽으로 1 m   {"), this, [this] { shiftLine(-1.0); });
        action("move")->setMenu(m);
    }

    plan_ = new PlanView; section_ = new SectionView;
    auto* central = new QWidget; central->setObjectName("central");
    auto* v = new QVBoxLayout(central); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(0);
    v->addWidget(buildRibbon());
    // v5 C2: 문서 탭 「홈 · 단면 · 도면(×)」 줄 36 px 이 늘 보인다(Strata 「홈 · 지도 · 도면」과 같음). 오른쪽 구석 = 모델 이름 · 저장 상태
    docTabsRow_ = new QWidget; docTabsRow_->setObjectName("docTabsRow"); docTabsRow_->setAttribute(Qt::WA_StyledBackground); docTabsRow_->setFixedHeight(36);
    { auto* th = new QHBoxLayout(docTabsRow_); th->setContentsMargins(8, 0, 8, 0); th->setSpacing(0);
        viewTabs_ = new QTabBar; viewTabs_->setDocumentMode(true); viewTabs_->setExpanding(false); viewTabs_->setDrawBase(false);
        viewTabs_->setObjectName("docTabs"); viewTabs_->setFocusPolicy(Qt::NoFocus); viewTabs_->setIconSize(QSize(16, 16)); viewTabs_->setTabsClosable(true);
        viewTabs_->addTab(kerf::icon(QStringLiteral("house"), 16, theme::Hand), QStringLiteral("홈"));
        viewTabs_->addTab(kerf::icon(QStringLiteral("profile"), 16, theme::Hand), QStringLiteral("단면"));
        for (int i = 0; i < 2; ++i) { viewTabs_->setTabButton(i, QTabBar::RightSide, nullptr); viewTabs_->setTabButton(i, QTabBar::LeftSide, nullptr); }   // 홈 · 단면은 닫기 없음
        th->addWidget(viewTabs_, 0, Qt::AlignBottom); th->addStretch(1);
        docTabCorner_ = new QLabel; docTabCorner_->setObjectName("docTabCorner"); th->addWidget(docTabCorner_, 0, Qt::AlignVCenter); }
    v->addWidget(docTabsRow_);
    body_ = new motion::FadeStack;
    startPage_ = buildStartPage();
    body_->addWidget(startPage_);
    workArea_ = new QWidget; workArea_->setObjectName("workArea");
    { auto* wl = new QVBoxLayout(workArea_); wl->setContentsMargins(0, 0, 0, 0); wl->setSpacing(0);
        notice_ = buildNotice(); wl->addWidget(notice_);
        split_ = new QSplitter(Qt::Horizontal); split_->setHandleWidth(1);
        sidePanel_ = buildSidePanel();
        planFrame_ = buildPlanFrame(); sectionFrame_ = buildSectionFrame();
        split_->addWidget(sidePanel_); split_->addWidget(planFrame_); split_->addWidget(sectionFrame_);
        split_->setStretchFactor(0, 0); split_->setStretchFactor(1, 5); split_->setStretchFactor(2, 6);
        split_->setChildrenCollapsible(false);
        split_->setSizes({348, 520, 520});   // 스펙 §2: 왼쪽 목록(+ 아래 수치) 348, 가운데 둘은 같은 폭 — 오른쪽 판 없음(T7)
        sidePanel_->setVisible(action("listpanel")->isChecked());
        wl->addWidget(split_, 1); }
    body_->addWidget(workArea_);
    v->addWidget(body_, 1);
    v->addWidget(buildCoordBar());
    setCentralWidget(central);
    setRibbonContext(src_ ? 1 : 0);
    rebuildRecentMenu();
    setActiveView(0);

    front_->setValue(QSettings().value("section/front", 0.0).toDouble());
    {   // 뒤 깊이: 기본 3 m. 1.2.0 까지 저장된 0.5(옛 기본값)는 사용자가 바꾼 표시가 없으면 새 기본값으로
        QSettings st;
        backUserSet_ = st.value("section/backUserSet", false).toBool();
        back_->setValue(resolveBackDepth(st.contains("section/back"), st.value("section/back").toDouble(), backUserSet_));
    }
    plan_->setBand(front_->value(), back_->value());
    if (auto g = QSettings().value("ui/geometry").toByteArray(); !g.isEmpty()) restoreGeometry(g);
    else resize(1600, 950);

    // ---- 숫자키·평행 이동(작업 영역에서만: 입력 칸 타이핑과 겹치지 않게)
    {
        const double depths[5] = {0.5, 1, 2, 3, 5};
        for (int i = 0; i < 5; ++i) {
            auto* sc = new QShortcut(QKeySequence(QString::number(i + 1)), workArea_);
            sc->setContext(Qt::WidgetWithChildrenShortcut);
            double dv = depths[i];
            QObject::connect(sc, &QShortcut::activated, this, [this, dv] { if (src_) setBackDepth(dv); });
        }
        {   // X: 세로 과장 1 → 2 → 5 → 10 → 1 (화면만)
            auto* sc = new QShortcut(QKeySequence(QStringLiteral("X")), workArea_);
            sc->setContext(Qt::WidgetWithChildrenShortcut);
            QObject::connect(sc, &QShortcut::activated, this, [this] {
                const double v = section_->verticalExaggeration();
                setVex(v < 2 ? 2 : v < 5 ? 5 : v < 10 ? 10 : 1);
            });
        }
        {   // F2 · Del: 고른 단면 이름 바꾸기 · 지우기 — 단면 목록 오른쪽 클릭 메뉴에 이미 적혀 있던 키(T9: 적혀만 있고 안 걸려 있던 것을 건다)
            auto* rn = new QShortcut(QKeySequence(Qt::Key_F2), workArea_); rn->setContext(Qt::WidgetWithChildrenShortcut);
            QObject::connect(rn, &QShortcut::activated, this, [this] { if (current_ >= 0) renameSection(current_); });
            auto* dl = new QShortcut(QKeySequence(Qt::Key_Delete), workArea_); dl->setContext(Qt::WidgetWithChildrenShortcut);
            QObject::connect(dl, &QShortcut::activated, this, [this] { if (current_ >= 0) deleteSection(current_); });
        }
        const std::pair<const char*, double> shifts[] = {{"]", 0.1}, {"[", -0.1}, {"}", 1.0}, {"{", -1.0}};
        for (auto& [k, d] : shifts) {
            auto* sc = new QShortcut(QKeySequence(QString::fromLatin1(k)), workArea_);
            sc->setContext(Qt::WidgetWithChildrenShortcut);
            double dd = d;
            QObject::connect(sc, &QShortcut::activated, this, [this, dd] { shiftLine(dd); });
        }
    }

    // ---- 연결 ----
    QObject::connect(undo_, &QUndoStack::canUndoChanged, this, [this](bool b) { action("undo")->setEnabled(b); });
    QObject::connect(undo_, &QUndoStack::canRedoChanged, this, [this](bool b) { action("redo")->setEnabled(b); });
    QObject::connect(undo_, &QUndoStack::undoTextChanged, this, [this](const QString& t) { action("undo")->setToolTip(t.isEmpty() ? QStringLiteral("되돌리기 [Ctrl+Z]") : QStringLiteral("되돌리기: %1 [Ctrl+Z]").arg(t)); });
    QObject::connect(undo_, &QUndoStack::redoTextChanged, this, [this](const QString& t) { action("redo")->setToolTip(t.isEmpty() ? QStringLiteral("다시 [Ctrl+Y]") : QStringLiteral("다시: %1 [Ctrl+Y]").arg(t)); });
    QObject::connect(action("undo"), &QAction::triggered, this, [this] { if (!plan_->drawMode()) undo_->undo(); });
    QObject::connect(action("redo"), &QAction::triggered, this, [this] { if (!plan_->drawMode()) undo_->redo(); });
    QObject::connect(action("open"), &QAction::triggered, this, [this] { chooseOpen(); });
    QObject::connect(action("recent"), &QAction::triggered, this, [this] {
        const QStringList rf = recentFiles();
        for (const QString& f : rf) if (QFileInfo::exists(f) && f != path_) { openFile(f); return; }
        if (!rf.isEmpty() && QFileInfo::exists(rf.first())) openFile(rf.first());
    });
    QObject::connect(action("close"), &QAction::triggered, this, [this] { closeScene(); });
    QObject::connect(action("about"), &QAction::triggered, this, [this] { dlgAbout(); });
    QObject::connect(action("keys"), &QAction::triggered, this, [this] { dlgKeys(); });
    QObject::connect(action("quit"), &QAction::triggered, this, [this] { close(); });
    QObject::connect(action("draw"), &QAction::toggled, this, [this](bool on) { if (plan_->drawMode() != on) plan_->setDrawMode(on); });
    plan_->onDrawModeChanged = [this](bool on) {
        action("draw")->setChecked(on);
        if (on) updateCtx(0, SectionLine{});
        guide_->setVisible(on); undoBtns_->setVisible(on);
        if (on) { guide_->place(); undoBtns_->place(); motion::fadeIn(undoBtns_, motion::kUndoIn); }
        action("esc")->setEnabled(on);
        section_->setDrawingHint(on);   // 단면 화면 위 「A′를 찍으면 단면이 여기에 나옵니다」(화판 2)
        updateEnabled();                // 그리는 동안 도면 · 자료 내보내기 꺼짐(화판 2 · 검토 UI 5)
        if (on) updateHeader();         // 정보 줄 「단면선을 긋는 중 …」 + 잘린 선 · 입면 글 비움(updateHeader 가 그리는 중을 안다 — 검토 I2)
        if (on) { setActiveView(0); showStatus(QString()); }   // v4 단계 8: 안내는 도구 줄 한 곳, 상태줄은 좌표만
        else {
            drawEnded_ = plan_->hasLine();
            QTimer::singleShot(0, this, [this] { drawEnded_ = false; });
            showStatus(plan_->hasLine() ? QStringLiteral("단면을 계산합니다 — 다음: 뒤 깊이(숫자키 1–5)를 고르거나 Ctrl+P 로 도면") : QString());
            updateHeader();             // 오른쪽 판 · 정보 줄을 고른 단면 값으로
        }
    };
    plan_->onDrawProgress = [this](int stage, const SectionLine& l) {
        updateCtx(stage, l);
    };
    QObject::connect(action("esc"), &QAction::triggered, this, [this] { if (plan_->drawMode()) plan_->setDrawMode(false); });
    QObject::connect(action("flip"), &QAction::triggered, this, [this] {
        if (!plan_->hasLine()) return;
        SectionLine l = plan_->line(); std::swap(l.a, l.b);
        plan_->setLine(l, true); requestSection(true);
        syncCurrentSection(); commitState(QStringLiteral("방향 반전")); updateHeader();
    });
    QObject::connect(action("move"), &QAction::triggered, this, [this] { shiftLine(0.1); });
    QObject::connect(action("addsec"), &QAction::triggered, this, [this] { addSection(); });
    QObject::connect(action("clear"), &QAction::triggered, this, [this] {
        plan_->setLine(SectionLine(), false); section_->clear(); last_ = SectionOutput(); updateEnabled();
        commitState(QStringLiteral("단면선 지우기")); updateHeader();
    });
    QObject::connect(action("fit"), &QAction::triggered, this, [this] { plan_->fitAll(); section_->fit(); section_->update(); });
    action("top")->setToolTip(QStringLiteral("3D로 돌린 뒤 평면 제자리 (T). 왼쪽 화면 N 을 눌러도 같습니다."));
    QObject::connect(action("top"), &QAction::triggered, this, [this] { plan_->homeView(); });
    QObject::connect(action("zoomin"), &QAction::triggered, this, [this] { plan_->zoomBy(1 / 1.4); section_->zoomBy(1.4); });
    QObject::connect(action("zoomout"), &QAction::triggered, this, [this] { plan_->zoomBy(1.4); section_->zoomBy(1 / 1.4); });
    const std::pair<const char*, const char*> tg[] = {{"image", "입면 영상"}, {"line", "단면선 표시"}, {"levels", "레벨선"}, {"ticks", "거리 눈금"}};
    for (auto& [k, name] : tg) {
        QString nm = QString::fromUtf8(name);
        QObject::connect(action(k), &QAction::toggled, this, [this, nm](bool) { section_->setStyle(secStyle()); commitState(nm); });
    }
    QObject::connect(action("ticks"), &QAction::toggled, this, [](bool on) { QSettings().setValue("section/ticks", on); });
    QObject::connect(opacity_, &QSlider::valueChanged, this, [this](int) { section_->setStyle(secStyle()); notifySheetDoc(); });
    QObject::connect(action("smooth"), &QAction::toggled, this, [this](bool on) { QSettings().setValue("section/smooth", on); requestSection(true); commitState(QStringLiteral("평활")); });
    QObject::connect(action("fade"), &QAction::toggled, this, [this](bool on) { QSettings().setValue("section/depthFade", on); section_->setStyle(secStyle()); requestSection(true); commitState(QStringLiteral("깊이 음영")); });
    QObject::connect(action("info"), &QAction::triggered, this, [this] { dlgInfo(); });
    QObject::connect(action("view1"), &QAction::toggled, this, [this](bool on) {
        if (!on && !action("view2")->isChecked()) { action("view1")->setChecked(true); return; }
        planFrame_->setVisible(on);
    });
    QObject::connect(action("view2"), &QAction::toggled, this, [this](bool on) {
        if (!on && !action("view1")->isChecked()) { action("view2")->setChecked(true); return; }
        sectionFrame_->setVisible(on);
    });
    QObject::connect(action("listpanel"), &QAction::toggled, this, [this](bool on) {
        sidePanel_->setVisible(on);
        if (sheetSide_) sheetSide_->setVisible(on);   // 조판 탭 목록도 같이
        QSettings().setValue("ui/sectionList", on);
    });
    QObject::connect(viewTabs_, &QTabBar::currentChanged, this, [this](int i) {
        if (i == 0) showHomeTab();
        else if (i == 1) showWorkTab();
        else if (sheetHost_) { body_->setCurrentWidget(sheetHost_); setRibbonContext(2); }
    });
    QObject::connect(viewTabs_, &QTabBar::tabCloseRequested, this, [this](int i) { if (i == 2) closeSheetTab(); });
    QObject::connect(action("full"), &QAction::triggered, this, [this] {
        if (!src_) { showStatus(QStringLiteral("모델을 연 뒤에 조판 탭을 여세요")); return; }
        dlgPlanSheet();
    });
    QObject::connect(action("sheet"), &QAction::triggered, this, [this] { dlgChooseSheet(); });
    QObject::connect(action("plansheet"), &QAction::triggered, this, [this] { dlgPlanSheet(); });
    QObject::connect(action("sectionsheet"), &QAction::triggered, this, [this] { dlgSheet(); });
    QObject::connect(action("dxf"), &QAction::triggered, this, [this] { dlgSectionDxf(); });
    QObject::connect(action("secimg"), &QAction::triggered, this, [this] { dlgSectionImage(); });
    QObject::connect(action("plan"), &QAction::triggered, this, [this] { dlgPlan(); });
    QObject::connect(action("xyz"), &QAction::triggered, this, [this] { dlgPoints(0); });
    QObject::connect(action("las"), &QAction::triggered, this, [this] { dlgPoints(1); });
    QObject::connect(action("csv"), &QAction::triggered, this, [this] { dlgProfileCsv(); });
    QObject::connect(action("secjson"), &QAction::triggered, this, [this] {
        if (!src_) return;
        QFileInfo mi(path_);
        QString dir = QFileInfo(mi.absolutePath()).isWritable() ? mi.absolutePath() : QDir::homePath();
        QString fn = QFileDialog::getSaveFileName(this, QStringLiteral("단면 목록 내보내기"), dir + "/" + mi.completeBaseName() + ".sections.json", QStringLiteral("단면 목록 (*.sections.json)"));
        if (fn.isEmpty()) return;
        QString m; bool ok = exportSectionsJson(fn, &m); report(ok, m);
    });
    QObject::connect(action("secimport"), &QAction::triggered, this, [this] { importSectionsJson(); });
    QObject::connect(action("projsave"), &QAction::triggered, this, [this] { saveProjectNow(); });
    QObject::connect(action("projsaveas"), &QAction::triggered, this, [this] { saveProjectAs(); });
    QObject::connect(action("projopen"), &QAction::triggered, this, [this] { openProjectDialog(); });
    QObject::connect(action("contrast"), &QAction::toggled, this, [this](bool on) { applyHighContrast(on); });
    auto bandChanged = [this] {
        plan_->setBand(front_->value(), back_->value());
        if (!applyingState_) { backUserSet_ = true; QSettings().setValue("section/backUserSet", true); }   // 사용자가 바꿈
        QSettings().setValue("section/front", front_->value()); QSettings().setValue("section/back", back_->value());
        if (plan_->hasLine()) { SectionLine l = plan_->line(); l.front = front_->value(); l.back = back_->value(); plan_->setLine(l, true); requestSection(true); }
        updateDepthChips();
        syncCurrentSection();
        commitState(QStringLiteral("두께·입면 깊이"), 1);
        updateHeader();
    };
    QObject::connect(front_, &QDoubleSpinBox::valueChanged, this, bandChanged);
    QObject::connect(back_, &QDoubleSpinBox::valueChanged, this, bandChanged);

    plan_->onLineChanged = [this](const SectionLine&, bool final) {
        if (final && drawEnded_) { drawEnded_ = false; fitNextResult_ = true; setActiveView(1); }
        requestSection(final); updateEnabled();
        if (final && !applyingState_) { syncCurrentSection(); commitState(QStringLiteral("단면선")); updateHeader(); }
    };
    plan_->onOpenRequest = [this] { chooseOpen(); };
    section_->onViewChanged = [this] { updateHeader(); };
    auto fmt = [](double v, int dec) { return QString::number(v, 'f', dec); };
    plan_->onCursor = [this, fmt](double X, double Y, double Z, bool hasZ, bool valid) {
        if (!valid) { cx_->clear(); cy_->clear(); cz_->clear(); pickFloorGen_ = pickWorker_->cancelAll(); showZSource(ZSource::None, QString()); return; }
        cx_->setText(fmt(X, 3)); cy_->setText(fmt(Y, 3)); cz_->setText(hasZ ? fmt(Z, 3) : QStringLiteral("—"));
        showZSource(hasZ ? ZSource::Coarse : ZSource::None, QStringLiteral("정밀 피킹 계산 중…"));
        requestPick(plan_->lastMousePos());
    };
    section_->onCursor = [this, fmt](double s, double zAbs, double X, double Y, bool valid) {
        plan_->setSyncMark(valid, s);
        if (!valid) { cx_->clear(); cy_->clear(); cz_->clear(); return; }
        cx_->setText(fmt(X, 3)); cy_->setText(fmt(Y, 3)); cz_->setText(fmt(zAbs, 3));
        showZSource(ZSource::SectionCursor, QStringLiteral("단면 화면의 커서 위치(s = %1 m, z)입니다. 평면 고리는 같은 거리의 단면선 위입니다.").arg(fmt(s, 3)));
    };
    secWorker_ = std::make_unique<CoalescingWorker>();
    pickWorker_ = std::make_unique<CoalescingWorker>();
    showStart(true);
    updateEnabled();
    updateHeader();
}

int MainWindow::addSectionAt(const SectionLine& l) {
    if (!src_) return -1;
    syncCurrentSection();
    const Vec3 o = srs().origin;
    std::vector<std::string> used; for (auto& s : sections_) used.push_back(s.name);
    int k = 0; while (std::find(used.begin(), used.end(), sectionLetterName(k)) != used.end()) ++k;
    SavedSection s; s.name = sectionLetterName(k);
    s.ax = l.a.x + o.x; s.ay = l.a.y + o.y; s.bx = l.b.x + o.x; s.by = l.b.y + o.y; s.front = front_->value(); s.back = back_->value();
    sections_.push_back(s);
    refreshSectionList();
    return int(sections_.size()) - 1;
}

// ---------------------------------------------------------------- v4 단계 1: --ui-audit (같은 일 단추 겹침 · 자리 규칙 잠금 시험)
// 보이는 단추를 훑어 FINAL_PLAN §5 단계 1의 기대값과 견준다. 키 지름길 · 오른쪽 클릭 메뉴는 셈에서 뺀다.
// 아직 만들지 않은 조판 쪽 항목(lists-match · sheet …)은 「not-checked」로 적고 결과에는 넣지 않는다(단계 10 · 11에서 더함).
bool MainWindow::uiAudit(const QString& dir, QString* out) {
    QStringList L;
    bool ok = true;
    auto pump = [](int ms) { QElapsedTimer t; t.start(); while (t.elapsed() < ms) QApplication::processEvents(QEventLoop::AllEvents, 10); };
    // 단추 이름: 글자(없으면 툴팁)에서 키 표시 · ▾ · & 를 떼어 낸 앞부분
    auto label = [](QAbstractButton* b) {
        QString t = b->text().isEmpty() ? b->toolTip() : b->text();
        t.remove(QChar(0x25BE)); t.remove(QLatin1Char('&'));
        for (const QString& cut : {QStringLiteral("  "), QStringLiteral(" ("), QStringLiteral("\t")}) { int i = t.indexOf(cut); if (i > 0) t = t.left(i); }
        return t.trimmed();
    };
    // 평면 머리와 단면 머리는 서로 다른 화면을 다루므로 같은 이름이어도 겹침이 아니다
    auto scope = [this](QWidget* w) {
        for (QWidget* p = w; p; p = p->parentWidget()) { if (p == planTitle_) return 1; if (p == secTitleBar_) return 2; }
        return 0;
    };
    auto skip = [](QAbstractButton* b) {
        const QString n = b->objectName();
        return n == QLatin1String("chip") || n == QLatin1String("stepBtn") || (b->text().trimmed().isEmpty() && b->toolTip().isEmpty());
    };

    // 0) 해상도 · DPI(B5, 사용자 지시 「4K · 와이드 어디서나 안정」): ① --size 로 연 창은 그 크기(가로 · 세로 모두) ② 창 최소 폭 ≤ 1280 ③ 보이는 단추 · 칸 · 판이 창 밖으로 안 나감
    //    ④ QT_SCALE_FACTOR 가 실제로 먹음(dpr == scale — ×2 실행이 가짜가 아니어야) ⑤ 아이콘은 dpr 1.25 · 1.75(Windows 125 · 175 %)에서도 그 배율로 구운 장을 냄(다른 배율 장을 줄이면 흐림)
    {
        const QString scale = qEnvironmentVariable("QT_SCALE_FACTOR", QStringLiteral("1"));
        int overflow = 0; QStringList over;
        auto check = [&](QWidget* w, const QString& name) {
            if (!w || !w->isVisibleTo(this)) return;
            const QRect r(w->mapTo(this, QPoint(0, 0)), w->size());
            if (r.right() >= width() || r.bottom() >= height() || r.left() < 0 || r.top() < 0) { ++overflow; if (over.size() < 6) over << name; }   // right() = x + w − 1 이라 >= 라야 1 px 넘침도 잡힌다(B5 검토 N1)
        };
        for (auto* b : ribbon_->chips()) check(b, b->text());
        check(srsLabel_, QStringLiteral("srs-text"));
        check(docTabCorner_, QStringLiteral("doc-corner"));
        const int minW = minimumSizeHint().width();
        const QSize want = property("auditSize").toSize();   // main.cpp --size W×H (없으면 ① 건너뜀)
        // 실제 화면(AC10)에서는 창이 화면 사용 가능 영역에 잘리고(이 PC 150 % · 2293×940 논리: 1040 요청 → 940 실측) dpr = OS 배율 × QT_SCALE_FACTOR 라
        // ①은 화면에 들어가는 변만, ④는 offscreen(OS 배율 1)에서만 판정한다(판정, build.md)
        const bool offscreen = QGuiApplication::platformName() == QLatin1String("offscreen");
        const QSize avail = screen() ? screen()->availableGeometry().size() : QSize();
        auto judgeDim = [&](int wantV, int availV) { return offscreen || !avail.isValid() || wantV <= availV; };
        const bool sizeOk = !want.isValid() || ((!judgeDim(want.width(), avail.width()) || width() == want.width()) && (!judgeDim(want.height(), avail.height()) || height() == want.height()));
        const bool scaleOk = !offscreen || qFuzzyCompare(devicePixelRatioF(), scale.toDouble() > 0 ? scale.toDouble() : 1.0);
        // 선 아이콘 · 리본 칩(보이는 것 전부): 그 배율로 바로 구운 기준 장과 크기 · dpr · 픽셀이 같아야 함(칩은 chipReference — dpr 값만 보던 옛 판정은 Qt 가 다른 장을 늘려 돌려줘도 통과했다, B5 검토 ⓓ)
        int iconDprBad = 0, chipDprBad = 0;
        for (qreal d : {1.25, 1.75}) {
            const QPixmap a = kerf::icon(QStringLiteral("search"), 18, theme::Hand).pixmap(QSize(18, 18), d);
            const QPixmap b = kerf::glyph(QStringLiteral("search"), 18, theme::Hand, d);
            if (a.size() != b.size() || !qFuzzyCompare(a.devicePixelRatio(), d) || a.toImage() != b.toImage()) ++iconDprBad;
            const int tile = ribbon_->look().tile;
            for (auto* c : ribbon_->chips())
                if (c->isVisibleTo(this)) {
                    const QPixmap ca = c->icon().pixmap(QSize(tile, tile), d, QIcon::Normal, QIcon::Off), cb = ribbon_->chipReference(c, d);
                    if (ca.size() != cb.size() || !qFuzzyCompare(ca.devicePixelRatio(), d) || ca.toImage() != cb.toImage()) ++chipDprBad;
                }
        }
        const bool envOk = sizeOk && minW <= 1280 && overflow == 0 && scaleOk && iconDprBad == 0 && chipDprBad == 0;
        L << QStringLiteral("ui-audit env size=%1x%2 want=%3x%4 scale=%5 dpr=%6 min-width=%7 overflow=%8%9 icon-dpr-bad=%10 chip-dpr-bad=%11%12%13").arg(width()).arg(height()).arg(want.width()).arg(want.height()).arg(scale).arg(devicePixelRatioF(), 0, 'f', 2).arg(minW).arg(overflow)
                 .arg(over.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(over.join(QLatin1Char(','))))
                 .arg(iconDprBad).arg(chipDprBad)
                 .arg(offscreen ? QString() : QStringLiteral(" screen-avail=%1x%2").arg(avail.width()).arg(avail.height()))   // 실제 화면일 때만: ① · ④ 가 어디까지 판정됐는지 읽을 수 있게
                 .arg(envOk ? QString() : QStringLiteral("  FAIL"));
        ok &= envOk;
    }

    // 1) 리본 탭 없음 · 문서 탭 「홈 · 단면[· 도면]」 상시(v5 C2)
    QStringList names;
    for (int i = 0; i < viewTabs_->count(); ++i) names << viewTabs_->tabText(i);
    const bool tabsOk = docTabsRow_ && docTabsRow_->isVisibleTo(this) && names.size() >= 2 && names[0] == QStringLiteral("홈") && names[1] == QStringLiteral("단면");
    L << QStringLiteral("ui-audit tabs=0 doctabs=%1%2").arg(names.join(QLatin1Char(','))).arg(tabsOk ? QString() : QStringLiteral("  FAIL"));
    ok &= tabsOk;

    // 2) 리본(v5 C1 · T6): 높이 118 · 묶음 6(모델 · 단면 · 입면 · 보기 · 자료 · 내보내기 — 기타 · 더보기 없음) · 1920 에서 타일 50 글자 보임 · 흙색 칩 하나(「도면」)
    const RibbonLook lk = ribbon_->look();
    int groups = int(ribbon_->findChildren<QFrame*>(QStringLiteral("ribbonGroup")).size());
    int groupsNamed = 0;
    for (const char* g : {"model", "sec", "elev", "view", "data", "out"}) if (ribbon_->group(QString::fromLatin1(g))) ++groupsNamed;
    int moreCount = int(act_.count(QStringLiteral("more")));
    for (auto* b : ribbon_->chips()) if (b->text() == QStringLiteral("더보기") || b->text() == QStringLiteral("기타")) ++moreCount;
    int clay = 0;
    for (auto* b : ribbon_->chips()) if (b->property("primary").toBool() && b->isVisibleTo(this)) ++clay;
    // 판정은 규칙으로(검토 I6): 창 폭에 들어가는 가장 큰 크기를 골랐는가 · 높이가 그 크기의 sizeHint 인가. 1920 이상이면 타일 50 · 글자 · 118(스펙 §12)
    const QList<int> lw = ribbon_->lookWidths();
    const bool ribRule = ribbon_->lookIndex() == Ribbon::chooseLook(lw, ribbon_->width()) && ribbon_->height() == ribbon_->sizeHint().height();
    const bool ribOk = ribRule && groups == 6 && groupsNamed == 6 && moreCount == 0 && clay == 1 && (ribbon_->width() < 1920 || (lk.tile == 50 && lk.labels && ribbon_->height() == 118));
    L << QStringLiteral("ui-audit ribbon height=%1 groups=%2 more=%11 look=%3 labels=%4 chips-clay=%5 width=%6 need50=%7 need32=%8 need20=%9%10")
             .arg(ribbon_->height()).arg(groups).arg(lk.tile).arg(lk.labels ? 1 : 0).arg(clay).arg(ribbon_->width()).arg(lw.value(0)).arg(lw.value(9)).arg(lw.value(lw.size() - 1))
             .arg(ribOk ? QString() : QStringLiteral("  FAIL")).arg(moreCount);
    L << QStringLiteral("ui-audit ribbon-widths %1").arg(ribbon_->widthReport());
    {   // 2c) 단면 문맥: 묶음 이름 흐림 0 · 입면 칸(칩 아닌 위젯)은 타일 세로 가운데(검토 UI 2 · 14)
        int dim = 0;
        for (const char* g : {"model", "sec", "elev", "view", "data", "out"})
            if (QFrame* f = ribbon_->group(QString::fromLatin1(g))) if (auto* cap = f->findChild<QLabel*>(QStringLiteral("ribbonGroupCaption"))) if (cap->property("dim").toBool()) ++dim;
        int elevDy = -1;
        if (depthBox_ && !ribbon_->chips().isEmpty()) {
            QToolButton* chip = ribbon_->chips().first();
            const int tileCy = chip->mapTo(ribbon_, QPoint(0, 6 + lk.tile / 2)).y();
            const int boxCy = depthBox_->mapTo(ribbon_, depthBox_->rect().center()).y();
            elevDy = std::abs(tileCy - boxCy);
        }
        const bool okc = dim == 0 && elevDy >= 0 && elevDy <= 3;
        L << QStringLiteral("ui-audit ribbon-context dim-captions=%1 elev-center-dy=%2%3").arg(dim).arg(elevDy).arg(okc ? QString() : QStringLiteral("  FAIL"));
        ok &= okc;
    }
    {   // 2d) 입면 아래 줄(사용자 지시 2026-10-10 「더보기 안의 영상 불투명도 · 단면선 굵기를 입면 밑으로」 · 평활도 같은 줄, T6): 리본에 글자가 보이는 크기면 입면 묶음 아래 줄에
        //     불투명도 · 굵기 · 평활이 보이고 「입면 ▾」 칩은 숨음, 글자 숨김 크기(리본이 낮아 두 줄이 안 들어감)면 아래 줄은 숨고 「입면 ▾」 칩 안에 셋. 아래 줄은 위 줄보다 넓지 않음(1920 타일 50 을 지킴).
        //     두 자리의 값은 하나: 아래 줄에서 바꾸면 그리기 설정(secStyle)과 칩 안쪽이 같이 바뀐다
        auto* row2 = ribbon_->findChild<QWidget*>(QStringLiteral("elevRow2"));
        auto* opRow = ribbon_->findChild<QSlider*>(QStringLiteral("opacityRow"));
        QSlider* opMenu = nullptr; QWidget* lwMenu = nullptr; int more = 0; bool smoothInMenu = false;
        const auto emIt = act_.find(QStringLiteral("elevmenu"));
        QToolButton* elevMenuChip = nullptr;
        for (auto* c : ribbon_->chips()) if (emIt != act_.end() && c->text() == QStringLiteral("입면")) elevMenuChip = c;
        if (QMenu* mm = emIt != act_.end() ? emIt->second->menu() : nullptr)
            for (QAction* a : mm->actions())
            {
                if (a->inherits("QWidgetAction")) {
                    if (QWidget* dw = static_cast<QWidgetAction*>(a)->defaultWidget()) {
                        if (auto* s = dw->findChild<QSlider*>(QStringLiteral("opacityMenu"))) { opMenu = s; if (a->isVisible()) ++more; }
                        if (auto* w = dw->findChild<QWidget*>(QStringLiteral("lineWidthMenu"))) { lwMenu = w; if (a->isVisible()) ++more; }
                    }
                } else if (a == action("smooth")) smoothInMenu = true;
            }
        const bool wide = lk.labels;
        const bool shown = row2 && row2->isVisibleTo(ribbon_);
        QToolButton* smoothRow = nullptr;
        if (row2) for (auto* b : row2->findChildren<QToolButton*>()) if (b->property("smoothChip").toBool()) smoothRow = b;
        const bool chipShown = elevMenuChip && elevMenuChip->isVisibleTo(ribbon_);
        const bool smoothOk = wide ? (smoothRow != nullptr && !chipShown) : (smoothInMenu && chipShown);   // 평활은 어느 크기에서나 한 번에 닿는다
        const bool placeOk = smoothOk && (wide ? (shown && more == 0) : (!shown && more == 2));
        // 폭: 입면 칸 전체(elevBlock)의 sizeHint 가 위 줄과 같아야(리본 폭 계산이 그대로) · 아래 줄은 슬라이더 최소 폭까지 넣어도 위 줄 안
        auto* blk = ribbon_->findChild<QWidget*>(QStringLiteral("elevBlock"));
        const int depthW = depthBox_ ? depthBox_->sizeHint().width() : 0, blkW = blk ? blk->sizeHint().width() : -1, rowMin = row2 ? row2->minimumSizeHint().width() : 0;
        const bool widthOk = blkW == depthW && rowMin <= depthW;   // 아래 줄 · 위 줄 폭이 같아야 아래 줄을 숨기고 보여도 리본 폭 계산(need50 · need32)이 그대로 — 크기가 오락가락하지 않음
        bool inside = true;
        if (shown) {   // 리본 안 · 짓눌림 없음(아래 줄 · 라벨 · 슬라이더 최소 폭)
            const QRect r(row2->mapTo(ribbon_, QPoint(0, 0)), row2->size());
            inside = ribbon_->rect().contains(r) && row2->width() >= rowMin && (!opRow || opRow->width() >= opRow->minimumWidth());
            for (auto* l : row2->findChildren<QLabel*>()) if (l->isVisibleTo(row2) && l->width() < l->sizeHint().width()) inside = false;
        }
        bool sync = false;
        if (opRow && opMenu && lwMenu && row2) {
            const int op0 = opRow->value();
            opRow->setValue(50);    // 줄 → 메뉴 · 그리기 설정
            bool opOk = std::fabs(secStyle().imageOpacity - 0.5) < 1e-9 && opMenu->value() == 50;
            opMenu->setValue(70);   // 메뉴 → 줄 · 그리기 설정
            opOk = opOk && std::fabs(secStyle().imageOpacity - 0.7) < 1e-9 && opRow->value() == 70;
            opRow->setValue(op0);
            const double lw0 = QSettings().value(QStringLiteral("view/sectionLineWidth"), 2.0).toDouble();
            auto chipOf = [](QWidget* host, double w) -> QToolButton* { for (auto* b : host->findChildren<QToolButton*>()) if (std::fabs(b->property("lineWidth").toDouble() - w) < 0.01) return b; return nullptr; };
            QToolButton *r3 = chipOf(row2, 3.0), *m3 = chipOf(lwMenu, 3.0), *r15 = chipOf(row2, 1.5), *m15 = chipOf(lwMenu, 1.5);
            const double hc = highContrast_ ? 1.5 : 1.0;
            bool lwOk = false;
            if (r3 && m3 && r15 && m15) {
                r3->click();    // 줄 → 메뉴 · 그리기 설정
                lwOk = std::fabs(secStyle().lineWidthPx - 3.0 * hc) < 1e-9 && m3->isChecked() && !m15->isChecked();
                if (!wide) {    // 메뉴 → 줄 · 그리기 설정: 메뉴 칸이 보이는 크기에서만(넓은 창에서는 숨긴 QWidgetAction 칸의 click() 이 먹지 않음 — 실측 2026-10-10)
                    m15->click();
                    lwOk = lwOk && std::fabs(secStyle().lineWidthPx - 1.5 * hc) < 1e-9 && r15->isChecked() && !r3->isChecked();
                }
                if (QToolButton* back = chipOf(row2, lw0)) back->click();   // 되돌림
            }
            sync = opOk && lwOk;
        }
        const bool okr = row2 && placeOk && widthOk && inside && sync;
        L << QStringLiteral("ui-audit elev-row2 shown=%1 more=%2 wide=%3 width=%4/%5 row-min=%6 inside=%7 sync=%8 smooth=%10 menu-chip=%11%9").arg(int(shown)).arg(more).arg(int(wide)).arg(blkW).arg(depthW)
                 .arg(rowMin).arg(int(inside)).arg(int(sync)).arg(okr ? QString() : QStringLiteral("  FAIL")).arg(int(smoothOk)).arg(int(chipShown));
        ok &= okr;
    }
    ok &= ribOk;
    if (!dir.isEmpty()) grab().save(QDir(dir).filePath(QStringLiteral("ribbon.png")));

    // 3) 겹침 · 아이콘 · 툴팁 — 지금(단면 문맥) 보이는 단추 전부
    QStringList dups, noIcon, noTip;
    QSet<QString> seenDup, seenIcon, seenTip;
    {
        struct E { QAbstractButton* b; int sc; };
        std::map<QString, std::vector<E>> byName;
        for (auto* b : findChildren<QAbstractButton*>()) {
            if (!b->isVisibleTo(this) || skip(b)) continue;
            const QString n = label(b);
            if (n.isEmpty()) continue;
            byName[n].push_back({b, scope(b)});
            const bool iconOnly = b->text().trimmed().isEmpty();
            if (b->icon().isNull() && !iconOnly && !seenIcon.contains(n)) { seenIcon.insert(n); noIcon << n; }
            if (iconOnly && b->toolTip().isEmpty() && b->accessibleName().isEmpty() && !seenTip.contains(n)) { seenTip.insert(n); noTip << n; }
        }
        for (const auto& [n, es] : byName) {
            bool dup = false;
            for (size_t i = 0; i < es.size() && !dup; ++i)
                for (size_t j = i + 1; j < es.size() && !dup; ++j)
                    if (!(es[i].sc && es[j].sc && es[i].sc != es[j].sc)) dup = true;
            if (dup && !seenDup.contains(n)) { seenDup.insert(n); dups << n; }
        }
    }
    L << QStringLiteral("ui-audit dup-actions=%1 dup-labels=%1").arg(dups.size());
    for (const QString& d : dups) L << QStringLiteral("dup: %1").arg(d);
    L << QStringLiteral("ui-audit same-name-different-action=not-checked");
    ok &= dups.isEmpty();
    L << QStringLiteral("ui-audit ctxbar=0");

    // 3b) 같은 아이콘 · 다른 일(검토 UI 6): 보이는 단추의 아이콘 이름이 같은데 글(「…」을 뗀)이 다르면 겹침. 평면 머리 ↔ 단면 머리는 예외
    QStringList dupIcons;
    {
        struct E { QString label; int sc; };
        std::map<QString, std::vector<E>> byIcon;
        for (auto* b : findChildren<QAbstractButton*>()) {
            if (!b->isVisibleTo(this) || skip(b)) continue;
            QString ic = ribbon_->chipIconName(b);
            if (ic.isEmpty()) ic = b->property("iconName").toString();
            if (ic.isEmpty() && b->inherits("QToolButton")) if (QAction* da = static_cast<QToolButton*>(b)->defaultAction()) ic = da->property("iconName").toString();
            if (ic.isEmpty()) continue;
            QString n = label(b); n.remove(QChar(0x2026)); n = n.trimmed();
            if (n.isEmpty()) continue;
            byIcon[ic].push_back({n, scope(b)});
        }
        for (const auto& [ic, es] : byIcon) {
            bool dup = false; QStringList ls;
            for (const E& e : es) if (!ls.contains(e.label)) ls << e.label;
            for (size_t i = 0; i < es.size() && !dup; ++i)
                for (size_t j = i + 1; j < es.size() && !dup; ++j)
                    if (es[i].label != es[j].label && !(es[i].sc && es[j].sc && es[i].sc != es[j].sc)) dup = true;
            if (dup) dupIcons << QStringLiteral("%1(%2)").arg(ic, ls.join(QLatin1Char('/')));
        }
    }
    L << QStringLiteral("ui-audit dup-icons=%1%2").arg(dupIcons.size()).arg(dupIcons.isEmpty() ? QString() : QStringLiteral("  FAIL"));
    for (const QString& d : dupIcons) L << QStringLiteral("dup-icon: %1").arg(d);
    ok &= dupIcons.isEmpty();

    // 3c) 리본 문맥(검토 I1): 홈에서는 단면 · 자료 · 도면 동작이 꺼지고 「닫기」는 살아 있다 — updateEnabled() 뒤에도 그대로여야 한다
    int ctxBad = 0;
    {
        const int keepCtx = ribbonCtx_;
        setRibbonContext(0); updateEnabled();
        for (const char* k : {"draw", "addsec", "flip", "move", "sheet", "csv", "plan", "points", "xyz", "las"}) if (action(k)->isEnabled()) ++ctxBad;
        if (src_ && !action("close")->isEnabled()) ++ctxBad;
        setRibbonContext(keepCtx); updateEnabled();
    }
    L << QStringLiteral("ui-audit context-home enabled-wrong=%1%2").arg(ctxBad).arg(ctxBad ? QStringLiteral("  FAIL") : QString());
    ok &= ctxBad == 0;

    // 4) 축척 칸은 화면에 하나(T8: 상태줄 배지는 없음 — 수평 좌표계는 작은 글)
    int scales = 0, badges = 0;
    for (auto* c : findChildren<QComboBox*>()) if (c->isVisibleTo(this) && c->findText(QStringLiteral("맞춤")) >= 0) ++scales;
    for (auto* b : findChildren<QToolButton*>(QStringLiteral("statusBadge"))) if (b->isVisibleTo(this)) ++badges;
    L << QStringLiteral("ui-audit scale-controls=%1 status-badges=%2%3").arg(scales).arg(badges).arg(scales == 1 && badges == 0 ? QString() : QStringLiteral("  FAIL"));
    ok &= scales == 1 && badges == 0;
    // 4b) 수평 좌표계 글(T8): 테두리 없는 작은 글 — 「수평 EPSG:5187 · 동부원점(GRS80)」 한 줄, ▾ 없음, 누르는 것 아님(좌표계는 모델마다 자동 판별 — 사용자 결정 2026-10-10)
    const QString wantSrs = (src_ && srsReport_.desc.horizontalEpsg) ? qs8(srsReport_.desc.horizontalLabelKo()) : QString();
    const bool srsTextOk = srsLabel_ && !srsLabel_->text().endsWith(QChar(0x25BE)) && (wantSrs.isEmpty() || srsLabel_->text() == wantSrs);
    L << QStringLiteral("ui-audit status-srs height=%1 caret=%2 text=%3%4").arg(srsLabel_ ? srsLabel_->height() : -1).arg(srsLabel_ && srsLabel_->text().endsWith(QChar(0x25BE)) ? 1 : 0)
             .arg(srsLabel_ ? srsLabel_->text() : QString()).arg(srsTextOk ? QString() : QStringLiteral("  FAIL"));
    ok &= srsTextOk;

    // 4c) 지운 것(T5 · G2 · 사용자 결정 2026-10-10): 리본 오른쪽 끝 단추 칸 · 좌표 입력(리본 · 판 단추 · 그리는 중 Enter) · 단면 찾기(칸 · Ctrl+F) · 영어 병기 — 화면에도 동작 표에도 하나도 없어야 한다
    int removedCorner = int(findChildren<QToolButton*>(QStringLiteral("ribbonCorner")).size());
    int removedCoord = int(act_.count(QStringLiteral("coord")));
    for (auto* b : findChildren<QAbstractButton*>()) if (b->text().startsWith(QStringLiteral("좌표 입력"))) ++removedCoord;
    int removedFind = int(findChildren<QLineEdit*>(QStringLiteral("ribbonSearch")).size());
    for (auto* sc : findChildren<QShortcut*>()) if (sc->key() == QKeySequence(QKeySequence::Find)) ++removedFind;
    int removedLang = int(act_.count(QStringLiteral("lang")));
    const bool removedOk = removedCorner == 0 && removedCoord == 0 && removedFind == 0 && removedLang == 0;
    L << QStringLiteral("ui-audit removed ribbon-corner=%1 coord-entry=%2 find-box=%3 bilingual=%4%5").arg(removedCorner).arg(removedCoord).arg(removedFind).arg(removedLang).arg(removedOk ? QString() : QStringLiteral("  FAIL"));
    ok &= removedOk;

    // 4d) 리본 정리(T6 · G3): 점군은 칩 하나(XYZ · LAS 는 그 메뉴 안) · 단축키는 「보기」 묶음 · 끝내기는 Ctrl+Q · 프로그램 정보는 F1 창 맨 아래 한 줄(더보기 삭제)
    int pointsChips = 0, xyzLasChips = 0, keysInView = 0;
    for (auto* b : ribbon_->chips()) {
        if (b->text() == QStringLiteral("점군")) ++pointsChips;
        if (b->text() == QStringLiteral("XYZ") || b->text() == QStringLiteral("LAS")) ++xyzLasChips;
        if (b->text() == QStringLiteral("단축키") && ribbon_->group(QStringLiteral("view")) && b->parentWidget() == ribbon_->group(QStringLiteral("view"))) ++keysInView;
    }
    const bool quitKey = act_.count(QStringLiteral("quit")) && action("quit")->shortcut() == QKeySequence(QStringLiteral("Ctrl+Q"));
    int aboutLink = 0;
    { std::unique_ptr<QDialog> kd(keysDialog()); for (auto* b : kd->findChildren<QAbstractButton*>(QStringLiteral("aboutLink"))) ++aboutLink; }
    const bool tidyOk = pointsChips == 1 && xyzLasChips == 0 && keysInView == 1 && quitKey && aboutLink == 1;
    L << QStringLiteral("ui-audit ribbon-tidy points-chips=%1 xyz-las-chips=%2 keys-in-view=%3 quit-ctrl-q=%4 about-in-keys=%5%6").arg(pointsChips).arg(xyzLasChips).arg(keysInView).arg(quitKey ? 1 : 0).arg(aboutLink).arg(tidyOk ? QString() : QStringLiteral("  FAIL"));
    ok &= tidyOk;

    // 4j) 움직임(T12 · G9): 시간은 motion.hpp 한 곳의 표 · 곡선 OutCubic · UI 전환 300 ms 이하 · 탭 전환 · 안내 칩 · 최종 교체 크로스페이드는 켜졌을 때만 · 꺼지면(자동화 · 설정 · OS) 즉시
    {
        const auto& tb = motion::table();
        int maxUi = 0, nUi = 0; for (const auto& e : tb) if (e.ui) { maxUi = std::max(maxUi, e.ms); ++nUi; }
        const bool tableOk = !tb.isEmpty() && maxUi <= 300 && maxUi > 0 && MainWindow::kOpeningSplashMs == motion::kSplash && motion::easing().type() == QEasingCurve::OutCubic && motion::kSplash == 5000;
        QSettings st; const QVariant keepAnim = st.value(QStringLiteral("ui/animations"));
        motion::setForcedOff(false); st.setValue(QStringLiteral("ui/animations"), true);
        const bool onNow = motion::animationsOn();
        bool tabFade = false, chipFade = false, xFade = false, offClean = false;
        qint64 xFadeEndMs = -1;
        if (body_ && body_->count() >= 2) {
            const int cur = body_->currentIndex();
            body_->setCurrentIndex(cur == 0 ? 1 : 0);
            tabFade = body_->fading();
            pump(motion::kTab + 150);
            tabFade = tabFade && !body_->fading();
            body_->setCurrentIndex(cur); pump(motion::kTab + 150);
        }
        if (guide_) {
            const bool wasVis = guide_->isVisible();
            guide_->hide(); guide_->setVisible(true);
            chipFade = motion::fadingNow(guide_);
            pump(motion::kChipIn + 150);
            chipFade = chipFade && !motion::fadingNow(guide_);
            if (!wasVis) { guide_->setVisible(false); pump(motion::kChipIn + 150); }
        }
        if (src_ && section_->hasResult()) {
            const SectionDoc d0 = section_->doc();
            section_->setResult(d0.r, d0.img, d0.imgS0, d0.imgZ1, d0.imgRes, true, true);
            xFade = section_->crossfadeActive();
            // 끝나는지를 본다(최대 2 s). 끝나는 시각은 큰 화면에서 그리기 시간에 좌우된다 — 4K ×2 에서 150 ms 페이드가 이 PC 288 ms(2코어 429 ms)에
            // 끝나고, CI 에서는 「kCrossfade + 150 ms」를 넘겨 이전 커밋(d6dce8c)을 다시 돌려도 같은 판정에서 떨어졌다(2026-10-11).
            // 전환 길이 규칙(≤ 300 ms)은 위 tableOk 가 motion 표에서 본다
            QElapsedTimer xt; xt.start();
            while (section_->crossfadeActive() && xt.elapsed() < 2000) QApplication::processEvents(QEventLoop::AllEvents, 10);
            xFadeEndMs = xt.elapsed();
            xFade = xFade && !section_->crossfadeActive();
        } else xFade = true;   // 결과가 없으면 판정할 것이 없다
        motion::setForcedOff(true);   // 꺼짐 = 즉시: 같은 전환이 겹침 없이 끝나야 한다
        if (body_ && body_->count() >= 2) { const int cur = body_->currentIndex(); body_->setCurrentIndex(cur == 0 ? 1 : 0); offClean = !body_->fading() && motion::ms(motion::kTab) == 0; body_->setCurrentIndex(cur); }
        if (keepAnim.isValid()) st.setValue(QStringLiteral("ui/animations"), keepAnim); else st.remove(QStringLiteral("ui/animations"));
        const bool motionOk = tableOk && onNow && tabFade && chipFade && xFade && offClean;
        L << QStringLiteral("ui-audit motion durations-from-one-place=%1 max-ms=%2 ui-entries=%3 easing=OutCubic splash=%4 tab-fade=%5 chip-fade=%6 crossfade=%7 crossfade-end-ms=%8 off-immediate=%9%10")
                 .arg(tableOk ? 1 : 0).arg(maxUi).arg(nUi).arg(motion::kSplash).arg(tabFade ? 1 : 0).arg(chipFade ? 1 : 0).arg(xFade ? 1 : 0).arg(xFadeEndMs).arg(offClean ? 1 : 0).arg(motionOk ? QString() : QStringLiteral("  FAIL"));
        ok &= motionOk;
    }

    // 4h) 거리 눈금(T10 · 사용자 요청 「입면 토층도 그려야 할 때가 있다 — 체크 표시를 볼 수 있게」): 입면 위에 세로 눈금(1 m · 5 m 굵게)을 그리고, D 키 · 단면 머리 아이콘 · 「그릴 것 ▾」 메뉴로 켜고 끈다(기본 켬)
    {
        const auto ti = act_.find(QStringLiteral("ticks"));
        const bool act = ti != act_.end();
        const bool key = act && ti->second->shortcut() == QKeySequence(QStringLiteral("D"));
        const bool on = act && ti->second->isChecked() && secStyle().showTicks;
        bool hdr = false, menu = false;
        for (auto* b : secTitleBar_->findChildren<QToolButton*>(QStringLiteral("viewToggle"))) if (act && b->accessibleName() == ti->second->text()) hdr = true;
        if (auto* mb = secTitleBar_->findChild<QToolButton*>(QStringLiteral("secTogglesMenu"))) if (mb->menu() && act) menu = mb->menu()->actions().contains(ti->second);
        const bool ticksOk = act && key && on && (hdr || menu);
        L << QStringLiteral("ui-audit dist-ticks action=%1 key-d=%2 on=%3 header=%4 menu=%5%6").arg(act ? 1 : 0).arg(key ? 1 : 0).arg(on ? 1 : 0).arg(hdr ? 1 : 0).arg(menu ? 1 : 0).arg(ticksOk ? QString() : QStringLiteral("  FAIL"));
        ok &= ticksOk;
    }

    // 4g) F1 창(T9): 표의 키 = 실제로 연결된 키(없는 키를 적거나 있는 키를 빼면 실패) · 두 열 · 글이 읽힘(대비 4.5:1) · 앱은 늘 밝은 색 체계
    {
        auto norm = [](const QKeySequence& k) { return k.toString(QKeySequence::PortableText); };
        QSet<QString> actual, table;
        for (const auto& kv : act_) {
            if (kv.first == QLatin1String("hatch") || kv.first == QLatin1String("outline")) continue;   // 아직 안 켜진 도구(H · O 는 R16 · R17 뒤 표에 들어온다)
            for (const QKeySequence& k : kv.second->shortcuts()) if (!k.isEmpty()) actual.insert(norm(k));
        }
        for (auto* sc : findChildren<QShortcut*>()) if (!sc->key().isEmpty()) actual.insert(norm(sc->key()));
        for (const KeyRow& r : keyRows()) for (const QString& q : r.seqs) table.insert(norm(QKeySequence(q)));
        QStringList missing, extra;
        for (const QString& k : actual) if (!table.contains(k)) missing << k;
        for (const QString& k : table) if (!actual.contains(k)) extra << k;
        missing.sort(); extra.sort();
        std::unique_ptr<QDialog> kd(keysDialog());
        kd->show();
        pump(100);
        auto* kt = kd->findChild<QTableWidget*>(QStringLiteral("keysTable"));
        double contrast = 0; bool twoCol = kt && kt->columnCount() == 4;
        if (kt) {
            const QImage img = kt->viewport()->grab().toImage().convertToFormat(QImage::Format_RGB32);
            QHash<QRgb, int> hist; for (int y = 0; y < img.height(); y += 2) for (int x = 0; x < img.width(); x += 2) ++hist[img.pixel(x, y)];
            QRgb bg = 0; int bn = -1; for (auto it = hist.begin(); it != hist.end(); ++it) if (it.value() > bn) { bn = it.value(); bg = it.key(); }
            auto lum = [](QRgb c) { auto f = [](int v) { double u = v / 255.0; return u <= 0.03928 ? u / 12.92 : std::pow((u + 0.055) / 1.055, 2.4); }; return 0.2126 * f(qRed(c)) + 0.7152 * f(qGreen(c)) + 0.0722 * f(qBlue(c)); };
            QRgb fg = bg; double best = 0; for (auto it = hist.begin(); it != hist.end(); ++it) { const double dd = std::fabs(lum(it.key()) - lum(bg)); if (dd > best) { best = dd; fg = it.key(); } }
            const double l1 = std::max(lum(bg), lum(fg)), l2 = std::min(lum(bg), lum(fg)); contrast = (l1 + 0.05) / (l2 + 0.05);
            if (!dir.isEmpty()) kd->grab().save(QDir(dir).filePath(QStringLiteral("keys.png")));
        }
        // offscreen 플랫폼은 색 체계를 알려 주지 않는다(Unknown) — 실제 Windows 에서는 시스템이 어두운 모드여도 Light 여야 한다(이 PC: 앱 어두운 모드 켜짐)
        const Qt::ColorScheme cs = QGuiApplication::styleHints()->colorScheme();
        const bool light = cs == Qt::ColorScheme::Light || (cs == Qt::ColorScheme::Unknown && QGuiApplication::platformName() == QLatin1String("offscreen"));
        const bool keysOk = missing.isEmpty() && extra.isEmpty() && twoCol && contrast >= 4.5 && light;
        L << QStringLiteral("ui-audit keys-table-match=%1 rows=%2 two-col=%3 contrast=%4 color-scheme=%5 missing=[%6] extra=[%7]%8").arg(missing.isEmpty() && extra.isEmpty() ? 1 : 0).arg(keyRows().size()).arg(twoCol ? 1 : 0)
                 .arg(contrast, 0, 'f', 1).arg(light ? QStringLiteral("light") : QStringLiteral("not-light(%1)").arg(int(QGuiApplication::styleHints()->colorScheme()))).arg(missing.join(QLatin1Char(','))).arg(extra.join(QLatin1Char(','))).arg(keysOk ? QString() : QStringLiteral("  FAIL"));
        ok &= keysOk;
    }

    // 4e) 오른쪽 판 삭제 · 고른 단면의 수치는 단면 목록 아래에 읽기 전용으로만(T7 · G4 · 사용자 결정 2026-10-10 「선택한 단면의 이 표시들도 단면 목록 밑에 확인용도로만」)
    const int inspectorCount = int(findChildren<QWidget*>(QStringLiteral("inspector")).size());
    int readoutRows = 0, readoutEditable = 0;
    const QFrame* roBox = findChild<QFrame*>(QStringLiteral("listReadout"));
    for (auto* v : findChildren<QLabel*>(QStringLiteral("roVal"))) if (v->isVisibleTo(this) && !v->text().trimmed().isEmpty() && v->text() != QStringLiteral("—")) ++readoutRows;
    if (roBox) readoutEditable = int(roBox->findChildren<QAbstractButton*>().size() + roBox->findChildren<QLineEdit*>().size());
    const bool readoutUnder = roBox && sidePanel_ && roBox->parentWidget() && sidePanel_->isAncestorOf(roBox);
    const bool listReadoutOk = inspectorCount == 0 && roBox && readoutUnder && readoutEditable == 0 && (!src_ || !plan_->hasLine() || readoutRows >= 9);
    L << QStringLiteral("ui-audit inspector=%1 list-readout rows=%2 read-only=%3 under-list=%4%5").arg(inspectorCount).arg(readoutRows).arg(roBox && readoutEditable == 0 ? 1 : 0).arg(readoutUnder ? 1 : 0).arg(listReadoutOk ? QString() : QStringLiteral("  FAIL"));
    ok &= listReadoutOk;

    // 4f) 높이 UI 삭제(T8 · G5 · 사용자 결정 2026-10-10 「높이 기준은 여기서 지정할 필요가 없다 — iTwin Capture 에서 이미 보정돼 들어온다」): 상태줄 높이 배지 · 지정 창 · 알림 띠의 지정 단추 ·
    //     홈의 높이 칸 · F1 표의 높이 기준 글 · 동작 height 가 하나도 없고, 수평 좌표계는 테두리 없는 작은 글(캔버스 Work 화판)이다. 표고 숫자(Z · 표고 (m) 눈금)는 그대로
    int heightUi = int(act_.count(QStringLiteral("height")));
    for (auto* b : findChildren<QAbstractButton*>()) if (b->isVisibleTo(this) && (b->text().startsWith(QStringLiteral("높이")) || b->text().contains(QStringLiteral("높이 기준")))) ++heightUi;
    for (auto* l : findChildren<QLabel*>()) if (l->isVisibleTo(this) && (l->text().startsWith(QStringLiteral("높이")) || l->text().contains(QStringLiteral("높이 기준")))) ++heightUi;
    if (startPage_) for (auto* t : startPage_->findChildren<QTableWidget*>())
        for (int c = 0; c < t->columnCount(); ++c) if (t->horizontalHeaderItem(c) && t->horizontalHeaderItem(c)->text() == QStringLiteral("높이")) ++heightUi;
    { std::unique_ptr<QDialog> kd(keysDialog());
      for (auto* t : kd->findChildren<QTableWidget*>())
          for (int r = 0; r < t->rowCount(); ++r) for (int c = 0; c < t->columnCount(); ++c) if (t->item(r, c) && t->item(r, c)->text().contains(QStringLiteral("높이 기준"))) ++heightUi; }
    const int statusBadgeCount = int(findChildren<QToolButton*>(QStringLiteral("statusBadge")).size());
    const bool srsPlain = srsLabel_ && !srsLabel_->inherits("QToolButton");   // 테두리 단추가 아닌 글
    const bool heightOk = heightUi == 0 && statusBadgeCount == 0 && srsPlain;
    L << QStringLiteral("ui-audit height-ui=%1 status-badges=%2 srs-plain=%3%4").arg(heightUi).arg(statusBadgeCount).arg(srsPlain ? 1 : 0).arg(heightOk ? QString() : QStringLiteral("  FAIL"));
    ok &= heightOk;

    // 5) 그리는 동안 상태줄 「다음:」 문장이 없는가
    int hint = -1, band = -1, guide2 = -1, compassOk = -1, noteOk = -1, drawRoOk = -1; QString guide2Why;
    if (src_) {
        const SectionLine keep = plan_->line(); const bool hadLine = plan_->hasLine();
        const QString keepMsg = msg_->text(); const bool planActive = planTitle_ && planTitle_->property("active").toBool();
        compassOk = plan_->compassHit(QPointF(plan_->width() - 34, plan_->height() - 60)) && !plan_->compassHit(QPointF(plan_->width() - 34, 40)) ? 1 : 0;   // 그리는 자리(오른쪽 아래)에서만 눌림(검토 I1)
        plan_->setDrawMode(true); pump(150);
        hint = msg_->text().trimmed().isEmpty() ? 0 : 1;
        band = guide_ && guide_->isVisible() ? 1 : 0;
        if (!dir.isEmpty()) grab().save(QDir(dir).filePath(QStringLiteral("draw.png")));
        // 5b) A′ 단계(검토 I3 · UI 3): 글 · 키 캡이 칩 안에 들어가고, 되돌리기 칸이 칩과 겹치지 않으며, 문장은 스펙 §5 그대로(길이는 평면 A′ 이름표가 든다)
        if (guide_ && undoBtns_ && planHost_) {
            SectionLine l2 = keep; l2.a = Vec2{0, 0}; l2.b = Vec2{7.43, 0};
            updateCtx(1, l2); pump(100);
            const bool inside = guide_->rect().contains(guide_->childrenRect());
            const bool apart = !undoBtns_->geometry().intersects(guide_->geometry()) && undoBtns_->geometry().right() <= planHost_->width();
            // 문장은 스펙 §5 그대로여야 한다. 글꼴이 없는 offscreen 은 글자가 네모(폭 2배)라 평면 폭을 다 써도 줄임표가 남을 수 있다 — 그때는 칩이 평면 폭을 다 썼는지로 판정(실측 Ruling)
            const QString spec = QStringLiteral("끝점 A′를 클릭하세요 · Shift = 축 맞춤");   // T5: 「숫자 = 길이」 · Enter 는 좌표 입력과 함께 없어짐 — 눌러도 아무 일도 없는 안내를 두지 않는다
            const bool keysOk = guide_->keys() == QStringList{QStringLiteral("Esc")};
            const bool textOk = guide_->hint() == spec || (guide_->hint().startsWith(spec.left(6)) && guide_->width() >= planHost_->width() - 2 * 12 - 32);   // 줄임표 알갱이(글자 한두 개) 허용
            // 도구 이름(단계)은 평면 폭에 들어가면 보여야 한다(검토 UI 5): 숨겼다면 칩이 평면 폭을 거의 다 쓴 경우만
            const bool titleOk = guide_->titleShown() || guide_->width() >= planHost_->width() - 2 * 12 - 32;
            noteOk = roNote_ && roNote_->text() == QStringLiteral("A 찍음 · 끝점 A′를 찍으세요") ? 1 : 0;
            {   // 그리는 중에도 같은 자리에 길이 · 방위 · A · 커서가 보인다(T7 — 오른쪽 판이 하던 일)
                QMap<QString, QString> ro;
                if (const QFrame* rb = findChild<QFrame*>(QStringLiteral("listReadout"))) {
                    const auto caps = rb->findChildren<QLabel*>(QStringLiteral("roCap")), vals = rb->findChildren<QLabel*>(QStringLiteral("roVal"));
                    for (int i = 0; i < caps.size() && i < vals.size(); ++i) if (caps[i]->isVisibleTo(this)) ro[caps[i]->text()] = vals[i]->text();
                }
                drawRoOk = ro.value(QStringLiteral("길이")) == QStringLiteral("7.43 m") && ro.contains(QStringLiteral("방위")) && !ro.value(QStringLiteral("A")).isEmpty() && ro.contains(QStringLiteral("커서")) ? 1 : 0;
            }
            // 화면을 옮겨 dpr 이 바뀌면(QEvent::DevicePixelRatioChange, Qt 6.6+) 칩 아이콘을 그 배율로 다시 구워야 한다(B5) — 같은 장이면 흐림
            const qint64 key0 = guide_->iconCacheKey();
            { QEvent dprEv(QEvent::DevicePixelRatioChange); QApplication::sendEvent(guide_, &dprEv); }
            const bool dprOk = guide_->iconCacheKey() != key0;
            guide2 = inside && apart && textOk && titleOk && dprOk && keysOk ? 1 : 0;
            guide2Why = QStringLiteral("keys=") + guide_->keys().join(QLatin1Char('+')) + QStringLiteral(" inside=%1 apart=%2 text=%3 title=%4 dpr-rerender=%9 guide-w=%5 host-w=%6 %8 hint=\"%7\"").arg(inside ? 1 : 0).arg(apart ? 1 : 0).arg(textOk ? 1 : 0).arg(titleOk ? 1 : 0).arg(guide_->width()).arg(planHost_->width()).arg(guide_->hint()).arg(guide_->debugWidths()).arg(dprOk ? 1 : 0);
            if (!dir.isEmpty()) grab().save(QDir(dir).filePath(QStringLiteral("draw2.png")));
        }
        plan_->setDrawMode(false); plan_->setLine(keep, hadLine); pump(100);
        showStatus(keepMsg); setActiveView(planActive ? 0 : 1);   // 감사가 바꾼 상태줄 · 활성 보기 되돌림(검토 M7)
    }
    L << QStringLiteral("ui-audit compass-hit=%1%2").arg(compassOk < 0 ? QStringLiteral("not-checked") : QString::number(compassOk)).arg(compassOk == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= compassOk != 0;
    L << QStringLiteral("ui-audit drawing-readout=%1%2").arg(drawRoOk < 0 ? QStringLiteral("not-checked") : QString::number(drawRoOk)).arg(drawRoOk == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= drawRoOk != 0;
    L << QStringLiteral("ui-audit draw-step2-note=%1%2").arg(noteOk < 0 ? QStringLiteral("not-checked") : QString::number(noteOk)).arg(noteOk == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= noteOk != 0;

    // 4i) 도구 커서(T11 · G8): 도구를 든 동안(긋기 A · A′, Shift = 축 맞춤)과 손잡이 위에서만 십자 + 단계 배지 그림 커서, 그 밖은 OS 화살표 — 핫스폿 (16,16) 논리 px · 지금 화면 배율로 구움
    {
        auto redInBadge = [](const QCursor& c) {   // 오른쪽 아래 배지 자리의 빨강 화소 수(글자 A · A′)
            const QImage im = c.pixmap().toImage().convertToFormat(QImage::Format_ARGB32);
            const double k = c.pixmap().devicePixelRatio();
            int n = 0;
            for (int y = int(18 * k); y < im.height(); ++y) for (int x = int(18 * k); x < im.width(); ++x) { const QRgb p = im.pixel(x, y); if (qAlpha(p) > 128 && qRed(p) > 200 && qGreen(p) < 80 && qBlue(p) < 80) ++n; }
            return n;
        };
        auto ringRed = [](const QCursor& c) {      // 손잡이 고리: 가운데에서 반지름 6–8 논리 px 사이의 빨강
            const QImage im = c.pixmap().toImage().convertToFormat(QImage::Format_ARGB32);
            const double k = c.pixmap().devicePixelRatio();
            int n = 0;
            for (int y = 0; y < im.height(); ++y) for (int x = 0; x < im.width(); ++x) {
                const double d = std::hypot(x / k - 16.0, y / k - 16.0); const QRgb p = im.pixel(x, y);
                if (d > 5.5 && d < 8.5 && qAlpha(p) > 128 && qRed(p) > 200 && qGreen(p) < 80 && qBlue(p) < 80) ++n;
            }
            return n;
        };
        auto sameImg = [](const QCursor& a, const QCursor& b) { return a.pixmap().toImage() == b.pixmap().toImage(); };
        const SectionLine keepLn = plan_->line(); const bool hadLn = plan_->hasLine();
        auto bitmap = [](const QCursor& c) { return c.shape() == Qt::BitmapCursor && !c.pixmap().isNull() && c.hotSpot() == QPoint(16, 16); };
        QCursor cA, cB, cBs, cEnd, cH, cOff;
        bool pressOk = false;
        if (src_) {
            plan_->setDrawMode(true); pump(50);
            cA = plan_->cursor();
            const QPointF mid(plan_->width() * 0.5, plan_->height() * 0.4);
            QMouseEvent press(QEvent::MouseButtonPress, mid, plan_->mapToGlobal(mid), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(plan_, &press);
            pressOk = plan_->drawStage() == 1;
            cB = plan_->cursor();
            const QPointF mv(mid.x() + 40, mid.y());
            QMouseEvent move(QEvent::MouseMove, mv, plan_->mapToGlobal(mv), Qt::NoButton, Qt::NoButton, Qt::ShiftModifier);
            QApplication::sendEvent(plan_, &move);
            cBs = plan_->cursor();
            plan_->setDrawMode(false); pump(50);
            cEnd = plan_->cursor();
            plan_->setLine(keepLn, hadLn); pump(50);
            if (hadLn) {
                const QPointF h = plan_->handleScreenPos(0);
                QMouseEvent onH(QEvent::MouseMove, h, plan_->mapToGlobal(h), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(plan_, &onH);
                cH = plan_->cursor();
                const QPointF away(h.x() + 120, h.y() + 120);
                QMouseEvent offH(QEvent::MouseMove, away, plan_->mapToGlobal(away), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
                QApplication::sendEvent(plan_, &offH);
                cOff = plan_->cursor();
            }
        }
        const bool stageAOk = bitmap(cA) && redInBadge(cA) > 0;
        const bool stageBOk = pressOk && bitmap(cB) && redInBadge(cB) > 0 && !sameImg(cA, cB);
        const bool shiftOk = bitmap(cBs) && !sameImg(cB, cBs);
        const bool endOk = cEnd.shape() == Qt::ArrowCursor;
        const bool handleOk = !hadLn || (bitmap(cH) && ringRed(cH) > 0 && cOff.shape() == Qt::ArrowCursor);
        const bool dprOk = !stageAOk || std::fabs(cA.pixmap().devicePixelRatio() - plan_->devicePixelRatioF()) < 1e-6;
        const bool cursorOk = !src_ || (stageAOk && stageBOk && shiftOk && endOk && handleOk && dprOk);
        L << QStringLiteral("ui-audit cursor stage-A=%1 stage-A2=%2 shift=%3 end-arrow=%4 handle=%5 dpr=%6 hot=16,16%7").arg(stageAOk ? 1 : 0).arg(stageBOk ? 1 : 0).arg(shiftOk ? 1 : 0).arg(endOk ? 1 : 0).arg(handleOk ? 1 : 0).arg(dprOk ? 1 : 0).arg(cursorOk ? QString() : QStringLiteral("  FAIL"));
        ok &= cursorOk;
    }
    L << QStringLiteral("ui-audit guide-step2=%1 %2%3").arg(guide2 < 0 ? QStringLiteral("not-checked") : QString::number(guide2)).arg(guide2Why).arg(guide2 == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= guide2 != 0;
    L << QStringLiteral("ui-audit next-hint-visible-while-tool=%1%2").arg(hint < 0 ? QStringLiteral("not-checked") : QString::number(hint)).arg(hint == 1 ? QStringLiteral("  FAIL") : QString());
    ok &= hint != 1;
    L << QStringLiteral("ui-audit guide-band=%1%2").arg(band < 0 ? QStringLiteral("not-checked") : QString::number(band)).arg(band == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= band != 0;

    // 6) 도면 탭(v5 C5 · 스펙 §8): 왼쪽 목록 348 · 오른쪽 판 320 · 고르는 곳은 목록 하나(판에 「평면도 | 단면도」 없음) · 흙색 주 단추 하나 ·
    //    알림은 「도면 점검」 상자 안에만(warnBox 없음) · 레벨선 pt 칸 둘 · 축척 격자 8 · 종이 아래 도구 줄 6칸 · 떠 있는 「도면 보기」 1 · 점검 항목 3 이상 · 목록 줄 = 평면 1 + 단면 n
    QString sheetLine = QStringLiteral("ui-audit sheet=not-checked");
    int listsMatch = -1;
    if (src_ && section_->hasResult()) {
        dlgSheet(); pump(400);
        if (QWidget* page = sheetHost_) {
            int typeSel = 0, primary = 0, noticesOut = 0, ptSpins = 0, grid = 0, tools = 0, guides = 0, checkItems = 0, rightW = -1;
            for (auto* b : page->findChildren<QToolButton*>())
                if (b->isVisibleTo(page) && b->objectName() == QLatin1String("chip") && (b->text() == QStringLiteral("평면도") || b->text() == QStringLiteral("단면도"))) ++typeSel;
            for (auto* b : page->findChildren<QPushButton*>()) if (b->isVisibleTo(page) && b->objectName() == QLatin1String("primary")) ++primary;
            for (auto* f : page->findChildren<QFrame*>()) if (f->isVisibleTo(page) && f->objectName() == QLatin1String("warnBox")) ++noticesOut;
            for (auto* s : page->findChildren<QDoubleSpinBox*>()) if (s->isVisibleTo(page) && s->suffix().trimmed() == QLatin1String("pt")) ++ptSpins;
            for (auto* b : page->findChildren<QToolButton*>(QStringLiteral("scaleGrid"))) if (b->isVisibleTo(page)) ++grid;
            for (auto* b : page->findChildren<QToolButton*>(QStringLiteral("floatCell"))) if (b->isVisibleTo(page)) ++tools;
            for (auto* g : page->findChildren<QFrame*>(QStringLiteral("guideBand"))) if (g->isVisibleTo(page)) ++guides;
            for (auto* l : page->findChildren<QLabel*>()) if (l->isVisibleTo(page) && (l->objectName() == QLatin1String("checkOk") || l->objectName() == QLatin1String("checkWarn"))) ++checkItems;
            for (auto* w : page->findChildren<QWidget*>(QStringLiteral("dialogSide"))) if (w->isVisibleTo(page)) rightW = w->width();
            const int sideW = sheetSide_ ? sheetSide_->width() : -1;
            listsMatch = sheetList_ && sheetList_->count() == 1 + int(sections_.size()) + 2 ? 1 : 0;   // 절 이름 줄 「평면」 「단면」 2개 포함
            const bool sheetOk = typeSel == 0 && primary == 1 && noticesOut == 0 && ptSpins == 2 && grid == 8 && tools == 6 && guides == 1 && checkItems >= 3 && sideW == 348 && rightW == 320;
            sheetLine = QStringLiteral("ui-audit sheet type-selectors=%1 primary=%2 notices-outside-check=%3 pt-spins=%4 scale-grid=%5 tools=%6 guide=%7 check-items=%8 list-w=%9 side-w=%10%11")
                            .arg(typeSel).arg(primary).arg(noticesOut).arg(ptSpins).arg(grid).arg(tools).arg(guides).arg(checkItems).arg(sideW).arg(rightW).arg(sheetOk ? QString() : QStringLiteral("  FAIL"));
            ok &= sheetOk;
            if (!dir.isEmpty()) grab().save(QDir(dir).filePath(QStringLiteral("sheet.png")));
        }
        closeSheetTab(); pump(200);
        const bool discard = viewTabs_ && viewTabs_->count() == 2 && !sheetHost_ && !embeddedSave_;
        L << QStringLiteral("ui-audit sheet-discard=%1%2").arg(discard ? 1 : 0).arg(discard ? QString() : QStringLiteral("  FAIL"));
        ok &= discard;
    }
    L << QStringLiteral("ui-audit lists-match=%1%2").arg(listsMatch < 0 ? QStringLiteral("not-checked") : QString::number(listsMatch)).arg(listsMatch == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= listsMatch != 0;
    L << sheetLine;

    // 8) 사각 테두리 없음(사용자 지시 2026-10-09 「앱 안에 UI 의 사각형 선들은 전부 라운드 처리하라」): 기본 규칙(상태 · 속성 선택자 없음)에
    //    border: Npx solid 가 있으면 border-radius 가 0 보다 커야 한다. 한 변 선(border-bottom 등)은 선이지 사각형이 아니라 제외
    QStringList square;
    {
        const QString css = theme::styleSheet(false);
        static const QRegularExpression rule(QStringLiteral("([^{}]+)\\{([^{}]*)\\}"));
        static const QRegularExpression solid(QStringLiteral("\\bborder:\\s*\\d+px\\s+solid"));
        static const QRegularExpression radius(QStringLiteral("border(-top-left|-top-right|-bottom-left|-bottom-right)?-radius:\\s*([0-9.]+)px"));
        auto it = rule.globalMatch(css);
        while (it.hasNext()) {
            const QRegularExpressionMatch m = it.next();
            const QString sel = m.captured(1).trimmed(), body = m.captured(2);
            if (sel.contains(QLatin1Char(':')) || sel.contains(QLatin1Char('['))) continue;
            if (!solid.match(body).hasMatch()) continue;
            const QRegularExpressionMatch rm = radius.match(body);
            if (!rm.hasMatch() || rm.captured(2).toDouble() <= 0) square << sel;
        }
    }
    L << QStringLiteral("ui-audit square-borders=%1%2").arg(square.size()).arg(square.isEmpty() ? QString() : QStringLiteral("  FAIL"));
    for (const QString& sq : square) L << QStringLiteral("square: %1").arg(sq);
    ok &= square.isEmpty();

    // 7) 아이콘 · 툴팁(D11)
    L << QStringLiteral("ui-audit icons-missing=%1 tooltip-missing=%2").arg(noIcon.size()).arg(noTip.size());
    for (const QString& n : noIcon) L << QStringLiteral("no-icon: %1").arg(n);
    for (const QString& n : noTip) L << QStringLiteral("no-tooltip: %1").arg(n);
    ok &= noIcon.isEmpty() && noTip.isEmpty();

    // 7b) 깊이 음영 체크(사용자 지시 2026-10-10 「깊이 음영 체크를 만들라」 · 답 「단면 머리 · 기본 끔」): 단면 머리에 글자 있는 체크 칸 —
    //     좁아서 켜기 · 끄기 상자가 「그릴 것 ▾」로 접히면 그 메뉴의 같은 동작. 체크 ↔ 동작 ↔ 그리기 설정(secStyle)이 함께 움직이고, 저장값이 없으면 꺼짐
    {
        auto* cb = secTitleBar_->findChild<QCheckBox*>(QStringLiteral("depthFadeCheck"));
        QAction* fa = action("fade");
        const bool inMenu = secTogglesMenu_ && secTogglesMenu_->isVisibleTo(secTitleBar_) && secTogglesMenu_->menu() && secTogglesMenu_->menu()->actions().contains(fa);
        const bool shown = cb && cb->isVisibleTo(secTitleBar_);
        const bool check = cb && (shown || inMenu);
        const bool label = shown ? cb->text() == QStringLiteral("깊이 음영") && cb->width() >= cb->sizeHint().width() : (cb && inMenu && fa->text() == QStringLiteral("깊이 음영"));
        const int defOff = QSettings().contains(QStringLiteral("section/depthFade")) ? -1 : (fa->isChecked() ? 0 : 1);   // -1 = 저장값이 있어 기본값을 잴 수 없음
        bool bound = false;
        if (cb) {
            const bool was = fa->isChecked();
            cb->click();   // 체크 칸 → 동작 · 그리기 설정
            pump(50);
            const bool fwd = fa->isChecked() == !was && secStyle().depthFade == !was && cb->isChecked() == !was;
            fa->toggle();  // 동작(메뉴 · 되돌리기 쪽) → 체크 칸
            pump(50);
            bound = fwd && cb->isChecked() == was && secStyle().depthFade == was;
        }
        const bool okf = check && label && bound && defOff != 0;
        L << QStringLiteral("ui-audit depth-fade check=%1 label=%2 bound=%3 default-off=%4 at=%5%6").arg(int(check)).arg(int(label)).arg(int(bound)).arg(defOff)
                 .arg(shown ? QStringLiteral("header") : inMenu ? QStringLiteral("menu") : QStringLiteral("none")).arg(okf ? QString() : QStringLiteral("  FAIL"));
        ok &= okf;
    }

    // 7c) 앱 이름(사용자 결정 2026-10-10 「Trench」): 보이는 이름 — 창 제목 · 표시 이름 · 홈 이름 라벨 — 이 모두 Trench 이고 옛 이름이 남지 않는다.
    //     내부 식별자(SectionViewer.exe · QSettings 이름)는 이 판정 밖(장부 Ruling 1)
    {
        const QString ttl = windowTitle(), dsp = QApplication::applicationDisplayName();
        QLabel* hl = findChild<QLabel*>(QStringLiteral("heroName"));
        const QString hero = hl ? hl->text() : QString();
        const auto good = [](const QString& s) { return s.contains(QLatin1String("Trench")) && !s.contains(QLatin1String("Kerf")); };
        const bool okn = good(ttl) && good(dsp) && hero == QLatin1String("Trench");
        L << QStringLiteral("ui-audit app-name title=\"%1\" display=\"%2\" home=\"%3\"%4").arg(ttl, dsp, hero, okn ? QString() : QStringLiteral("  FAIL"));
        ok &= okn;
    }

    // 2b) 좁은 창 1280×800 — 맨 끝에서(offscreen 에서는 resize 를 되돌려도 판 폭이 돌아오지 않아 뒤 항목을 망친다, 실측): 글자 숨김 · 타일 32 이하(스펙 §3.3 · §12)
    // 고르기 규칙 + 실제로 줄어든 창(B5): 창 폭 ≤ 1280 · 그 리본 타일 ≤ 32 · 글자 숨김 · 맞춤 상태의 평면 · 단면 그림이 잘리지 않음(자동 맞춤) ·
    // 사용자가 확대한 보기는 창 크기가 바뀌어도 축척이 그대로(zoom-kept — 지킴 시험, 자동 맞춤이 사용자 보기까지 다시 맞추면 0)
    // 1280 좁은 창(spec 20261010-01 §3-A): 실행 크기가 1280 이하면 probe(1600×1000)를 거쳐 실제로 줄어든 창에서 잰다(ⓔ — 1280 실행의 plan-fits · zoom-kept 가 빈 값이 아니게).
    // 같은 블록에서 단면 머리(sec-header) · 정보 줄(strip) · 평면 눈금 글(plan-tick) · 홈 한 줄 글(home) · 세로 과장 뒤 맞춤 상태(vex-fit)를 상태별(run · 1280w · 1280L · 1280p · 1280d · probe · 1280v)로 잰다
    struct Fit { int secOverlap = 0; bool kept = true, toggles = true, carets = true; int caretInk = -1, caretNeed = 0; QStringList secAt; int stripBad = 0; bool stateKept = true, gapKept = true; int tipMissing = 0; QStringList stripAt; int tickBad = 0; QStringList tickAt; int tickStep = -1; double tickPxPerM = 0; } F;
    auto measure = [&](const QString& st, bool ticks) {
        pump(100);
        // 단면 머리: 겹침 · 밖 · 짓눌림 합. 제목 · 축척 칸 · 맞춤 · 확대 · 축소는 늘 보이고, 켜기 · 끄기 넷은 상자(viewToggles)나 「그릴 것」 메뉴(secTogglesMenu, 같은 QAction 넷)로 닿는다
        const int so = overlapCount(secTitleBar_);
        F.secOverlap += so; F.secAt << QStringLiteral("%1:%2:%3").arg(st).arg(secHeaderStep_).arg(so);
        QSet<QString> seenBtn;
        for (auto* b : secTitleBar_->findChildren<QToolButton*>())
            if (b->isVisibleTo(secTitleBar_)) if (QAction* a = b->defaultAction()) seenBtn.insert(a->property("iconName").toString());
        if (!(secTitle_ && secTitle_->isVisibleTo(secTitleBar_) && scaleCombo_ && scaleCombo_->isVisibleTo(secTitleBar_)
              && seenBtn.contains(QStringLiteral("square-dashed")) && seenBtn.contains(QStringLiteral("zoom-in")) && seenBtn.contains(QStringLiteral("zoom-out"))))
            F.kept = false;
        bool reach = false;
        if (QWidget* box = secTitleBar_->findChild<QWidget*>(QStringLiteral("viewToggles"))) reach = box->isVisibleTo(secTitleBar_);
        if (!reach)
            if (auto* mb = secTitleBar_->findChild<QToolButton*>(QStringLiteral("secTogglesMenu")))
                if (mb->isVisibleTo(secTitleBar_) && mb->menu()) {
                    const QList<QAction*> acts = mb->menu()->actions();
                    reach = acts.contains(action("image")) && acts.contains(action("line")) && acts.contains(action("levels")) && acts.contains(action("fade")) && acts.contains(action("ticks"));
                }
        if (!reach) F.toggles = false;
        // 보이는 메뉴 단추(「그릴 것」 · 세로 과장)는 ▾ 가 보여야 한다(스펙 §4 「그릴 것 ▾」 · 「세로 ×1 ▾」 — QSS 가 메뉴 표시를 지우므로 글 ▾ 또는 아이콘에 구운 ▾, rework 2 UI S1).
        // 구운 ▾ 는 픽셀로 잰다(rework 3 ⓑ): 단추 아이콘의 그 배율 장에서 ▾ 삼각형(kerf::caretTriangle — 그리기와 같은 모양) 안쪽 픽셀 중 먹색인 수가
        // 리본 메뉴 칩 ▾ 넓이(6 × 4 삼각형 = 12 논리 px²) × dpr² 이상이어야 한다 — 속성(caretBaked)만 믿던 판정은 테가 안쪽을 덮어 「점」이 된 ▾ 도 통과시켰다
        for (QToolButton* mb : {secTogglesMenu_, vexBtn_})
            if (mb && mb->isVisibleTo(secTitleBar_)) {
                const bool textCaret = mb->toolButtonStyle() != Qt::ToolButtonIconOnly && mb->text().contains(QChar(0x25BE));
                if (textCaret) continue;
                const qreal d = mb->devicePixelRatioF();
                const QImage a = mb->icon().pixmap(mb->iconSize(), d).toImage().convertToFormat(QImage::Format_ARGB32);
                QPolygonF tri = kerf::caretTriangle(mb->iconSize().width());
                for (QPointF& q : tri) q *= a.width() / qreal(mb->iconSize().width());   // 논리 → 그 장의 픽셀
                auto ink = [](QRgb c) { return qAlpha(c) > 200 && std::abs(qRed(c) - theme::Hand.red()) < 40 && std::abs(qGreen(c) - theme::Hand.green()) < 40 && std::abs(qBlue(c) - theme::Hand.blue()) < 40; };
                int inked = 0;
                for (int y = 0; y < a.height(); ++y)
                    for (int x = 0; x < a.width(); ++x) if (tri.containsPoint(QPointF(x + 0.5, y + 0.5), Qt::OddEvenFill) && ink(a.pixel(x, y))) ++inked;
                const qreal s = a.width() / qreal(mb->iconSize().width());
                const int need = int(std::lround(12.0 * s * s));
                F.caretInk = F.caretInk < 0 ? inked : std::min(F.caretInk, inked);
                F.caretNeed = need;
                if (inked < need) F.carets = false;
            }
        // 정보 줄: 겹침 · 밖 · 짓눌림 + 잘린 글, 계산 상태 글은 늘 보임, 빈 구간 꼬리는 늘 보임(rework 2 correctness C1), 줄인 글에는 전체 글 툴팁
        int tip = 0;
        const int sb = overlapCount(infoStrip_) + clippedLabels(infoStrip_, &tip);
        F.stripBad += sb; F.tipMissing += tip; F.stripAt << QStringLiteral("%1:%2:%3").arg(st).arg(stripStep_).arg(sb);
        if (stripState_ && !stripState_->text().isEmpty() && !stripState_->isVisibleTo(infoStrip_)) F.stateKept = false;
        if (stripCut_ && stripCut_->isVisibleTo(infoStrip_) && stripCutShort_.contains(QStringLiteral("빈 구간")) && !stripCut_->text().contains(stripGapTail(stripCutShort_))) F.gapKept = false;
        // 평면 눈금 글(run · 1280w): 숫자 글 사각형끼리 겹치는 쌍 수. px/m 은 위에서 본 화면이라 1/mpp
        if (ticks && plan_->hasLine()) {
            const QVector<QRectF> rs = plan_->tickLabelRects();
            int n = 0;
            for (int i = 0; i < rs.size(); ++i) for (int j = i + 1; j < rs.size(); ++j) if (rs[i].intersects(rs[j])) ++n;
            F.tickBad += n; F.tickAt << QStringLiteral("%1:%2:%3").arg(st).arg(plan_->tickLabelStepM()).arg(n);
            if (st == QLatin1String("1280w")) { F.tickStep = plan_->tickLabelStepM(); F.tickPxPerM = plan_->metersPerPixel() > 0 ? 1.0 / plan_->metersPerPixel() : 0; }
        }
    };
    constexpr int kNarrowFloor = 24;   // 1280 에서도 타일 24 이상(2026-10-09 「스펙을 실제로 고친다」 이후 실측: 20 → T5 24 → T6 32). 정확한 값은 규칙(창 폭에 들어가는 가장 큰 크기)이 정하고 출력에 적는다
    const RibbonLook nk = Ribbon::looks().at(Ribbon::chooseLook(lw, 1280));   // lookWidths 에 리본 좌우 여백이 이미 들어 있다
    const QSize keepSize = size();
    const QString keepMsg2 = msg_ ? msg_->text() : QString(); const bool planActive2 = planTitle_ && planTitle_->property("active").toBool();
    measure(QStringLiteral("run"), true);
    // refit(rework 2 code S2 · correctness C3): 제목을 바꾼 뒤 updateHeader 한 번 = 머리 다시 맞춤 많아야 1번(방위 글까지 바꾼 끝에서), 같은 입력으로 또 부르면 0번
    int refitA = -1, refitB = -1;
    if (secTitle_) {
        setSecTitleText(QStringLiteral("단면 감사"));
        const int p0 = secHeaderPasses_; updateHeader(); refitA = secHeaderPasses_ - p0;
        const int p1 = secHeaderPasses_; updateHeader(); refitB = secHeaderPasses_ - p1;
    }
    // 홈(rework 2 code B1 · UI B1): 실행 크기와 1280 에서 잰다 — 잘린 라벨(빈 글 · 「…」만 · 「…」보다 좁음 포함)과 이어서 작업 칸(폴더 칸 제외)에서 줄인 글 수
    int homeClipped = 0, homeTip = 0, homeMinW = -1, homeStale = -1, homeCells = 0, homeModeBad = 0; QStringList homeNames, homeAt, homeModes;
    auto measureHome = [&](const QString& st, bool stale) {
        if (!startPage_) return;
        showStart(true); pump(250);
        if (stale) {   // AC14: 홈을 두 번 그린 뒤(모델이 열린 채 showStart(true) 두 번) startPage_ 의 직접 자식 중 어느 레이아웃도 관리하지 않는 보이는 위젯 = 0 — deleteLater 대기분은 먼저 비운다
            showStart(true); pump(250);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            std::function<bool(QLayout*, QWidget*)> managed = [&managed](QLayout* l, QWidget* w) -> bool {
                if (!l) return false;
                if (l->indexOf(w) >= 0) return true;
                for (int i = 0; i < l->count(); ++i) if (QLayout* sub = l->itemAt(i)->layout()) if (managed(sub, w)) return true;
                return false;
            };
            homeStale = 0;
            for (QObject* o : startPage_->children())
                if (o->isWidgetType()) { auto* w = static_cast<QWidget*>(o); if (w->isVisibleTo(startPage_) && !managed(startPage_->layout(), w)) ++homeStale; }
        }
        QStringList nm; int tip = 0;
        const int c = clippedLabels(startPage_, &tip, &nm);
        int cells = 0;
        for (QWidget* cw : startPage_->findChildren<QWidget*>(QStringLiteral("ccCells"))) {
            if (!cw->isVisibleTo(startPage_)) continue;
            for (QLabel* l : cw->findChildren<QLabel*>()) {
                if (!l->isVisibleTo(cw)) continue;
                bool inFolder = false;
                for (QWidget* p = l->parentWidget(); p && p != cw; p = p->parentWidget()) if (p->objectName() == QLatin1String("ccCellFirst")) inFolder = true;
                const QVariant full = l->property("fullText");
                if (!inFolder && full.isValid() && l->text() != full.toString()) { ++cells; if (nm.size() < 6) nm << full.toString().left(16); }
            }
        }
        // 칸 배치(rework 3 ⓐ): 칸 폭이 폴더 2 : 나머지 1(옛 배치, 정수 반올림 ±2 px)이면 ratio, 아니면 content. 실행 크기 1920 이상은 ratio(모양 그대로), 1280(합성)은 content(좁을 때만 내용 폭)
        QString mode = QStringLiteral("none");
        for (QWidget* cw : startPage_->findChildren<QWidget*>(QStringLiteral("ccCells"))) {
            if (!cw->isVisibleTo(startPage_)) continue;
            QList<int> ws;
            for (QObject* o : cw->children()) if (o->isWidgetType() && static_cast<QWidget*>(o)->isVisibleTo(cw)) ws << static_cast<QWidget*>(o)->width();
            if (ws.size() == 5) {   // 폴더 · 마지막 열림 · 수평 좌표계 · 단면 · 도면(T8: 높이 칸 없음)
                bool ratio = std::abs(ws[0] - 2 * ws[1]) <= 2;
                for (int i = 2; i < 5; ++i) ratio = ratio && std::abs(ws[i] - ws[1]) <= 1;
                mode = ratio ? QStringLiteral("ratio") : QStringLiteral("content");
            }
        }
        if ((st == QLatin1String("run") && keepSize.width() >= 1920 && mode != QLatin1String("ratio")) || (st == QLatin1String("1280w") && mode != QLatin1String("content"))) ++homeModeBad;
        homeModes << st + QLatin1Char(':') + mode;
        homeClipped += c; homeTip += tip; homeCells += cells; homeMinW = std::max(homeMinW, minimumSizeHint().width());
        for (const QString& s : nm) if (homeNames.size() < 6) homeNames << st + QLatin1Char(':') + s;
        homeAt << QStringLiteral("%1:%2:%3").arg(st).arg(c).arg(cells);
        showStart(false); pump(100);
    };
    measureHome(QStringLiteral("run"), false);
    const QSize probe = keepSize.width() > 1280 ? keepSize : QSize(1600, 1000);
    if (keepSize.width() <= 1280) {   // probe 크기에서 「맞춤 상태」를 명시해 둔다 — 1280 실행은 처음부터 1280 에 맞춰져 있어 이것 없이는 맞춤 길을 꺼도 1280 으로 돌아오면 들어맞는다(판정, build.md)
        resize(probe); pump(250);
        if (src_) plan_->fitAll();
        if (section_->hasResult()) section_->fit();
        pump(50);
    }
    const QSize beforeNarrow = size();
    resize(1280, 800); pump(250);
    const bool resized = size() != beforeNarrow;
    const RibbonLook ak = ribbon_->look();
    const int narrowW = width(), narrowMinW = minimumSizeHint().width();
    const int planFits = src_ ? (plan_->contentFits() ? 1 : 0) : -1, secFits = section_->hasResult() ? (section_->contentFits() ? 1 : 0) : -1;
    measure(QStringLiteral("1280w"), true);
    if (secTitle_) {   // 1280L: 40자 긴 단면 이름(Review Focus 1) — updateHeader 와 같은 길(setSecTitleText)
        setSecTitleText(QStringLiteral("%1 단면").arg(QStringLiteral("트렌치 3호 북벽 서쪽 끝 A–A′ (2026-10-09 다시 그음)")));
        measure(QStringLiteral("1280L"), false);
        updateHeader();
    }
    if (stripState_ && stripCut_) {   // 1280p: 가장 긴 정보 줄 글(계산 중 + 빈 구간 있음) — updateHeader 와 같은 길(setStripTexts)
        setStripTexts(QStringLiteral("<span style='color:#7A5A00'>단면선 정확 · 배경 미리보기 → 최종 계산 중…</span>"),
                      QStringLiteral("잘린 선 12줄 · 1234점 · 빈 구간 12곳 10.25 m"), QStringLiteral("잘린 선 12줄 · 빈 구간 12곳 10.25 m"));
        measure(QStringLiteral("1280p"), false);
        updateHeader();
    }
    if (src_) {   // 1280d: 그리는 중(정보 줄 「단면선을 긋는 중 — …」) → 끄고 선 되살림
        const SectionLine keepL = plan_->line(); const bool hadL = plan_->hasLine();
        plan_->setDrawMode(true);
        measure(QStringLiteral("1280d"), false);
        plan_->setDrawMode(false); plan_->setLine(keepL, hadL); pump(100);
        updateHeader();
    }
    measureHome(QStringLiteral("1280w"), true);   // home: 1280 에서 홈의 한 줄 글이 잘리지 않고 창 최소 폭 ≤ 1280(ⓕ — 긴 모델 이름은 §4 회귀 7 이 따로 잰다) + AC14
    if (!dir.isEmpty()) grab().save(QDir(dir).filePath(QStringLiteral("narrow.png")));
    int zoomKept = -1;
    if (src_ && section_->hasResult()) {
        section_->zoomBy(1.4); plan_->zoomBy(1 / 1.4); pump(50);
        const double ppm = section_->xf().ppm, mpp = plan_->metersPerPixel();
        resize(probe); pump(250);
        zoomKept = section_->xf().ppm == ppm && plan_->metersPerPixel() == mpp ? 1 : 0;   // 축척 값은 같아야 함(허용오차 없음 — 맞춤 상태가 아니면 resize 가 ppm · mpp 를 다시 계산하지 않는다)
        if (probe != keepSize) measure(QStringLiteral("probe"), false);
        section_->fit(); plan_->fitAll(); pump(50);
    } else { resize(probe); pump(250); if (probe != keepSize) measure(QStringLiteral("probe"), false); }
    // vex-fit(keep, Q3 사용자 답 「사용자가 바꾼 보기로 본다」): 맞춤 상태(probe) → 세로 ×10 → ppm A → 1280 → ppm B, 같아야 1(허용오차 없음). 그 채로 1280v 머리 측정 → 되돌림
    int vexFit = -1; double ppmA = 0, ppmB = 0;
    if (src_ && section_->hasResult()) {
        section_->fit(); section_->setVerticalExaggeration(10); updateVexUi(); pump(50);
        ppmA = section_->xf().ppm;
        resize(1280, 800); pump(250);
        ppmB = section_->xf().ppm;
        vexFit = ppmA == ppmB ? 1 : 0;
        measure(QStringLiteral("1280v"), false);
        section_->setVerticalExaggeration(1); section_->fit(); updateVexUi(); pump(50);
    }
    if (size() != keepSize) { resize(keepSize); pump(250); }
    showStatus(keepMsg2); setActiveView(planActive2 ? 0 : 1);
    const bool secOk = F.secOverlap == 0 && F.kept && F.toggles && F.carets && refitA <= 1 && refitB <= 0;
    L << QStringLiteral("ui-audit sec-header-overlap=%1 kept=%2 toggles-reachable=%3 carets=%4 caret-ink=%5/%6 refit=%7/%8 at=%9%10").arg(F.secOverlap).arg(F.kept ? 1 : 0).arg(F.toggles ? 1 : 0).arg(F.carets ? 1 : 0).arg(F.caretInk).arg(F.caretNeed).arg(refitA).arg(refitB)
             .arg(F.secAt.join(QLatin1Char(','))).arg(secOk ? QString() : QStringLiteral("  FAIL"));
    ok &= secOk;
    const bool stripOk = F.stripBad == 0 && F.stateKept && F.gapKept && F.tipMissing == 0;
    L << QStringLiteral("ui-audit strip-clipped=%1 state-kept=%2 gap-kept=%3 elide-tip-missing=%4 at=%5%6").arg(F.stripBad).arg(F.stateKept ? 1 : 0).arg(F.gapKept ? 1 : 0).arg(F.tipMissing).arg(F.stripAt.join(QLatin1Char(','))).arg(stripOk ? QString() : QStringLiteral("  FAIL"));
    ok &= stripOk;
    if (F.tickAt.isEmpty()) L << QStringLiteral("ui-audit plan-tick-overlap=not-checked");
    else {
        L << QStringLiteral("ui-audit plan-tick-overlap=%1 step=%2 px-per-m=%3 at=%4%5").arg(F.tickBad).arg(F.tickStep).arg(F.tickPxPerM, 0, 'f', 1).arg(F.tickAt.join(QLatin1Char(','))).arg(F.tickBad == 0 ? QString() : QStringLiteral("  FAIL"));
        ok &= F.tickBad == 0;
    }
    const bool homeOk = homeClipped == 0 && homeCells == 0 && homeModeBad == 0 && homeTip == 0 && homeMinW <= 1280;
    L << QStringLiteral("ui-audit home-clipped=%1%2 cells-elided=%3 cells-mode=%4 elide-tip-missing=%5 min-width=%6 at=%7%8").arg(homeClipped)
             .arg(homeNames.isEmpty() ? QString() : QStringLiteral(" [%1]").arg(homeNames.join(QLatin1Char(','))))
             .arg(homeCells).arg(homeModes.join(QLatin1Char(','))).arg(homeTip).arg(homeMinW).arg(homeAt.join(QLatin1Char(','))).arg(homeOk ? QString() : QStringLiteral("  FAIL"));
    ok &= homeOk;
    L << QStringLiteral("ui-audit home-stale-children=%1%2").arg(homeStale).arg(homeStale > 0 ? QStringLiteral("  FAIL") : QString());   // AC14
    ok &= homeStale <= 0;
    L << QStringLiteral("ui-audit vex-fit=%1 mode=keep ppm=%2/%3%4").arg(vexFit).arg(ppmA, 0, 'g', 6).arg(ppmB, 0, 'g', 6).arg(vexFit == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= vexFit != 0;
    const bool narrowOk = nk.tile >= kNarrowFloor && narrowW <= 1280 && ak.tile == nk.tile && ak.labels == nk.labels && planFits != 0 && secFits != 0 && zoomKept != 0 && resized;
    L << QStringLiteral("ui-audit narrow rule-at-1280 look=%1 labels=%2 window-min-width=%3 actual-width=%4 actual-look=%5 actual-labels=%6 plan-fits=%7 section-fits=%8 zoom-kept=%9 probe=%10x%11 resized=%12%13")
             .arg(nk.tile).arg(nk.labels ? 1 : 0).arg(narrowMinW).arg(narrowW).arg(ak.tile).arg(ak.labels ? 1 : 0).arg(planFits).arg(secFits).arg(zoomKept).arg(probe.width()).arg(probe.height()).arg(resized ? 1 : 0).arg(narrowOk ? QString() : QStringLiteral("  FAIL"));
    ok &= narrowOk;

    if (!dir.isEmpty()) grab().save(QDir(dir).filePath(QStringLiteral("work.png")));
    L << QStringLiteral("ui-audit RESULT %1").arg(ok ? QStringLiteral("ok") : QStringLiteral("fail"));
    if (!dir.isEmpty()) {
        QFile f(QDir(dir).filePath(QStringLiteral("ui-audit.txt")));
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write((L.join(QLatin1Char('\n')) + QLatin1Char('\n')).toUtf8());
    }
    if (out) *out = L.join(QLatin1Char('\n'));
    return ok;
}

// ---------------------------------------------------------------- 아이콘 전체 캡처(v5 A3)
// 모든 theme::Ico 이름에 SVG 가 있어야 하고, 묶음의 이름 전부를 칩 모양(타일 50 · 아이콘 28)으로 격자에 그려 PNG 로 남긴다
bool MainWindow::iconSheet(const QString& png, QString* out) {
    QStringList L, missing;
    for (int i = 0; i < theme::kIcoCount; ++i) {
        const QString n = QString::fromLatin1(theme::iconName(theme::Ico(i)));
        if (!kerf::hasIcon(n) && !missing.contains(n)) missing << n;
    }
    const QStringList names = kerf::iconNames();
    const int cell = 76, cols = 12, rows = std::max(1, (int(names.size()) + cols - 1) / cols);
    QPixmap pm(cell * cols, cell * rows);
    pm.fill(theme::Ground);
    {
        QPainter p(&pm);
        p.setFont(theme::uiFont(10)); p.setPen(theme::Muted);
        const kerf::ChipColors cc = theme::chipColors();
        for (int i = 0; i < names.size(); ++i) {
            const int x = (i % cols) * cell, y = (i / cols) * cell;
            kerf::chipIcon(names[i], 50, 28, kerf::ChipKind::Normal, cc).paint(&p, QRect(x + 13, y + 4, 50, 50));
            p.drawText(QRect(x, y + 56, cell, 16), Qt::AlignCenter, names[i]);
        }
    }
    const bool saved = !png.isEmpty() && pm.save(png);
    const bool ok = missing.isEmpty() && !names.isEmpty() && (png.isEmpty() || saved);
    L << QStringLiteral("icon-sheet n=%1 missing=%2 png=%3 RESULT %4").arg(names.size()).arg(missing.size()).arg(saved ? png : QStringLiteral("-")).arg(ok ? QStringLiteral("ok") : QStringLiteral("fail"));
    for (const QString& m : missing) L << QStringLiteral("icon-missing: %1").arg(m);
    if (out) *out = L.join(QLatin1Char('\n'));
    return ok;
}
