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
#include <QCryptographicHash>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QSet>
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
           levels == o.levels && fade == o.fade && smooth == o.smooth && heightDecl == o.heightDecl && current == o.current;
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
    labels_.push_back({a, ko, en, 2});
    return a;
}

void MainWindow::retranslate() {
    for (auto& L : labels_) {
        QString t = bilingual_ ? QStringLiteral("%1 %2").arg(L.ko, L.en) : L.ko;
        if (L.kind >= 100 && L.kind < 200) static_cast<QTabBar*>(L.obj)->setTabText(L.kind - 100, t);
        else if (L.kind == 1) static_cast<QLabel*>(L.obj)->setText(t);
    }
}

// ---------------------------------------------------------------- 리본(디자인 v5 §3: 탭 없음 · 묶음 7 · 타일 칩 · 오른쪽 끝 좌표 입력 · 단면 찾기)
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
    for (auto* sp : {front_, back_}) { sp->setRange(0, kMaxBandDepth); sp->setDecimals(2); sp->setSingleStep(0.05); sp->setSuffix(" m"); sp->setFixedWidth(56); sp->setFixedHeight(24); sp->setAlignment(Qt::AlignRight); }
    front_->setToolTip(QStringLiteral("단면선 앞쪽(보는 사람 쪽) 두께, 0–5 m"));
    back_->setToolTip(QStringLiteral("단면선 뒤쪽(보는 방향) 깊이 — 입면 영상(배경)에 보이는 깊이, 0–5 m, 기본 3 m(입면도용). 숫자키 1–5 = 0.5 / 1 / 2 / 3 / 5 m"));
    auto* chips = new QWidget; { auto* h = new QHBoxLayout(chips); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(0);
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
    ribbon_->addWidget(QStringLiteral("elev"), depthBox_);
    // 레벨선 간격 글(옛 가짜 입력칸)은 리본에서 뺐다 — 갱신 코드가 쓰므로 위젯만 숨겨 둔다(범례는 정보 줄)
    lvLine_ = lab(QStringLiteral("0.10 m"), "mono"); lvLabel_ = lab(QStringLiteral("0.50 m"), "mono");
    for (auto* l : {lvLine_, lvLabel_}) { l->setParent(ribbon_); l->hide(); }
    // ---- 보기: 목록 · 평면 · 단면 · 고대비(보기 켜짐 = 귀리색 타일)
    ribbon_->addGroup(QStringLiteral("view"), QStringLiteral("보기"));
    add("view", "listpanel", K::Shown, "list", QStringLiteral("목록"));
    add("view", "view1", K::Shown, "map", QStringLiteral("평면"));
    add("view", "view2", K::Shown, "profile", QStringLiteral("단면"));
    add("view", "contrast", K::Shown, "contrast", QStringLiteral("고대비"));
    // ---- 자료: 숫자 · 좌표 자료만(그림은 「도면」)
    ribbon_->addGroup(QStringLiteral("data"), QStringLiteral("자료"));
    add("data", "csv", K::Normal, "csv", QStringLiteral("CSV"));
    add("data", "plan", K::Normal, "geotiff", QStringLiteral("GeoTIFF"));
    add("data", "xyz", K::Normal, "points-xyz", QStringLiteral("XYZ"));
    add("data", "las", K::Normal, "cloud-las", QStringLiteral("LAS"));
    { auto* mn = new QMenu(this); mn->addAction(action("secjson")); mn->addAction(action("secimport")); action("seclist")->setMenu(mn); }
    add("data", "seclist", K::Normal, "sections-list", QStringLiteral("목록 파일"));
    // ---- 내보내기: 도면(흙색 주 단추 — 단면 탭에서 한 화면의 유일한 흙색)
    ribbon_->addGroup(QStringLiteral("out"), QStringLiteral("내보내기"));
    sheetChip_ = add("out", "sheet", K::Primary, "sheet", QStringLiteral("도면"));
    // ---- 기타: 단축키 · 더보기 ▾(정보 · 영상 불투명도 · 단면선 굵기 · 매끈하게 · 영어 병기 · 끝내기)
    ribbon_->addGroup(QStringLiteral("etc"), QStringLiteral("기타"));
    add("etc", "keys", K::Normal, "keyboard", QStringLiteral("단축키"));
    { auto* mn = new QMenu(this);
        opacity_ = new QSlider(Qt::Horizontal); opacity_->setRange(10, 100); opacity_->setValue(100); opacity_->setFixedWidth(110);
        auto* opw = rowOf({lab(QStringLiteral("영상 불투명도"), "hint"), opacity_}); opw->setContentsMargins(12, 4, 12, 4);
        auto* wa = new QWidgetAction(mn); wa->setDefaultWidget(opw); mn->addAction(wa);
        auto* lw = new QWidget; { auto* h = new QHBoxLayout(lw); h->setContentsMargins(0, 0, 0, 0); h->setSpacing(0);
            const double ws[3] = {1.5, 2.0, 3.0};
            double cur = QSettings().value("view/sectionLineWidth", 2.0).toDouble();
            std::vector<QToolButton*> bs;
            for (int i = 0; i < 3; ++i) {
                auto* c = new QToolButton; c->setObjectName("chip"); c->setCheckable(true); c->setFocusPolicy(Qt::NoFocus);
                c->setText(QStringLiteral("%1 px").arg(ws[i], 0, 'g', 2)); c->setChecked(std::fabs(cur - ws[i]) < 0.01);
                if (i == 0) c->setProperty("pos", "first");
                if (i == 2) c->setProperty("pos", "last");
                bs.push_back(c); h->addWidget(c);
            }
            for (int i = 0; i < 3; ++i) {
                double wv = ws[i];
                QObject::connect(bs[size_t(i)], &QToolButton::clicked, this, [this, bs, i, wv] {
                    for (int j = 0; j < 3; ++j) bs[size_t(j)]->setChecked(j == i);
                    QSettings().setValue("view/sectionLineWidth", wv); section_->setStyle(secStyle());
                });
            } }
        auto* lwBox = rowOf({lab(QStringLiteral("단면선 굵기"), "hint"), lw}); lwBox->setContentsMargins(12, 4, 12, 4);
        auto* wa2 = new QWidgetAction(mn); wa2->setDefaultWidget(lwBox); mn->addAction(wa2);
        mn->addSeparator();
        mn->addAction(action("smooth")); mn->addAction(action("lang"));
        mn->addSeparator();
        mn->addAction(action("about")); mn->addAction(action("quit"));
        action("more")->setMenu(mn); }
    add("etc", "more", K::Normal, "ellipsis", QStringLiteral("더보기"));
    // ---- 오른쪽 끝(Strata 「지역」 + 「주소·지번 찾기」 자리): 좌표 입력 · 단면 찾기
    auto* corner = new QWidget; { auto* ch = new QHBoxLayout(corner); ch->setContentsMargins(0, 0, 0, 0); ch->setSpacing(6);
        coordBtn_ = new QToolButton; auto* coordBtn = coordBtn_; coordBtn->setObjectName("ribbonCorner");
        coordBtn->setIcon(kerf::icon(QStringLiteral("crosshair"), 16, theme::Hand)); coordBtn->setIconSize(QSize(16, 16));
        coordBtn->setText(QStringLiteral("좌표 입력")); coordBtn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); coordBtn->setFocusPolicy(Qt::NoFocus);
        coordBtn->setToolTip(QStringLiteral("A · A′ 좌표를 쳐서 새 단면 (Enter)"));
        coordBtn->setFixedSize(100, 40);   // Strata 「지역」 자리(77×40)와 같은 높이
        QObject::connect(coordBtn, &QToolButton::clicked, this, [this] { if (!src_) return; if (!plan_->drawMode()) plan_->setDrawMode(true); dlgCoordEntry(); });
        ch->addWidget(coordBtn);
        findBox_ = new QLineEdit; findBox_->setObjectName("ribbonSearch"); findBox_->setPlaceholderText(QStringLiteral("단면 찾기  Ctrl+F")); findBox_->setClearButtonEnabled(true);
        findBox_->setToolTip(QStringLiteral("단면 목록을 이름으로 거릅니다 (Ctrl+F)"));
        findBox_->addAction(kerf::icon(QStringLiteral("search"), 14, theme::Faint), QLineEdit::LeadingPosition);   // Strata 찾기 칸의 돋보기
        findBox_->setFixedSize(150, 32);   // Strata 「주소·지번 찾기」 158×32 과 같은 줄
        QObject::connect(findBox_, &QLineEdit::textChanged, this, [this](const QString& t) { filterSectionList(t); });
        ch->addWidget(findBox_); }
    ribbon_->setCorner(corner);
    { auto* sc = new QShortcut(QKeySequence::Find, this); QObject::connect(sc, &QShortcut::activated, this, [this] { findBox_->setFocus(); findBox_->selectAll(); }); }
    return ribbon_;
}

// 리본 문맥(디자인 v5 §3.1): 홈 = 모델 묶음만 살아 있고 나머지 흐림, 단면 = 전부, 도면 = 단면 · 입면 꺼짐 + 「도면」 칩은 보기 켜짐 모양
void MainWindow::setRibbonContext(int ctx) {
    if (!ribbon_) return;
    ribbonCtx_ = ctx;
    const bool home = ctx == 0, sheet = ctx == 2;
    for (const char* g : {"sec", "elev", "view", "data", "out"}) ribbon_->setGroupDim(QString::fromLatin1(g), home);
    if (sheetChip_) ribbon_->setChipOn(sheetChip_, sheet);
    if (docTabCorner_) docTabCorner_->setText(src_ ? QStringLiteral("%1 · 저장됨").arg(QFileInfo(path_).completeBaseName()) : QString());
    updateEnabled();   // 켜고 끄기는 updateEnabled 한 곳(검토 I1)
}

