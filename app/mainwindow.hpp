// 주 창: 리본(파일/홈/보기/분석/추출/내보내기) + View 1 평면 / View 2 단면 + 하단 좌표줄. moc 없이(람다·std::function) 작성.
#pragma once
#include <QAction>
#include <QMainWindow>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include "asec/engine.hpp"
#include "asec/pointcloud.hpp"
#include "asec/schedule.hpp"
#include "asec/pick.hpp"
#include "asec/tiff.hpp"
#include "planview.hpp"
#include "theme.hpp"
#include <map>
#include "sectionview.hpp"

class QLabel;
class QLineEdit;
class QProgressBar;
class QDoubleSpinBox;
class QSlider;
class QTabBar;
class QStackedWidget;
class QToolButton;
class QSplitter;
class QCheckBox;

/// 열기 결과(작업 스레드에서 만들고 GUI 스레드에서 적용)
struct OpenedScene {
    QString path;
    std::shared_ptr<asec::MeshSource> src;
    std::vector<std::shared_ptr<DisplayMesh>> display;  // OBJ: 화면 메시 전체 / 3MX: 커서 Z 용 거친 메시(텍스처 없음)
    std::vector<asec::fs::path> streamRoots;            // 3MX: LOD 스트리밍 루트(비면 정적 표시)
    asec::Vec3 center;
    asec::Box3 bounds;
    size_t displayTris = 0, displayMeshes = 0;
    QString kind;  // "3MX" / "OBJ"
    size_t layers = 1;                   // 3MX meshPyramid 레이어 수(병합 3MX > 1)
    std::vector<std::string> warnings;   // 열기 경고(레이어 제외 등)
};

struct SectionExportParams { int format = 0; double denom = 20, dpi = 300; };  // format 0 PNG 1 TIFF 2 GeoTIFF
struct DxfParams { bool world3d = false; double denom = 20; bool image = true; double imageDpi = 200; };
struct PlanParams { bool wholeModel = true; double denom = 100, dpi = 200; bool overlayLine = true; asec::Box3 area; };
struct PointParams { asec::PointFormat format = asec::PointFormat::XYZ; int area = 0; double spacing = 0; bool rgb = true; asec::Box3 box; };

class MainWindow : public QMainWindow {
public:
    MainWindow();
    ~MainWindow() override;

    // ---- 작업(동기). 대화상자는 작업 스레드에서, 명령줄 자동화는 GUI 스레드에서 직접 호출 ----
    static bool loadScene(const QString& path, OpenedScene& out, QString* err, const std::function<void(double)>& progress = {});
    void applyScene(OpenedScene&& s);
    bool computeNow(const asec::SectionLine& l, QString* err);  // 동기 단면(자동화용)
    // 내보내기: 입력을 모두 인자로 받음(스레드 안전). src 는 공유 포인터 사본, doc 은 GUI 스레드에서 뜬 스냅숏
    static bool exportSectionImage(const SectionDoc& doc, asec::MeshSource& src, const SectionExportParams& p, const QString& path, QString* msg,
                                   const std::atomic<bool>* cancel = nullptr);
    static bool exportSectionDxf(const SectionDoc& doc, asec::MeshSource& src, const DxfParams& p, const QString& path, QString* msg,
                                 const std::atomic<bool>* cancel = nullptr);
    static bool exportPlan(asec::MeshSource& src, const PlanParams& p, const asec::SectionLine* line, const QString& path, QString* msg,
                           const std::atomic<bool>* cancel = nullptr);
    static bool exportPointCloud(asec::MeshSource& src, const PointParams& p, const asec::SectionLine& line, const QString& path, QString* msg,
                                 const std::atomic<bool>* cancel = nullptr, const std::function<void(size_t)>& progress = {});
    static bool exportProfileCsv(const SectionDoc& doc, const QString& path, QString* msg);
    std::shared_ptr<asec::MeshSource> source() const { return src_; }
    SectionDoc sectionDoc() const { return section_->doc(); }
    bool hasScene() const { return bool(src_); }
    bool hasSection() const;
    void setLineLocal(const asec::SectionLine& l);
    void setThickness(double front, double back);
    const asec::SrsInfo& srs() const;
    const asec::SrsReport& srsReport() const { return srsReport_; }
    QString cursorText() const;   // 자동화 로그: 좌표줄 X/Y/Z + Z 출처
    struct SectionCounters { int previewsShown = 0, finalsShown = 0; double lastMs = 0; size_t lastTris = 0; };
    const SectionCounters& sectionCounters() const { return secCounters_; }
    /// 단면선 끌기 흉내(자동화): final=false 는 끄는 중, true 는 놓음
    void dragLine(const asec::SectionLine& l, bool final) { plan_->setLine(l, true); requestSection(final); }
    PlanView* plan() const { return plan_; }
    SectionView* sectionView() const { return section_; }
    void selectRibbonTab(int i);
    void openFile(const QString& path);

protected:
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
    void closeEvent(QCloseEvent*) override;

private:
    PlanView* plan_ = nullptr;
    SectionView* section_ = nullptr;
    QWidget* planFrame_ = nullptr; QWidget* sectionFrame_ = nullptr;
    QSplitter* split_ = nullptr;
    QTabBar* tabs_ = nullptr;
    QStackedWidget* pages_ = nullptr;
    QLineEdit *cx_ = nullptr, *cy_ = nullptr, *cz_ = nullptr;
    QLabel *msg_ = nullptr, *info_ = nullptr, *srsLabel_ = nullptr, *srsBanner_ = nullptr;
    asec::SrsReport srsReport_;   // 열린 모델의 좌표계 점검(표시·경고)
    QProgressBar* progress_ = nullptr;
    QDoubleSpinBox *front_ = nullptr, *back_ = nullptr;
    QSlider* opacity_ = nullptr;
    QToolButton* viewNum_[8] = {};
    std::map<QString, QAction*> act_;
    struct Label { QObject* obj; QString ko, en; int kind; };  // kind 0 탭 1 그룹라벨 2 동작 3 제목
    std::vector<Label> labels_;
    bool bilingual_ = true;

