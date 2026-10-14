#include "mainwindow.hpp"

#include <QApplication>
#include <QButtonGroup>
#include <QStyle>
#include <QUndoStack>
#include <QMenu>
#include <QToolButton>
#include <QCryptographicHash>
#include <QInputDialog>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <algorithm>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <cstdio>
#include <unordered_map>
#include "asec/obj.hpp"
#include "sqlite3.h"   // SQLITE_VERSION(프로그램 정보)
#include "stb_image.h"

using namespace asec;
using clk = std::chrono::steady_clock;

extern const char* const kVersion;
const char* const kVersion = "1.2.1";
// 저작권 표시(여는 화면·프로그램 정보). 권리자 이름은 사용자가 정한다(비어 있으면 연도만)
const char* const kCopyrightYear = "2026";
const char* const kCopyrightHolder = "권영인";   // 사용자 지정 2026-10-11
QString copyrightLine() {
    const QString who = QString::fromUtf8(kCopyrightHolder).trimmed();
    return QStringLiteral("© %1%2").arg(QString::fromUtf8(kCopyrightYear), who.isEmpty() ? QString() : QStringLiteral(" ") + who);
}
/// 가져다 쓴 외부 구성요소와 라이선스(packaging/licenses/THIRD_PARTY.txt 와 같은 목록, 한 줄)
QString thirdPartyLine() {
    return QStringLiteral("Qt %1 (LGPL-3.0, 동적 링크) · OpenCTM (zlib) · SQLite (public domain) · stb (public domain/MIT) · nlohmann/json (MIT)").arg(QString::fromLatin1(qVersion()));
}
static constexpr double kDepthFade = kDepthFadeStrength;
// 모델별 설정 키(경로 기준, 대소문자 무시)
static QString heightSettingsKey(const QString& path) {
    QByteArray h = QCryptographicHash::hash(QFileInfo(path).absoluteFilePath().toLower().toUtf8(), QCryptographicHash::Sha1).toHex().left(16);
    return QStringLiteral("heightDatum/") + QString::fromLatin1(h);
}

// ---------------------------------------------------------------- 공통 도우미
static fs::path toFs(const QString& q) {
#ifdef _WIN32
    return fs::path(q.toStdWString());
#else
    return fs::u8path(q.toStdString());
#endif
}
static QString qs(const std::string& s) { return QString::fromUtf8(s.c_str()); }

static void decodeTextureFull(Texture& t) {
    if (!t.rgba.empty() || t.encoded.empty()) return;
    int w, h, c;
    unsigned char* p = stbi_load_from_memory(t.encoded.data(), int(t.encoded.size()), &w, &h, &c, 4);
    if (!p) return;
    t.rgba.w = w; t.rgba.h = h; t.rgba.px.assign(p, p + size_t(w) * h * 4);
    stbi_image_free(p);
}

static QImage toQImage(const RgbaImage& im) {
    if (im.empty()) return {};
    return QImage(im.px.data(), im.w, im.h, im.w * 4, QImage::Format_RGBA8888).copy();
}
static RgbaImage fromQImage(const QImage& q0) {
    QImage q = q0.convertToFormat(QImage::Format_RGBA8888);
    RgbaImage r; r.w = q.width(); r.h = q.height(); r.px.resize(size_t(r.w) * r.h * 4);
    for (int y = 0; y < r.h; ++y) std::memcpy(&r.px[size_t(y) * r.w * 4], q.constScanLine(y), size_t(r.w) * 4);
    return r;
}