void MainWindow::showHomeTab() { showStart(true); }

void MainWindow::closeSheetTab() {
    discardSheetTab();
    showWorkTab();
}

// 「도면」 탭 · 조판 위젯 · 저장 함수를 버린다(검토 I4): 모델을 닫거나 다른 모델을 열어도 옛 모델의 도면이 남아 다른 이름으로 저장되지 않게
void MainWindow::discardSheetTab() {
    if (viewTabs_ && viewTabs_->count() > 2) { QSignalBlocker b(viewTabs_); viewTabs_->removeTab(2); }
    if (sheetHost_) { if (body_) body_->removeWidget(sheetHost_); sheetHost_->deleteLater(); sheetHost_ = nullptr; }
    embeddedSave_ = nullptr;
}

// 리본 오른쪽 「단면 찾기」: 이름에 글자가 든 줄만 보인다(빈 글이면 전부)
void MainWindow::filterSectionList(const QString& text) {
    if (!secList_) return;
    const QString t = text.trimmed();
    for (int i = 0; i < secList_->count() && i < int(sections_.size()); ++i)
        secList_->item(i)->setHidden(!t.isEmpty() && !QString::fromStdString(sections_[size_t(i)].name).contains(t, Qt::CaseInsensitive));
}

// ---------------------------------------------------------------- 지금 도구 줄(그리는 중에만)
// 그리는 동안 떠 있는 안내(v5 C4 · 스펙 §5): 「단면선 긋기 › A 찾는 중 | 평면에서 시작점 A를 클릭하세요 | Esc」
void MainWindow::updateCtx(int stage, const SectionLine& l) {
    if (!guide_) return;
    if (stage == 0) {
        guide_->setTool(QStringLiteral("section-line"), QStringLiteral("단면선 긋기 › A 찾는 중"));
        guide_->setHint(QStringLiteral("평면에서 시작점 A를 클릭하세요"));
        guide_->setKeys({QStringLiteral("Esc")});
        if (inspName_) {   // 오른쪽 판도 「그리는 중」(화판 2)
            inspName_->setText(QStringLiteral("새 단면 · 그리는 중"));
            if (inspNote_) inspNote_->setText(QStringLiteral("평면에서 시작점 A를 찍으세요. 길이 · 방위 · 커서는 A′를 찾는 동안 여기 보입니다"));
            if (inspLen_) inspLen_->setText(QStringLiteral("—"));
            if (inspDepth_) inspDepth_->setText(QStringLiteral("—"));
            if (inspBody_) inspBody_->clear();
        }
        return;
    }
    const double L = SectionFrame(l).L;
    guide_->setTool(QStringLiteral("section-line"), QStringLiteral("단면선 긋기 › A′ 찾는 중"));
    guide_->setHint(QStringLiteral("끝점 A′를 클릭하세요 · Shift = 축 맞춤 · 숫자 = 길이"));   // 스펙 §5 그대로 — 길이는 평면 A′ 이름표 「A′ · 7.43 m」가 든다
    guide_->setKeys({QStringLiteral("Enter"), QStringLiteral("Esc")});
    if (inspLen_ && inspBody_ && L > 1e-6) {
        const Vec3 org = src_ ? src_->srs.origin : Vec3();
        inspLen_->setText(QStringLiteral("%1 m").arg(L, 0, 'f', 2));
        inspBody_->setText(QStringLiteral("방위  %1\n보는 쪽  %2\n\nA   %3\n    %4\n커서  %5\n    %6")
                               .arg(qs8(formatAzimuth(sectionAzimuthDeg(l)))).arg(qs8(facingKo(sectionAzimuthDeg(l))))
                               .arg(l.a.x + org.x, 0, 'f', 3).arg(l.a.y + org.y, 0, 'f', 3).arg(l.b.x + org.x, 0, 'f', 3).arg(l.b.y + org.y, 0, 'f', 3));
    }
}

// ---------------------------------------------------------------- 알림 띠(노란 띠 대신)
QFrame* MainWindow::buildNotice() {
    auto* f = new QFrame; f->setObjectName("notice"); f->setProperty("level", "caution"); f->setAttribute(Qt::WA_StyledBackground);
    auto* h = new QHBoxLayout(f); h->setContentsMargins(12, 5, 8, 5); h->setSpacing(8);
    auto* ic = new QLabel(QString::fromUtf8("▲")); ic->setObjectName("noticeIcon");
    h->addWidget(ic);
    noticeText_ = new QLabel; noticeText_->setTextFormat(Qt::RichText); noticeText_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    h->addWidget(noticeText_, 1);
    auto* set = new QPushButton(QStringLiteral("높이 기준 지정…")); set->setStyleSheet("QPushButton{padding:3px 12px;}");
    auto* more = new QPushButton(QStringLiteral("자세히")); more->setObjectName("quiet"); more->setStyleSheet("QPushButton{padding:3px 10px;}");
    auto* x = new QToolButton; x->setIcon(theme::icon(theme::Ico::Close, 16)); x->setAutoRaise(true); x->setToolTip(QStringLiteral("이 알림 닫기(좌표계 표시는 상태줄 오른쪽에 계속)"));
    for (QWidget* b : {static_cast<QWidget*>(set), static_cast<QWidget*>(more), static_cast<QWidget*>(x)}) { b->setFocusPolicy(Qt::NoFocus); h->addWidget(b); }
    QObject::connect(set, &QPushButton::clicked, this, [this] { dlgHeightDatum(); });
    QObject::connect(more, &QPushButton::clicked, this, [this] { dlgSrsDetails(); });
    QObject::connect(x, &QToolButton::clicked, f, [f] { f->setVisible(false); });
    f->setVisible(false);
    return f;
}

