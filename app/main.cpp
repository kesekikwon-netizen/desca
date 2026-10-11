// Trench(발굴 평·단면) 진입점. 명령줄 자동화(화면 캡처·내보내기 시험용):
//   SectionViewer [파일.3mx|.3sm|.obj] [--line AX AY BX BY] [--local] [--front m] [--back m] [--size WxH] [--tab N]
//                 [--shot out.png] [--export-png f] [--export-tiff f] [--export-geotiff f] [--export-dxf f] [--dxf3d]
//                 [--export-plan f] [--plan-view] [--export-xyz f] [--export-las f] [--area whole|band|view] [--spacing m] [--norgb]
//                 [--scale N] [--dpi N] [--log f] [--perf-log f.csv] [--pick X Y]... [--hover] [--quit]
//   --wheel-test: 평면·단면 휠 확대/축소(커서 고정 오차 px, 애니메이션 시간, 프레임) + 레벨선 간격 기록
//   --depth-fade on|off: 입면 깊이 음영(기본 설정값)
//   --pick X Y: 그 실좌표(--local 이면 로컬)에서 잎 메시 연직 정밀 피킹 → 로그(Z, 출처, 시간)
//   --hover: 평면 보기 가운데로 마우스 이동을 흉내 → 좌표줄 Z 와 Z 출처(대략 → 잎 표면)를 로그
//   --perf-log: 평면 보기 카메라 경로(맞춤→확대→이동→축소)를 재생하며 프레임마다 시간·LOD 상태를 CSV 로 기록
//   1.2: --settings DIR(설정을 DIR 의 INI 로 — 시험용) --export-pdf f / --export-sheet-png f / --export-sheet-tiff f / --export-sheet-dxf f
//        [--paper A4L|A4P|A3L|A3P] [--sheet-scale N] [--split] --export-dialog-shot f --undo-test --ctx-shot f --ui-audit DIR --icon-sheet f.png --cursor-sheet f.png
//        --extra-line AX AY BX BY(단면 목록에 더함, 반복 가능) --start-shot f(파일 없이: 시작 화면)
//   --cut-check: 잘린 면 정확도(단면선 윗면 vs 잎 연직 피킹, 미리보기 vs 최종) + 단면선이 배경 위에 순수 빨강으로 그려졌는지(화면·내보내기)
//   --vex N: 단면 화면 세로 과장(1·2·5·10, 화면만) / 창 제목 중복 검사는 항상 로그(window-title=…)
//   --plan-cam X Y mpp: 캡처 전에 평면 카메라를 실좌표 중심·m/px 로(평면-단면 정합 확인)
//   --open-sheet section 과 함께: --sheet-back m(도면 탭에서 뒤 깊이 변경 → 조판이 따라오는지) ·
//     --sheet-stress(불투명도 끌기 · 뒤 깊이 연타 · 단면 탭에서 변경 · 목록에서 다른 단면 → 조판 미리보기 계산이 쌓이지 않는지, 실패 종료 코드 12)
//   .trench.json 을 파일로 주면 그 프로젝트의 모델을 열고 상태를 되살림(로그 project:)
#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QPixmap>
#include <QPainter>
#include <algorithm>
#include <QDialog>
#include <QSettings>
#include <QFileInfo>
#include <QStyleHints>
#include <array>
#include <memory>
#include <QFile>
#include <QScreen>
#include <QFontDatabase>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QTimer>
#include <QPushButton>
#include <QSpinBox>
#include <QDir>
#include <QGuiApplication>
#include <QEventLoop>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QSlider>
#include <QTabBar>
#include <QDoubleSpinBox>
#include "asec/pick.hpp"
#include "mainwindow.hpp"
#include "guideband.hpp"
#include "cursors.hpp"
#include "motion.hpp"
#include "theme.hpp"

using namespace asec;
extern const char* const kVersion;

static void processFor(int ms) {
    auto t0 = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(ms)) { QApplication::processEvents(QEventLoop::AllEvents, 20); }
}

// --cut-check: 잘린 면이 정확한지(잎 피킹 대비 mm), 미리보기(거친 배경)에서도 단면선은 잎인지, 단면선이 배경 위에 그려지는지
template <class LogFn>
static void runCutCheck(MainWindow& w, LogFn log) {
    auto src = w.source();
    const SectionDoc d = w.sectionDoc();
    const SectionResult& r = d.r;
    const Vec3 o = r.srs.origin;
    const SectionFrame fr(r.line);
    // 1) 1 cm 간격 s 마다: 단면선(모든 선분 보간)의 가장 높은 점 vs 그 XY 의 잎 메시 연직 피킹(윗면). 닫힌 고리(덤불 수관) 포함
    std::vector<double> diff; size_t steep = 0;
    for (double sx = 0.005; sx < fr.L; sx += 0.01) {
        double zTop = -1e300, slope = 0;
        for (auto& pl : r.profile)
            for (size_t i = 1; i < pl.size(); ++i) {
                const Vec2 a = pl[i - 1], b = pl[i];
                const double lo = std::min(a.x, b.x), hi = std::max(a.x, b.x);
                if (sx < lo || sx > hi || hi - lo < 1e-9) continue;
                const double z = a.y + (b.y - a.y) * (sx - a.x) / (b.x - a.x);
                if (z > zTop) { zTop = z; slope = std::fabs((b.y - a.y) / (b.x - a.x)); }
            }
        if (zTop < -1e299) continue;
        if (slope > 3) { ++steep; continue; }   // 거의 수직인 벽: 연직 피킹으로 비교할 수 없음
        const Vec2 xy = fr.planXY(sx, 0);
        PickResult pr; std::string e;
        if (pickVertical(*src, xy.x, xy.y, pr, &e) && pr.hit) {
            const double dd = std::fabs(pr.local.z - zTop);
            diff.push_back(dd);
            if (dd > 0.005) log(QStringLiteral("cut-accuracy outlier: s=%1 profile-top=%2 pick=%3 (Δ %4 mm)").arg(sx, 0, 'f', 3).arg(zTop + o.z, 0, 'f', 3).arg(pr.world.z, 0, 'f', 3).arg(dd * 1000, 0, 'f', 1));
        }
    }
    std::sort(diff.begin(), diff.end());
    auto q = [&](double f) { return diff.empty() ? 0.0 : diff[size_t(f * (diff.size() - 1))]; };
    size_t n5 = 0, n1 = 0; for (double x : diff) { n5 += x <= 0.005; n1 += x <= 0.001; }
    std::vector<double> in5(diff.begin(), diff.begin() + n5);   // 5 mm 안 표본(이탈은 위에 하나씩 기록)
    auto q5 = [&](double f) { return in5.empty() ? 0.0 : in5[size_t(f * (in5.size() - 1))]; };
    log(QStringLiteral("cut-accuracy: back=%1m samples=%2@1cm (walls>3:1 skipped %3) |profile-top − leaf-pick| median=%4mm p95=%5mm p99=%6mm max=%7mm")
            .arg(r.line.back, 0, 'f', 2).arg(diff.size()).arg(steep).arg(q(0.5) * 1000, 0, 'f', 2).arg(q(0.95) * 1000, 0, 'f', 2).arg(q(0.99) * 1000, 0, 'f', 2).arg(q(1.0) * 1000, 0, 'f', 2));
    log(QStringLiteral("cut-accuracy: within1mm=%1/%2 within5mm=%3/%2 outliers>5mm=%4 max-within5mm=%5mm")
            .arg(n1).arg(diff.size()).arg(n5).arg(diff.size() - n5).arg(q5(1.0) * 1000, 0, 'f', 2));
    // 2) 미리보기(배경 거친 LOD) 단면선 = 최종(잎) 단면선?
    {
        SectionRequest rq; rq.line = r.line; rq.meshRes = std::max(0.008, fr.L / 700.0); rq.imageRes = rq.meshRes; rq.wantImage = true;
        SectionOutput pre; std::string e;
        rq.meshRes = rq.imageRes;
        if (computeSection(*src, rq, pre, &e)) {
            size_t nA = 0, nB = 0; double dmax = 0;
            for (auto& pl : r.profile) nA += pl.size();
            for (auto& pl : pre.result.profile) nB += pl.size();
            if (nA == nB) { auto ia = r.profile.begin(); for (auto& pl : pre.result.profile) { for (size_t i = 0; i < pl.size(); ++i) dmax = std::max(dmax, (pl[i] - (*ia)[i]).len()); ++ia; } }
            log(QStringLiteral("cut-preview: image-lod-depth=%1 cut-from-leaf=%2 cut-depth=%3 vertices preview=%4 final=%5 max-diff=%6mm")
                    .arg(pre.stats.maxDepth).arg(pre.cutFromLeaf ? 1 : 0).arg(pre.cutStats.maxDepth).arg(nB).arg(nA).arg(nA == nB ? QString::number(dmax * 1000, 'f', 3) : QStringLiteral("n/a")));
        }
    }
    // 3) 단면선이 배경 위(순수 빨강)인지: 화면 그림과 내보내기 그림에서 단면선 꼭짓점 자리 픽셀
    auto onTop = [&](const QImage& im, const SectionXf& xf, bool exp, int& n) {
        int red = 0; n = 0;
        const double ppmZ = exp ? xf.ppm : xf.ppmZ();
        for (auto& pl : r.profile)
            for (size_t i = 1; i + 1 < pl.size(); ++i) {
                const double x = xf.plot.left() + (pl[i].x - xf.s0) * xf.ppm, y = xf.plot.top() + (xf.zTop - (pl[i].y + o.z)) * ppmZ;
                if (!xf.plot.adjusted(2, 2, -2, -2).contains(QPointF(x, y))) continue;
                ++n; bool hit = false;
                for (int dy = -1; dy <= 1 && !hit; ++dy) for (int dx = -1; dx <= 1 && !hit; ++dx) {
                    QRgb c = im.pixel(int(x) + dx, int(y) + dy);
                    hit = qRed(c) >= 250 && qGreen(c) <= 8 && qBlue(c) <= 8;
                }
                red += hit;
            }
        return red;
    };
    {
        SectionView* sv = w.sectionView();
        QImage im = sv->grab().toImage().convertToFormat(QImage::Format_RGB32);
        const double dpr = im.devicePixelRatio();
        SectionXf xf = sv->xf();
        if (dpr != 1.0) { xf.plot = QRectF(xf.plot.topLeft() * dpr, xf.plot.size() * dpr); xf.ppm *= dpr; }
        int n = 0; int red = onTop(im, xf, false, n);
        log(QStringLiteral("cut-on-top screen: vertices=%1 pure-red=%2 (%3%)").arg(n).arg(red).arg(n ? 100.0 * red / n : 0, 0, 'f', 1));
        SectionXf ex; const double ppm = 1000.0 / 40 / 25.4 * 150;   // 1:40, 150 dpi
        QSize sz = sectionExportLayout(d, ppm, 150 / 96.0, ex);
        QImage img(sz, QImage::Format_RGB32); img.fill(Qt::white);
        { QPainter p(&img); SectionImgGeo g{d.imgS0, d.imgZ1, d.imgRes}; paintSectionDoc(p, d, QRectF(QPointF(0, 0), QSizeF(sz)), ex, 150 / 96.0, d.img, true, QString(), &g); }
        int n2 = 0; int red2 = onTop(img, ex, true, n2);
        log(QStringLiteral("cut-on-top export(1:40,150dpi): vertices=%1 pure-red=%2 (%3%) image=%4x%5").arg(n2).arg(red2).arg(n2 ? 100.0 * red2 / n2 : 0, 0, 'f', 1).arg(sz.width()).arg(sz.height()));
    }
}