static QImage displayTexture(Texture& t) {
    QImage q;
    if (!t.rgba.empty()) q = toQImage(t.rgba);
    else if (!t.encoded.empty()) {
        int w, h, c;
        unsigned char* p = stbi_load_from_memory(t.encoded.data(), int(t.encoded.size()), &w, &h, &c, 4);
        if (p) { q = QImage(p, w, h, w * 4, QImage::Format_RGBA8888).copy(); stbi_image_free(p); }
    }
    if (!q.isNull() && std::max(q.width(), q.height()) > 1024) q = q.scaled(1024, 1024, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return q;
}

static std::shared_ptr<DisplayMesh> makeDisplay(const Mesh& m, const Vec3& c, std::unordered_map<const Texture*, QImage>& texCache, bool withTexture = true) {
    auto d = std::make_shared<DisplayMesh>();
    size_t n = m.vertexCount();
    d->pos.resize(n * 3);
    for (size_t i = 0; i < n; ++i) {
        d->pos[3 * i] = float(m.pos[3 * i] - c.x); d->pos[3 * i + 1] = float(m.pos[3 * i + 1] - c.y); d->pos[3 * i + 2] = float(m.pos[3 * i + 2] - c.z);
    }
    d->uv = m.uv;
    d->idx = m.idx;
    d->nrm.assign(n * 3, 0.f);
    for (size_t t = 0; t + 2 < m.idx.size(); t += 3) {
        uint32_t a = m.idx[t], b = m.idx[t + 1], e = m.idx[t + 2];
        if (a >= n || b >= n || e >= n) continue;
        float ux = m.pos[3 * b] - m.pos[3 * a], uy = m.pos[3 * b + 1] - m.pos[3 * a + 1], uz = m.pos[3 * b + 2] - m.pos[3 * a + 2];
        float vx = m.pos[3 * e] - m.pos[3 * a], vy = m.pos[3 * e + 1] - m.pos[3 * a + 1], vz = m.pos[3 * e + 2] - m.pos[3 * a + 2];
        float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
        if (nz < 0) { nx = -nx; ny = -ny; nz = -nz; }  // 위를 향하게(감김 방향 무관)
        for (uint32_t k : {a, b, e}) { d->nrm[3 * k] += nx; d->nrm[3 * k + 1] += ny; d->nrm[3 * k + 2] += nz; }
    }
    if (m.texture && withTexture) {
        auto it = texCache.find(m.texture.get());
        if (it == texCache.end()) it = texCache.emplace(m.texture.get(), displayTexture(*m.texture)).first;
        d->tex = it->second;
    }
    return d;
}

bool MainWindow::loadScene(const QString& path, OpenedScene& out, QString* err, const std::function<void(double)>& progress) {
    out = OpenedScene();
    out.path = path;
    fs::path p = toFs(path);
    std::string e;
    std::string ext = p.extension().u8string();
    for (auto& ch : ext) ch = char(std::tolower(static_cast<unsigned char>(ch)));
    std::vector<MeshPtr> disp;
    if (ext == ".obj") {
        auto s = std::make_shared<StaticSource>();
        if (!loadObj(p, s->meshes, &e)) { if (err) *err = qs(e); return false; }
        if (!findMetadataXml(p, s->srs)) s->srs = SrsInfo();
        for (auto& m : s->meshes) { if (m->texture) decodeTextureFull(*m->texture); s->bounds.add(m->bbox); }
        disp = s->meshes;
        out.bounds = s->bounds;
        out.src = s;
        out.kind = "OBJ";
    } else {
        // 3SM(ScalableMesh, SQLite): 노드 색인만 읽고 바로 연다(변환·캐시 없음). 노드는 가상 타일로 3MX 와 같은 경로로 스트리밍된다(asec/sm3.hpp).
        // 3MX: 루트 타일 머리만 읽고 바로 연다. 화면은 평면 보기의 LOD 스트리밍이 거친 것부터 채운다.
        const bool sm = ext == ".3sm" || isSqliteFile(p);
        std::shared_ptr<TmxSource> s;
        if (sm) {
            auto ss = std::make_shared<SmSource>();
            ss->cache->textureDecoder = decodeTextureFull;
            if (!ss->openSm(p, &e)) { if (err) *err = qs(e); return false; }
            s = ss;
        } else {
            s = std::make_shared<TmxSource>();
            s->cache->textureDecoder = decodeTextureFull;
            if (!s->open(p, &e)) { if (err) *err = qs(e); return false; }
        }
        out.streamRoots = s->scene.roots();   // 병합 3MX: 레이어 전부
        out.layers = s->scene.layers.size();
        out.warnings = s->scene.warnings;
        // 커서 Z 용 거친 메시: 루트 타일 노드(지오메트리만, 텍스처 디코드 없음)
        Box3 bb;
        for (auto& root : out.streamRoots) {
            TmxTile rt;
            if (readTmxTile(root, rt, &e))
                for (size_t i = 0; i < rt.nodes.size(); ++i)
                    if (decodeNode(rt, i, &e)) for (auto& m : rt.nodes[i].meshes) { disp.push_back(m); bb.add(m->bbox); }
        }
        out.bounds = s->bounds.valid() ? s->bounds : bb;
        if (!s->bounds.valid()) s->bounds = out.bounds;
        out.src = s;
        out.kind = sm ? "3SM" : "3MX";
    }
    if (!out.bounds.valid()) { if (err) *err = QStringLiteral("메시가 비어 있습니다"); return false; }
    out.center = out.bounds.center();
    std::unordered_map<const Texture*, QImage> texCache;
    for (size_t i = 0; i < disp.size(); ++i) {
        out.display.push_back(makeDisplay(*disp[i], out.center, texCache, out.streamRoots.empty()));
        out.displayTris += disp[i]->triangleCount();
        if (progress && (i % 16 == 0)) progress(0.7 + 0.3 * double(i) / disp.size());
    }
    out.displayMeshes = disp.size();
    return true;
}

// 리본·창 구성·생성자는 mainwindow_ui.cpp

MainWindow::~MainWindow() {
    if (undo_) { QObject::disconnect(undo_, nullptr, this, nullptr); delete undo_; undo_ = nullptr; }   // 동작 표(act_)보다 먼저
    pickWorker_.reset();
    secWorker_.reset();  // 실행 중인 단면 취소 + 합류
    cancelTask_ = true;
    if (task_.joinable()) task_.join();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    QSettings().setValue("ui/geometry", saveGeometry());
    saveModelState();
    if (taskBusy_) {
        if (QMessageBox::question(this, windowTitle(), QStringLiteral("작업이 진행 중입니다. 중단하고 끝낼까요?")) != QMessageBox::Yes) { e->ignore(); return; }
        cancelTask_ = true;
    }
    e->accept();
}

const SrsInfo& MainWindow::srs() const { static SrsInfo none; return src_ ? src_->srs : none; }
bool MainWindow::hasSection() const { return section_->hasResult(); }
void MainWindow::selectDocTab(int i) { if (viewTabs_ && i >= 0 && i < viewTabs_->count()) viewTabs_->setCurrentIndex(i); }

SectionStyle MainWindow::secStyle() const {
    SectionStyle s;
    s.showImage = action("image")->isChecked(); s.showLine = action("line")->isChecked(); s.showLevels = action("levels")->isChecked(); s.showTicks = action("ticks")->isChecked();
    s.imageOpacity = opacity_->value() / 100.0;
    s.depthFade = action("fade")->isChecked();
    s.lineWidthPx = QSettings().value("view/sectionLineWidth", 2.0).toDouble() * (highContrast_ ? 1.5 : 1.0);
    if (current_ >= 0 && current_ < int(sections_.size())) PlanView::splitLetters(QString::fromUtf8(sections_[size_t(current_)].name.c_str()), s.nameA, s.nameB);   // B–B′ → B · B′
    return s;
}

void MainWindow::showStatus(const QString& s) { msg_->setText(s); msg_->setToolTip(s); }

void MainWindow::report(bool ok, const QString& m) {
    if (ok) QMessageBox::information(this, QStringLiteral("내보내기 완료"), m);
    else QMessageBox::warning(this, QStringLiteral("내보내기 실패"), m);
    showStatus(ok ? QStringLiteral("내보내기 완료") : m.section('\n', 0, 0));
}

void MainWindow::setProgress(double f) {
    if (f < 0) { progress_->setVisible(false); return; }
    progress_->setVisible(true);
    if (f > 1.5) progress_->setRange(0, 0); else { progress_->setRange(0, 1000); progress_->setValue(int(f * 1000)); }
}

// 켜고 끄는 곳은 여기 하나(검토 I1): 모델 · 작업 중 · 리본 문맥(홈 0 · 단면 1 · 도면 2, 스펙 §3.1) · 그리는 중을 한 번에 본다.
// 홈 = 모델 묶음만(열기 · 최근 · 닫기), 도면 = 단면 · 입면 꺼짐, 그리는 중 = 도면 · 자료 내보내기 꺼짐(화판 2)
void MainWindow::updateEnabled() {
    if (!section_ || !plan_ || !front_ || !back_) return;
    const bool sc = bool(src_), sec = section_->hasResult(), busy = taskBusy_;
    const bool home = ribbonCtx_ == 0, secCtx = ribbonCtx_ == 1, drawing = plan_->drawMode();
    const bool elevCtx = secCtx || ribbonCtx_ == 2;   // 입면 묶음은 도면 탭에서도 쓴다(조판이 따라옴 — 2026-10-11)
    for (const char* k : {"fit", "top", "zoomin", "zoomout"}) action(k)->setEnabled(sc && !(busy && QString(k) != "fit"));
    action("close")->setEnabled(sc && !busy);
    for (const char* k : {"projsave", "projsaveas"}) action(k)->setEnabled(sc && !busy);
    action("projopen")->setEnabled(!busy);
    for (const char* k : {"draw", "addsec"}) action(k)->setEnabled(sc && !busy && secCtx);
    for (const char* k : {"flip", "clear", "move"}) action(k)->setEnabled(sc && plan_->hasLine() && secCtx);
    for (const char* k : {"dxf", "secimg", "csv", "info"}) action(k)->setEnabled(sec && !busy && !home && !drawing);
    for (const char* k : {"plan", "points", "xyz", "las", "sheet", "plansheet", "sectionsheet"}) action(k)->setEnabled(sc && !busy && !home && !drawing);
    for (const char* k : {"secjson", "secimport", "image", "line", "levels", "ticks", "fade", "smooth", "listpanel"}) action(k)->setEnabled(sc && !busy && !home);
    for (const char* k : {"seclist", "view1", "view2", "contrast"}) action(k)->setEnabled(!home);
    for (QWidget* wd : {static_cast<QWidget*>(front_), static_cast<QWidget*>(back_)}) wd->setEnabled(sc && elevCtx);
    for (auto* c : depthChip_) if (c) c->setEnabled(sc && elevCtx);
    if (depthBox_) depthBox_->setEnabled(sc && elevCtx);
    for (QWidget* wd : {elevRow2_, static_cast<QWidget*>(opacityMenu_), lineWidthMenu_}) if (wd) wd->setEnabled(sc && elevCtx);   // 입면 아래 줄 · 입면 ▾ 안의 같은 칸도 입면 칸과 같이
    action("elevmenu")->setEnabled(sc && elevCtx);
    action("open")->setEnabled(!busy); action("recent")->setEnabled(!busy && !recentFiles().isEmpty());
}

void MainWindow::runTask(const QString& what, std::function<bool(QString*)> work, std::function<void(bool, const QString&)> done) {
    if (taskBusy_) { QMessageBox::information(this, windowTitle(), QStringLiteral("다른 작업이 진행 중입니다.")); return; }
    if (task_.joinable()) task_.join();
    taskBusy_ = true; cancelTask_ = false;
    showStatus(what + QStringLiteral(" …"));
    setProgress(2.0);
    updateEnabled();
    task_ = std::thread([this, work, done] {
        QString m;
        bool ok = false;
        try { ok = work(&m); } catch (const std::exception& e) { m = QStringLiteral("오류: %1").arg(QString::fromUtf8(e.what())); }
        QMetaObject::invokeMethod(this, [this, ok, m, done] {
            taskBusy_ = false;
            setProgress(-1);
            updateEnabled();
            done(ok, m);
        }, Qt::QueuedConnection);
    });
}

// ---------------------------------------------------------------- 열기
void MainWindow::chooseOpen() {
    QSettings st;
    QString f = QFileDialog::getOpenFileName(this, QStringLiteral("실사 메시 · 프로젝트 열기"), st.value("dir/open").toString(),
                                             QStringLiteral("실사 메시 · 프로젝트 (*.3mx *.3sm *.obj *.trench.json);;3MX 실사 메시 (*.3mx);;3SM ScalableMesh (*.3sm);;OBJ 메시 (*.obj);;Trench 프로젝트 (*.trench.json);;모든 파일 (*.*)"));
    if (f.isEmpty()) return;
    st.setValue("dir/open", QFileInfo(f).absolutePath());
    openFile(f);
}

void MainWindow::openFile(const QString& path) {
    if (asec::isProjectFileName(QFileInfo(path).fileName().toStdString())) { openProjectFile(path); return; }   // .trench.json → 모델을 찾아 연다
    auto res = std::make_shared<OpenedScene>();
    runTask(QStringLiteral("여는 중: %1").arg(QFileInfo(path).fileName()),
        [this, path, res](QString* m) {
            return loadScene(path, *res, m, [this](double f) { QMetaObject::invokeMethod(this, [this, f] { setProgress(f); }, Qt::QueuedConnection); });
        },
        [this, res, path](bool ok, const QString& m) {
            if (!ok) { QMessageBox::warning(this, windowTitle(), QStringLiteral("열 수 없습니다:\n%1\n\n%2").arg(path, m)); showStatus(QString()); return; }
            applyScene(std::move(*res));
        });
}

void MainWindow::applyScene(OpenedScene&& s) {
    secFloorGen_ = secWorker_->cancelAll();
    pickFloorGen_ = pickWorker_->cancelAll();
    discardSheetTab();   // 다른 모델의 도면 탭을 버린다(검토 I4)
    src_ = s.src; path_ = s.path; kind_ = s.kind; displayTris_ = s.displayTris;
    section_->clear(); last_ = SectionOutput();
    if (!s.streamRoots.empty()) plan_->setStreamingScene(s.streamRoots, s.center, s.bounds, src_->srs, std::move(s.display));
    else plan_->setScene(std::move(s.display), s.center, s.bounds, src_->srs);
    streaming_ = !s.streamRoots.empty();
    sceneBounds_ = s.bounds; sceneWarnings_ = s.warnings;
    // 높이 기준은 앱에서 다루지 않는다(T8 · 사용자 결정 2026-10-10): 예전에 저장된 모델별 지정값 · 마지막 지정값은 설정에 그대로 두되 읽어 적용하지 않는다
    refreshSrs();
    info_->setText(streaming_ ? QStringLiteral("%1%2 · LOD 스트리밍").arg(kind_, s.layers > 1 ? QStringLiteral(" · 레이어 %1개").arg(s.layers) : QString())
                              : QStringLiteral("%1 · 화면 %2만 삼각형").arg(kind_).arg(displayTris_ / 10000.0, 0, 'f', 1));
    setWindowTitle(QStringLiteral("%1 — Trench %2").arg(QFileInfo(path_).fileName(), QString::fromUtf8(kVersion)));
    showStart(false);
    addRecent(path_);
    restoreModelState();
    showStatus(plan_->hasLine() ? QStringLiteral("마지막 단면선을 되살렸습니다 — 다음: 뒤 깊이(1–6) 또는 Ctrl+P 도면")
                                : QStringLiteral("열림. 다음: 「단면선」(S)으로 평면에서 시작점 A, 끝점 A′ 를 클릭하세요"));
    msg_->setToolTip(QStringLiteral("열림"));
    updateEnabled();
    updateHeader();
    saveModelState();
    plan_->setFocus();
}

void MainWindow::closeScene() {
    if (taskBusy_) return;
    saveModelState();
    if (plan_->drawMode()) plan_->setDrawMode(false);
    secFloorGen_ = secWorker_->cancelAll();
    pickFloorGen_ = pickWorker_->cancelAll();
    src_.reset();
    plan_->clearScene(); section_->clear(); last_ = SectionOutput();
    srsLabel_->setText(QStringLiteral("수평 —")); srsLabel_->setToolTip(QString()); srsLabel_->setProperty("state", "none"); info_->clear();   // 모델 없음 = 회색(검토 UI 4)
    srsLabel_->style()->unpolish(srsLabel_); srsLabel_->style()->polish(srsLabel_);
    notice_->setVisible(false); srsReport_ = SrsReport();
    src_.reset(); path_.clear(); projectPath_.clear();
    sections_.clear(); thumbs_.clear(); current_ = -1; refreshSectionList();
    undo_->clear();
    setWindowTitle(QStringLiteral("Trench %1").arg(QString::fromUtf8(kVersion)));
    discardSheetTab();   // 옛 모델의 「도면」 탭 · 조판 위젯이 남지 않게(검토 I4)
    showStart(true);
    updateEnabled();
    updateHeader();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* e) {
    if (e->mimeData()->hasUrls())
        for (auto& u : e->mimeData()->urls()) {
            QString f = u.toLocalFile().toLower();
            if (f.endsWith(".3mx") || f.endsWith(".3sm") || f.endsWith(".obj")) { e->acceptProposedAction(); return; }
        }
}
void MainWindow::dropEvent(QDropEvent* e) {
    for (auto& u : e->mimeData()->urls()) {
        QString f = u.toLocalFile();
        if (f.toLower().endsWith(".3mx") || f.toLower().endsWith(".3sm") || f.toLower().endsWith(".obj") || f.toLower().endsWith(".trench.json")) { openFile(f); return; }
    }
}

// ---------------------------------------------------------------- 단면 계산(작업 스레드, 마지막 요청 우선)
void MainWindow::setLineLocal(const SectionLine& l) { plan_->setLine(l, true); }
void MainWindow::setThickness(double f, double b) { front_->setValue(f); back_->setValue(b); }
double MainWindow::frontDepth() const { return front_->value(); }
double MainWindow::backDepth() const { return back_->value(); }
void MainWindow::setDepthFade(bool on) { action("fade")->setChecked(on); }

static SectionRequest makeRequest(const SectionLine& line, bool smooth, bool final, bool fade) {
    SectionRequest rq;
    rq.line = line;
    rq.depthFade = fade ? kDepthFade : 0.0;
    rq.cleanup.smooth = smooth;
    double L = SectionFrame(line).L;
    // 최종 화면 영상: 최대 12,000 px 폭·1.5 mm/px(이전 4,000 px·3 mm) — 조판(600 dpi)과 비슷하게 또렷하게(사용자 요청 2026-10-11)
    rq.imageRes = final ? std::max(0.0015, L / 12000.0) : std::max(0.008, L / 700.0);
    rq.maxImagePixels = final ? size_t(96) << 20 : size_t(3) << 20;
    // 미리보기: 영상 해상도에 충분한 거친 LOD(빠름). 최종: 최고 해상도 잎
    rq.meshRes = final ? 0.0 : rq.imageRes;
    // 잘린 면은 미리보기에서도 잎(정확). 성능 비교용으로만 끔: SECTIONVIEWER_COARSE_PREVIEW_CUT=1
    static const bool coarseCut = qEnvironmentVariableIntValue("SECTIONVIEWER_COARSE_PREVIEW_CUT") == 1;
    rq.leafProfileAlways = !coarseCut;
    return rq;
}

void MainWindow::requestSection(bool final) {
    if (!src_ || !plan_->hasLine()) return;
    SectionLine l = plan_->line(); l.front = front_->value(); l.back = back_->value();
    if (SectionFrame(l).L < 0.01) return;
    auto src = src_;
    SectionRequest rq = makeRequest(l, action("smooth")->isChecked(), final, action("fade")->isChecked());
    secWorker_->submit(final, [this, src, rq](uint64_t gen, bool fin, const std::atomic<bool>* cancel) {
        auto out = std::make_shared<SectionOutput>();
        std::string err;
        bool ok = computeSection(*src, rq, *out, &err, cancel);
        if (cancel->load()) return;  // 더 새 요청이 이 계산을 취소함
        QString e = ok ? QString() : (err.empty() ? QStringLiteral("계산 실패") : qs(err));
        QMetaObject::invokeMethod(this, [this, out, gen, fin, e] { onSectionDone(e.isEmpty() ? std::move(*out) : SectionOutput(), gen, fin, e); },
                                  Qt::QueuedConnection);
    });
    section_->setBusy(true);
}

void MainWindow::onSectionDone(SectionOutput&& out, uint64_t gen, bool final, const QString& err) {
    if (gen <= secFloorGen_ || !secGate_.accept(gen)) return;  // 오래된 결과(장면 바뀜 또는 더 새 결과가 이미 표시됨)
    bool newest = gen == secWorker_->latestGen();
    if (!err.isEmpty()) { section_->setBusy(!newest); showStatus(err); return; }
    last_ = std::move(out);
    const bool wasPreview = section_->hasResult() && !lastFinal_;   // 미리보기가 떠 있던 자리에 최종이 오면 영상만 크로스페이드
    lastFinal_ = final;
    (final ? secCounters_.finalsShown : secCounters_.previewsShown)++;
    secCounters_.lastMs = last_.msCollect + last_.msCut + last_.msImage; secCounters_.lastTris = last_.stats.triangles;
    section_->setStyle(secStyle());
    QImage secIm = toQImage(last_.image.img);
    section_->setResult(last_.result, secIm, last_.image.s0, last_.image.z1, last_.image.res, !fitNextResult_, final && wasPreview);
    if (plan_) plan_->setSectionPlane(secIm, last_.result.line, last_.image.s0, last_.image.z1, last_.image.res);
    if (fitNextResult_ && final) fitNextResult_ = false;
    if (final) notifySheetDoc();   // 도면 탭이 열려 있으면 새 단면으로 조판 갱신
    section_->setBusy(!(final && newest));
    size_t nv = 0, nClosed = 0;
    for (auto& pl : last_.result.profile) { nv += pl.size(); if (pl.size() > 3 && (pl.front() - pl.back()).len() < 1e-9) ++nClosed; }
    double L = SectionFrame(last_.result.line).L;
    const QString detail = (QStringLiteral("%9 단면 %1 m · %10 %2 · 삼각형 %3 · 윤곽 %4개(닫힘 %5) %6점 · %7 ms%8")
                   .arg(L, 0, 'f', 2).arg(last_.stats.leafNodes).arg(last_.stats.triangles).arg(last_.result.profile.size()).arg(nClosed).arg(nv)
                   .arg(int(last_.msCollect + last_.msCut + last_.msImage))
                   .arg(last_.stats.fallbackNodes ? QStringLiteral(" · 상위 LOD 대체 %1").arg(last_.stats.fallbackNodes) : QString())
                   .arg(last_.previewLod ? QStringLiteral("[미리보기·거친 LOD]") : QStringLiteral("[최종·잎]"))
                   .arg(last_.previewLod ? QStringLiteral("타일") : QStringLiteral("잎 타일")));
    if (final && newest) {
        // 기복이 화면 높이에 비해 아주 작으면(1:1 에서 평평해 보임) 세로 과장을 권함 — 화면만, 도면은 1:1
        vexRelief_ = profileRelief(last_.result.profile);
        vexSuggest_ = suggestVerticalExaggeration(vexRelief_, section_->fitVisibleHeight());
        // 스펙 §6: 다음 할 일 한 문장. 세로 과장 권장은 단면 머리 「세로 ×1 ▾」 툴팁이 든다(검토 UI 1)
        showStatus(QStringLiteral("%1 최종 %2 m — 다음: 리본의 「도면」(Ctrl+P)으로 종이에 옮기세요.").arg(sectionName()).arg(L, 0, 'f', 2));
        if (current_ >= 0) { thumbs_[current_] = makeThumb(); refreshSectionList(); }
        saveModelState();
    } else showStatus(last_.cutFromLeaf ? QStringLiteral("미리보기 — 단면선은 잎(정확), 배경 영상만 거친 LOD · 최종 계산 중…") : QStringLiteral("미리보기(거친 LOD) — 최종(잎) 계산 중…"));
    msg_->setToolTip(detail);
    updateEnabled();
    updateHeader();
}

bool MainWindow::computeNow(const SectionLine& line, QString* err) {
    if (!src_) { if (err) *err = QStringLiteral("열린 메시가 없습니다"); return false; }
    uint64_t g = secWorker_->cancelAll();
    secFloorGen_ = g - 1;
    SectionLine l = line; l.front = front_->value(); l.back = back_->value();
    plan_->setLine(l, true);  // (onLineChanged 를 부르지 않음)
    SectionOutput out; std::string e;
    if (!computeSection(*src_, makeRequest(l, action("smooth")->isChecked(), true, action("fade")->isChecked()), out, &e)) { if (err) *err = qs(e); return false; }
    onSectionDone(std::move(out), g, true, QString());
    syncCurrentSection();
    return true;
}

// ---------------------------------------------------------------- 내보내기(스레드 안전, 정적)
static QString fileStem(const QString& path) { QFileInfo fi(path); return fi.absolutePath() + "/" + fi.completeBaseName(); }
static QString mm(double m) { return QString::number(m * 1000.0, 'f', m < 0.01 ? 2 : 1); }

bool MainWindow::exportSectionImage(const SectionDoc& doc0, MeshSource& src, const SectionExportParams& p, const QString& path, QString* msg,
                                    const std::atomic<bool>* cancel) {
    const double res = groundResolution(p.denom, p.dpi), ppm = 1.0 / res, ui = p.dpi / 96.0;
    SectionDoc doc = doc0;
    // 1) 출력 해상도 그대로 입면 영상 다시 그리기(같은 단면선·정리 → 같은 윤곽)
    SectionRequest rq;
    rq.line = doc.r.line;
    rq.cleanup.smooth = false;
    rq.depthFade = doc.st.depthFade ? kDepthFade : 0.0;
    rq.imageRes = res;
    rq.maxImagePixels = size_t(160) << 20;
    SectionXf xf;
    QSize sz = sectionExportLayout(doc, ppm, ui, xf);
    if (double(sz.width()) * sz.height() > 400e6) {
        if (msg) *msg = QStringLiteral("출력 영상이 너무 큽니다(%1 × %2 px). 축척을 줄이거나 DPI 를 낮추세요.").arg(sz.width()).arg(sz.height());
        return false;
    }
    SectionOutput out; std::string e;
    if (!computeSection(src, rq, out, &e, cancel)) { if (msg) *msg = qs(e); return false; }
    // 화면과 같은 윤곽(평활 설정 포함)을 쓰기 위해 profile 은 스냅숏 것을 유지, 영상만 교체
    QImage img = toQImage(out.image.img);
    SectionImgGeo geo{out.image.s0, out.image.z1, out.image.res};
    QImage canvas(sz, QImage::Format_RGBA8888);
    canvas.fill(Qt::white);
    {
        QPainter pt(&canvas);
        QString footer = QStringLiteral("Trench · 1:%1 · %2 dpi · %3").arg(p.denom, 0, 'f', 0).arg(p.dpi, 0, 'f', 0).arg(QDate::currentDate().toString("yyyy-MM-dd"));
        paintSectionDoc(pt, doc, QRectF(0, 0, sz.width(), sz.height()), xf, ui, img, true, footer, &geo);
    }
    const int dpm = int(std::lround(p.dpi / 0.0254));
    canvas.setDotsPerMeterX(dpm); canvas.setDotsPerMeterY(dpm);
    std::string err;
    QStringList files{QFileInfo(path).fileName()};
    if (p.format == 0) {
        if (!canvas.convertToFormat(QImage::Format_RGB32).save(path, "PNG")) { if (msg) *msg = QStringLiteral("PNG 저장 실패: %1").arg(path); return false; }
    } else {
        TiffOptions o; o.dpi = p.dpi; o.alpha = false;
        o.description = "Section " + doc.r.srs.shortLabel();
        if (p.format == 2) {
            // 왼쪽 위 픽셀 모서리의 (거리, 표고): 여백까지 포함해 정확히
            o.geo = sectionGeoRef(doc.r, xf.s0 - xf.plot.left() / ppm, xf.zTop + xf.plot.top() / ppm, res);
        }
        if (!writeTiff(toFs(path), fromQImage(canvas), o, &err)) { if (msg) *msg = qs(err); return false; }
        if (p.format == 2) {
            QString stem = fileStem(path);
            writeWorldFile(toFs(stem + ".tfw"), o.geo, &err);
            writeSectionSidecar(toFs(stem + ".json"), doc.r, o.geo, p.denom, p.dpi, sz.width(), sz.height(), &err);
            files << QFileInfo(stem + ".tfw").fileName() << QFileInfo(stem + ".json").fileName();
        }
    }
    if (msg)
        *msg = QStringLiteral("%1\n\n크기 %2 × %3 px · 지상 해상도 %4 mm/px (1:%5, %6 dpi)\n인쇄 크기 %7 × %8 mm%9")
                   .arg(files.join(", ")).arg(sz.width()).arg(sz.height()).arg(mm(res)).arg(p.denom, 0, 'f', 0).arg(p.dpi, 0, 'f', 0)
                   .arg(sz.width() / p.dpi * 25.4, 0, 'f', 1).arg(sz.height() / p.dpi * 25.4, 0, 'f', 1)
                   .arg(std::fabs(out.image.res - res) > 1e-9 ? QStringLiteral("\n(영상 크기 한도로 입면 영상은 %1 mm/px 로 그림)").arg(mm(out.image.res)) : QString());
    return true;
}

bool MainWindow::exportSectionDxf(const SectionDoc& doc, MeshSource& src, const DxfParams& p, const QString& path, QString* msg,
                                  const std::atomic<bool>* cancel) {
    DxfExportOptions o;
    o.mode = p.world3d ? DxfCoordMode::World3D : DxfCoordMode::Drawing2D;
    o.scaleDenom = p.denom;
    o.levelMinorPt = clampLineWeightPt(p.levelMinorPt); o.levelMajorPt = clampLineWeightPt(p.levelMajorPt);
    QString pngName;
    if (p.image) {
        SectionRequest rq;
        rq.line = doc.r.line;
        rq.imageRes = groundResolution(p.denom, p.imageDpi);
        rq.depthFade = doc.st.depthFade ? kDepthFade : 0.0;
        rq.maxImagePixels = size_t(80) << 20;
        SectionOutput out; std::string e;
        if (!computeSection(src, rq, out, &e, cancel)) { if (msg) *msg = qs(e); return false; }
        auto& im = out.image;
        if (!im.img.empty()) {
            QString png = fileStem(path) + "_image.png";
            QImage q = toQImage(im.img);
            q.setDotsPerMeterX(int(std::lround(p.imageDpi / 0.0254))); q.setDotsPerMeterY(q.dotsPerMeterX());
            if (!q.save(png, "PNG")) { if (msg) *msg = QStringLiteral("PNG 저장 실패: %1").arg(png); return false; }
            pngName = QFileInfo(png).fileName();
            o.image = true; o.imageFile = pngName.toStdString();
            o.imageW = im.img.w; o.imageH = im.img.h; o.imageS0 = im.s0; o.imageZ0 = im.z0(); o.imageRes = im.res;
        }
    }
    std::string err;
    if (!asec::exportSectionDxf(doc.r, o, toFs(path), &err)) { if (msg) *msg = qs(err); return false; }
    size_t nv = 0; for (auto& pl : doc.r.profile) nv += pl.size();
    if (msg)
        *msg = QStringLiteral("%1%2\n\n%3 · 단면선 %4개(%5점), 레이어 SECTION_PROFILE = 빨강(1)\n레벨선 10 cm / 50 cm / 1 m, 표고 라벨, 축, A·A′ 표시")
                   .arg(QFileInfo(path).fileName(), pngName.isEmpty() ? QString() : ", " + pngName,
                        p.world3d ? QStringLiteral("3D 실좌표(EPSG) 배치") : QStringLiteral("2D 도면: X = A 기준 거리, Y = 절대 표고"))
                   .arg(doc.r.profile.size()).arg(nv);
    return true;
}

bool MainWindow::exportPlan(MeshSource& src, const PlanParams& p, const SectionLine* line, const QString& path, QString* msg, const std::atomic<bool>* cancel) {
    Box3 area = p.wholeModel ? src.bounds : p.area;
    if (!p.wholeModel) { Box3 b = src.bounds; area.mn.x = std::max(area.mn.x, b.mn.x); area.mn.y = std::max(area.mn.y, b.mn.y); area.mx.x = std::min(area.mx.x, b.mx.x); area.mx.y = std::min(area.mx.y, b.mx.y); }
    if (!area.valid() || area.mx.x <= area.mn.x || area.mx.y <= area.mn.y) { if (msg) *msg = QStringLiteral("내보낼 범위가 비어 있습니다"); return false; }
    const double res = groundResolution(p.denom, p.dpi);
    // 픽셀 격자를 해상도 배수에 맞춤(인접 타일 이어붙이기 쉬움)
    double x0 = std::floor(area.mn.x / res) * res, y1 = std::ceil(area.mx.y / res) * res;
    int W = int(std::ceil((area.mx.x - x0) / res)), H = int(std::ceil((y1 - area.mn.y) / res));
    if (double(W) * H > 200e6) {
        if (msg) *msg = QStringLiteral("출력 영상이 너무 큽니다(%1 × %2 px, 지상 해상도 %3 mm). 축척을 줄이거나(예: 1/200) DPI 를 낮추거나 「현재 화면」 범위를 쓰세요.").arg(W).arg(H).arg(mm(res));
        return false;
    }
    std::vector<MeshPtr> meshes; std::string e;
    if (auto* t = dynamic_cast<TmxSource*>(&src)) {
        LeafStats st;
        if (!t->areaMeshes(area, res, meshes, &st, &e, cancel)) { if (msg) *msg = qs(e); return false; }
    } else if (auto* s = dynamic_cast<StaticSource*>(&src)) {
        for (auto& m : s->meshes) if (!m->bbox.valid() || !(m->bbox.mx.x < area.mn.x || m->bbox.mn.x > area.mx.x || m->bbox.mx.y < area.mn.y || m->bbox.mn.y > area.mx.y)) meshes.push_back(m);
    }
    RgbaImage img;
    if (!renderPlan(meshes, x0, y1, res, W, H, img, cancel)) { if (msg) *msg = QStringLiteral("평면 영상 생성 실패(취소 또는 메모리)"); return false; }
    if (p.overlayLine && line && SectionFrame(*line).L > 0) {
        QImage q(img.px.data(), img.w, img.h, img.w * 4, QImage::Format_RGBA8888);  // img 메모리에 직접 그림
        QPainter pt(&q);
        pt.setRenderHint(QPainter::Antialiasing);
        auto P = [&](Vec2 v) { return QPointF((v.x - x0) / res, (y1 - v.y) / res); };
        const double px = std::max(1.5, 0.35 / 25.4 * p.dpi);  // 0.35 mm 선
        BandQuad b = sectionBand(*line, 0.0);
        QPolygonF band; for (auto& c : b.p) band << P(c);
        pt.setPen(QPen(theme::SectionRed, px * 0.6)); pt.setBrush(QColor(255, 0, 0, 28)); pt.drawPolygon(band);
        pt.setPen(QPen(theme::SectionRed, px, Qt::SolidLine, Qt::RoundCap)); pt.drawLine(P(line->a), P(line->b));
        QFont f(theme::fontFamily()); f.setPixelSize(int(std::max(10.0, 3.0 / 25.4 * p.dpi))); f.setBold(true); pt.setFont(f);
        pt.drawText(P(line->a) + QPointF(px * 2, -px * 2), "A"); pt.drawText(P(line->b) + QPointF(px * 2, -px * 2), QString::fromUtf8("A′"));
    }
    TiffOptions o; o.dpi = p.dpi; o.alpha = true;
    o.geo = planGeoRef(src.srs, x0, y1, res);
    o.description = "Plan orthophoto " + src.srs.shortLabel();
    std::string err;
    if (!writeTiff(toFs(path), img, o, &err)) { if (msg) *msg = qs(err); return false; }
    writeWorldFile(toFs(fileStem(path) + ".tfw"), o.geo, &err);
    if (msg)
        *msg = QStringLiteral("%1 (+ .tfw)\n\n%2 × %3 px · 픽셀 %4 mm (1:%5, %6 dpi)\n좌상단 X %7, Y %8 · %9")
                   .arg(QFileInfo(path).fileName()).arg(W).arg(H).arg(mm(res)).arg(p.denom, 0, 'f', 0).arg(p.dpi, 0, 'f', 0)
                   .arg(o.geo.tieX, 0, 'f', 3).arg(o.geo.tieY, 0, 'f', 3)
                   .arg(o.geo.epsg > 0 ? QStringLiteral("EPSG:%1").arg(o.geo.epsg) + (o.geo.vertEpsg > 0 ? QStringLiteral(" + 수직 EPSG:%1 (GeoKey 4096)").arg(o.geo.vertEpsg) : QString())
                                       : QStringLiteral("좌표계 미상(로컬 좌표 + 원점)"));
    return true;
}

bool MainWindow::exportPointCloud(MeshSource& src, const PointParams& p, const SectionLine& line, const QString& path, QString* msg,
                                  const std::atomic<bool>* cancel, const std::function<void(size_t)>& progress) {
    PointExportOptions o;
    o.format = p.format; o.spacing = p.spacing; o.rgb = p.rgb;
    o.area = p.area == 0 ? PointArea::Whole : p.area == 1 ? PointArea::Band : PointArea::Box;
    if (p.area == 1) o.band = sectionBand(line, 0.0);
    if (p.area == 2) o.box = p.box;
    PointExportStats st; std::string e;
    bool ok;
    if (auto* t = dynamic_cast<TmxSource*>(&src)) ok = exportPointsTmx(*t->cache, t->scene.roots(), src.srs, o, toFs(path), &st, &e, cancel, progress);
    else {
        auto* s = dynamic_cast<StaticSource*>(&src);
        auto visit = [s](const std::function<bool(const MeshPtr&)>& cb) { for (auto& m : s->meshes) if (!cb(m)) return false; return true; };
        ok = exportPoints(visit, src.srs, o, toFs(path), &st, &e, cancel, progress);
    }
    if (!ok) { if (msg) *msg = qs(e); return false; }
    if (msg) {
        auto& b = st.worldBounds;
        *msg = QStringLiteral("%1\n\n점 %2개 (꼭짓점 %3개에서, 메시 %4개)\n범위 X %5 ~ %6\n     Y %7 ~ %8\n     Z %9 ~ %10\n%11")
                   .arg(QFileInfo(path).fileName()).arg(st.points).arg(st.vertices).arg(st.meshes)
                   .arg(b.mn.x, 0, 'f', 3).arg(b.mx.x, 0, 'f', 3).arg(b.mn.y, 0, 'f', 3).arg(b.mx.y, 0, 'f', 3).arg(b.mn.z, 0, 'f', 3).arg(b.mx.z, 0, 'f', 3)
                   .arg(p.format == PointFormat::LAS ? QStringLiteral("LAS 1.2 점 형식 2(RGB), 축척 0.001 m") : QStringLiteral("XYZ 텍스트: X Y Z R G B (실좌표, SRSOrigin 적용)"));
    }
    return true;
}

bool MainWindow::exportProfileCsv(const SectionDoc& doc, const QString& path, QString* msg) {
    FILE* f =
#ifdef _WIN32
        _wfopen(toFs(path).wstring().c_str(), L"wb");
#else
        std::fopen(path.toUtf8().constData(), "wb");
#endif
    if (!f) { if (msg) *msg = QStringLiteral("파일을 만들 수 없습니다: %1").arg(path); return false; }
    std::fprintf(f, "part,closed,s_m,X,Y,Z\n");
    int k = 0; size_t n = 0;
    for (auto& pl : doc.r.profile) {
        ++k;
        bool closed = pl.size() > 3 && (pl.front() - pl.back()).len() < 1e-9;
        for (auto& q : pl) { Vec3 w = sectionToWorld(doc.r, q.x, q.y); std::fprintf(f, "%d,%d,%.4f,%.4f,%.4f,%.4f\n", k, closed ? 1 : 0, q.x, w.x, w.y, w.z); ++n; }
    }
    std::fclose(f);
    if (msg) *msg = QStringLiteral("%1\n\n윤곽 %2개, %3점 (X Y Z = 실좌표)").arg(QFileInfo(path).fileName()).arg(k).arg(n);
    return true;
}

// ---------------------------------------------------------------- 대화상자
namespace {
struct ScaleRow {
    QComboBox* combo = nullptr;
    QDoubleSpinBox* custom = nullptr;
    QSpinBox* dpi = nullptr;
    double denom() const { int i = combo->currentIndex(); double d = combo->itemData(i).toDouble(); return d > 0 ? d : custom->value(); }
};
ScaleRow addScaleRows(QFormLayout* f, const QString& key, double defDenom, int defDpi, const std::function<void()>& changed) {
    QSettings st;
    ScaleRow r;
    r.combo = new QComboBox;
    for (int d : {10, 20, 40, 50, 100, 200, 500}) r.combo->addItem(QStringLiteral("1/%1").arg(d), double(d));
    r.combo->addItem(QStringLiteral("사용자 정의…"), 0.0);
    r.custom = new QDoubleSpinBox; r.custom->setRange(1, 100000); r.custom->setDecimals(0); r.custom->setPrefix("1 / ");
    double den = st.value(key + "/denom", defDenom).toDouble();
    int idx = r.combo->findData(den);
    r.combo->setCurrentIndex(idx >= 0 ? idx : r.combo->count() - 1);
    r.custom->setValue(den);
    r.custom->setVisible(idx < 0);
    auto* row = new QWidget; auto* h = new QHBoxLayout(row); h->setContentsMargins(0, 0, 0, 0); h->addWidget(r.combo); h->addWidget(r.custom); h->addStretch();
    f->addRow(QStringLiteral("축척 (Scale)"), row);
    r.dpi = new QSpinBox; r.dpi->setRange(50, 2400); r.dpi->setSingleStep(50); r.dpi->setSuffix(" dpi");
    r.dpi->setValue(st.value(key + "/dpi", defDpi).toInt());
    f->addRow(QStringLiteral("해상도 (DPI)"), r.dpi);
    QComboBox* c = r.combo; QDoubleSpinBox* cu = r.custom;
    QObject::connect(c, &QComboBox::currentIndexChanged, c, [c, cu, changed](int i) { cu->setVisible(c->itemData(i).toDouble() <= 0); changed(); });
    QObject::connect(cu, &QDoubleSpinBox::valueChanged, cu, [changed](double) { changed(); });
    QObject::connect(r.dpi, &QSpinBox::valueChanged, r.dpi, [changed](int) { changed(); });
    return r;
}
void saveScale(const QString& key, const ScaleRow& r) { QSettings st; st.setValue(key + "/denom", r.denom()); st.setValue(key + "/dpi", r.dpi->value()); }

QDialog* makeDialog(QWidget* parent, const QString& title, QFormLayout*& form, QLabel*& calc, QVBoxLayout*& outer) {
    auto* d = new QDialog(parent);
    d->setWindowTitle(title);
    d->setAttribute(Qt::WA_DeleteOnClose, false);
    outer = new QVBoxLayout(d);
    form = new QFormLayout; form->setLabelAlignment(Qt::AlignRight); form->setHorizontalSpacing(12); form->setVerticalSpacing(8);
    outer->addLayout(form);
    calc = new QLabel; calc->setObjectName("calc"); calc->setWordWrap(true); calc->setMinimumWidth(380);
    outer->addWidget(calc);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bb->button(QDialogButtonBox::Ok)->setText(QStringLiteral("내보내기…"));
    bb->button(QDialogButtonBox::Ok)->setObjectName("primary");
    bb->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("취소"));
    QObject::connect(bb, &QDialogButtonBox::accepted, d, &QDialog::accept);
    QObject::connect(bb, &QDialogButtonBox::rejected, d, &QDialog::reject);
    outer->addWidget(bb);
    return d;
}
}  // namespace