// ---------------------------------------------------------------- 보기 머리
QWidget* MainWindow::buildPlanFrame() {
    auto* fr = new QWidget; fr->setObjectName("viewFrame"); fr->setAttribute(Qt::WA_StyledBackground);
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
    auto* fr = new QWidget; fr->setObjectName("viewFrame"); fr->setAttribute(Qt::WA_StyledBackground);
    auto* v = new QVBoxLayout(fr); v->setContentsMargins(0, 0, 0, 0); v->setSpacing(0);
    secTitleBar_ = new QWidget; secTitleBar_->setObjectName("viewTitle"); secTitleBar_->setAttribute(Qt::WA_StyledBackground); secTitleBar_->setFixedHeight(32); secTitleBar_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    auto* h = new QHBoxLayout(secTitleBar_); h->setContentsMargins(10, 0, 6, 0); h->setSpacing(2);
    secTitle_ = lab(QStringLiteral("단면"), "title"); h->addWidget(secTitle_);
    h->addSpacing(8);
    secFacing_ = lab(QString(), "facing"); h->addWidget(secFacing_);
    h->addStretch();
    // v4: 그릴 것 켜기·끄기 넷은 영향을 받는 이 머리에만(리본에서 뺐다). 같은 동작이라 키(I · L · V)와 상태를 같이 씀
    {
        auto* box = new QWidget; box->setObjectName("viewToggles"); box->setAttribute(Qt::WA_StyledBackground);
        auto* bl = new QHBoxLayout(box); bl->setContentsMargins(2, 2, 2, 2); bl->setSpacing(2);
        const std::pair<const char*, theme::Ico> tg[] = {{"image", theme::Ico::Image}, {"line", theme::Ico::Line}, {"levels", theme::Ico::Levels}, {"fade", theme::Ico::Band}};
        for (const auto& [key, ico] : tg) {
            auto* b = new QToolButton; b->setObjectName("viewToggle"); b->setIcon(theme::icon(ico, 16)); b->setIconSize(QSize(16, 16));
            b->setFocusPolicy(Qt::NoFocus); b->setFixedSize(28, 24);
            bindButton(b, action(key));
            b->setAccessibleName(action(key)->text());
            bl->addWidget(b);
        }
        h->addWidget(box); h->addSpacing(6);
    }
    // 세로:가로 — 누르면 화면 세로 과장(×1·×2·×5·×10, X 키로 돌아가며). 기복이 작으면 추천을 함께 보여 줌
    vexBtn_ = new QToolButton; vexBtn_->setObjectName("vexBtn"); vexBtn_->setFocusPolicy(Qt::NoFocus);
    vexBtn_->setAutoRaise(true); vexBtn_->setPopupMode(QToolButton::InstantPopup);
    vexBtn_->setIcon(theme::icon(theme::Ico::Vex, 16)); vexBtn_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // v4 D11
    {
        auto* m = new QMenu(vexBtn_);
        for (int k : {1, 2, 5, 10}) {
            QAction* a = m->addAction(k == 1 ? QStringLiteral("세로:가로 1:1 (실제 비율)") : QStringLiteral("세로 ×%1 과장 (화면만)").arg(k));
            QObject::connect(a, &QAction::triggered, this, [this, k] { setVex(k); });
        }
        vexBtn_->setMenu(m);
    }
    h->addWidget(vexBtn_); h->addSpacing(4);
    heightBadge_ = new QToolButton; heightBadge_->setObjectName("heightBadge"); heightBadge_->setFocusPolicy(Qt::NoFocus);
    QObject::connect(heightBadge_, &QToolButton::clicked, this, [this] { dlgHeightDatum(); });
    heightBadge_->setParent(secTitleBar_); heightBadge_->hide();   // v4: 높이 배지는 상태줄 하나만(감사 2 · 17). 갱신 코드는 그대로 둠
    h->addSpacing(6);
    auto mk = [&](theme::Ico ic, const QString& tip, std::function<void()> fn) {
        auto* a = new QAction(theme::icon(ic, 16), tip, this); a->setProperty("iconName", QString::fromLatin1(theme::iconName(ic)));
        QObject::connect(a, &QAction::triggered, this, fn);
        h->addWidget(iconBtn(a));
    };
    mk(theme::Ico::Fit, QStringLiteral("맞춤"), [this] { section_->fit(); section_->update(); });
    mk(theme::Ico::ZoomIn, QStringLiteral("확대"), [this] { section_->zoomBy(1.4); });
    mk(theme::Ico::ZoomOut, QStringLiteral("축소"), [this] { section_->zoomBy(1 / 1.4); });
    mk(theme::Ico::Max, QStringLiteral("이 보기만 크게 / 나란히"), [this] { bool solo = action("view1")->isChecked(); action("view2")->setChecked(true); action("view1")->setChecked(!solo); });
    v->addWidget(secTitleBar_);
    auto* strip = new QWidget; strip->setObjectName("infoStrip"); strip->setAttribute(Qt::WA_StyledBackground); strip->setFixedHeight(26); strip->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
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

QString MainWindow::heightBadgeText(QString* state, QString* tip) const {
    if (!src_) { if (state) *state = "none"; if (tip) *tip = QStringLiteral("모델을 열면 높이 기준이 여기 보입니다"); return QStringLiteral("높이 —"); }
    const SrsDesc& d = srsReport_.desc;
    QString t, st = "ok";
    const VDatumInfo& vi = vdatumInfo(d.vdatum);
    const bool named = d.vdatum == VDatum::EGM96 || d.vdatum == VDatum::EGM2008 || d.vdatum == VDatum::KVD1964 || d.vdatum == VDatum::KNGeoid;
    QString name = named && vi.shortName && *vi.shortName ? QString::fromUtf8(vi.shortName) : qs8(d.verticalKo());
    const int ve = d.verticalEpsg ? d.verticalEpsg : vi.epsg;
    if (!d.known()) { st = "error"; t = QStringLiteral("● 좌표계 없음"); }
    else if (!d.heightDeclared && (d.vertKind == VertKind::Unspecified || (d.vertKind == VertKind::Ellipsoidal && d.promotedTo3D))) {
        st = "warn"; t = d.vertKind == VertKind::Unspecified ? QStringLiteral("▲ 높이 기준 모름") : QStringLiteral("▲ 높이 타원체고 표기 · 확인");
    } else t = QStringLiteral("높이 %1%2").arg(name, d.heightDeclared ? QStringLiteral(" · 지정함") : QString());   // 스펙 §6 「높이 KVD1964 ▾」 — EPSG 는 툴팁에
    if (state) *state = st;
    if (tip) *tip = QStringLiteral("%1\n%2%3누르면 「높이 기준 지정」(이름표만 바뀌고 Z 값은 그대로)").arg(qs8(srsReport_.labelKo), ve ? QStringLiteral("높이 EPSG:%1\n").arg(ve) : QString(), heightNote_.isEmpty() ? QString() : QStringLiteral("(%1)\n").arg(heightNote_));
    return t;
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
    vexBtn_->setText(plain);
    vexBtn_->setToolTip(tip);
    vexBtn_->setProperty("state", state);
    vexBtn_->style()->unpolish(vexBtn_); vexBtn_->style()->polish(vexBtn_);
}

QString MainWindow::windowTitleCheck() const {
    const QString t = windowTitle(), dn = QGuiApplication::applicationDisplayName();
    // Qt 는 제목이 표시 이름으로 끝나지 않으면 「 - 표시 이름」을 붙인다(Windows·X11) → 이름이 두 번 보이던 원인
    const bool dup = !dn.isEmpty() && !t.endsWith(dn);
    return QStringLiteral("title=\"%1\" displayName=\"%2\" appended=%3").arg(t, dn).arg(dup ? 1 : 0);
}

void MainWindow::updateHeader() {
    if (!secTitle_) return;
    QString st, tip;
    QString bt = heightBadgeText(&st, &tip);
    for (auto* b : {heightBadge_, heightBadge2_}) {
        if (!b) continue;
        b->setText(bt + QStringLiteral(" ▾")); b->setToolTip(tip); b->setProperty("state", st);   // 둘 다 「… ▾」(스펙 §6)
        b->style()->unpolish(b); b->style()->polish(b);
    }
    updateVexUi();
    const bool has = plan_ && plan_->hasLine();
    const QString monoB = QStringLiteral("<b style='font-family:Consolas,\"DejaVu Sans Mono\",monospace'>%1</b>");
    secTitle_->setText(has ? QStringLiteral("%1 단면").arg(sectionName()) : QStringLiteral("단면"));
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
        stripState_->setText(last_.previewLod || !lastFinal_ ? (last_.cutFromLeaf ? QStringLiteral("<span style='color:#7A5A00'>단면선 정확 · 배경 미리보기 → 최종 계산 중…</span>") : QStringLiteral("<span style='color:#7A5A00'>미리보기(거친) → 최종 계산 중…</span>"))
                                                             : QStringLiteral("<span style='color:#3F6B31'>✓</span> 최종(잎)"));
        LevelPlan lp = sectionLevelPlan(section_->xf().ppmZ(), 1.0, false);
        lvLine_->setText(QStringLiteral("%1 m").arg(lp.lineCm / 100.0, 0, 'f', 2));
        lvLabel_->setText(QStringLiteral("%1 m").arg(lp.labelCm / 100.0, 0, 'f', 2));
        if (scaleCombo_) { QSignalBlocker b(scaleCombo_); scaleCombo_->setEditText(QStringLiteral("1:") + ds); }
    } else {
        stripScale_->setText(QStringLiteral("화면 축척 —")); stripState_->clear();
        if (scaleCombo_) { QSignalBlocker b(scaleCombo_); scaleCombo_->setEditText(QStringLiteral("—")); }
    }
    if (secCount_) secCount_->setText(QString::number(sections_.size()));
    refreshSteps();
    if (stripBand_) stripBand_->setText(has ? QStringLiteral("입면 뒤 0–%1 m").arg(back_->value(), 0, 'f', 2) : QString());
    if (stripCut_) {   // v4 단계 5 · 화판 2: 「잘린 선 n줄 · m점 · 빈 구간 없음 / k곳 x m」 — 빈 구간은 core profileGaps
        if (has && section_->hasResult()) {
            const auto& r = section_->result();
            size_t nv = 0; for (auto& pl : r.profile) nv += pl.size();
            const std::vector<SGap> gaps = profileGaps(r.profile, SectionFrame(plan_->line()).L);
            double gl = 0; for (const SGap& g : gaps) gl += g.s1 - g.s0;
            stripCut_->setText(gaps.empty() ? QStringLiteral("잘린 선 %1줄 · %2점 · 빈 구간 없음").arg(r.profile.size()).arg(nv)
                                            : QStringLiteral("잘린 선 %1줄 · %2점 · 빈 구간 %3곳 %4 m").arg(r.profile.size()).arg(nv).arg(gaps.size()).arg(gl, 0, 'f', 2));
        } else stripCut_->setText(QStringLiteral("잘린 선 —"));
    }
    if (!inspName_) return;
    inspName_->setText(has ? sectionName() : QStringLiteral("고른 단면이 없습니다"));
    QString note;
    if (current_ >= 0 && current_ < int(sections_.size())) note = qs8(sections_[size_t(current_)].note);
    if (inspNote_) inspNote_->setText(has ? (note.isEmpty() ? secFacing_->text() : note + QStringLiteral(" · ") + secFacing_->text()) : QStringLiteral("단면선을 그으면 여기 수치가 모입니다"));
    if (!has) {
        inspLen_->setText(QStringLiteral("—"));
        inspDepth_->setText(QStringLiteral("—"));
        inspBody_->setText(QString());
        return;
    }
    const SectionLine& ln = plan_->line();
    const SectionFrame fr(ln);
    const Vec3 org = src_ ? src_->srs.origin : Vec3();
    inspLen_->setText(QStringLiteral("%1 m").arg(fr.L, 0, 'f', 2));
    QString body;
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
        inspDepth_->setText(QStringLiteral("%1 m").arg(std::max(0.0, top - zLo), 0, 'f', 2));
        size_t nv = 0; for (auto& pl : r.profile) nv += pl.size();
        body = QStringLiteral("방위  %1\n보는 쪽  %2\n앞 / 뒤  %3 / %4 m\n지표 EL.  %5 m\n바닥 EL.  %6 m\n잘린 선  %7줄 · %8점")
                   .arg(qs8(formatAzimuth(sectionAzimuthDeg(ln))))
                   .arg(qs8(facingKo(sectionAzimuthDeg(ln))))
                   .arg(ln.front, 0, 'f', 2).arg(ln.back, 0, 'f', 2)
                   .arg(QStringLiteral("%1 – %2").arg(std::min(zA, zB) + org.z, 0, 'f', 2).arg(std::max(zA, zB) + org.z, 0, 'f', 2)).arg(zLo + org.z, 0, 'f', 2)
                   .arg(r.profile.size()).arg(nv);
    } else {
        inspDepth_->setText(QStringLiteral("—"));
        body = QStringLiteral("방위  %1\n보는 쪽  %2\n앞 / 뒤  %3 / %4 m")
                   .arg(qs8(formatAzimuth(sectionAzimuthDeg(ln))))
                   .arg(qs8(facingKo(sectionAzimuthDeg(ln))))
                   .arg(ln.front, 0, 'f', 2).arg(ln.back, 0, 'f', 2);
    }
    body += QStringLiteral("\n\nA   %1\n    %2\nA′  %3\n    %4")
                .arg(ln.a.x + org.x, 0, 'f', 3).arg(ln.a.y + org.y, 0, 'f', 3)
                .arg(ln.b.x + org.x, 0, 'f', 3).arg(ln.b.y + org.y, 0, 'f', 3);
    inspBody_->setText(body);
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
    zSrc_ = new QLabel(QStringLiteral("—")); zSrc_->setObjectName("zSrc"); zSrc_->setMinimumWidth(48);
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
    // 배지 둘: 「수평 EPSG:5186 ▾」(누르면 좌표계 상세) · 「높이 KVD1964 ▾」(누르면 높이 기준 지정). 흙색 테, 확인 전이면 노란 테 ▲
    cellStart();
    srsLabel_ = new QToolButton; srsLabel_->setObjectName("statusBadge"); srsLabel_->setFocusPolicy(Qt::NoFocus);
    srsLabel_->setText(QStringLiteral("수평 — ▾")); srsLabel_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon); srsLabel_->setProperty("state", "none");
    srsLabel_->setIcon(kerf::icon(QStringLiteral("map"), 14, theme::ClayText)); srsLabel_->setIconSize(QSize(14, 14));
    QObject::connect(srsLabel_, &QToolButton::clicked, this, [this] { dlgSrsDetails(); });
    h->addWidget(srsLabel_);
    h->addSpacing(8);
    heightBadge2_ = new QToolButton; heightBadge2_->setObjectName("statusBadge"); heightBadge2_->setFocusPolicy(Qt::NoFocus);
    heightBadge2_->setIcon(theme::icon(theme::Ico::Height, 14)); heightBadge2_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);   // v4 D11
    QObject::connect(heightBadge2_, &QToolButton::clicked, this, [this] { dlgHeightDatum(); });
    h->addWidget(heightBadge2_);
    return w;
}