// 여는 화면 창: 테두리 없는 맨 위 창, 화면 가운데. 전체 길이 MainWindow::kOpeningSplashMs(1 초). Esc · 클릭 = 건너뜀(끝 장면을 0.3 초 보이고 열림).
// 시스템 애니메이션이 꺼져 있으면 처음부터 끝 장면 0.3 초
class OpeningSplash : public QWidget {
public:
    OpeningSplash() : QWidget(nullptr, Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint) {
        setFixedSize(960, 600);
        setAttribute(Qt::WA_OpaquePaintEvent);
        if (QScreen* sc = QGuiApplication::primaryScreen()) move(sc->availableGeometry().center() - rect().center());
    }
    // captureDir 이 있으면 장면 시각(120 · 400 · 700 · 1000 ms)마다 그 창을 직접 찍는다(시험용 --splash-live)
    void run(int ms, const QString& captureDir = QString(), int escAtMs = -1) {   // escAtMs ≥ 0 = 그 시각에 Esc 를 눌러 건너뛰기 시험(--splash-esc)
        total_ = ms;
        if (!motion::animationsOn()) { finalOnly_ = true; skipMs_ = 300; skipClock_.start(); }
        const int caps[] = {motion::kSplash * 12 / 100, motion::kSplash * 40 / 100, motion::kSplash * 70 / 100, motion::kSplash};   // 장면 시각 = 전체 길이의 .12 · .40 · .70 · 1.0
        int nextCap = 0;
        clock_.start();
        if (escAtMs >= 0) QTimer::singleShot(escAtMs, this, [this] { QKeyEvent ev(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier); keyPressEvent(&ev); });
        show(); raise(); activateWindow();
        QTimer tick; tick.setInterval(16);
        QEventLoop loop;
        QObject::connect(&tick, &QTimer::timeout, this, [&] {
            repaint();
            const qint64 e = clock_.elapsed();
            if (!captureDir.isEmpty() && nextCap < 4 && e >= caps[nextCap]) {
                grab().save(QStringLiteral("%1/splash_live_%2ms.png").arg(captureDir).arg(caps[nextCap]));
                fprintf(stdout, "splash-live %d ms (t=%.2f) visible=%d pos=%d,%d\n", int(e), std::min(1.0, e / double(total_)), isVisible() ? 1 : 0, x(), y());
                fflush(stdout);
                ++nextCap;
            }
            const bool capturesDone = captureDir.isEmpty() || nextCap >= 4;
            if (skipMs_ >= 0) { if (skipClock_.elapsed() >= skipMs_) loop.quit(); }
            else if (e >= total_ && capturesDone) loop.quit();
        });
        tick.start();
        loop.exec();
        tick.stop();
        close();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        const double t = (finalOnly_ || skipMs_ >= 0) ? 1.0 : (total_ > 0 ? std::min(1.0, clock_.elapsed() / double(total_)) : 1.0);
        MainWindow::paintOpeningSplash(p, size(), t);
    }
    void keyPressEvent(QKeyEvent* e) override { if (e->key() == Qt::Key_Escape) skip(); else QWidget::keyPressEvent(e); }
    void mousePressEvent(QMouseEvent*) override { skip(); }
private:
    void skip() { if (skipMs_ < 0) { skipMs_ = 300; skipClock_.start(); } }
    QElapsedTimer clock_, skipClock_;
    int total_ = MainWindow::kOpeningSplashMs;
    int skipMs_ = -1;          // ≥ 0 = 끝 장면만 보이는 중(건너뜀 · 애니메이션 끔) — 이 시간(ms)이 지나면 닫힘
    bool finalOnly_ = false;
};

#ifdef _WIN32
// 그래픽이 두 개인 노트북(내장 + NVIDIA/AMD)에서 성능 좋은 쪽을 쓰게 하는 드라이버 약속. 실행 파일이 이 이름으로 내보내야 드라이버가 읽는다
// (사용자 질문 2026-10-11 「포터블은 어느 컴퓨터에서도 그곳의 GPU 를 쓰나」)
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