QString MainWindow::askSavePath(const QString& key, const QString& suggested, const QString& filter) {
    QSettings st;
    QString dir = st.value("dir/" + key, st.value("dir/open").toString()).toString();
    QString f = QFileDialog::getSaveFileName(this, QStringLiteral("저장할 파일"), dir.isEmpty() ? suggested : dir + "/" + suggested, filter);
    if (!f.isEmpty()) st.setValue("dir/" + key, QFileInfo(f).absolutePath());
    return f;
}

static QString baseName(const QString& path) { return path.isEmpty() ? QStringLiteral("section") : QFileInfo(path).completeBaseName(); }

// 내보내기 전: 화면 결과가 미리보기(거친 LOD)이면 잎 메시로 최종 단면을 먼저 계산(내보내기는 항상 최종 기준)
bool MainWindow::ensureFinalSection() {
    if (!section_->hasResult()) return false;
    if (!last_.previewLod) return true;
    QString e;
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool ok = computeNow(plan_->line(), &e);
    QApplication::restoreOverrideCursor();
    if (!ok) report(false, QStringLiteral("최종 단면 계산 실패: ") + e);
    return ok && section_->hasResult();
}

void MainWindow::dlgSectionImage() {
    if (!section_->hasResult() || !src_) return;
    if (!ensureFinalSection()) return;
    SectionDoc doc = section_->doc();
    QFormLayout* f; QLabel* calc; QVBoxLayout* outer;
    std::unique_ptr<QDialog> d(makeDialog(this, QStringLiteral("단면 영상 내보내기 (Section Image)"), f, calc, outer));
    auto* fmt = new QComboBox;
    fmt->addItem(QStringLiteral("PNG 영상")); fmt->addItem(QStringLiteral("TIFF 영상")); fmt->addItem(QStringLiteral("GeoTIFF (단면 로컬 좌표: X = 거리, Y = 표고) + .tfw + .json"));
    fmt->setCurrentIndex(QSettings().value("secimg/format", 0).toInt());
    f->addRow(QStringLiteral("형식 (Format)"), fmt);
    ScaleRow sr;
    auto upd = [&] {
        double denom = sr.denom(), dpi = sr.dpi->value(), res = groundResolution(denom, dpi);
        SectionXf xf; QSize sz = sectionExportLayout(doc, 1.0 / res, dpi / 96.0, xf);
        double mb = double(sz.width()) * sz.height() * 4 / 1048576.0;
        calc->setText(QStringLiteral("지상 해상도 <b>%1 mm/px</b> (= 0.0254 × %2 ÷ %3)<br>영상 약 <b>%4 × %5 px</b> (%6 MB) · 인쇄 크기 %7 × %8 mm<br>현재 표시(영상·단면선·레벨선·불투명도) 그대로, 단면선 빨강")
                          .arg(mm(res)).arg(denom, 0, 'f', 0).arg(dpi, 0, 'f', 0).arg(sz.width()).arg(sz.height()).arg(mb, 0, 'f', 0)
                          .arg(sz.width() / dpi * 25.4, 0, 'f', 0).arg(sz.height() / dpi * 25.4, 0, 'f', 0));
    };
    sr = addScaleRows(f, "secimg", 20, 300, upd);
    upd();
    if (d->exec() != QDialog::Accepted) return;
    saveScale("secimg", sr); QSettings().setValue("secimg/format", fmt->currentIndex());
    SectionExportParams p; p.format = fmt->currentIndex(); p.denom = sr.denom(); p.dpi = sr.dpi->value();
    const char* ext = p.format == 0 ? ".png" : ".tif";
    QString path = askSavePath("export", baseName(path_) + QStringLiteral("_단면_1-%1%2").arg(p.denom, 0, 'f', 0).arg(ext),
                               p.format == 0 ? QStringLiteral("PNG (*.png)") : QStringLiteral("TIFF (*.tif *.tiff)"));
    if (path.isEmpty()) return;
    auto src = src_;
    runTask(QStringLiteral("단면 영상 내보내는 중"), [doc, src, p, path, this](QString* m) { return exportSectionImage(doc, *src, p, path, m, &cancelTask_); },
            [this](bool ok, const QString& m) { report(ok, m); });
}