// 좌표계 상세(상태줄 「수평」 배지 · 알림 띠 「자세히」)
void MainWindow::dlgSrsDetails() {
    QString t = QStringLiteral("<b>%1</b><br><br>").arg(qs8(srsReport_.labelKo).toHtmlEscaped());
    for (auto& w : srsReport_.warnings) t += QStringLiteral("▲ ") + qs8(w).toHtmlEscaped() + "<br>";
    t += "<br><small>" + qs8(srsReport_.tooltipKo).toHtmlEscaped().replace("\n", "<br>") + "</small>";
    QMessageBox mb(this); mb.setWindowTitle(QStringLiteral("좌표계 확인")); mb.setTextFormat(Qt::RichText); mb.setText(t); mb.exec();
}

// ---------------------------------------------------------------- 단면 목록(P1-1)
QWidget* MainWindow::buildSidePanel() {
    auto* w = new QWidget; w->setObjectName("sidePanel"); w->setAttribute(Qt::WA_StyledBackground);
    w->setFixedWidth(348);   // 스펙 §2.1: 판 폭은 고정(348), 가운데 화면만 늘어난다 — 창을 줄였다 늘려도 판이 줄지 않게
    auto* v = new QVBoxLayout(w); v->setContentsMargins(10, 8, 10, 8); v->setSpacing(6);
    auto* head = new QHBoxLayout; head->setSpacing(6);
    head->addWidget(lab(QStringLiteral("단면 목록"), "sectionHead"));
    secCount_ = lab(QStringLiteral("0"), "monoFaint"); head->addWidget(secCount_);
    head->addStretch();   // v4: 「＋ 새 단면」은 리본 · N 키와 같아 목록 머리에서 뺐다
    v->addLayout(head);
    secList_ = new QListWidget; secList_->setObjectName("sectionList"); secList_->setFocusPolicy(Qt::NoFocus);
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
    auto* foot = lab(QStringLiteral("누르면 그 단면으로 · 두 번 누르면 이름 바꾸기\n목록은 이 PC 설정에 모델별로 저장(파일 → 단면 목록 내보내기)"), "faint");
    foot->setWordWrap(true);
    v->addWidget(foot);
    return w;
}

QWidget* MainWindow::buildInspector() {
    auto* w = new QWidget; w->setObjectName("sidePanel"); w->setAttribute(Qt::WA_StyledBackground);
    w->setFixedWidth(272);   // 스펙 §2.1: 오른쪽 판 272 고정
    auto* v = new QVBoxLayout(w); v->setContentsMargins(14, 10, 14, 12); v->setSpacing(8);
    v->addWidget(lab(QStringLiteral("선택한 단면"), "sectionHead"));
    inspName_ = lab(QStringLiteral("고른 단면이 없습니다"), "bigTitle");
    inspNote_ = lab(QString(), "hint"); inspNote_->setWordWrap(true);
    v->addWidget(inspName_);
    v->addWidget(inspNote_);
    auto* figs = new QHBoxLayout;
    auto fig = [&](const QString& cap, QLabel*& out) {
        auto* box = new QVBoxLayout;
        box->addWidget(lab(cap, "faint"));
        out = lab(QStringLiteral("—"), "mono");
        out->setStyleSheet(QStringLiteral("font-size:20px;"));
        box->addWidget(out);
        figs->addLayout(box);
    };
    fig(QStringLiteral("길이"), inspLen_);
    fig(QStringLiteral("깊이 (지표 → 바닥)"), inspDepth_);
    v->addLayout(figs);
    inspBody_ = new QLabel; inspBody_->setObjectName("mono"); inspBody_->setWordWrap(true); inspBody_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    v->addWidget(inspBody_);
    v->addStretch();
    // v4: 「이 단면으로 도면」은 리본 「도면」과 같아 목록 오른쪽 클릭으로만. 판 아래는 좌표 입력 · 복사(읽기 전용 판)
    auto* row = new QHBoxLayout; row->setSpacing(6);
    auto* coordBtn = new QPushButton(kerf::icon(QStringLiteral("crosshair"), 16, theme::Hand), QStringLiteral("좌표 입력…  Enter")); coordBtn->setProperty("iconName", QStringLiteral("crosshair"));
    QObject::connect(coordBtn, &QPushButton::clicked, this, [this] { if (src_) dlgCoordEntry(); });
    auto* copyBtn = new QPushButton(kerf::icon(QStringLiteral("copy"), 16, theme::Hand), QStringLiteral("복사")); copyBtn->setProperty("iconName", QStringLiteral("copy"));
    copyBtn->setToolTip(QStringLiteral("끝점 좌표 · 길이 · 방위를 글로 복사"));
    QObject::connect(copyBtn, &QPushButton::clicked, this, [this] {
        QGuiApplication::clipboard()->setText(inspBody_->text());
        showStatus(QStringLiteral("단면 수치를 복사했습니다"));
    });
    row->addWidget(coordBtn, 1); row->addWidget(copyBtn);
    v->addLayout(row);
    { auto* note = lab(QStringLiteral("이 판은 읽기만 합니다. 값을 바꾸는 곳은 리본과 평면입니다."), "faint"); note->setWordWrap(true); v->addWidget(note); }
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
    auto* th = new QLabel; th->setFixedSize(64, 30); th->setStyleSheet("QLabel{background:#FFFFFF;border:1px solid #DEDCD1;border-radius:3px;}");
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
    if (findBox_ && !findBox_->text().trimmed().isEmpty()) filterSectionList(findBox_->text());   // 거르기는 목록을 다시 만들어도 남는다(검토 I5)
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
    s.image = action("image")->isChecked(); s.profile = action("line")->isChecked(); s.levels = action("levels")->isChecked();
    s.fade = action("fade")->isChecked(); s.smooth = action("smooth")->isChecked();
    s.heightDecl = src_ ? int(src_->srs.heightDeclared) : 0;
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
    const std::pair<const char*, bool> toggles[] = {{"image", s.image}, {"line", s.profile}, {"levels", s.levels}, {"fade", s.fade}, {"smooth", s.smooth}};
    for (auto& [k, v] : toggles) action(k)->setChecked(v);   // applyingState_ 가 되돌리기 기록을 막음(단추 동기화는 신호로)
    section_->setStyle(secStyle());
    if (src_ && int(src_->srs.heightDeclared) != s.heightDecl) setHeightDeclaration(VDatum(s.heightDecl), true);
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
    QString hs, tip; heightBadgeText(&hs, &tip);
    QString ht;   // 홈 카드 · 최근 표의 「높이」 칸은 기준 이름만(배지 글 통째는 잘림 — 검토 UI 3)
    {
        const SrsDesc& d = srsReport_.desc; const VDatumInfo& vi = vdatumInfo(d.vdatum);
        const bool named = d.vdatum == VDatum::EGM96 || d.vdatum == VDatum::EGM2008 || d.vdatum == VDatum::KVD1964 || d.vdatum == VDatum::KNGeoid;
        ht = !d.known() ? QStringLiteral("좌표계 없음") : hs == "warn" ? QStringLiteral("확인 전") : (named && vi.shortName && *vi.shortName ? QString::fromUtf8(vi.shortName) : qs8(d.verticalKo()));
    }
    st.setValue(k + "height", ht); st.setValue(k + "heightState", hs);
    st.setValue(k + "lastOpened", QDateTime::currentDateTime().toString(Qt::ISODate));
}