int main(int argc, char** argv) {
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--settings") {   // 시험용: 사용자 설정을 건드리지 않게
            QSettings::setDefaultFormat(QSettings::IniFormat);
            QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, QString::fromLocal8Bit(argv[i + 1]));
        }
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QSurfaceFormat fmt; fmt.setSamples(4); fmt.setDepthBufferSize(24); QSurfaceFormat::setDefaultFormat(fmt);
    QApplication app(argc, argv);
    {   // 포터블(사용자 요청 2026-10-11): exe 옆에 portable.txt 가 있으면 설정을 레지스트리 대신 exe 옆 settings\ 에(폴더째 옮겨도 따라감). --settings 가 우선
        bool haveSettingsArg = false;
        for (int i = 1; i + 1 < argc; ++i) if (std::string(argv[i]) == "--settings") { haveSettingsArg = true; MainWindow::projectRootOverride = QString::fromLocal8Bit(argv[i + 1]); }
        const QString appDir = QCoreApplication::applicationDirPath();
        if (!haveSettingsArg && QFileInfo::exists(appDir + "/portable.txt")) {
            QSettings::setDefaultFormat(QSettings::IniFormat);
            QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, appDir + "/settings");
        }
    }
    QApplication::setOrganizationName("ExcavSection");
    QApplication::setApplicationName("SectionViewer");
    // 표시 이름 = 창 제목 끝과 같게(「… — Trench x.y.z」). Qt(Windows·X11)는 제목이 표시 이름으로 끝나지 않으면
    // 「 - 표시 이름」을 덧붙여 앱 이름이 두 번 보였음(1.2.0 신고)
    QApplication::setApplicationDisplayName(QStringLiteral("Trench %1").arg(QString::fromUtf8(kVersion)));
    QFont f = theme::uiFont(13); f.setStyleStrategy(QFont::PreferAntialias);   // 한글 본문 13 px
    QApplication::setFont(f);
    app.styleHints()->setColorScheme(Qt::ColorScheme::Light);   // 앱은 늘 밝은 색 체계(T9 · Qt 6.8+) — 윈도우 어두운 모드에서 창 · 표 바탕이 어두워 글이 안 보이던 버그
    app.setStyle("Fusion");
    app.setStyleSheet(theme::styleSheet(QSettings().value("ui/highContrast", false).toBool()));
    app.setWindowIcon(kerf::appIcon());   // Trench 앱 아이콘 — 같은 SVG(app/icons/app/)에서 .ico 도 만든다(T2)

    QStringList a = app.arguments();
    QString splashLive;   // --splash-live DIR: 실제 여는 화면 창을 1 초 돌리며 장면 시각(120 · 400 · 700 · 1000 ms)마다 찍음
    int sheetWheel = 0;
    int sheetPick = -1;   // --sheet-pick N: 조판 탭 왼쪽 도면 목록 N 번째 줄을 누름(0 = 평면도)
    bool sheetEdit = false;   // --sheet-edit: 휠 전에 「조판편집」 단추를 누름   // --sheet-wheel N: 조판 탭 미리보기에 실제 휠 N 칸(확대 +, 축소 -)
    bool sheetStress = false; // --sheet-stress: 도면 탭을 연 채 불투명도 끌기 · 뒤 깊이 연타 · 단면 탭에서 바꾸기 → 조판 미리보기 계산이 쌓이지 않는지(종료 코드 12)
    double sheetBack = -1;    // --sheet-back m: 도면 탭이 열린 채 리본 「뒤」 깊이를 바꿔 단면 재계산 → 조판이 따라오는지 점검(2026-10-11)
    QString file, shot, logPath, perfPath, depthFade, dialogShot, ctxShot, paperArg, startShot, sheetCheck, splashShot, openSheet, uiAuditDir, sheetBatch;
    QString iconSheetPng; bool iconSheet = false;
    int splashEsc = -1;   // --splash-esc N: 여는 화면을 띄우고 N ms 에 Esc 를 눌러 건너뛰는지 잰다(종료 6 = 늦게 닫힘)
    QString cursorSheetPng;   // --cursor-sheet f.png: 도구 커서 전부 × 배율 1 · 2 · 3 을 격자 PNG 로 쓰고 눈 검사(실패 종료 코드 11)
    QString appIconDir;   // --app-icon DIR: 앱 아이콘 PNG 다섯 장을 쓰고 눈 검사(실패 종료 코드 12)
    bool undoTest = false, split = false; double sheetScale = 0, sectionScale = 0;
    QString lodShot;
    double camX = 0, camY = 0, camMpp = 0, vexArg = 0, pitchArg = -1;
    bool cutCheck = false;
    std::vector<std::array<double, 4>> extraLines;
    bool haveLine = false, local = false, quit = false;
    double ax = 0, ay = 0, bx = 0, by = 0, front = -1, back = -1, denom = 20, dpi = 300, spacing = 0;
    int W = 0, H = 0, tab = -1;
    bool dxf3d = false, rgb = true, planView = false, hover = false, wheelTest = false, syncCursor = false;
    std::vector<std::pair<double, double>> picks;
    QString area = "whole";
    QList<QPair<QString, QString>> exports;
    for (int i = 1; i < a.size(); ++i) {
        QString s = a[i];
        auto nx = [&]() { return i + 1 < a.size() ? a[++i] : QString(); };
        if (s == "--line" && i + 4 < a.size()) { ax = a[i + 1].toDouble(); ay = a[i + 2].toDouble(); bx = a[i + 3].toDouble(); by = a[i + 4].toDouble(); i += 4; haveLine = true; }
        else if (s == "--local") local = true;
        else if (s == "--depth-fade") depthFade = nx();
        else if (s == "--wheel-test") wheelTest = true;
        else if (s == "--front") front = nx().toDouble();
        else if (s == "--back") back = nx().toDouble();
        else if (s == "--shot") shot = nx();
        else if (s == "--size") { QStringList p = nx().split('x'); if (p.size() == 2) { W = p[0].toInt(); H = p[1].toInt(); } }
        else if (s == "--tab") tab = nx().toInt();
        else if (s == "--scale") denom = nx().toDouble();
        else if (s == "--dpi") dpi = nx().toDouble();
        else if (s == "--spacing") spacing = nx().toDouble();
        else if (s == "--area") area = nx();
        else if (s == "--dxf3d") dxf3d = true;
        else if (s == "--norgb") rgb = false;
        else if (s == "--plan-view") planView = true;
        else if (s == "--log") logPath = nx();
        else if (s == "--perf-log") perfPath = nx();
        else if (s == "--quit") quit = true;
        else if (s == "--pick" && i + 2 < a.size()) { picks.push_back({a[i + 1].toDouble(), a[i + 2].toDouble()}); i += 2; }
        else if (s == "--hover") hover = true;
        else if (s == "--sync-cursor") syncCursor = true;
        else if (s == "--vex") vexArg = nx().toDouble();
        else if (s == "--cut-check") cutCheck = true;
        else if (s == "--plan-cam" && i + 3 < a.size()) { camX = a[i + 1].toDouble(); camY = a[i + 2].toDouble(); camMpp = a[i + 3].toDouble(); i += 3; }
        else if (s == "--plan-pitch") pitchArg = nx().toDouble();
        else if (s == "--settings") nx();
        else if (s == "--export-dialog-shot") dialogShot = nx();
        else if (s == "--ctx-shot") ctxShot = nx();
        else if (s == "--app-icon") appIconDir = nx();
        else if (s == "--cursor-sheet") cursorSheetPng = nx();
        else if (s == "--splash-esc") splashEsc = nx().toInt();
        else if (s == "--icon-sheet") { iconSheet = true; iconSheetPng = nx(); }   // v5 A3: 아이콘 이름 검사 + 격자 캡처(실패 종료 코드 10)
        else if (s == "--lod-shot") lodShot = nx();
        else if (s == "--section-scale") sectionScale = nx().toDouble();
        else if (s == "--start-shot") startShot = nx();
        else if (s == "--splash-shot") splashShot = nx();
        else if (s == "--splash-live") splashLive = nx();
        else if (s == "--undo-test") undoTest = true;
        else if (s == "--ui-audit") uiAuditDir = nx();   // v4 단계 1: 겹침 · 자리 규칙 잠금 시험(DIR 에 ui-audit.txt · 캡처)
        else if (s == "--paper") paperArg = nx().toUpper();
        else if (s == "--sheet-scale") sheetScale = nx().toDouble();
        else if (s == "--sheet-check") sheetCheck = nx();
        else if (s == "--sheet-batch") sheetBatch = nx();   // v5 E4: 고른 도면 전부(평면 1 + 단면 n)를 DIR 에 SVG 로 — 수가 안 맞으면 종료 코드 9
        else if (s == "--open-sheet") openSheet = nx();
        else if (s == "--sheet-wheel") sheetWheel = nx().toInt();
        else if (s == "--sheet-edit") sheetEdit = true;
        else if (s == "--sheet-pick") sheetPick = nx().toInt();
        else if (s == "--sheet-back") sheetBack = nx().toDouble();
        else if (s == "--sheet-stress") sheetStress = true;
        else if (s == "--split") split = true;
        else if (s == "--extra-line" && i + 4 < a.size()) { extraLines.push_back({a[i + 1].toDouble(), a[i + 2].toDouble(), a[i + 3].toDouble(), a[i + 4].toDouble()}); i += 4; }
        else if (s.startsWith("--export-")) exports.append({s.mid(9), nx()});
        else if (!s.startsWith("--")) file = s;
    }

    if (!cursorSheetPng.isEmpty()) {   // 모델 · 창 없이 돈다 — 커서는 캡처에 안 찍히므로 이 PNG 를 눈으로 본다(G8)
        QString rep; const bool ok = kerf::writeCursorSheet(cursorSheetPng, &rep);
        fprintf(stdout, "%s\n", rep.toUtf8().constData());
        return ok ? 0 : 11;
    }
    if (!appIconDir.isEmpty()) {   // 모델 · 창 없이 돈다 — packaging/assets/make_app_ico.py 가 이 PNG 로 app.ico 를 만든다
        QString rep; const bool ok = kerf::writeAppIcons(appIconDir, &rep);
        fprintf(stdout, "%s\n", rep.toUtf8().constData());
        return ok ? 0 : 12;
    }
    if (splashEsc >= 0) {   // Esc 건너뜀 시험: 끝 장면 0.3 초를 보이고 닫혀야 한다(Esc 시각 + 300 ms 부근, 창 띄우는 시간 여유 300 ms)
        OpeningSplash sp; QElapsedTimer et; et.start();
        sp.run(MainWindow::kOpeningSplashMs, QString(), splashEsc);
        const qint64 ms = et.elapsed();
        const bool ok = ms >= splashEsc + 250 && ms <= splashEsc + 300 + 300;
        fprintf(stdout, "splash-esc esc-at=%d closed-ms=%lld%s\n", splashEsc, static_cast<long long>(ms), ok ? "" : "  FAIL");
        return ok ? 0 : 6;
    }
    if (!splashLive.isEmpty()) { QDir().mkpath(splashLive); OpeningSplash sp; sp.run(MainWindow::kOpeningSplashMs, splashLive); return 0; }
    if (!splashShot.isEmpty() && file.isEmpty()) {   // 장면 네 장(t = .12 · .40 · .70 · 1.0 — 끝 장면이 splashShot, 나머지는 이름 뒤 _tNN) + 길이 판정(T3: 네 장 · 1000 ms, 아니면 종료 6)
        const double times[] = {0.12, 0.40, 0.70, 1.0};
        bool ok = true; int frames = 0; QSize sz;
        for (double tt : times) {
            const QImage im = MainWindow::renderOpeningSplash(QSize(960, 600), tt);
            QString f = splashShot;
            if (tt < 1.0) f.insert(f.lastIndexOf('.') < 0 ? f.size() : f.lastIndexOf('.'), QStringLiteral("_t%1").arg(std::lround(tt * 100)));
            ok = im.save(f) && ok; ++frames; sz = im.size();
        }
        const bool timing = frames == 4 && MainWindow::kOpeningSplashMs == motion::kSplash && motion::kSplash == 5000;
        fprintf(stdout, "splash-shot %s: %s (%dx%d) frames=%d total-ms=%d%s\n", ok ? "ok" : "FAILED", splashShot.toUtf8().constData(), sz.width(), sz.height(), frames, MainWindow::kOpeningSplashMs, timing ? "" : "  FAIL");
        return ok && timing ? 0 : 6;
    }
    // 자동화(감사 · 캡처 · 내보내기 · 시험)는 늘 움직임 없이 돈다 — 시각이 결과를 흔들지 않게. 여는 화면은 이 아래에서 따로 판단한다
    motion::setForcedOff(quit || !uiAuditDir.isEmpty() || !shot.isEmpty() || !startShot.isEmpty() || !sheetCheck.isEmpty() || !exports.isEmpty() || !sheetBatch.isEmpty() || undoTest || !ctxShot.isEmpty() || !dialogShot.isEmpty() || haveLine);
    MainWindow w;
    if (W > 0 && H > 0) { w.resize(W, H); w.setProperty("auditSize", QSize(W, H)); }   // --ui-audit env: 요청한 크기로 열렸는지(B5)
    if (file.isEmpty() && startShot.isEmpty() && shot.isEmpty() && !quit) {
        // 여는 화면: 화면 가운데 맨 위에 1 초 「T를 판다」 모션(초당 60 번 다시 그림, Esc · 클릭 = 건너뜀)
        OpeningSplash splash;
        splash.run(MainWindow::kOpeningSplashMs);
    }
    w.show();
    if (file.isEmpty() && !startShot.isEmpty()) {   // 시작 화면 캡처
        processFor(800);
        QPixmap pm = w.grab();
        bool ok = pm.save(startShot);
        fprintf(stdout, "start-shot %s: %s (%dx%d)\n", ok ? "ok" : "FAILED", startShot.toUtf8().constData(), pm.width(), pm.height());
        return ok ? 0 : 6;
    }
    int rc = 0;
    QFile logF(logPath);
    bool logOk = !logPath.isEmpty() && logF.open(QIODevice::WriteOnly | QIODevice::Text);
    auto log = [&](const QString& m) {
        QByteArray b = (m + "\n").toUtf8();
        if (logOk) { logF.write(b); logF.flush(); }
        fputs(b.constData(), stdout); fflush(stdout);
    };
    if (iconSheet) {   // v5 A3: 모델 없이도 돈다 — 모든 Ico 이름에 SVG 가 있는지 + 격자 캡처
        QString rep;
        const bool ok = w.iconSheet(iconSheetPng, &rep);
        log(rep);
        if (!ok) rc = 10;
        if (file.isEmpty() && quit) return rc;
    }
    if (file.isEmpty()) return app.exec();

    bool automated = haveLine || !shot.isEmpty() || !exports.isEmpty() || quit || !perfPath.isEmpty() || !startShot.isEmpty() || !sheetCheck.isEmpty();
    if (!automated) { QTimer::singleShot(0, &w, [&] { w.openFile(file); }); return app.exec(); }

    processFor(200);
    OpenedScene sc; QString err;
    if (asec::isProjectFileName(QFileInfo(file).fileName().toStdString())) {   // 프로젝트 파일 → 그 모델을 열고 상태를 적용
        QString model;
        if (!w.stageProject(file, &model, &err, false)) { log("open-error: " + err); return 2; }
        log(QStringLiteral("project: %1 -> %2").arg(file, model));
        file = model;
    }
    auto t0 = std::chrono::steady_clock::now();
    if (!MainWindow::loadScene(file, sc, &err)) { log("open-error: " + err); return 2; }
    log(QStringLiteral("open-ok: %1 kind=%2 displayTris=%3 srs=%4 origin=%5,%6,%7 ms=%8").arg(file, sc.kind).arg(sc.displayTris)
            .arg(QString::fromStdString(sc.src->srs.shortLabel())).arg(sc.src->srs.origin.x, 0, 'f', 3).arg(sc.src->srs.origin.y, 0, 'f', 3).arg(sc.src->srs.origin.z, 0, 'f', 3)
            .arg(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(), 0, 'f', 0));
    log(QStringLiteral("layers=%1 open-warnings=%2").arg(sc.layers).arg(sc.warnings.size()));
    auto tApply = std::chrono::steady_clock::now();
    auto msSince = [](std::chrono::steady_clock::time_point t) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count(); };
    w.applyScene(std::move(sc));
    if (front >= 0 || back >= 0) w.setThickness(front >= 0 ? front : w.frontDepth(), back >= 0 ? back : w.backDepth());  // 성능 기록(단면 끌기)도 이 두께로
    if (!depthFade.isEmpty()) w.setDepthFade(depthFade != "off" && depthFade != "0");
    {
        const SrsReport& r = w.srsReport();
        QString ws;
        for (auto& x : r.warnings) ws += " | " + QString::fromStdString(x);
        log(QStringLiteral("srs-report: %1 latlon=%2,%3 maxLocalXY=%4 f32step-mm=%5 warnings=%6%7").arg(QString::fromStdString(r.labelKo))
                .arg(r.lat, 0, 'f', 6).arg(r.lon, 0, 'f', 6).arg(r.maxLocalXY, 0, 'f', 1).arg(r.float32StepMm, 0, 'f', 3).arg(r.warnings.size()).arg(ws));
    }
    {   // LOD 스트리밍: 거친 모델이 처음 보일 때까지 / 현재 시점 세부가 다 찰 때까지
        double first = -1;
        while (msSince(tApply) < 60000) {
            QApplication::processEvents(QEventLoop::AllEvents, 10);
            if (first < 0 && w.plan()->lastFrame().draw > 0) first = msSince(tApply);
            if (w.plan()->streamIdle() && w.plan()->lastFrame().draw > 0) break;
        }
        if (w.plan()->streaming())
            log(QStringLiteral("stream: first-draw-ms=%1 idle-ms=%2 nodes=%3 depth=%4 gpuMB=%5").arg(first, 0, 'f', 0).arg(msSince(tApply), 0, 'f', 0)
                    .arg(w.plan()->lastFrame().draw).arg(w.plan()->lastFrame().maxDepth).arg(w.plan()->lastFrame().gpuBytes / 1048576.0, 0, 'f', 1));
    }
    if (!perfPath.isEmpty()) {
        // 카메라 경로 재생: 맞춤(전체) → 2배씩 확대 6단계(각 20프레임) → 확대 상태로 가로 이동 60프레임 → 축소 40프레임
        QFile pf(perfPath);
        if (!pf.open(QIODevice::WriteOnly | QIODevice::Text)) { log("perf-log open failed"); rc = 5; }
        else {
            QTextStream ts(&pf);
            ts << "frame,t_ms,phase,center_x,center_y,mpp,frame_ms,paint_ms,draw_nodes,uploaded,evicted,wanted,queued,loading,gpu_mb,max_depth,idle,upload_kb,upload_ms,staging\n";
            PlanView* pv = w.plan();
            Box3 b = pv->bounds();
            double cx = b.center().x, cy = b.center().y, mpp0 = pv->metersPerPixel();
            struct Step { const char* phase; double x, y, mpp; };
            std::vector<Step> path;
            for (int i = 0; i < 20; ++i) path.push_back({"fit", cx, cy, mpp0});
            for (int z = 1; z <= 6; ++z)
                for (int i = 0; i < 20; ++i) path.push_back({"zoom", cx, cy, mpp0 / std::pow(2.0, z - 1 + (i + 1) / 20.0)});
            double mz = mpp0 / 64.0, span = (b.mx.x - b.mn.x) * 0.3;
            for (int i = 0; i < 60; ++i) path.push_back({"pan", cx - span / 2 + span * i / 59.0, cy, mz});
            for (int i = 0; i < 40; ++i) path.push_back({"zoomout", cx + span / 2, cy, mz * std::pow(64.0, (i + 1) / 40.0)});
            auto t0 = std::chrono::steady_clock::now();
            double worst = 0, sum = 0;
            for (size_t i = 0; i < path.size(); ++i) {
                auto tf = std::chrono::steady_clock::now();
                pv->setCamera(path[i].x, path[i].y, path[i].mpp);
                pv->repaint();
                QApplication::processEvents(QEventLoop::AllEvents, 5);
                double fms = msSince(tf);
                worst = std::max(worst, fms); sum += fms;
                const auto& F = pv->lastFrame();
                ts << i << "," << QString::number(msSince(t0), 'f', 2) << "," << path[i].phase << "," << QString::number(path[i].x, 'f', 3) << ","
                   << QString::number(path[i].y, 'f', 3) << "," << QString::number(path[i].mpp, 'g', 6) << "," << QString::number(fms, 'f', 3) << ","
                   << QString::number(F.ms, 'f', 3) << "," << F.draw << "," << F.uploaded << "," << F.evicted << "," << F.wanted << "," << F.queued << ","
                   << F.loading << "," << QString::number(F.gpuBytes / 1048576.0, 'f', 2) << "," << F.maxDepth << "," << (F.idle ? 1 : 0) << "," << F.uploadBytes / 1024 << "," << QString::number(F.uploadMs, 'f', 2) << "," << F.staging << "\n";
            }
            // 단면선 끌기: 60 프레임(약 16 ms 간격) 동안 선을 옮기며 미리보기 요청 → 놓음(최종). 화면 프레임 시간과 미리보기 갱신 수
            {
                double L = std::min(b.mx.x - b.mn.x, 60.0) * 0.8;
                double y0 = cy - (b.mx.y - b.mn.y) * 0.25, y1 = cy + (b.mx.y - b.mn.y) * 0.25;
                pv->setCamera(cx, cy, mpp0);
                processFor(100);
                auto c0 = w.sectionCounters();
                auto td = std::chrono::steady_clock::now();
                double dworst = 0, dsum = 0;
                for (int i = 0; i < 60; ++i) {
                    auto tf = std::chrono::steady_clock::now();
                    SectionLine l; l.a = Vec2(cx - L / 2, y0 + (y1 - y0) * i / 59.0); l.b = Vec2(cx + L / 2, y0 + (y1 - y0) * i / 59.0);
                    w.dragLine(l, i == 59);
                    pv->repaint();
                    QApplication::processEvents(QEventLoop::AllEvents, 5);
                    while (msSince(tf) < 16.0) QApplication::processEvents(QEventLoop::AllEvents, 2);
                    double fms = msSince(tf);
                    const auto& F = pv->lastFrame();
                    dworst = std::max(dworst, F.ms); dsum += F.ms;
                    ts << (path.size() + i) << "," << QString::number(msSince(t0), 'f', 2) << ",secdrag," << QString::number(cx, 'f', 3) << ","
                       << QString::number((y0 + (y1 - y0) * i / 59.0), 'f', 3) << "," << QString::number(mpp0, 'g', 6) << "," << QString::number(fms, 'f', 3) << ","
                       << QString::number(F.ms, 'f', 3) << "," << F.draw << "," << F.uploaded << "," << F.evicted << "," << F.wanted << "," << F.queued << ","
                       << F.loading << "," << QString::number(F.gpuBytes / 1048576.0, 'f', 2) << "," << F.maxDepth << "," << (F.idle ? 1 : 0) << "," << F.uploadBytes / 1024 << "," << QString::number(F.uploadMs, 'f', 2) << "," << F.staging << "\n";
                }
                double dragMs = msSince(td);
                int pv1 = w.sectionCounters().previewsShown - c0.previewsShown;
                auto tfin = std::chrono::steady_clock::now();
                while (msSince(tfin) < 30000 && w.sectionCounters().finalsShown == c0.finalsShown) QApplication::processEvents(QEventLoop::AllEvents, 5);
                log(QStringLiteral("section-drag: frames=60 drag-ms=%1 previews-shown=%2 (%3/s) paint-avg-ms=%4 paint-worst-ms=%5 final-after-release-ms=%6 final-tris=%7 final-compute-ms=%8")
                        .arg(dragMs, 0, 'f', 0).arg(pv1).arg(pv1 * 1000.0 / dragMs, 0, 'f', 1).arg(dsum / 60, 0, 'f', 2).arg(dworst, 0, 'f', 2)
                        .arg(msSince(tfin), 0, 'f', 0).arg(w.sectionCounters().lastTris).arg(w.sectionCounters().lastMs, 0, 'f', 0));
            }
            ts.flush();
            log(QStringLiteral("perf: frames=%1 avg-ms=%2 worst-ms=%3 csv=%4 (GL=%5)").arg(path.size()).arg(sum / path.size(), 0, 'f', 2).arg(worst, 0, 'f', 2)
                    .arg(perfPath, QString::fromLatin1(reinterpret_cast<const char*>(pv->glRenderer().constData()))));
        }
    }
    for (auto& pk : picks) {
        auto src = w.source();
        Vec3 o = local ? Vec3() : w.srs().origin;
        double lx = pk.first - o.x, ly = pk.second - o.y;
        PickResult r; std::string e;
        bool ok = pickVertical(*src, lx, ly, r, &e);
        // 비교: 거친 LOD(0.5 m/px 에 충분한 단계)로 읽은 높이
        double coarseZ = std::nan("");
        {
            std::vector<MeshPtr> ms; LeafStats st;
            SectionLine sl; sl.a = Vec2(lx - 0.02, ly); sl.b = Vec2(lx + 0.02, ly); sl.front = sl.back = 0.02;
            if (src->bandMeshes(sectionBand(sl), 0.5, ms, &st, &e, nullptr)) {
                double t = 1e300;
                if (rayMeshes(ms, Vec3(lx, ly, src->bounds.mx.z + 10), Vec3(0, 0, -1), 0, t)) coarseZ = src->bounds.mx.z + 10 - t + src->srs.origin.z;
            }
        }
        if (!ok) log("pick-error: " + QString::fromStdString(e));
        else if (!r.hit) log(QStringLiteral("pick %1 %2: no surface").arg(pk.first, 0, 'f', 3).arg(pk.second, 0, 'f', 3));
        else log(QStringLiteral("pick %1 %2: Z=%3 source=%4 leafTiles=%5 depth=%6 tris=%7 ms=%8 coarse0.5m-Z=%9 (diff %10 mm)")
                     .arg(r.world.x, 0, 'f', 3).arg(r.world.y, 0, 'f', 3).arg(r.world.z, 0, 'f', 4).arg(QString::fromUtf8(zSourceKo(r.source)))
                     .arg(r.stats.leafNodes).arg(r.stats.maxDepth).arg(r.trianglesTested).arg(r.ms, 0, 'f', 1).arg(coarseZ, 0, 'f', 4)
                     .arg((coarseZ - r.world.z) * 1000, 0, 'f', 1));
    }
    if (hover) {
        PlanView* pv = w.plan();
        QPointF c(pv->width() / 2.0, pv->height() / 2.0);
        QMouseEvent ev(QEvent::MouseMove, c, pv->mapToGlobal(c), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(pv, &ev);
        log("hover-immediate: " + w.cursorText());
        auto th = std::chrono::steady_clock::now();
        while (msSince(th) < 10000 && !w.cursorText().contains(QStringLiteral("잎")) && !w.cursorText().contains(QStringLiteral("없습니다"))) processFor(20);
        log(QStringLiteral("hover-final(%1 ms): ").arg(msSince(th), 0, 'f', 0) + w.cursorText());
    }
    if (front >= 0 || back >= 0) w.setThickness(front >= 0 ? front : 0.0, back >= 0 ? back : 0.5);
    if (tab >= 0) w.selectDocTab(tab);
    processFor(300);
    log("window-" + w.windowTitleCheck());
    log(QStringLiteral("depth: front=%1m back=%2m backUserSet=%3").arg(w.frontDepth(), 0, 'f', 2).arg(w.backDepth(), 0, 'f', 2).arg(w.backUserSet() ? 1 : 0));
    if (haveLine) {
        Vec3 o = local ? Vec3() : w.srs().origin;
        SectionLine l; l.a = Vec2(ax - o.x, ay - o.y); l.b = Vec2(bx - o.x, by - o.y);
        QElapsedTimer secClock; secClock.start();
        if (!w.computeNow(l, &err)) { log("section-error: " + err); rc = 3; }
        else {
            const qint64 secWallMs = secClock.elapsed();
            auto d = w.sectionDoc();
            size_t nv = 0; for (auto& pl : d.r.profile) nv += pl.size();
            const auto& so = w.lastSection();
            log(QStringLiteral("section-ok: polylines=%1 vertices=%2 z=[%3,%4] image=%5x%6 collect-ms=%7 cut-ms=%8 image-ms=%9 leaves=%10 tris=%11 cut-leaves=%12 read-ms=%13 decode-ms=%14 texMB=%15 wall-ms=%16")
                    .arg(d.r.profile.size()).arg(nv).arg(d.r.zMin + d.r.srs.origin.z, 0, 'f', 2).arg(d.r.zMax + d.r.srs.origin.z, 0, 'f', 2).arg(d.img.width()).arg(d.img.height())
                    .arg(int(so.msCollect)).arg(int(so.msCut)).arg(int(so.msImage)).arg(so.stats.leafNodes).arg(so.stats.triangles).arg(so.cutStats.leafNodes)
                    .arg(int(so.stats.msRead)).arg(int(so.stats.msDecode)).arg(so.stats.texBytes >> 20).arg(secWallMs));
            { const TileCacheCounters tc = tileCacheCounters();
              log(QStringLiteral("cache: tex-decodes=%1 tex-ms=%2 evict-scans=%3 evicted=%4").arg(tc.texDecodes).arg(int(tc.texMs)).arg(tc.evictScans).arg(tc.evicted)); }
            log(QStringLiteral("vex-suggest: relief=%1cm visible1to1=%2m suggest=x%3").arg(w.vexRelief() * 100, 0, 'f', 1)
                    .arg(w.sectionView()->fitVisibleHeight(), 0, 'f', 2).arg(w.vexSuggestion()));
            if (vexArg > 0) { w.setVex(vexArg); log(QStringLiteral("vex: x%1 (화면만)").arg(w.sectionView()->verticalExaggeration(), 0, 'g', 3)); }
            if (cutCheck) { processFor(300); runCutCheck(w, log); }
        }
    }
    processFor(400);
    if (haveLine && w.hasSection()) {   // 화면 레벨선 간격(맞춤 상태) 기록
        SectionView* sv = w.sectionView();
        LevelPlan lp = sectionLevelPlan(sv->xf().ppm, 1.0, false);
        log(QStringLiteral("levels-screen: ppm=%1 line=%2cm label=%3cm").arg(sv->xf().ppm, 0, 'f', 2).arg(lp.lineCm).arg(lp.labelCm));
        for (double denom2 : {20.0, 40.0, 100.0}) {
            double ppm = 1000.0 / denom2 / 25.4 * dpi;
            LevelPlan e = sectionLevelPlan(ppm, dpi / 96.0, true);
            log(QStringLiteral("levels-export 1:%1 %2dpi: line=%3cm label=%4cm").arg(denom2, 0, 'f', 0).arg(dpi, 0, 'f', 0).arg(e.lineCm).arg(e.labelCm));
        }
    }
    if (haveLine && w.hasSection() && !extraLines.empty()) {   // 단면 목록: 더 넣고 각각 계산(썸네일) → 첫 단면으로
        Vec3 o = local ? Vec3() : w.srs().origin;
        for (auto& e : extraLines) {
            SectionLine l; l.a = Vec2(e[0] - o.x, e[1] - o.y); l.b = Vec2(e[2] - o.x, e[3] - o.y);
            int idx = w.addSectionAt(l);
            auto c0 = w.sectionCounters();
            w.selectSectionAt(idx);
            auto te = std::chrono::steady_clock::now();
            while (msSince(te) < 60000 && w.sectionCounters().finalsShown == c0.finalsShown) QApplication::processEvents(QEventLoop::AllEvents, 10);
            log(QStringLiteral("extra-section %1: final in %2 ms").arg(idx).arg(msSince(te), 0, 'f', 0));
        }
        auto c0 = w.sectionCounters();
        w.selectSectionAt(0);
        auto te = std::chrono::steady_clock::now();
        while (msSince(te) < 60000 && w.sectionCounters().finalsShown == c0.finalsShown) QApplication::processEvents(QEventLoop::AllEvents, 10);
        processFor(300);
        log(QStringLiteral("sections: %1").arg(w.sectionCount()));
    }
    if (!uiAuditDir.isEmpty()) {   // v4 단계 1
        QDir().mkpath(uiAuditDir);
        QString al; const bool ok = w.uiAudit(uiAuditDir, &al);
        for (const QString& line : al.split(QLatin1Char('\n'))) log(line);
        if (!ok) rc = 9;
    }
    if (undoTest) {
        QString ul; bool ok = w.undoTest(&ul);
        log(ul.trimmed()); log(QStringLiteral("undo-test %1").arg(ok ? "ok" : "FAILED"));
        if (!ok) rc = 7;
        processFor(300);
        auto c0 = w.sectionCounters();
        auto te = std::chrono::steady_clock::now();
        while (msSince(te) < 30000 && w.sectionCounters().finalsShown == c0.finalsShown) QApplication::processEvents(QEventLoop::AllEvents, 10);
    }
    if (wheelTest) {
        // 휠 확대/축소: 커서 아래 지점이 고정되는지(px 오차)와 애니메이션 중 프레임 시간
        PlanView* pv = w.plan();
        auto sendWheel = [](QWidget* wd, QPointF at, int sign) {   // 실제 휠 이벤트 경로로 시험(120 = 한 칸)
            QWheelEvent ev(at, wd->mapToGlobal(at), QPoint(), QPoint(0, 120 * sign), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
            QApplication::sendEvent(wd, &ev);
        };
        auto runPlan = [&](double notches, const char* name) {
            QPointF at(pv->width() * 0.3, pv->height() * 0.4);
            Vec2 b0, b1; pv->screenToLocalXYPublic(at, b0);
            double mpp0 = pv->metersPerPixel(), worst = 0; int frames = 0;
            auto t0 = std::chrono::steady_clock::now();
            for (int i = 0; i < int(std::fabs(notches)); ++i) { sendWheel(pv, at, notches > 0 ? 1 : -1); processFor(40); worst = std::max(worst, pv->lastFrame().ms); }
            while (pv->zoomAnimating() && msSince(t0) < 5000) { QApplication::processEvents(QEventLoop::AllEvents, 5); worst = std::max(worst, pv->lastFrame().ms); }
            double tAnim = msSince(t0);
            for (int i = 0; i < 30; ++i) { processFor(16); worst = std::max(worst, pv->lastFrame().ms); ++frames; }
            pv->screenToLocalXYPublic(at, b1);
            double drift = (b1 - b0).len() / pv->metersPerPixel();
            log(QStringLiteral("wheel-plan %1: notches=%2 mpp %3→%4 anchor-drift-px=%5 anim-ms=%6 frame-worst-ms(애니+정착)=%7 depth=%8 idle=%9")
                    .arg(name).arg(notches).arg(mpp0, 0, 'g', 4).arg(pv->metersPerPixel(), 0, 'g', 4).arg(drift, 0, 'f', 3).arg(tAnim, 0, 'f', 0)
                    .arg(worst, 0, 'f', 1).arg(pv->lastFrame().maxDepth).arg(pv->streamIdle() ? 1 : 0));
        };
        runPlan(6, "in"); runPlan(-6, "out");
        SectionView* sv = w.sectionView();
        if (sv->hasResult()) {
            auto runSec = [&](double notches, const char* name) {
                QPointF at(sv->width() * 0.35, sv->height() * 0.55);
                double s0, z0, s1, z1; sv->screenToSZ(at, s0, z0);
                double ppm0 = sv->xf().ppm;
                auto t0 = std::chrono::steady_clock::now();
                for (int i = 0; i < int(std::fabs(notches)); ++i) { sendWheel(sv, at, notches > 0 ? 1 : -1); processFor(40); }
                while (sv->zoomAnimating() && msSince(t0) < 5000) QApplication::processEvents(QEventLoop::AllEvents, 5);
                double tAnim = msSince(t0);
                sv->screenToSZ(at, s1, z1);
                double drift = std::hypot(s1 - s0, z1 - z0) * sv->xf().ppm;
                LevelPlan lp = sectionLevelPlan(sv->xf().ppm, 1.0, false);
                log(QStringLiteral("wheel-section %1: notches=%2 ppm %3→%4 anchor-drift-px=%5 anim-ms=%6 levels line=%7cm label=%8cm")
                        .arg(name).arg(notches).arg(ppm0, 0, 'f', 2).arg(sv->xf().ppm, 0, 'f', 2).arg(drift, 0, 'f', 3).arg(tAnim, 0, 'f', 0).arg(lp.lineCm).arg(lp.labelCm));
            };
            runSec(8, "in");
            if (!shot.isEmpty()) {   // 확대 상태 단면(레벨선 10 cm / 숫자 50 cm 확인용)
                QString zp = shot; zp.insert(zp.lastIndexOf('.') < 0 ? zp.size() : zp.lastIndexOf('.'), "_section_wheelzoom");
                sv->grab().save(zp); log("wheel-shot: " + zp);
            }
            runSec(-14, "out"); sv->fit(); sv->update();
        }
        pv->fitAll();
    }
    for (auto& e : exports) {
        QString kind = e.first, path = e.second, m;
        bool ok = false;
        auto src = w.source();
        if (kind == "png" || kind == "tiff" || kind == "geotiff") {
            SectionExportParams p; p.format = kind == "png" ? 0 : kind == "tiff" ? 1 : 2; p.denom = denom; p.dpi = dpi;
            ok = w.hasSection() && MainWindow::exportSectionImage(w.sectionDoc(), *src, p, path, &m);
        } else if (kind == "dxf") {
            DxfParams p; p.world3d = dxf3d; p.denom = denom; p.imageDpi = std::min(dpi, 200.0);
            ok = w.hasSection() && MainWindow::exportSectionDxf(w.sectionDoc(), *src, p, path, &m);
        } else if (kind == "plan") {
            PlanParams p; p.denom = denom; p.dpi = dpi; p.wholeModel = !planView;
            if (planView && !w.plan()->viewRectLocal(p.area)) p.wholeModel = true;
            SectionLine l = w.plan()->line();
            ok = MainWindow::exportPlan(*src, p, w.plan()->hasLine() ? &l : nullptr, path, &m);
        } else if (kind == "xyz" || kind == "las") {
            PointParams p; p.format = kind == "las" ? PointFormat::LAS : PointFormat::XYZ; p.spacing = spacing; p.rgb = rgb;
            p.area = area == "band" ? 1 : area == "view" ? 2 : 0;
            if (p.area == 2 && !w.plan()->viewRectLocal(p.box)) p.area = 0;
            ok = MainWindow::exportPointCloud(*src, p, w.plan()->line(), path, &m);
        } else if (kind == "csv") {
            ok = MainWindow::exportProfileCsv(w.sectionDoc(), path, &m);
        } else if (kind == "plan-svg") {
            PlanSheetParams sp = w.defaultPlanSheetParams();
            sp.format = 4;
            sp.spec.denom = sheetScale > 0 ? sheetScale : snapScaleDenom10(denom);
            if (!paperArg.isEmpty()) { sp.spec.paper = paperArg.startsWith("A3") ? Paper::A3 : Paper::A4; sp.spec.landscape = !paperArg.endsWith("P"); }
            ok = MainWindow::exportPlanSheet(*src, sp, path, &m);
        } else if (kind == "svg") {
            SheetParams sp = w.defaultSheetParams();
            sp.spec.denom = sheetScale > 0 ? sheetScale : denom; sp.dpi = dpi;
            if (!paperArg.isEmpty()) { sp.spec.paper = paperArg.startsWith("A3") ? Paper::A3 : Paper::A4; sp.spec.landscape = !paperArg.endsWith("P"); }
            ok = w.hasSection() && MainWindow::exportSectionSvg(w.sectionDoc(), *src, sp, path, &m);
        } else if (kind == "pdf" || kind == "sheet-png" || kind == "sheet-tiff" || kind == "sheet-dxf") {
            SheetParams sp = w.defaultSheetParams();
            sp.format = kind == "pdf" ? 0 : kind == "sheet-dxf" ? 1 : kind == "sheet-png" ? 2 : 3;
            sp.spec.denom = sheetScale > 0 ? sheetScale : denom; sp.dpi = dpi; sp.split = split;
            if (!paperArg.isEmpty()) { sp.spec.paper = paperArg.startsWith("A3") ? Paper::A3 : Paper::A4; sp.spec.landscape = !paperArg.endsWith("P"); }
            SectionDoc d = w.sectionDoc();
            SheetLayout L = layoutSheet(sp.spec, SectionFrame(d.r.line).L, d.r.zMax - d.r.zMin);
            ok = w.hasSection() && MainWindow::exportSheet(d, *src, sp, path, &m);
            m += QStringLiteral(" | sheet %1x%2 fits=%3").arg(L.cols).arg(L.rows).arg(L.fits ? 1 : 0);
        } else m = "unknown export kind";
        log(QStringLiteral("export-%1 %2: %3").arg(kind, ok ? "ok" : "FAILED", QString(m).replace('\n', " | ")));
        if (!ok) rc = 4;
    }
    if (!dialogShot.isEmpty() && w.hasSection()) {   // 도면 창 캡처(넘침 경고가 보이게 기본 1:40 A4 가로)
        SheetParams sp = w.defaultSheetParams();
        if (sheetScale > 0) sp.spec.denom = sheetScale;
        if (!paperArg.isEmpty()) { sp.spec.paper = paperArg.startsWith("A3") ? Paper::A3 : Paper::A4; sp.spec.landscape = !paperArg.endsWith("P"); }
        sp.showBaseline = true;
        bool acc = false;
        std::unique_ptr<QDialog> dlg(w.buildSheetDialog(sp, acc));
        dlg->show();
        processFor(700);
        bool ok = dlg->grab().save(dialogShot);
        log(QStringLiteral("dialog-shot %1: %2").arg(ok ? "ok" : "FAILED", dialogShot));
        dlg->close();
    }
    if (sectionScale > 0 && w.hasSection()) { w.sectionView()->setScreenDenom(sectionScale); processFor(200); }
    if (camMpp > 0) {   // 평면 카메라를 실좌표(X,Y)·m/px 로(정합 확인용 캡처)
        PlanView* pv = w.plan();
        const Vec3 o = local ? Vec3() : w.srs().origin;
        pv->setCamera(camX - o.x, camY - o.y, camMpp);
        auto tc = std::chrono::steady_clock::now();
        auto msC = [&] { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tc).count(); };
        processFor(300);
        while (msC() < 30000 && !pv->streamIdle()) QApplication::processEvents(QEventLoop::AllEvents, 10);
        processFor(300);
        log(QStringLiteral("plan-cam: X=%1 Y=%2 mpp=%3 depth=%4 idle=%5").arg(camX, 0, 'f', 3).arg(camY, 0, 'f', 3).arg(camMpp).arg(pv->lastFrame().maxDepth).arg(pv->streamIdle() ? 1 : 0));
    }
    if (pitchArg >= 0) { w.plan()->setViewPitch(pitchArg); processFor(200); log(QStringLiteral("plan-pitch: %1").arg(pitchArg, 0, 'f', 1)); }
    if (syncCursor && w.hasSection()) {
        SectionView* sv = w.sectionView();
        QPointF at(sv->width() * 0.42, sv->height() * 0.48);
        QMouseEvent ev(QEvent::MouseMove, at, sv->mapToGlobal(at), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(sv, &ev);
        processFor(200);
    }
    if (openSheet == QStringLiteral("plan")) { w.dlgPlanSheet(); processFor(500); }
    else if (openSheet == QStringLiteral("section") && w.hasSection()) { w.dlgSheet(); processFor(300); }
    auto sheetScaleText = [&]() { QString t; for (QSpinBox* sb : w.findChildren<QSpinBox*>()) if (sb->isVisible() && sb->prefix() == QStringLiteral("1:")) t = QString::number(sb->value()); return t; };
    if (!openSheet.isEmpty() && sheetPick >= 0) {
        QListWidget* lw = nullptr;
        if (QWidget* page = w.findChild<QWidget*>(QStringLiteral("sheetPage"))) lw = page->findChild<QListWidget*>(QStringLiteral("sectionList"));
        if (lw && sheetPick < lw->count()) {
            log(QStringLiteral("sheet-pick: rows=%1 pick=%2").arg(lw->count()).arg(sheetPick));
            emit lw->itemClicked(lw->item(sheetPick));
            processFor(2500);
            QString head;
            if (QWidget* dlg = w.findChild<QWidget*>(QStringLiteral("sheetDialog"))) {
                if (QWidget* hd = dlg->findChild<QWidget*>(QStringLiteral("dialogHead")))
                    for (QLabel* l : hd->findChildren<QLabel*>()) head += l->text() + QStringLiteral(" | ");
            } else head = QStringLiteral("(plan)");
            log(QStringLiteral("sheet-pick head: %1").arg(head));
        } else log(QStringLiteral("sheet-pick: list not found"));
    }
    if (!openSheet.isEmpty() && sheetBack >= 0) {   // 도면 탭에서 입면 조절: 뒤 깊이 칸이 켜져 있고, 바꾸면 단면·조판이 새 값으로
        QDoubleSpinBox* bs = w.findChild<QDoubleSpinBox*>(QStringLiteral("backSpin"));
        if (bs) {
            const bool en = bs->isEnabled();
            bs->setValue(sheetBack);
            processFor(6000);   // 단면 재계산(최종) + 조판 고해상 미리보기
            const SectionDoc d = w.sectionView()->doc();
            log(QStringLiteral("sheet-back: spin-enabled=%1 set=%2 doc.back=%3 imgRes=%4").arg(en ? 1 : 0).arg(sheetBack, 0, 'f', 2).arg(d.r.line.back, 0, 'f', 2).arg(d.imgRes, 0, 'g', 3));
        } else log(QStringLiteral("sheet-back: backSpin not found"));
    }
    if (openSheet == QStringLiteral("section") && sheetStress) {
        // 사용자 신고 2026-10-11 「실행하면 너무 느려진다」: 도면 탭이 있는 동안 단면이 다시 계산될 때마다 · 불투명도를 움직일 때마다
        // 고해상 입면(1.5 mm/px) 계산을 새로 띄우고 앞 계산을 멈추지 않아 쌓였다. 여기서 그 수를 잰다
        QSlider* op = w.findChild<QSlider*>(QStringLiteral("opacityRow"));
        QDoubleSpinBox* bs = w.findChild<QDoubleSpinBox*>(QStringLiteral("backSpin"));
        QTabBar* tabs = w.findChild<QTabBar*>(QStringLiteral("docTabs"));
        if (!op || !bs || !tabs) { log(QStringLiteral("sheet-stress: widgets not found op=%1 back=%2 tabs=%3").arg(op != nullptr).arg(bs != nullptr).arg(tabs != nullptr)); rc = 12; }
        else {
            auto C = [] { return MainWindow::sheetPreviewCounters(); };
            auto settle = [&](int ms) { QElapsedTimer t; t.start(); processFor(ms); while (C().active > 0 && t.elapsed() < 60000) processFor(100); };
            settle(1500);
            const auto c0 = C();
            for (int v = op->value() - 1, n = 0; n < 40 && v >= 10; --v, ++n) { op->setValue(v); processFor(5); }   // 불투명도 끌기(스타일만)
            settle(1500);
            const auto c1 = C();
            const double b0 = bs->value();
            bs->setValue(b0 > 1.0 ? b0 - 0.5 : b0 + 0.5); processFor(30);   // 뒤 깊이 연타 셋(도면 탭에서)
            bs->setValue(b0 > 1.0 ? b0 - 1.0 : b0 + 1.0); processFor(30);
            bs->setValue(b0 > 1.0 ? b0 - 0.75 : b0 + 0.75);
            settle(4000);
            const auto c2 = C();
            tabs->setCurrentIndex(1); processFor(300);                         // 단면 탭으로 — 조판이 안 보임
            bs->setValue(b0);
            settle(4000);
            const auto c3 = C();
            tabs->setCurrentIndex(2); processFor(200);                         // 도면 탭으로 돌아오면 그때 한 번
            settle(4000);
            const auto c4 = C();
            int pickDepth = -1, pickOpen = -1;   // 목록에서 다른 단면을 고른 뒤 뒤 깊이를 바꾸면 새 조판이 따라오는가(단면이 둘 이상일 때만)
            QListWidget* lw = nullptr;
            if (QWidget* page = w.findChild<QWidget*>(QStringLiteral("sheetPage"))) lw = page->findChild<QListWidget*>(QStringLiteral("sectionList"));
            if (lw) {
                // 목록 줄: 머리(-1) · 평면도(0) · 단면(1..). 지금 단면이 아닌 단면 줄을 누르면 새 조판이 열린다(지금 단면이면 아무 일 없음 → 다음 줄)
                MainWindow::SheetPreviewCounters c5 = c4;
                for (int i = 0; i < lw->count() && pickOpen <= 0; ++i) {
                    if (lw->item(i)->data(Qt::UserRole).toInt() < 1) continue;
                    const auto before = C();
                    emit lw->itemClicked(lw->item(i));
                    settle(4000);
                    c5 = C();
                    pickOpen = c5.started - before.started;
                    if (QWidget* page = w.findChild<QWidget*>(QStringLiteral("sheetPage"))) lw = page->findChild<QListWidget*>(QStringLiteral("sectionList"));   // 새 조판이면 목록도 새것
                    if (!lw) break;
                }
                if (QDoubleSpinBox* bs2 = w.findChild<QDoubleSpinBox*>(QStringLiteral("backSpin"))) bs2->setValue(bs2->value() > 1.0 ? bs2->value() - 0.5 : bs2->value() + 0.5);
                settle(4000);
                pickDepth = C().started - c5.started;
            }
            const auto c6 = C();
            const int open = c0.started, opac = c1.started - c0.started, depth = c2.started - c1.started, hidden = c3.started - c2.started, shown = c4.started - c3.started;
            const bool ok = open == 1 && opac == 0 && depth >= 1 && depth <= 2 && hidden == 0 && shown == 1 && c6.peak <= 2 && pickDepth != 0;
            log(QStringLiteral("sheet-stress: open=%1 opacity=%2 depth=%3 hidden=%4 shown=%5 pick-open=%6 pick-depth=%7 peak=%8 active=%9 %10")
                    .arg(open).arg(opac).arg(depth).arg(hidden).arg(shown).arg(pickOpen).arg(pickDepth).arg(c6.peak).arg(c6.active).arg(ok ? QStringLiteral("ok") : QStringLiteral("FAIL")));
            if (!ok) rc = 12;
        }
    }
    if (!openSheet.isEmpty() && sheetEdit) {
        for (QPushButton* b : w.findChildren<QPushButton*>()) if (b->isVisible() && b->text() == QStringLiteral("조판편집")) { b->click(); break; }
        processFor(100);
        log(QStringLiteral("sheet-edit: on"));
    }
    if (!openSheet.isEmpty() && sheetWheel != 0) log(QStringLiteral("sheet-scale before wheel: 1:%1").arg(sheetScaleText()));
    if (!openSheet.isEmpty() && sheetWheel != 0) {   // 실제 휠 이벤트 → 멈춘 뒤 보이는 범위를 다시 렌더하는지 확인용
        if (QWidget* pv = w.findChild<QWidget*>(QStringLiteral("sheetPreview"))) {
            QPointF at(pv->width() * 0.45, pv->height() * 0.45);
            for (int i = 0; i < std::abs(sheetWheel); ++i) {
                QWheelEvent ev(at, pv->mapToGlobal(at), QPoint(), QPoint(0, sheetWheel > 0 ? 120 : -120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
                QApplication::sendEvent(pv, &ev);
                processFor(30);
            }
            processFor(2500);
            log(QStringLiteral("sheet-wheel: %1 notches sent").arg(sheetWheel));
            log(QStringLiteral("sheet-scale after wheel: 1:%1").arg(sheetScaleText()));
        } else log(QStringLiteral("sheet-wheel: preview not found"));
    }
    if (!shot.isEmpty()) {
        processFor(500);
        // QOpenGLWidget 위 QPainter 덧그림까지 포함하려면 창 시스템에서 직접 캡처(실패 시 위젯 grab)
        QPixmap pm = w.screen() ? w.screen()->grabWindow(w.winId()) : QPixmap();
        if (pm.isNull() || pm.width() < 10) pm = w.grab();
        bool ok = pm.save(shot);
        log(QStringLiteral("shot %1: %2 (%3x%4)").arg(ok ? "ok" : "FAILED", shot).arg(pm.width()).arg(pm.height()));
    }
    if (!ctxShot.isEmpty()) {   // 그리기 중(A 찍고 A′ 로 움직이는 중) 지금 도구 줄
        PlanView* pv = w.plan();
        const SectionLine keep = pv->line(); const bool hadLine = pv->hasLine();
        pv->setDrawMode(true);
        processFor(200);
        QPointF pa(pv->width() * 0.30, pv->height() * 0.55), pb(pv->width() * 0.72, pv->height() * 0.50);
        QMouseEvent pr(QEvent::MouseButtonPress, pa, pv->mapToGlobal(pa), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(pv, &pr);
        QMouseEvent rl(QEvent::MouseButtonRelease, pa, pv->mapToGlobal(pa), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(pv, &rl);
        for (int i = 1; i <= 10; ++i) {
            QPointF q = pa + (pb - pa) * (i / 10.0);
            QMouseEvent mv(QEvent::MouseMove, q, pv->mapToGlobal(q), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(pv, &mv);
            processFor(30);
        }
        processFor(500);
        QPixmap pm = w.screen() ? w.screen()->grabWindow(w.winId()) : QPixmap();
        if (pm.isNull() || pm.width() < 10) pm = w.grab();
        bool ok = pm.save(ctxShot);
        log(QStringLiteral("ctx-shot %1: %2 guide=%3").arg(ok ? "ok" : "FAILED", ctxShot).arg(w.guideBand() && w.guideBand()->isVisible() ? 1 : 0));
        pv->setDrawMode(false);
        pv->setLine(keep, hadLine);   // 그리기 전 단면선 되살림
        processFor(100);
    }
    if (!lodShot.isEmpty()) {   // 디테일 불러오는 중 카드(평면 확대 직후)
        PlanView* pv = w.plan();
        pv->zoomBy(1 / 6.0);
        auto tl = std::chrono::steady_clock::now();
        while (msSince(tl) < 3000 && pv->streamIdle()) QApplication::processEvents(QEventLoop::AllEvents, 5);
        processFor(120);
        QPixmap pm = w.screen() ? w.screen()->grabWindow(w.winId()) : QPixmap();
        if (pm.isNull() || pm.width() < 10) pm = w.grab();
        bool ok = pm.save(lodShot);
        log(QStringLiteral("lod-shot %1: %2 idle=%3 queued=%4 loading=%5").arg(ok ? "ok" : "FAILED", lodShot).arg(pv->streamIdle() ? 1 : 0).arg(pv->lastFrame().queued).arg(pv->lastFrame().loading));
        auto t2 = std::chrono::steady_clock::now();
        while (msSince(t2) < 30000 && !pv->streamIdle()) QApplication::processEvents(QEventLoop::AllEvents, 10);
        pv->fitAll(); processFor(200);
    }
    if (!sheetCheck.isEmpty()) {
        QString rep;
        bool ck = w.runSheetCheck(sheetCheck, &rep);
        log(rep);
        if (!ck) rc = 8;
    }
    if (!sheetBatch.isEmpty() && w.hasSection()) {   // v5 E4: 평면도 1 + 단면 n 장이 모두 생겨야 ok
        QStringList lg;
        w.setAllSheetsChecked(true);
        SheetParams sp = w.defaultSheetParams(); sp.format = 4;
        const int n = w.saveCheckedSheets(sheetBatch, sp, &lg);
        for (const QString& l : lg) log(l);
        const int expect = 1 + w.sectionCount();
        const QStringList files = QDir(sheetBatch).entryList({QStringLiteral("*.svg")}, QDir::Files);
        const bool okb = n == expect && files.size() >= expect;
        log(QStringLiteral("sheet-batch saved=%1 expected=%2 files=%3 RESULT %4").arg(n).arg(expect).arg(files.join(QLatin1Char(' '))).arg(okb ? QStringLiteral("ok") : QStringLiteral("fail")));
        if (!okb) rc = 9;
    }
    if (quit) return rc;
    return app.exec();
}

#ifdef _WIN32
// GUI 하위 시스템 진입점(Qt EntryPoint 라이브러리 대신). 인수는 QApplication 이 GetCommandLineW 로 유니코드로 다시 읽는다.
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return main(__argc, __argv); }
#endif