void MainWindow::dlgSectionDxf() {
    if (!section_->hasResult() || !src_) return;
    if (!ensureFinalSection()) return;
    SectionDoc doc = section_->doc();
    QFormLayout* f; QLabel* calc; QVBoxLayout* outer;
    std::unique_ptr<QDialog> d(makeDialog(this, QStringLiteral("단면 DXF 내보내기 (Section DXF)"), f, calc, outer));
    auto* mode = new QComboBox;
    mode->addItem(QStringLiteral("2D 도면 — X = A 기준 거리, Y = 절대 표고"));
    mode->addItem(QStringLiteral("3D 실좌표 — 단면 수직면에 EPSG 좌표로 배치"));
    mode->setCurrentIndex(QSettings().value("dxf/mode", 0).toInt());
    f->addRow(QStringLiteral("좌표 (Coordinates)"), mode);
    auto* img = new QCheckBox(QStringLiteral("입면 영상 포함 (IMAGE 개체 + 같은 폴더에 PNG)"));
    img->setChecked(QSettings().value("dxf/image", true).toBool());
    f->addRow(QString(), img);
    ScaleRow sr;
    auto upd = [&] {
        double denom = sr.denom(), dpi = sr.dpi->value();
        calc->setText(QStringLiteral("문자 높이 2 mm × 1/%1 = <b>%2 m</b> · 영상 픽셀 <b>%3 mm</b><br>레이어: SECTION_PROFILE(빨강 1), LEVEL_10CM / 50CM / 1M, LEVEL_TEXT, SECTION_AXIS, SECTION_IMAGE")
                          .arg(denom, 0, 'f', 0).arg(0.002 * denom, 0, 'f', 3).arg(mm(groundResolution(denom, dpi))));
    };
    sr = addScaleRows(f, "dxf", 20, 200, upd);
    upd();
    if (d->exec() != QDialog::Accepted) return;
    saveScale("dxf", sr); QSettings().setValue("dxf/mode", mode->currentIndex()); QSettings().setValue("dxf/image", img->isChecked());
    DxfParams p; p.world3d = mode->currentIndex() == 1; p.denom = sr.denom(); p.imageDpi = sr.dpi->value(); p.image = img->isChecked();
    p.levelMinorPt = clampLineWeightPt(QSettings().value("sheet/levelMinorPt", 0.2).toDouble()); p.levelMajorPt = clampLineWeightPt(QSettings().value("sheet/levelMajorPt", 0.5).toDouble());   // 도면 판의 pt 와 같은 값(검토 M3)
    QString path = askSavePath("export", baseName(path_) + QStringLiteral("_단면.dxf"), QStringLiteral("DXF (*.dxf)"));
    if (path.isEmpty()) return;
    auto src = src_;
    runTask(QStringLiteral("DXF 내보내는 중"), [doc, src, p, path, this](QString* m) { return exportSectionDxf(doc, *src, p, path, m, &cancelTask_); },
            [this](bool ok, const QString& m) { report(ok, m); });
}

