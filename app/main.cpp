// 발굴 단면뷰어 진입점. 명령줄 자동화(화면 캡처·내보내기 시험용):
//   SectionViewer [파일.3mx|.obj] [--line AX AY BX BY] [--local] [--front m] [--back m] [--size WxH] [--tab N]
//                 [--shot out.png] [--export-png f] [--export-tiff f] [--export-geotiff f] [--export-dxf f] [--dxf3d]
//                 [--export-plan f] [--plan-view] [--export-xyz f] [--export-las f] [--area whole|band|view] [--spacing m] [--norgb]
//                 [--scale N] [--dpi N] [--log f] [--perf-log f.csv] [--quit]
//   --perf-log: 평면 보기 카메라 경로(맞춤→확대→이동→축소)를 재생하며 프레임마다 시간·LOD 상태를 CSV 로 기록
#include <QApplication>
#include <QFile>
#include <QScreen>
#include <QFontDatabase>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QTimer>
#include "mainwindow.hpp"
#include "theme.hpp"

using namespace asec;

static void processFor(int ms) {
    auto t0 = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(ms)) { QApplication::processEvents(QEventLoop::AllEvents, 20); }
}

int main(int argc, char** argv) {
    QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QSurfaceFormat fmt; fmt.setSamples(4); fmt.setDepthBufferSize(24); QSurfaceFormat::setDefaultFormat(fmt);
    QApplication app(argc, argv);
    QApplication::setOrganizationName("ExcavSection");
    QApplication::setApplicationName("SectionViewer");
    QApplication::setApplicationDisplayName(QStringLiteral("발굴 단면뷰어"));
    QFont f(theme::fontFamily()); f.setPointSizeF(9); f.setStyleStrategy(QFont::PreferAntialias);
    QApplication::setFont(f);
    app.setStyle("Fusion");
    app.setStyleSheet(theme::styleSheet());
    QIcon ic; for (const char* r : {":/app_256.png"}) ic.addFile(r);
    ic.addFile(QApplication::applicationDirPath() + "/app.ico");
    app.setWindowIcon(ic);

    QStringList a = app.arguments();
    QString file, shot, logPath, perfPath;
    bool haveLine = false, local = false, quit = false;
    double ax = 0, ay = 0, bx = 0, by = 0, front = -1, back = -1, denom = 20, dpi = 300, spacing = 0;
    int W = 0, H = 0, tab = -1;
    bool dxf3d = false, rgb = true, planView = false;
    QString area = "whole";
    QList<QPair<QString, QString>> exports;
    for (int i = 1; i < a.size(); ++i) {
        QString s = a[i];
        auto nx = [&]() { return i + 1 < a.size() ? a[++i] : QString(); };
        if (s == "--line" && i + 4 < a.size()) { ax = a[i + 1].toDouble(); ay = a[i + 2].toDouble(); bx = a[i + 3].toDouble(); by = a[i + 4].toDouble(); i += 4; haveLine = true; }
        else if (s == "--local") local = true;
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
        else if (s.startsWith("--export-")) exports.append({s.mid(9), nx()});
        else if (!s.startsWith("--")) file = s;
    }

    MainWindow w;
    if (W > 0 && H > 0) w.resize(W, H);
    w.show();
    if (file.isEmpty()) return app.exec();

    int rc = 0;
    QFile logF(logPath);
    bool logOk = !logPath.isEmpty() && logF.open(QIODevice::WriteOnly | QIODevice::Text);
    auto log = [&](const QString& m) {
        QByteArray b = (m + "\n").toUtf8();
        if (logOk) { logF.write(b); logF.flush(); }
        fputs(b.constData(), stdout); fflush(stdout);
    };
    bool automated = haveLine || !shot.isEmpty() || !exports.isEmpty() || quit || !perfPath.isEmpty();
    if (!automated) { QTimer::singleShot(0, &w, [&] { w.openFile(file); }); return app.exec(); }

    processFor(200);
    OpenedScene sc; QString err;
    auto t0 = std::chrono::steady_clock::now();
    if (!MainWindow::loadScene(file, sc, &err)) { log("open-error: " + err); return 2; }
    log(QStringLiteral("open-ok: %1 kind=%2 displayTris=%3 srs=%4 origin=%5,%6,%7 ms=%8").arg(file, sc.kind).arg(sc.displayTris)
            .arg(QString::fromStdString(sc.src->srs.srs)).arg(sc.src->srs.origin.x, 0, 'f', 3).arg(sc.src->srs.origin.y, 0, 'f', 3).arg(sc.src->srs.origin.z, 0, 'f', 3)
            .arg(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(), 0, 'f', 0));
    auto tApply = std::chrono::steady_clock::now();
    auto msSince = [](std::chrono::steady_clock::time_point t) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t).count(); };
    w.applyScene(std::move(sc));
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
            ts << "frame,t_ms,phase,center_x,center_y,mpp,frame_ms,paint_ms,draw_nodes,uploaded,evicted,wanted,queued,loading,gpu_mb,max_depth,idle\n";
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
                   << F.loading << "," << QString::number(F.gpuBytes / 1048576.0, 'f', 2) << "," << F.maxDepth << "," << (F.idle ? 1 : 0) << "\n";
            }
            ts.flush();
            log(QStringLiteral("perf: frames=%1 avg-ms=%2 worst-ms=%3 csv=%4 (GL=%5)").arg(path.size()).arg(sum / path.size(), 0, 'f', 2).arg(worst, 0, 'f', 2)
                    .arg(perfPath, QString::fromLatin1(reinterpret_cast<const char*>(pv->glRenderer().constData()))));
        }
    }
    if (front >= 0 || back >= 0) w.setThickness(front >= 0 ? front : 0.0, back >= 0 ? back : 0.5);
    if (tab >= 0) w.selectRibbonTab(tab);
    processFor(300);
    if (haveLine) {
        Vec3 o = local ? Vec3() : w.srs().origin;
        SectionLine l; l.a = Vec2(ax - o.x, ay - o.y); l.b = Vec2(bx - o.x, by - o.y);
        if (!w.computeNow(l, &err)) { log("section-error: " + err); rc = 3; }
        else {
            auto d = w.sectionDoc();
            size_t nv = 0; for (auto& pl : d.r.profile) nv += pl.size();
            log(QStringLiteral("section-ok: polylines=%1 vertices=%2 z=[%3,%4] image=%5x%6").arg(d.r.profile.size()).arg(nv)
                    .arg(d.r.zMin + d.r.srs.origin.z, 0, 'f', 2).arg(d.r.zMax + d.r.srs.origin.z, 0, 'f', 2).arg(d.img.width()).arg(d.img.height()));
        }
    }
    processFor(400);
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
        } else m = "unknown export kind";
        log(QStringLiteral("export-%1 %2: %3").arg(kind, ok ? "ok" : "FAILED", QString(m).replace('\n', " | ")));
        if (!ok) rc = 4;
    }
    if (!shot.isEmpty()) {
        processFor(500);
        // QOpenGLWidget 위 QPainter 덧그림까지 포함하려면 창 시스템에서 직접 캡처(실패 시 위젯 grab)
        QPixmap pm = w.screen() ? w.screen()->grabWindow(w.winId()) : QPixmap();
        if (pm.isNull() || pm.width() < 10) pm = w.grab();
        bool ok = pm.save(shot);
        log(QStringLiteral("shot %1: %2 (%3x%4)").arg(ok ? "ok" : "FAILED", shot).arg(pm.width()).arg(pm.height()));
    }
    if (quit) return rc;
    return app.exec();
}

#ifdef _WIN32
// GUI 하위 시스템 진입점(Qt EntryPoint 라이브러리 대신). 인수는 QApplication 이 GetCommandLineW 로 유니코드로 다시 읽는다.
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { return main(__argc, __argv); }
#endif