void MainWindow::restoreModelState() {
    sections_.clear(); thumbs_.clear(); current_ = -1;
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
    if (undo_) undo_->clear();
    lastState_ = captureState();
    updateDepthChips(); refreshSectionList(); updateHeader();
}

// ---------------------------------------------------------------- 시작 화면
QWidget* MainWindow::buildStartPage() {
    auto* w = new QWidget; w->setObjectName("startPage"); w->setAttribute(Qt::WA_StyledBackground);
    new QVBoxLayout(w);
    return w;
}

// 홈(디자인 v5 C3 · 스펙 §7 · Strata 홈과 같은 뼈대): 이름 · 설명 · 단추 둘 | 이어서 작업 카드 | 최근 모델 삼선표 + 찾기 | 작업 순서 세 걸음 | 처음 쓰는 키
void MainWindow::rebuildStartPage() {
    if (!startPage_) return;
    auto* lay = static_cast<QVBoxLayout*>(startPage_->layout());
    while (auto* it = lay->takeAt(0)) { if (it->widget()) it->widget()->deleteLater(); delete it; }
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
    g->setColumnMinimumWidth(0, 420); g->setColumnStretch(1, 1); g->setColumnMinimumWidth(2, 320);
    g->setRowStretch(1, 1);

    // ---- 왼쪽 위: 앱 아이콘 56 · 「Kerf | 발굴 평·단면」 · 설명 · 「모델 열기 Ctrl+O」 「최근 ▾」 · 끌어 놓기 안내
    auto* hero = new QWidget; { auto* v = new QVBoxLayout(hero); v->setContentsMargins(0, 40, 0, 0); v->setSpacing(12);
        auto* nm = new QHBoxLayout; nm->setSpacing(14);
        auto* ic = new QLabel; { QPixmap pm(56 * 2, 56 * 2); pm.setDevicePixelRatio(2); pm.fill(Qt::transparent);
            QPainter p(&pm); p.setRenderHint(QPainter::Antialiasing); p.setPen(Qt::NoPen); p.setBrush(theme::Action); p.drawRoundedRect(QRectF(0, 0, 56, 56), 12, 12);
            QPainterPath pr; pr.moveTo(10, 22); pr.lineTo(20, 22); pr.lineTo(25, 36); pr.lineTo(31, 36); pr.lineTo(36, 22); pr.lineTo(46, 22);
            p.setPen(QPen(theme::Card, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p.setBrush(Qt::NoBrush); p.drawPath(pr); p.end();
            ic->setPixmap(pm); ic->setFixedSize(56, 56); }
        nm->addWidget(ic); nm->addWidget(lab(QStringLiteral("Kerf"), "heroName"));
        auto* sub = lab(QStringLiteral("발굴 평·단면"), "homeSub"); nm->addWidget(sub); nm->addStretch();
        v->addLayout(nm);
        auto* desc = lab(QStringLiteral("3MX · 3SM · OBJ 실사 메시에서 평면도 · 단면도를 잘라 보고서 도면 밑그림(SVG · DXF)까지 만듭니다."), "homeLead"); desc->setWordWrap(true);
        v->addWidget(desc);
        auto* bt = new QHBoxLayout; bt->setSpacing(8);
        auto* ob = new QPushButton(kerf::icon(QStringLiteral("folder"), 16, theme::Hand), QStringLiteral("모델 열기   Ctrl+O")); ob->setObjectName("homeBtn"); ob->setFixedHeight(40);
        QObject::connect(ob, &QPushButton::clicked, this, [this] { chooseOpen(); });
        auto* rb = new QToolButton; rb->setObjectName("homeBtn"); rb->setText(QStringLiteral("최근 ▾")); rb->setIcon(kerf::icon(QStringLiteral("clock"), 16, theme::Hand)); rb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        rb->setFixedHeight(40); rb->setMenu(recentMenu_); rb->setPopupMode(QToolButton::InstantPopup); rb->setEnabled(!rf.isEmpty()); rb->setFocusPolicy(Qt::NoFocus);
        bt->addWidget(ob); bt->addWidget(rb); bt->addStretch();
        v->addLayout(bt);
        v->addWidget(lab(QStringLiteral("파일을 창 어디에나 끌어다 놓아도 열립니다 · *.3mx(권장) · *.3sm · *.obj"), "lab"));
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
            tv->addWidget(lab(f0.isEmpty() ? QStringLiteral("아직 연 모델이 없습니다") : QFileInfo(f0).completeBaseName(), "heroName"));
            tv->addWidget(lab(f0.isEmpty() ? QStringLiteral("모델을 열면 마지막 단면선 · 뒤 깊이 · 화면이 여기 남습니다")
                                           : (cnt ? QStringLiteral("마지막 단면 %1 · 뒤 %2 m 그대로 열립니다").arg(meta(f0, "names").toString().section(QStringLiteral(" · "), 0, 0), QString::number(meta(f0, "back").toDouble(), 'f', 2))
                                                  : QStringLiteral("단면선을 그으면 여기에 남습니다")), "lab")); }
        bv->addWidget(top);
        bv->addStretch(1);   // 칸 줄은 카드 바닥에(스펙 §7: 칸 높이 48)
        auto* cells = new QWidget; cells->setObjectName("ccCells"); cells->setAttribute(Qt::WA_StyledBackground); cells->setFixedHeight(56);
        { auto* ch = new QHBoxLayout(cells); ch->setContentsMargins(0, 0, 0, 0); ch->setSpacing(0);
            const QString folder = f0.isEmpty() ? QStringLiteral("—") : QFileInfo(f0).absolutePath().split(QLatin1Char('/'), Qt::SkipEmptyParts).join(QStringLiteral(" › "));   // 화판: 역슬래시는 한글 글꼴에서 ₩ 로 보임
            const QString sheets = meta(f0, "sheets").toString();
            const QStringList ks = {QStringLiteral("폴더"), QStringLiteral("마지막 열림"), QStringLiteral("수평 좌표계"), QStringLiteral("높이"), QStringLiteral("단면"), QStringLiteral("도면")};
            const QStringList vs = {folder, f0.isEmpty() ? QStringLiteral("—") : ago(meta(f0, "lastOpened").toString()), dash(meta(f0, "srs").toString()), dash(meta(f0, "height").toString()),
                                    f0.isEmpty() ? QStringLiteral("—") : QStringLiteral("%1개").arg(cnt), sheets.isEmpty() ? QStringLiteral("—") : sheets};
            for (int i = 0; i < 6; ++i) {
                auto* c = new QWidget; c->setObjectName(i ? "ccCell" : "ccCellFirst"); c->setAttribute(Qt::WA_StyledBackground);
                auto* cv = new QVBoxLayout(c); cv->setContentsMargins(12, 6, 12, 6); cv->setSpacing(0);
                cv->addWidget(lab(ks[i], "cap"));
                auto* vl = lab(vs[i]); vl->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
                if (i == 3) { const QString hs = meta(f0, "heightState").toString(); if (hs == "warn") vl->setStyleSheet("color:#7A5A00;"); else if (hs == "error") vl->setStyleSheet("color:#A33B3B;"); }
                cv->addWidget(vl);
                ch->addWidget(c, i == 0 ? 2 : 1);
            } }
        bv->addWidget(cells);
        h->addWidget(body, 1);
        auto* side = new QWidget; side->setObjectName("ccSide"); side->setAttribute(Qt::WA_StyledBackground); side->setFixedWidth(280);
        { auto* sv = new QVBoxLayout(side); sv->setContentsMargins(20, 16, 20, 16); sv->setSpacing(10);
            auto* go = new QPushButton(ok0 ? QStringLiteral("이어서 열기  →") : (f0.isEmpty() ? QStringLiteral("모델 열기…") : QStringLiteral("원본 없음"))); go->setObjectName("primary"); go->setFixedHeight(40);
            go->setEnabled(ok0 || f0.isEmpty());
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
                sv->addWidget(lab(QStringLiteral("썸네일을 누르면 그 단면으로 엽니다"), "lab"));
                auto* tr = new QHBoxLayout; tr->setSpacing(8);
                for (int i = 0; i < thumbs.size(); ++i) {
                    auto* col = new QWidget; auto* cv = new QVBoxLayout(col); cv->setContentsMargins(0, 0, 0, 0); cv->setSpacing(2);
                    auto* pic = new QToolButton; pic->setObjectName("thumb"); pic->setFocusPolicy(Qt::NoFocus); pic->setIcon(QPixmap::fromImage(thumbs[i].first)); pic->setIconSize(QSize(72, 40)); pic->setFixedSize(76, 44);
                    pic->setToolTip(thumbs[i].second);
                    QObject::connect(pic, &QToolButton::clicked, this, [this, f0, i] { pendingSection_ = i; openFile(f0); });
                    cv->addWidget(pic); auto* nl = lab(thumbs[i].second.isEmpty() ? QStringLiteral("단면") : thumbs[i].second, "thumbName"); nl->setAlignment(Qt::AlignCenter); cv->addWidget(nl);
                    tr->addWidget(col);
                }
                tr->addStretch(); sv->addLayout(tr);
            }
            sv->addStretch(); }
        h->addWidget(side); }
    g->addWidget(card, 0, 1, 1, 2);

    // ---- 최근 모델: 머리 「최근 모델 n」 + 찾기, 삼선표(모델 · 상태 · 수평 · 높이 · 단면 · 마지막 열림), 도움말 한 줄
    auto* recent = new QWidget; { auto* rv = new QVBoxLayout(recent); rv->setContentsMargins(0, 0, 0, 0); rv->setSpacing(8);
        auto* hd = new QHBoxLayout; hd->setSpacing(8);
        hd->addWidget(lab(QStringLiteral("최근 모델"), "sectionHead")); hd->addWidget(lab(QString::number(rf.size()), "cap")); hd->addStretch();
        auto* find = new QLineEdit; find->setObjectName("homeFind"); find->setPlaceholderText(QStringLiteral("이름 · 폴더로 찾기")); find->setClearButtonEnabled(true); find->setFixedSize(260, 32);
        hd->addWidget(find);
        rv->addLayout(hd);
        auto* tbl = new QTableWidget(std::max<int>(1, int(rf.size())), 6); tbl->setObjectName("recentTable");
        tbl->setHorizontalHeaderLabels({QStringLiteral("모델"), QStringLiteral("상태"), QStringLiteral("수평"), QStringLiteral("높이"), QStringLiteral("단면"), QStringLiteral("마지막 열림")});
        tbl->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        tbl->verticalHeader()->setVisible(false); tbl->setEditTriggers(QAbstractItemView::NoEditTriggers); tbl->setSelectionBehavior(QAbstractItemView::SelectRows);
        tbl->setSelectionMode(QAbstractItemView::SingleSelection); tbl->setShowGrid(false); tbl->setFocusPolicy(Qt::NoFocus);
        tbl->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        for (int c = 1; c < 6; ++c) tbl->horizontalHeader()->setSectionResizeMode(c, QHeaderView::ResizeToContents);
        tbl->horizontalHeader()->setFixedHeight(28);
        tbl->verticalHeader()->setDefaultSectionSize(48);
        if (rf.isEmpty()) { tbl->setItem(0, 0, new QTableWidgetItem(QStringLiteral("아직 연 모델이 없습니다 — 「모델 열기」로 시작하세요"))); tbl->setSpan(0, 0, 1, 6); }
        for (int i = 0; i < rf.size(); ++i) {
            const QString& f = rf[i];
            const bool ok = QFileInfo::exists(f);
            auto* it0 = new QTableWidgetItem; it0->setToolTip(QDir::toNativeSeparators(f)); tbl->setItem(i, 0, it0);
            { auto* cell = new QWidget; cell->setAttribute(Qt::WA_TransparentForMouseEvents);
                auto* cv = new QVBoxLayout(cell); cv->setContentsMargins(8, 4, 8, 4); cv->setSpacing(0);
                auto* nm = lab(QFileInfo(f).fileName(), ok ? "secName" : "faintName");
                auto* pth = lab(QDir::toNativeSeparators(QFileInfo(f).absolutePath()), "monoFaint"); pth->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
                cv->addWidget(nm); cv->addWidget(pth);
                tbl->setCellWidget(i, 0, cell); }
            tbl->setItem(i, 1, new QTableWidgetItem(ok ? QStringLiteral("열 수 있음") : QStringLiteral("● 원본 없음")));
            if (!ok) tbl->item(i, 1)->setForeground(theme::Block);
            tbl->setItem(i, 2, new QTableWidgetItem(dash(meta(f, "srs").toString())));
            auto* hi = new QTableWidgetItem(dash(meta(f, "height").toString()));
            { const QString hs = meta(f, "heightState").toString(); if (hs == "warn") hi->setForeground(theme::Caution); else if (hs == "error") hi->setForeground(theme::Block); }
            tbl->setItem(i, 3, hi);
            auto* ci = new QTableWidgetItem(QString::number(meta(f, "count").toInt())); ci->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter); tbl->setItem(i, 4, ci);
            auto* ai = new QTableWidgetItem(ago(meta(f, "lastOpened").toString())); ai->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter); tbl->setItem(i, 5, ai);
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
        rv->addWidget(lab(QStringLiteral("두 번 누르면 엶 · 오른쪽 클릭 = 폴더 열기 · 목록에서 지우기(모델 파일은 그대로)"), "lab"));
        rv->addStretch(1); }
    g->addWidget(recent, 1, 0, 1, 2);

    // ---- 작업 순서 세 걸음(카드 없음): ✓ 끝 · ● 다음 · 숫자 나중
    auto* steps = new QWidget; { auto* sv = new QVBoxLayout(steps); sv->setContentsMargins(0, 0, 0, 0); sv->setSpacing(14);
        auto* hd = new QHBoxLayout; hd->addWidget(lab(QStringLiteral("작업 순서"), "sectionHead")); hd->addStretch();
        hd->addWidget(lab(f0.isEmpty() ? QString() : QStringLiteral("%1 기준").arg(QFileInfo(f0).completeBaseName()), "cap"));
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
            if (isDone) mark->setPixmap(kerf::glyph(QStringLiteral("check"), 12, theme::Card, devicePixelRatioF())); else mark->setText(isNext ? QStringLiteral("●") : QString::number(i + 1));
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
    { auto* kh = new QHBoxLayout(keys); kh->setContentsMargins(0, 12, 0, 16); kh->setSpacing(32);
        kh->addWidget(lab(QStringLiteral("처음 쓰는 키"), "sectionHead"));
        const char* kk[][2] = {{"S", "단면선 긋기"}, {"1–5", "뒤 깊이 0.5 · 1 · 2 · 3 · 5 m"}, {"H", "잘린 돌 칠하기"}, {"O", "윤곽 따기"}, {"Ctrl+P", "도면"}, {"Ctrl+Z", "되돌리기 200단계"}};
        for (auto& k : kk) { auto* one = new QHBoxLayout; one->setSpacing(8); one->addWidget(kbd(QString::fromUtf8(k[0]))); one->addWidget(lab(QString::fromUtf8(k[1]), "lab")); kh->addLayout(one); }
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
    if (on && plan_ && plan_->drawMode()) plan_->setDrawMode(false);   // 홈으로 가면 그리기 도구를 내려놓는다(검토 I2: 숨은 평면에서 그리기가 이어지지 않게)
    if (on) rebuildStartPage();
    body_->setCurrentIndex(on ? 0 : 1);
    if (guide_ && on) { guide_->hide(); undoBtns_->hide(); }
    if (viewTabs_) { QSignalBlocker b(viewTabs_); viewTabs_->setTabEnabled(1, bool(src_)); viewTabs_->setCurrentIndex(on ? 0 : 1); }
    setRibbonContext(on ? 0 : 1);
    showStatus(on ? (src_ ? QStringLiteral("%1 열림 — 「단면」 탭에서 이어서 작업합니다").arg(QFileInfo(path_).completeBaseName()) : QStringLiteral("먼저 모델을 열거나, 파일을 창에 끌어다 놓으세요.")) : QString());
    refreshSteps();
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

void MainWindow::applyHighContrast(bool on) {
    highContrast_ = on;
    QSettings().setValue("ui/highContrast", on);
    qApp->setStyleSheet(theme::styleSheet(on));
    section_->setStyle(secStyle());
    updateHeader();
}

void MainWindow::dlgKeys() {
    QDialog d(this); d.setWindowTitle(QStringLiteral("단축키"));
    auto* v = new QVBoxLayout(&d);
    auto* t = new QTableWidget(0, 2); t->setHorizontalHeaderLabels({QStringLiteral("키"), QStringLiteral("하는 일")});
    t->verticalHeader()->setVisible(false); t->setEditTriggers(QAbstractItemView::NoEditTriggers); t->setShowGrid(false);
    t->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    const char* rows[][2] = {
        {"Ctrl+O", "모델 열기"}, {"Ctrl+Shift+O", "최근 모델 다시 열기(마지막 단면선·화면 그대로)"}, {"S", "단면선 그리기 — A 클릭, A′ 클릭"},
        {"Shift (그리는 중)", "동서·남북으로 고정"}, {"Enter (그리는 중)", "A/A′ 좌표 직접 입력"}, {"Esc", "그리기 취소"},
        {"1 2 3 4 5", "뒤 깊이 0.5 / 1 / 2 / 3 / 5 m (기본 3 m — 입면도용 배경)"}, {"[  ]", "평행 이동 −0.1 / +0.1 m (보는 쪽 +)"}, {"{  }", "평행 이동 −1 / +1 m"},
        {"X", "세로 과장 ×1 → ×2 → ×5 → ×10 (화면만 · 도면은 1:1)"},
        {"R", "방향 반전(A↔A′)"}, {"N", "새 단면 추가"}, {"I · L · V", "입면 영상 · 단면선 · 레벨선 켜기/끄기"},
        {"Ctrl+Z / Ctrl+Y", "되돌리기 / 다시(단면선·두께·표시·높이 기준)"}, {"F11", "전체화면 / 원래 크기"}, {"Ctrl+P", "도면 — 평면도 조판 또는 단면도 조판"},
        {"Ctrl+D · Ctrl+E", "단면 DXF · 단면 영상(GeoTIFF 포함)"}, {"F · T · + · −", "맞춤 · 위에서 · 확대 · 축소"}, {"Ctrl+1 · Ctrl+2", "평면 · 단면 보기 켜기/끄기"},
        {"F1", "이 표"}};
    for (auto& r : rows) {
        int i = t->rowCount(); t->insertRow(i);
        auto* k = new QTableWidgetItem(QString::fromUtf8(r[0])); k->setFont(theme::monoFont(12));
        t->setItem(i, 0, k); t->setItem(i, 1, new QTableWidgetItem(QString::fromUtf8(r[1])));
    }
    t->resizeColumnToContents(0);
    v->addWidget(t);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close);
    QObject::connect(bb, &QDialogButtonBox::rejected, &d, &QDialog::reject);
    v->addWidget(bb);
    d.resize(560, 620);
    d.exec();
}

QDialog* MainWindow::coordEntryDialog() {
    if (!src_) return nullptr;
    const Vec3 o = srs().origin;
    SectionLine cur = plan_->line();
    bool haveA = plan_->drawStage() == 1;
    auto* d = new QDialog(this);
    d->setWindowTitle(QStringLiteral("좌표 입력"));
    d->setFixedWidth(560);
    auto* v = new QVBoxLayout(d);
    v->setContentsMargins(24, 20, 24, 20);
    v->setSpacing(12);
    auto* q = new QLabel(QStringLiteral("A와 A′는 어디입니까?"));
    QFont serif;
    serif.setFamilies({QStringLiteral("Batang"), QStringLiteral("Noto Serif KR"), QStringLiteral("Malgun Gothic")});
    serif.setPixelSize(20);
    q->setFont(serif);
    v->addWidget(q);
    auto* hint = new QLabel(QStringLiteral("실좌표(m)입니다. 메시 로컬에 SRSOrigin을 더한 값이고, 높이는 바꾸지 않습니다."));
    hint->setWordWrap(true);
    hint->setObjectName("hint");
    v->addWidget(hint);
    auto* g = new QGridLayout;
    auto mk = [&](double val) { auto* s = new QDoubleSpinBox; s->setRange(-1e8, 1e8); s->setDecimals(3); s->setValue(val); return s; };
    double ax = cur.a.x + o.x, ay = cur.a.y + o.y, bx = cur.b.x + o.x, by = cur.b.y + o.y;
    if (!plan_->hasLine() && !haveA) { ax = plan_->cameraX() + o.x; ay = plan_->cameraY() + o.y; bx = ax + 10; by = ay; }
    auto *sax = mk(ax), *say = mk(ay), *sbx = mk(bx), *sby = mk(by);
    g->addWidget(lab(QStringLiteral("X (동)"), "hint"), 0, 1);
    g->addWidget(lab(QStringLiteral("Y (북)"), "hint"), 0, 2);
    g->addWidget(lab(QStringLiteral("A"), "title"), 1, 0);
    g->addWidget(sax, 1, 1);
    g->addWidget(say, 1, 2);
    g->addWidget(lab(QStringLiteral("A′"), "title"), 2, 0);
    g->addWidget(sbx, 2, 1);
    g->addWidget(sby, 2, 2);
    v->addLayout(g);
    auto* foot = new QHBoxLayout;
    foot->addStretch();
    auto* cancel = new QPushButton(QStringLiteral("취소"));
    auto* ok = new QPushButton(QStringLiteral("단면선 적용"));
    ok->setObjectName("primary");
    ok->setDefault(true);
    foot->addWidget(cancel);
    foot->addWidget(ok);
    v->addLayout(foot);
    QObject::connect(cancel, &QPushButton::clicked, d, &QDialog::reject);
    QObject::connect(ok, &QPushButton::clicked, d, [this, d, sax, say, sbx, sby, o] {
        SectionLine l;
        l.a = {sax->value() - o.x, say->value() - o.y};
        l.b = {sbx->value() - o.x, sby->value() - o.y};
        l.front = front_->value();
        l.back = back_->value();
        if (SectionFrame(l).L <= 1e-3) { QMessageBox::warning(this, windowTitle(), QStringLiteral("A 와 A′ 가 같습니다")); return; }
        if (plan_->drawStage() >= 0) plan_->finishDrawAt(l);
        else { plan_->setLine(l, true); fitNextResult_ = true; requestSection(true); syncCurrentSection(); commitState(QStringLiteral("단면선 좌표 입력")); }
        d->accept();
    });
    return d;
}

void MainWindow::dlgCoordEntry() {
    QDialog* d = coordEntryDialog();
    if (!d) return;
    d->exec();
    d->deleteLater();
}

// ---------------------------------------------------------------- 생성자
MainWindow::MainWindow() {
    setWindowTitle(QStringLiteral("Kerf %1").arg(QString::fromUtf8(kVersion)));
    setAcceptDrops(true);
    bilingual_ = QSettings().value("ui/bilingual", false).toBool();
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
    action("fade")->setToolTip(QStringLiteral("깊이 음영 (Depth Shading) — 입면 영상에서 단면선보다 뒤에 있는 면일수록 옅게 그려 빨간 단면선이 잘 보이게 합니다(내보내기도 같음)"));
    makeAction("info", QStringLiteral("단면 정보"), "Section Info", I::Info);
    makeAction("height", QStringLiteral("높이 기준 지정"), "Height Datum", I::Height);
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
    makeAction("secimport", QStringLiteral("단면 목록 가져오기"), "Import Sections", I::Open);
    makeAction("seclist", QStringLiteral("단면 목록"), "Section List", I::Csv);                 // v5 리본 「자료 › 목록 ▾」(내보내기 · 가져오기)
    makeAction("more", QStringLiteral("더보기"), "More", I::Info);                              // v5 리본 「기타 › 더보기 ▾」
    makeAction("hatch", QStringLiteral("잘린 돌 칠하기"), "Hatch Cut Stone", I::Band, "H", true)->setEnabled(false);   // A1 UI(F단계)에서 켠다
    makeAction("outline", QStringLiteral("윤곽 따기"), "Trace Outline", I::Draw, "O", true)->setEnabled(false);          // A3 UI(G단계)에서 켠다
    makeAction("contrast", QStringLiteral("고대비"), "High Contrast", I::Band, QString(), true)->setChecked(highContrast_);
    makeAction("lang", QStringLiteral("영어 병기"), "English", I::Lang, QString(), true)->setChecked(bilingual_);
    makeAction("coord", QStringLiteral("좌표 입력"), "Enter Coordinates", I::Draw, "Return")->setEnabled(false);
    makeAction("esc", QStringLiteral("그리기 취소"), "Cancel", I::Close, "Esc")->setEnabled(false);
    makeAction("undo", QStringLiteral("되돌리기"), "Undo", I::Undo, "Ctrl+Z")->setEnabled(false);
    makeAction("redo", QStringLiteral("다시"), "Redo", I::Redo, "Ctrl+Y")->setEnabled(false);
    action("redo")->setShortcuts({QKeySequence("Ctrl+Y"), QKeySequence("Ctrl+Shift+Z")});
    action("smooth")->setChecked(QSettings().value("section/smooth", false).toBool());
    action("fade")->setChecked(QSettings().value("section/depthFade", true).toBool());
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
    body_ = new QStackedWidget;
    startPage_ = buildStartPage();
    body_->addWidget(startPage_);
    workArea_ = new QWidget; workArea_->setObjectName("workArea");
    { auto* wl = new QVBoxLayout(workArea_); wl->setContentsMargins(0, 0, 0, 0); wl->setSpacing(0);
        notice_ = buildNotice(); wl->addWidget(notice_);
        split_ = new QSplitter(Qt::Horizontal); split_->setHandleWidth(1);
        sidePanel_ = buildSidePanel();
        planFrame_ = buildPlanFrame(); sectionFrame_ = buildSectionFrame();
        inspector_ = buildInspector();
        split_->addWidget(sidePanel_); split_->addWidget(planFrame_); split_->addWidget(sectionFrame_); split_->addWidget(inspector_);
        split_->setStretchFactor(0, 0); split_->setStretchFactor(1, 5); split_->setStretchFactor(2, 6); split_->setStretchFactor(3, 0);
        split_->setChildrenCollapsible(false);
        split_->setSizes({348, 520, 520, 272});   // 스펙 §2: 왼쪽 목록 348 · 오른쪽 판 272, 가운데 둘은 같은 폭
        sidePanel_->setVisible(action("listpanel")->isChecked());
        wl->addWidget(split_, 1); }
    body_->addWidget(workArea_);
    v->addWidget(body_, 1);
    v->addWidget(buildCoordBar());
    setCentralWidget(central);
    retranslate();
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
        if (on) { guide_->place(); undoBtns_->place(); }
        action("coord")->setEnabled(on); action("esc")->setEnabled(on);
        section_->setDrawingHint(on);   // 단면 화면 위 「A′를 찍으면 단면이 여기에 나옵니다」(화판 2)
        if (stripState_) stripState_->setText(on ? QStringLiteral("단면선을 긋는 중 — 단면은 A′를 찍은 뒤 계산됩니다") : QString());
        updateEnabled();                // 그리는 동안 도면 · 자료 내보내기 꺼짐(화판 2 · 검토 UI 5)
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
    QObject::connect(action("coord"), &QAction::triggered, this, [this] { if (plan_->drawMode()) dlgCoordEntry(); });
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
    const std::pair<const char*, const char*> tg[] = {{"image", "입면 영상"}, {"line", "단면선 표시"}, {"levels", "레벨선"}};
    for (auto& [k, name] : tg) {
        QString nm = QString::fromUtf8(name);
        QObject::connect(action(k), &QAction::toggled, this, [this, nm](bool) { section_->setStyle(secStyle()); commitState(nm); });
    }
    QObject::connect(opacity_, &QSlider::valueChanged, this, [this](int) { section_->setStyle(secStyle()); });
    QObject::connect(action("smooth"), &QAction::toggled, this, [this](bool on) { QSettings().setValue("section/smooth", on); requestSection(true); commitState(QStringLiteral("평활")); });
    QObject::connect(action("fade"), &QAction::toggled, this, [this](bool on) { QSettings().setValue("section/depthFade", on); section_->setStyle(secStyle()); requestSection(true); commitState(QStringLiteral("깊이 음영")); });
    QObject::connect(action("info"), &QAction::triggered, this, [this] { dlgInfo(); });
    QObject::connect(action("height"), &QAction::triggered, this, [this] { dlgHeightDatum(); });
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
    QObject::connect(action("contrast"), &QAction::toggled, this, [this](bool on) { applyHighContrast(on); });
    QObject::connect(action("lang"), &QAction::toggled, this, [this](bool on) { bilingual_ = on; QSettings().setValue("ui/bilingual", on); retranslate(); });
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

    // 1) 리본 탭 없음 · 문서 탭 「홈 · 단면[· 도면]」 상시(v5 C2)
    QStringList names;
    for (int i = 0; i < viewTabs_->count(); ++i) names << viewTabs_->tabText(i);
    const bool tabsOk = docTabsRow_ && docTabsRow_->isVisibleTo(this) && names.size() >= 2 && names[0] == QStringLiteral("홈") && names[1] == QStringLiteral("단면");
    L << QStringLiteral("ui-audit tabs=0 doctabs=%1%2").arg(names.join(QLatin1Char(','))).arg(tabsOk ? QString() : QStringLiteral("  FAIL"));
    ok &= tabsOk;

    // 2) 리본(v5 C1): 높이 118 · 묶음 7 · 1920 에서 타일 50 글자 보임 · 흙색 칩 하나(「도면」)
    const RibbonLook lk = ribbon_->look();
    int groups = 0;
    for (const char* g : {"model", "sec", "elev", "view", "data", "out", "etc"}) if (ribbon_->group(QString::fromLatin1(g))) ++groups;
    int clay = 0;
    for (auto* b : ribbon_->chips()) if (b->property("primary").toBool() && b->isVisibleTo(this)) ++clay;
    // 판정은 규칙으로(검토 I6): 창 폭에 들어가는 가장 큰 크기를 골랐는가 · 높이가 그 크기의 sizeHint 인가. 1920 이상이면 타일 50 · 글자 · 118(스펙 §12)
    const QList<int> lw = ribbon_->lookWidths();
    const bool ribRule = ribbon_->lookIndex() == Ribbon::chooseLook(lw, ribbon_->width()) && ribbon_->height() == ribbon_->sizeHint().height();
    const bool ribOk = ribRule && groups == 7 && clay == 1 && (ribbon_->width() < 1920 || (lk.tile == 50 && lk.labels && ribbon_->height() == 118));
    L << QStringLiteral("ui-audit ribbon height=%1 groups=%2 look=%3 labels=%4 chips-clay=%5 width=%6 need50=%7 need32=%8 need20=%9%10")
             .arg(ribbon_->height()).arg(groups).arg(lk.tile).arg(lk.labels ? 1 : 0).arg(clay).arg(ribbon_->width()).arg(lw.value(0)).arg(lw.value(9)).arg(lw.value(lw.size() - 1))
             .arg(ribOk ? QString() : QStringLiteral("  FAIL"));
    L << QStringLiteral("ui-audit ribbon-widths %1").arg(ribbon_->widthReport());
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

    // 3c) 리본 문맥(검토 I1): 홈에서는 단면 · 자료 · 도면 동작과 「좌표 입력」이 꺼지고 「닫기」는 살아 있다 — updateEnabled() 뒤에도 그대로여야 한다
    int ctxBad = 0;
    {
        const int keepCtx = ribbonCtx_;
        setRibbonContext(0); updateEnabled();
        for (const char* k : {"draw", "addsec", "flip", "move", "sheet", "csv", "plan", "xyz", "las"}) if (action(k)->isEnabled()) ++ctxBad;
        if (src_ && !action("close")->isEnabled()) ++ctxBad;
        if (coordBtn_ && coordBtn_->isEnabled()) ++ctxBad;
        setRibbonContext(keepCtx); updateEnabled();
    }
    L << QStringLiteral("ui-audit context-home enabled-wrong=%1%2").arg(ctxBad).arg(ctxBad ? QStringLiteral("  FAIL") : QString());
    ok &= ctxBad == 0;

    // 3d) 「단면 찾기」 거르기는 목록을 다시 만들어도 남는다(검토 I5)
    int filterKeep = -1;
    if (findBox_ && secList_ && secList_->count() > 0) {
        findBox_->setText(QStringLiteral("\u2603zzz")); refreshSectionList(); pump(50);
        filterKeep = secList_->count() > 0 && secList_->item(0)->isHidden() ? 1 : 0;
        findBox_->clear(); refreshSectionList(); pump(50);
    }
    L << QStringLiteral("ui-audit filter-persists=%1%2").arg(filterKeep < 0 ? QStringLiteral("not-checked") : QString::number(filterKeep)).arg(filterKeep == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= filterKeep != 0;

    // 4) 축척 칸 · 높이 배지는 화면에 하나씩
    int scales = 0, badges = 0;
    for (auto* c : findChildren<QComboBox*>()) if (c->isVisibleTo(this) && c->findText(QStringLiteral("맞춤")) >= 0) ++scales;
    for (auto* b : findChildren<QToolButton*>(QStringLiteral("statusBadge"))) if (b->isVisibleTo(this)) ++badges;
    L << QStringLiteral("ui-audit scale-controls=%1 status-badges=%2%3").arg(scales).arg(badges).arg(scales == 1 && badges == 2 ? QString() : QStringLiteral("  FAIL"));
    ok &= scales == 1 && badges == 2;
    // 4b) 배지는 스펙 §6: 높이 24 · 둘 다 「… ▾」(검토 UI 7)
    const bool badgeOk = srsLabel_ && heightBadge2_ && srsLabel_->height() == 24 && heightBadge2_->height() == 24
                      && srsLabel_->text().endsWith(QChar(0x25BE)) && heightBadge2_->text().endsWith(QChar(0x25BE));
    L << QStringLiteral("ui-audit status-badge height=%1,%2 caret=%3,%4%5").arg(srsLabel_ ? srsLabel_->height() : -1).arg(heightBadge2_ ? heightBadge2_->height() : -1)
             .arg(srsLabel_ && srsLabel_->text().endsWith(QChar(0x25BE)) ? 1 : 0).arg(heightBadge2_ && heightBadge2_->text().endsWith(QChar(0x25BE)) ? 1 : 0).arg(badgeOk ? QString() : QStringLiteral("  FAIL"));
    ok &= badgeOk;

    // 5) 그리는 동안 상태줄 「다음:」 문장이 없는가
    int hint = -1, band = -1, guide2 = -1; QString guide2Why;
    if (src_) {
        const SectionLine keep = plan_->line(); const bool hadLine = plan_->hasLine();
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
            const QString spec = QStringLiteral("끝점 A′를 클릭하세요 · Shift = 축 맞춤 · 숫자 = 길이");
            const bool textOk = guide_->hint() == spec || (guide_->hint().startsWith(spec.left(6)) && guide_->width() >= planHost_->width() - 2 * 12 - 32);   // 줄임표 알갱이(글자 한두 개) 허용
            guide2 = inside && apart && textOk ? 1 : 0;
            guide2Why = QStringLiteral("inside=%1 apart=%2 text=%3 guide-w=%4 host-w=%5 hint=\"%6\"").arg(inside ? 1 : 0).arg(apart ? 1 : 0).arg(textOk ? 1 : 0).arg(guide_->width()).arg(planHost_->width()).arg(guide_->hint());
            if (!dir.isEmpty()) grab().save(QDir(dir).filePath(QStringLiteral("draw2.png")));
        }
        plan_->setDrawMode(false); plan_->setLine(keep, hadLine); pump(100);
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
    }
    L << QStringLiteral("ui-audit lists-match=%1%2").arg(listsMatch < 0 ? QStringLiteral("not-checked") : QString::number(listsMatch)).arg(listsMatch == 0 ? QStringLiteral("  FAIL") : QString());
    ok &= listsMatch != 0;
    L << sheetLine;

    // 7) 아이콘 · 툴팁(D11)
    L << QStringLiteral("ui-audit icons-missing=%1 tooltip-missing=%2").arg(noIcon.size()).arg(noTip.size());
    for (const QString& n : noIcon) L << QStringLiteral("no-icon: %1").arg(n);
    for (const QString& n : noTip) L << QStringLiteral("no-tooltip: %1").arg(n);
    ok &= noIcon.isEmpty() && noTip.isEmpty();

    // 2b) 좁은 창 1280×800 — 맨 끝에서(offscreen 에서는 resize 를 되돌려도 판 폭이 돌아오지 않아 뒤 항목을 망친다, 실측): 글자 숨김 · 타일 32 이하(스펙 §3.3 · §12)
    // 창 최소 폭(판 넷의 합)이 1280 보다 넓을 수 있으므로 고르기 규칙 자체를 판정한다. 창이 실제로 1280 으로 줄어드는지는 B5(해상도) 에서
    const RibbonLook nk = Ribbon::looks().at(Ribbon::chooseLook(lw, 1280));   // lookWidths 에 리본 좌우 여백이 이미 들어 있다
    const bool narrowOk = nk.tile <= 32 && !nk.labels;
    const QSize keepSize = size();
    resize(1280, 800); pump(250);
    L << QStringLiteral("ui-audit narrow rule-at-1280 look=%1 labels=%2 window-min-width=%3 actual-look=%4%5").arg(nk.tile).arg(nk.labels ? 1 : 0).arg(minimumSizeHint().width()).arg(ribbon_->look().tile).arg(narrowOk ? QString() : QStringLiteral("  FAIL"));
    if (!dir.isEmpty()) grab().save(QDir(dir).filePath(QStringLiteral("narrow.png")));
    resize(keepSize); pump(250);
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