void MainWindow::dlgPlan() {
    if (!src_) return;
    QFormLayout* f; QLabel* calc; QVBoxLayout* outer;
    std::unique_ptr<QDialog> d(makeDialog(this, QStringLiteral("평면 GeoTIFF 내보내기 (Plan Orthophoto)"), f, calc, outer));
    auto* area = new QComboBox; area->addItem(QStringLiteral("모델 전체")); area->addItem(QStringLiteral("현재 화면 범위 (View 1)"));
    f->addRow(QStringLiteral("범위 (Area)"), area);
    auto* ov = new QCheckBox(QStringLiteral("단면선·두께 띠 표시 (빨강)")); ov->setChecked(plan_->hasLine()); ov->setEnabled(plan_->hasLine());
    f->addRow(QString(), ov);
    Box3 view; bool hasView = plan_->viewRectLocal(view);
    if (!hasView) area->setEnabled(false);
    ScaleRow sr;
    auto upd = [&] {
        double res = groundResolution(sr.denom(), sr.dpi->value());
        Box3 b = area->currentIndex() == 0 || !hasView ? src_->bounds : view;
        double w = std::min(b.mx.x, src_->bounds.mx.x) - std::max(b.mn.x, src_->bounds.mn.x), h = std::min(b.mx.y, src_->bounds.mx.y) - std::max(b.mn.y, src_->bounds.mn.y);
        double W = std::ceil(std::max(0.0, w) / res), H = std::ceil(std::max(0.0, h) / res);
        int epsg = src_->srs.epsg();
        calc->setText(QStringLiteral("픽셀 크기(지상 해상도) <b>%1 mm</b> · 영상 <b>%2 × %3 px</b> (%4 m × %5 m)<br>좌표계 %6 · GeoKey + ModelTiepoint + ModelPixelScale, .tfw 함께")
                          .arg(mm(res)).arg(W, 0, 'f', 0).arg(H, 0, 'f', 0).arg(w, 0, 'f', 2).arg(h, 0, 'f', 2)
                          .arg(epsg > 0 ? qs(src_->srs.describe().horizontalLabelKo()) : QStringLiteral("⚠ EPSG 미확인 — ") + qs(src_->srs.describe().horizontalLabelKo())));
    };
    QObject::connect(area, &QComboBox::currentIndexChanged, d.get(), [&](int) { upd(); });
    sr = addScaleRows(f, "plan", 100, 200, upd);
    upd();
    if (d->exec() != QDialog::Accepted) return;
    saveScale("plan", sr);
    PlanParams p; p.wholeModel = area->currentIndex() == 0 || !hasView; p.area = view; p.denom = sr.denom(); p.dpi = sr.dpi->value(); p.overlayLine = ov->isChecked();
    QString path = askSavePath("export", baseName(path_) + QStringLiteral("_평면_1-%1.tif").arg(p.denom, 0, 'f', 0), QStringLiteral("GeoTIFF (*.tif *.tiff)"));
    if (path.isEmpty()) return;
    auto src = src_;
    bool hasLine = plan_->hasLine();
    SectionLine line = plan_->line();
    runTask(QStringLiteral("평면 GeoTIFF 만드는 중"), [src, p, path, hasLine, line, this](QString* m) { return exportPlan(*src, p, hasLine ? &line : nullptr, path, m, &cancelTask_); },
            [this](bool ok, const QString& m) { report(ok, m); });
}