    std::shared_ptr<asec::MeshSource> src_;
    QString path_, kind_;
    size_t displayTris_ = 0;
    bool streaming_ = false;

    // 단면 작업(asec::CoalescingWorker): 끄는 동안 미리보기는 합치고(거친 LOD), 놓으면 잎으로 최종 계산
    std::unique_ptr<asec::CoalescingWorker> secWorker_;
    // 커서 정밀 Z: 잎 메시 CPU 피킹(마지막 요청만 계산)
    std::unique_ptr<asec::CoalescingWorker> pickWorker_;
    asec::ResultGate pickGate_;
    uint64_t pickFloorGen_ = 0;
    QLabel* zSrc_ = nullptr;
    void requestPick(const QPointF& screen);
    void showZSource(asec::ZSource s, const QString& detail);
    asec::ResultGate secGate_;   // GUI 스레드 소유: 이미 보여준 것보다 새 세대만 받음
    uint64_t secFloorGen_ = 0;   // 장면 바꿈/닫기 이후 세대만 받음
    SectionCounters secCounters_;
    asec::SectionOutput last_;  // GUI 스레드 소유(마지막 완료 결과)
    bool lastFinal_ = false;

    // 일반 작업(열기·내보내기)
    std::thread task_;
    std::atomic<bool> taskBusy_{false};
    std::atomic<bool> cancelTask_{false};

    QAction* action(const QString& key) const { return act_.at(key); }
    QAction* makeAction(const QString& key, const QString& ko, const QString& en, theme::Ico ico, const QString& shortcut = {}, bool checkable = false);
    QWidget* buildRibbon();
    QWidget* buildViewFrame(int num, const QString& ko, const QString& en, QWidget* content, QWidget* bar, QWidget** titleOut);
    QWidget* buildCoordBar();
    void retranslate();
    void requestSection(bool final);
    void onSectionDone(asec::SectionOutput&& out, uint64_t gen, bool final, const QString& err);
    void runTask(const QString& what, std::function<bool(QString*)> work, std::function<void(bool, const QString&)> done);
    void setProgress(double f);
    void updateEnabled();
    void showStatus(const QString& s);
    void chooseOpen();
    void closeScene();
    void dlgSectionImage();
    void dlgSectionDxf();
    void dlgPlan();
    void dlgPoints(int presetFormat);
    void dlgProfileCsv();
    void dlgInfo();
    void dlgAbout();
    QString askSavePath(const QString& key, const QString& suggested, const QString& filter);
    SectionStyle secStyle() const;
    void report(bool ok, const QString& m);
    void applySrsReport();
};