void MainWindow::dlgPoints(int presetFormat) {
    if (!src_) return;
    QFormLayout* f; QLabel* calc; QVBoxLayout* outer;
    std::unique_ptr<QDialog> d(makeDialog(this, QStringLiteral("점군 내보내기 (Point Cloud)"), f, calc, outer));
    auto* fmt = new QComboBox; fmt->addItem(QStringLiteral("XYZ 텍스트 (X Y Z R G B)")); fmt->addItem(QStringLiteral("LAS 1.2 (점 형식 2, RGB)"));
    fmt->setCurrentIndex(presetFormat);
    f->addRow(QStringLiteral("형식 (Format)"), fmt);
    auto* area = new QComboBox;
    area->addItem(QStringLiteral("모델 전체"));
    area->addItem(QStringLiteral("단면 두께 띠 안 (앞 %1 m / 뒤 %2 m)").arg(front_->value(), 0, 'f', 2).arg(back_->value(), 0, 'f', 2));
    area->addItem(QStringLiteral("현재 화면 범위 (View 1)"));
    f->addRow(QStringLiteral("범위 (Area)"), area);
    auto* sp = new QComboBox;
    sp->addItem(QStringLiteral("원본 꼭짓점 전부 (1 mm 안 중복만 제거)"), 0.0);
    for (double s : {0.005, 0.01, 0.02, 0.05, 0.10}) sp->addItem(QStringLiteral("간격 %1 cm 솎기").arg(s * 100, 0, 'g', 2), s);
    sp->setCurrentIndex(QSettings().value("points/spacingIdx", 0).toInt());
    f->addRow(QStringLiteral("솎기 (Decimation)"), sp);
    auto* rgb = new QCheckBox(QStringLiteral("텍스처에서 색(RGB) 추출")); rgb->setChecked(true);
    f->addRow(QString(), rgb);
    calc->setText(QStringLiteral("최고 해상도(잎) 타일의 꼭짓점 · 좌표 = 로컬 + SRSOrigin (실좌표 %1)<br>타일 경계의 같은 점은 한 번만 기록. 큰 모델은 수 분 걸릴 수 있습니다.")
                      .arg(qs(src_->srs.describe().horizontalLabelKo())));
    if (d->exec() != QDialog::Accepted) return;
    QSettings().setValue("points/spacingIdx", sp->currentIndex());
    PointParams p;
    p.format = fmt->currentIndex() == 1 ? PointFormat::LAS : PointFormat::XYZ;
    p.area = area->currentIndex();
    if (p.area == 1 && !plan_->hasLine()) { QMessageBox::information(this, windowTitle(), QStringLiteral("단면선이 없습니다. 먼저 단면선을 그리세요.")); return; }
    if (p.area == 2 && !plan_->viewRectLocal(p.box)) p.area = 0;
    p.spacing = sp->currentData().toDouble();
    p.rgb = rgb->isChecked();
    QString path = askSavePath("export", baseName(path_) + (p.format == PointFormat::LAS ? QStringLiteral("_점군.las") : QStringLiteral("_점군.xyz")),
                               p.format == PointFormat::LAS ? QStringLiteral("LAS (*.las)") : QStringLiteral("XYZ (*.xyz *.txt)"));
    if (path.isEmpty()) return;
    auto src = src_;
    SectionLine line = plan_->line(); line.front = front_->value(); line.back = back_->value();
    runTask(QStringLiteral("점군 내보내는 중"),
            [src, p, path, line, this](QString* m) {
                return exportPointCloud(*src, p, line, path, m, &cancelTask_, [this](size_t n) {
                    QMetaObject::invokeMethod(this, [this, n] { showStatus(QStringLiteral("점군 내보내는 중 … %1점").arg(n)); }, Qt::QueuedConnection);
                });
            },
            [this](bool ok, const QString& m) { report(ok, m); });
}

void MainWindow::dlgProfileCsv() {
    if (!section_->hasResult()) return;
    if (src_ && !ensureFinalSection()) return;
    QString path = askSavePath("export", baseName(path_) + QStringLiteral("_단면선.csv"), QStringLiteral("CSV (*.csv)"));
    if (path.isEmpty()) return;
    QString m;
    bool ok = exportProfileCsv(section_->doc(), path, &m);
    report(ok, m);
}

void MainWindow::dlgInfo() {
    if (!section_->hasResult()) return;
    const auto& r = last_.result;
    SectionFrame f(r.line);
    Vec3 A = sectionToWorld(r, 0, 0), B = sectionToWorld(r, f.L, 0);
    size_t nv = 0, nc = 0; for (auto& pl : r.profile) { nv += pl.size(); if (pl.size() > 3 && (pl.front() - pl.back()).len() < 1e-9) ++nc; }
    QString t = QStringLiteral(
        "<table cellspacing=4>"
        "<tr><td><b>A</b></td><td>X %1 &nbsp; Y %2</td></tr>"
        "<tr><td><b>A′</b></td><td>X %3 &nbsp; Y %4</td></tr>"
        "<tr><td>길이 · 방위각</td><td>%5 m · %6°</td></tr>"
        "<tr><td>두께 띠</td><td>앞 %7 m / 뒤 %8 m</td></tr>"
        "<tr><td>표고 범위</td><td>%9 ~ %10 m</td></tr>"
        "<tr><td>사용 타일(잎)</td><td>%11개 · 삼각형 %12 · 상위 LOD 대체 %13</td></tr>"
        "<tr><td>절단 선분 → 윤곽</td><td>%14 → %15개 (닫힌 고리 %16), %17점</td></tr>"
        "<tr><td>입면 영상</td><td>%18 × %19 px, %20 mm/px</td></tr>"
        "<tr><td>계산 시간</td><td>수집 %21 · 절단 %22 · 영상 %23 ms</td></tr>"
        "<tr><td>좌표계</td><td>%24 · 원점 %25, %26, %27</td></tr></table>")
        .arg(A.x, 0, 'f', 3).arg(A.y, 0, 'f', 3).arg(B.x, 0, 'f', 3).arg(B.y, 0, 'f', 3)
        .arg(f.L, 0, 'f', 3).arg(sectionAzimuthDeg(r.line), 0, 'f', 2).arg(r.line.front, 0, 'f', 2).arg(r.line.back, 0, 'f', 2)
        .arg(r.zMin + r.srs.origin.z, 0, 'f', 2).arg(r.zMax + r.srs.origin.z, 0, 'f', 2)
        .arg(last_.stats.leafNodes).arg(last_.stats.triangles).arg(last_.stats.fallbackNodes)
        .arg(r.rawSegments).arg(r.profile.size()).arg(nc).arg(nv)
        .arg(last_.image.img.w).arg(last_.image.img.h).arg(mm(last_.image.res))
        .arg(int(last_.msCollect)).arg(int(last_.msCut)).arg(int(last_.msImage))
        .arg(qs(r.srs.describe().horizontalLabelKo())).arg(r.srs.origin.x, 0, 'f', 3).arg(r.srs.origin.y, 0, 'f', 3).arg(r.srs.origin.z, 0, 'f', 3);
    QMessageBox mb(this); mb.setWindowTitle(QStringLiteral("단면 정보 (Section Info)")); mb.setTextFormat(Qt::RichText); mb.setText(t); mb.exec();
}

// 여는 화면 「T를 판다」(Trench, kOpeningSplashMs = 1.0 초 — 캔버스 「여는 화면」 화판): 크림 타일에 지표선이 서고 → 트렌치가 파 내려가고 → 빨간 단면선이 벽면을 따라 그어지고
// → 「rench」 가 들어와 Trench 가 완성된다. t 0~1 = 전체 길이. 시간표(t): 타일 0–.14 · 지표선 .02–.14 · 파기 .12–.50 · 단면선 .45–.85 · 「rench」 .55–.80 · 부제 .80–1.0
// (모두 OutCubic). 값은 960×600 기준을 k = 너비/960 으로 키운다. 끝 장면(t ≥ .85)의 타일은 앱 아이콘 그림(kerf::appPixmap) 그대로 — 아이콘과 한 SVG 에서 나온다.
void MainWindow::paintOpeningSplash(QPainter& p, const QSize& sz, double t) {
    t = std::clamp(t, 0.0, 1.0);
    // 그림 움직임은 전체 5 초 중 앞 2 초(tm 0→1)에 끝나고, 그 뒤는 끝 장면 + 저작권 줄을 보인다. 진행 막대·상태 글의 시계는 t(실제 시간)
    const double tm = std::min(1.0, t * (double(motion::kSplash) / 2000.0));
    auto seg = [&](double a, double b) { double u = std::clamp((tm - a) / (b - a), 0.0, 1.0); return 1 - std::pow(1 - u, 3.0); };   // OutCubic
    const QRect im(QPoint(0, 0), sz.isEmpty() ? QSize(960, 600) : sz);
    p.fillRect(im, theme::Ground);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);
    const double k = im.width() / 960.0;
    const double tileIn = seg(0.0, 0.14), ground = seg(0.02, 0.14), dig = seg(0.12, 0.50), cut = seg(0.45, 0.85), word = seg(0.55, 0.80), sub = seg(0.80, 1.0);

    // 1) 타일 160 px at (110, 180): 위로 10 px 떠오르며 나타남
    const double side = 160 * k;
    const QRectF tile(110 * k, 180 * k + (1 - tileIn) * 10 * k, side, side);
    p.setOpacity(tileIn);
    if (tm >= 0.85) {   // 끝 장면 = 앱 아이콘 그대로
        QPixmap ic = kerf::appPixmap(int(std::lround(side)), p.device()->devicePixelRatioF());
        p.drawPixmap(tile.topLeft(), ic);
    } else {
        const double u = side / 24.0;   // 24 격자(아이콘 SVG 와 같은 좌표)
        auto X = [&](double g) { return tile.left() + g * u; };
        auto Y = [&](double g) { return tile.top() + g * u; };
        p.save();
        QPainterPath rr; rr.addRoundedRect(tile, side * 0.23, side * 0.23);
        p.setClipPath(rr);
        p.fillRect(tile, theme::Ground);
        struct Band { double y0, y1; QColor c; };
        const Band bands[] = {{8, 12.5, QColor(0xE3, 0xDA, 0xCC)}, {12.5, 16.5, QColor(0xD9, 0xB9, 0x9B)}, {16.5, 21, QColor(0xB5, 0x57, 0x3A)}, {21, 24, QColor(0x8F, 0x42, 0x29)}};
        for (const Band& b : bands) p.fillRect(QRectF(tile.left(), Y(b.y0), side, (b.y1 - b.y0) * u), b.c);
        p.fillRect(QRectF(X(9), Y(8), 6 * u, 11 * u * dig), theme::Ground);   // 파 내려간 자리(크림)가 위에서 아래로 자란다
        p.setPen(QPen(theme::Hand, std::max(1.0, 0.3 * u), Qt::SolidLine, Qt::FlatCap));   // 지표선(왼쪽에서 오른쪽으로)
        p.drawLine(QPointF(X(0), Y(8)), QPointF(X(24 * ground), Y(8)));
        // 빨간 단면선: 지표 → 슬롯 벽 → 바닥 → 벽 → 지표(길이 9 · 11 · 6 · 11 · 9 = 46 격자)를 앞에서부터 cut 만큼
        const QPointF pts[] = {{0, 8}, {9, 8}, {9, 19}, {15, 19}, {15, 8}, {24, 8}};
        double left = cut * 46.0;
        QPainterPath cp; cp.moveTo(X(pts[0].x()), Y(pts[0].y()));
        for (int i = 1; i < 6 && left > 0; ++i) {
            const QPointF a = pts[i - 1], b = pts[i];
            const double len = std::hypot(b.x() - a.x(), b.y() - a.y()), f = std::min(1.0, left / len);
            cp.lineTo(X(a.x() + (b.x() - a.x()) * f), Y(a.y() + (b.y() - a.y()) * f));
            left -= len;
        }
        if (cut > 0) { p.setPen(QPen(theme::SectionRed, 2.2 * u, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin)); p.setBrush(Qt::NoBrush); p.drawPath(cp); }
        p.restore();
        const double rw = std::max(1.0, side / 128.0);   // 안쪽 가는 테(앱 아이콘과 같음)
        p.setPen(QPen(QColor(20, 20, 19, 31), rw)); p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(QRectF(tile.left() + rw / 2, tile.top() + rw / 2, side - rw, side - rw), side * 0.23 - rw / 2, side * 0.23 - rw / 2);
    }
    p.setOpacity(1.0);

    // 2) 「rench」: 명조(Batang → Noto Serif KR), 「h」 키가 타일 높이와 같게 · 기준선 = 타일 아래 모서리 · 오른쪽 여백 48 안에 들어오게(글꼴이 넓으면 줄임). 왼쪽에서 24 px 들어옴
    if (word > 0) {
        QFont f = theme::uiFont(230);
        f.setFamilies({QStringLiteral("Batang"), QStringLiteral("Noto Serif KR"), QStringLiteral("Malgun Gothic")});
        f.setPixelSize(std::max(8, int(std::lround(230 * k))));
        const double hh = QFontMetricsF(f).tightBoundingRect(QStringLiteral("h")).height();
        if (hh > 0) f.setPixelSize(std::max(8, int(std::lround(f.pixelSize() * side / hh))));
        const double availW = (960 - 48 - 286) * k, textW = QFontMetricsF(f).horizontalAdvance(QStringLiteral("rench"));
        if (textW > availW) f.setPixelSize(std::max(8, int(std::lround(f.pixelSize() * availW / textW))));
        p.setOpacity(word);
        p.setFont(f); p.setPen(theme::Ink);
        p.drawText(QPointF(286 * k - (1 - word) * 24 * k, tile.top() + side), QStringLiteral("rench"));
        p.setOpacity(1.0);
    }
    // 3) 부제(마지막 .2 초에 나타남)
    if (sub > 0) {
        p.setOpacity(sub);
        p.setFont(theme::uiFont(int(std::lround(16 * k))));
        p.setPen(theme::Muted);
        p.drawText(QRectF(110 * k, 392 * k, 780 * k, 24 * k), Qt::AlignLeft | Qt::AlignVCenter,
                   QStringLiteral("발굴 평·단면 · 3MX · 3SM · OBJ 실사 메시 · %1").arg(QString::fromUtf8(kVersion)));
        // 저작권 · 가져다 쓴 외부 구성요소(사용자 요청 2026-10-11)
        p.setFont(theme::uiFont(int(std::lround(12 * k))));
        p.setPen(theme::Faint);
        p.drawText(QRectF(110 * k, 420 * k, 800 * k, 20 * k), Qt::AlignLeft | Qt::AlignVCenter, copyrightLine() + QStringLiteral(" · 모든 권리 보유"));
        p.drawText(QRectF(110 * k, 440 * k, 800 * k, 20 * k), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("사용 구성요소: ") + thirdPartyLine());
        p.setOpacity(1.0);
    }
    // 4) 아래 기반암 띠 + 상태 글(.30 · .85 에서 바뀜) + 진행 막대(맨 위 3 px, 1 초 시계)
    p.fillRect(QRectF(0, im.height() - 72 * k, im.width(), 72 * k), QColor(0x26, 0x26, 0x24));
    p.setPen(theme::Ground);
    p.setFont(theme::uiFont(int(std::lround(14 * k))));
    const QString status = tm >= 0.85 ? QStringLiteral("준비 완료") : tm >= 0.30 ? QStringLiteral("잠시 후 화면이 열립니다") : QStringLiteral("앱을 준비하고 있습니다");
    p.drawText(QRectF(48 * k, im.height() - 64 * k, 500 * k, 24 * k), Qt::AlignLeft | Qt::AlignVCenter, status);
    p.setPen(theme::Edge);
    p.setFont(theme::uiFont(int(std::lround(12 * k))));
    p.drawText(QRectF(48 * k, im.height() - 38 * k, 700 * k, 22 * k), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("원본 파일은 바꾸지 않습니다 · Esc = 건너뜀"));
    p.drawText(QRectF(im.width() - 420 * k, im.height() - 38 * k, 372 * k, 22 * k), Qt::AlignRight | Qt::AlignVCenter, copyrightLine() + QStringLiteral(" · Trench · Qt %1").arg(QString::fromLatin1(qVersion()).section(QLatin1Char('.'), 0, 1)));
    p.fillRect(QRectF(0, im.height() - 72 * k, im.width() * t, 3 * k), theme::Action);
}

QImage MainWindow::renderOpeningSplash(const QSize& size, double t) {
    QImage im(size.isEmpty() ? QSize(960, 600) : size, QImage::Format_RGB32);
    QPainter p(&im);
    paintOpeningSplash(p, im.size(), t);
    return im;
}

void MainWindow::dlgAbout() {
    QMessageBox mb(this);
    mb.setWindowTitle(QStringLiteral("프로그램 정보"));   // Qt 가 「 - Trench x.y.z」를 붙임
    mb.setIconPixmap(windowIcon().pixmap(64, 64));
    mb.setTextFormat(Qt::RichText);
    mb.setText(QStringLiteral("<b>Trench</b> v%1 · 발굴 평·단면<br>3MX · 3SM 실사 메시 단면 · 10 cm 레벨선 · DXF / GeoTIFF / 점군 내보내기<br>"
                              "%2 · 모든 권리 보유<br><br>"
                              "<small><b>가져다 쓴 외부 구성요소</b><br>"
                              "· Qt %3 — The Qt Company, LGPL-3.0 (수정 없이 동적 링크)<br>"
                              "· OpenCTM 1.0.3 — Marcus Geelnard, zlib 라이선스 (LZMA SDK 공개 도메인 포함)<br>"
                              "· SQLite %4 — public domain (3SM 읽기)<br>"
                              "· stb_image / stb_image_write — Sean Barrett, 공개 도메인 또는 MIT<br>"
                              "· nlohmann/json 3.11.3 — Niels Lohmann, MIT<br>"
                              "· Catch2 3.7.1 — BSL-1.0 (시험 전용, 배포에 포함되지 않음)<br>"
                              "3MX 는 Bentley 가 공개한 형식 사양에 따라, 3SM 은 파일 구조를 직접 확인해 읽습니다. Bentley Systems 와 관계가 없으며 그 코드·아이콘·로고를 쓰지 않습니다.<br>"
                              "전문은 설치 폴더 licenses/THIRD_PARTY.txt</small>").arg(kVersion, copyrightLine(), qVersion(), QString::fromLatin1(SQLITE_VERSION)));
    mb.exec();
}

// ---------------------------------------------------------------- 좌표계 표시(늘 수평 + 높이 기준, 경고는 노란 띠)
void MainWindow::applySrsReport() {
    const SrsReport& r = srsReport_;
    bool warn = !r.warnings.empty();
    // 상태줄: 수평 좌표계만 짧게(높이는 옆 배지). 경고는 한 줄 알림 띠(「자세히」에 전체)
    const SrsDesc& d = r.desc;
    srsLabel_->setText(qs(d.horizontalLabelKo()));   // 「수평 EPSG:5187 · 동부원점(GRS80)」 — 표시만(테두리 · ▾ 없음, 높이 글 없음 — T8)
    srsLabel_->setToolTip(qs(r.horizontalTooltipKo));
    srsLabel_->setProperty("state", !d.known() ? "error" : (warn ? "warn" : "ok"));
    srsLabel_->style()->unpolish(srsLabel_); srsLabel_->style()->polish(srsLabel_);
    if (warn) {
        QString first = qs(r.warnings.front()).section('\n', 0, 0);
        if (int dot = first.indexOf(QStringLiteral("니다.")); dot > 0) first = first.left(dot + 3);   // 첫 문장만(전체는 「자세히」)
        noticeText_->setText(QStringLiteral("<b>%1</b> &nbsp;%2%3").arg(d.known() ? QStringLiteral("좌표계 확인") : QStringLiteral("좌표계 없음"),
            first.toHtmlEscaped(), r.warnings.size() > 1 ? QStringLiteral(" <span style='color:#5E5D59'>(외 %1건)</span>").arg(r.warnings.size() - 1) : QString()));
        noticeText_->setToolTip(qs(r.horizontalTooltipKo));
        notice_->setProperty("level", d.known() ? "caution" : "block");
        notice_->style()->unpolish(notice_); notice_->style()->polish(notice_);
    }
    notice_->setVisible(warn);
    updateHeader();
}

// ---------------------------------------------------------------- 좌표계 다시 점검
void MainWindow::refreshSrs() {
    if (!src_) return;
    srsReport_ = analyzeSrs(src_->srs, sceneBounds_);
    for (auto& w : sceneWarnings_) srsReport_.warnings.push_back(w);   // 병합 3MX 레이어 경고 등
    applySrsReport();
}

// ---------------------------------------------------------------- 커서 정밀 Z(잎 메시, CPU double)
void MainWindow::requestPick(const QPointF& screen) {
    if (!src_) return;
    Vec3 o, d;
    if (!plan_->screenRayLocal(screen, o, d)) return;
    auto src = src_;
    pickWorker_->submit(false, [this, src, o, d](uint64_t gen, bool, const std::atomic<bool>* cancel) {
        auto r = std::make_shared<PickResult>();
        std::string err;
        bool ok = pickRay(*src, o, d, *r, &err, cancel);
        if (cancel->load() || !ok) return;
        QMetaObject::invokeMethod(this, [this, r, gen] {
            if (gen <= pickFloorGen_ || !pickGate_.accept(gen)) return;   // 커서가 떠났거나 더 새 결과가 있음
            if (!r->hit) { cz_->setText(QStringLiteral("—")); showZSource(ZSource::None, QStringLiteral("커서 아래에 표면이 없습니다")); return; }
            cx_->setText(QString::number(r->world.x, 'f', 3)); cy_->setText(QString::number(r->world.y, 'f', 3)); cz_->setText(QString::number(r->world.z, 'f', 3));
            showZSource(r->source, QStringLiteral("잎 타일 %1 · 삼각형 %2 검사 · %3 ms%4")
                                       .arg(r->stats.leafNodes).arg(r->trianglesTested).arg(r->ms, 0, 'f', 1)
                                       .arg(r->stats.fallbackNodes ? QStringLiteral(" · 상위 LOD 대체 %1").arg(r->stats.fallbackNodes) : QString()));
        }, Qt::QueuedConnection);
    });
}

void MainWindow::showZSource(ZSource s, const QString& detail) {
    const char* shortK = s == ZSource::LeafSurface ? "잎 표면" : s == ZSource::LeafWithFallback ? "표면·일부 대체" : s == ZSource::Coarse ? "대략" : s == ZSource::SectionCursor ? "단면 커서" : "—";
    zSrc_->setText(s == ZSource::None ? QStringLiteral("—") : QString::fromUtf8(shortK));
    bool coarse = s == ZSource::Coarse || s == ZSource::LeafWithFallback;
    zSrc_->setProperty("coarse", coarse);
    zSrc_->style()->unpolish(zSrc_); zSrc_->style()->polish(zSrc_);
    QString tip = QStringLiteral("Z 출처: %1 (모델 값 그대로, 변환 없음)").arg(qs(zSourceKo(s)));
    if (!detail.isEmpty()) tip += "\n" + detail;
    if (srsReport_.precisionWarning) tip += QStringLiteral("\n⚠ 로컬 좌표가 커서 메시 좌표 간격이 약 %1 mm 입니다(float32).").arg(srsReport_.float32StepMm, 0, 'f', 1);
    zSrc_->setToolTip(tip);
}

QString MainWindow::cursorText() const {
    return QStringLiteral("X=%1 Y=%2 Z=%3 [Z: %4]").arg(cx_->text(), cy_->text(), cz_->text(), zSrc_->text());
}
