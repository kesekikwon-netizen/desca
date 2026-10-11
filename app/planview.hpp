// 평면 보기: 위에서 본 정사(orthographic) 텍스처 메시 + 빨간 단면선·두께 띠(괄호선) 편집
#pragma once
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <QRect>
#include <QVector>
#include <atomic>
#include <functional>
#include <memory>
#include "cursors.hpp"
#include "asec/section.hpp"
#include "asec/tmx.hpp"
#include "asec/stream.hpp"
#include <unordered_map>

/// GPU 에 올릴 메시(장면 중심 기준 float 좌표)
struct DisplayMesh {
    std::vector<float> pos, uv, nrm;
    std::vector<uint32_t> idx;
    QImage tex;  // RGBA8888, 행0 = 위
};

/// 화면 메시의 XY 격자 색인: 커서 아래 Z(가장 높은 면)
class HeightIndex {
public:
    void build(const std::vector<std::shared_ptr<DisplayMesh>>& ms, double cell);
    bool z(double x, double y, double& out) const;  // 중심 기준 좌표
private:
    struct Tri { float x[3], y[3], z[3]; };
    std::vector<Tri> tris_;
    std::vector<std::vector<uint32_t>> cells_;
    double x0_ = 0, y0_ = 0, cell_ = 1;
    int nx_ = 0, ny_ = 0;
};

class PlanView : public QOpenGLWidget, protected QOpenGLFunctions {
public:
    explicit PlanView(QWidget* parent = nullptr);
    ~PlanView() override;

    /// center = 로컬 좌표의 장면 중심(메시는 이미 이 값을 뺀 상태), bounds = 로컬 상자
    void setScene(std::vector<std::shared_ptr<DisplayMesh>> meshes, const asec::Vec3& center, const asec::Box3& bounds, const asec::SrsInfo& srs);
    /// 3MX: 시점 의존 LOD 스트리밍(roots = 각 layer 의 루트 .3mxb). heightMeshes = 커서 Z 용 거친 메시(선택)
    void setStreamingScene(const std::vector<asec::fs::path>& roots, const asec::Vec3& center, const asec::Box3& bounds, const asec::SrsInfo& srs,
                           std::vector<std::shared_ptr<DisplayMesh>> heightMeshes = {});
    bool hasScene() const { return hasScene_; }
    void clearScene();

    // ---- 스트리밍 상태(자동화·성능 기록용)
    struct FrameInfo { double ms = 0; size_t draw = 0, uploaded = 0, evicted = 0, wanted = 0, queued = 0, loading = 0; size_t residentBytes = 0, gpuBytes = 0; int maxDepth = -1; bool idle = true;
                       size_t uploadBytes = 0, staging = 0; double uploadMs = 0;
                       size_t burstDone = 0, burstTotal = 0; };   // burst*: 이번 불러오기 묶음(LOD 카드 「12/40」)
    /// GPU 올리기 예산(프레임당 바이트). 설정 view/uploadBudgetKB(기본 3072), 0 이면 옛 방식(노드 통째)
    static size_t uploadBudgetBytes();
    const FrameInfo& lastFrame() const { return lastFrame_; }
    bool streamIdle() const { return !streamer_ || lastFrame_.idle; }
    bool streaming() const { return bool(streamer_); }
    void setCamera(double cx, double cy, double mpp);  // 로컬 XY 중심 + m/px(위에서 보기)
    double cameraX() const { return double(target_.x()) + center_.x; }
    double cameraY() const { return double(target_.y()) + center_.y; }
    static size_t gpuBudgetBytes();
    QByteArray glRenderer() const { return glRenderer_; }

    void setDrawMode(bool on);
    bool drawMode() const { return drawStage_ >= 0; }
    void setLine(const asec::SectionLine& l, bool has);
    const asec::SectionLine& line() const { return line_; }
    bool hasLine() const { return hasLine_; }
    /// 지금 단면선의 이름(「B–B′」) — 끝 글자 칩에 쓴다. 비면 A–A′
    void setLineName(const QString& n) { lineName_ = n; update(); }
    /// 다른 단면들(목록의 나머지): 평면에 가늘게 함께 그린다(사용자 결정 2026-10-11 — 새 단면을 그려도 A 는 남는다)
    struct OtherSection { asec::SectionLine line; QString name; };
    void setOtherSections(std::vector<OtherSection> v) { others_ = std::move(v); update(); }
    /// 「B–B′」 → "B", "B′". 모양이 다르면 A, A′
    static void splitLetters(const QString& name, QString& a, QString& b);
    void setBand(double front, double back);
    /// 단면 커서가 가리키는 거리(A 에서 m). 켜지면 단면선 위에 고리를 그린다.
    void setSyncMark(bool on, double s);
    /// 단면 영상을 단면 평면에 세운다. z1·s0·res 는 단면 영상 지리(로컬 m).
    void setSectionPlane(const QImage& img, const asec::SectionLine& line, double s0, double z1, double res);
    void setViewPitch(double deg);

    void fitAll();
    /// 감사용: 장면 상자(XY)가 지금 창 크기에서 화면에 다 들어가는가(창을 줄여도 맞춤이 유지되는지 — B5)
    bool contentFits() const;
    /// 단면선 위 m 숫자 간격(m, 스펙 §5): {1, 2, 5, 10} 중 숫자 글끼리 겹치지 않는 가장 작은 값
    int tickLabelStepM() const;
    /// 감사 · 그리기 공용: 단면선 위 m 숫자 글 사각형(화면 px). 단면선이 없거나 화면 길이 < 80 px 면 빈 목록
    QVector<QRectF> tickLabelRects() const;
    void topView();
    void homeView();
    QPointF compassCenter() const;                  // 나침반 자리(그리기 · 눌림 판정이 같은 값을 쓴다 — 검토 I1)
    bool compassHit(const QPointF& p) const;
    void zoomBy(double f);
    /// 현재 화면에 보이는 로컬 XY 범위(평면 기준)
    bool viewRectLocal(asec::Box3& out) const;
    const asec::Box3& bounds() const { return bounds_; }
    double metersPerPixel() const { return mpp_; }
    /// 휠 확대/축소(커서 위치 고정). smooth=true 면 약 0.1 초에 걸쳐 부드럽게(설정 view/smoothZoom). 자동화·시험용으로도 씀
    void wheelZoom(const QPointF& at, double notches);
    bool zoomAnimating() const { return zoomPending_ != 0.0; }
    bool screenToLocalXYPublic(const QPointF& sp, asec::Vec2& out) const { return screenToLocalXY(sp, out); }
    /// 화면 점 → 광선(로컬 double 좌표: 원점, 방향). 정밀 피킹(asec::pickRay)용
    bool screenRayLocal(const QPointF& sp, asec::Vec3& o, asec::Vec3& d) const;
    QPointF lastMousePos() const { return QPointF(lastMouse_); }

    std::function<void(const asec::SectionLine&, bool final)> onLineChanged;
    std::function<void(double X, double Y, double Z, bool hasZ, bool valid)> onCursor;  // 실좌표
    std::function<void(bool)> onDrawModeChanged;
    std::function<void()> onOpenRequest;
    std::function<void(int stage, const asec::SectionLine&)> onDrawProgress;   // 그리는 중(0 A 대기, 1 A′ 대기): 지금 도구 줄 길이·방위
    int drawStage() const { return drawStage_; }
    void setCursorHighContrast(bool on);   // 고대비: 커서 선 2 · 흰 테 1.5
    QPointF handleScreenPos(int which) const;   // 손잡이 화면 위치(0 A · 1 A′ · 2 가운데) — 커서 감사가 손잡이 위로 마우스를 옮길 때 씀

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void resizeEvent(QResizeEvent* e) override;   // GL 없이도(offscreen) 불리는 길: 맞춤 상태면 fitAll, 아니면 행렬만(B5). 기본 구현을 먼저 부른다(프레임버퍼 크기)
    void paintGL() override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void leaveEvent(QEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;   // Shift 를 놓으면 A′ 단계 커서가 보통 십자로(축 맞춤 끝)
    bool event(QEvent*) override;                // 화면 배율이 바뀌면(DevicePixelRatioChange) 커서를 새 배율로 다시 굽는다(B5 규칙 ②)

private:
    // ---- 커서(G8): 도구를 든 동안 · 손잡이 위에서만 십자 + 배지, 그 밖은 OS 커서. 같은 모양이면 다시 걸지 않는다
    void useShape(Qt::CursorShape s);
    void useTool(kerf::ToolCursor k, bool shift);
    void refreshCursor(Qt::KeyboardModifiers m);   // 지금 상태(그리는 중 단계 · 보조키)에 맞는 커서
    int cursorKey_ = -1;                           // -1 없음 · 0.. 도구 커서(종류 × 2 + Shift) · 1000+ OS 모양
    kerf::ToolCursor toolKind_ = kerf::ToolCursor::DrawA; bool toolShift_ = false, cursorHc_ = false;
    struct Gpu { GLuint vbo = 0, ibo = 0, tex = 0; GLsizei count = 0; bool hasTex = false; size_t bytes = 0; int texW = 0, texH = 0, texLevels = 0; };
    // 내려간 노드의 텍스처를 같은 크기 다음 노드가 다시 씀(새로 잡는 비용 = 소프트웨어 GL 에서 수~수십 ms 를 피함)
    struct PoolTex { GLuint tex; int w, h, levels; size_t bytes; };
    std::vector<PoolTex> texPool_;
    size_t texPoolBytes_ = 0;
    void freeTexPool();
    std::vector<std::shared_ptr<DisplayMesh>> meshes_;
    std::vector<Gpu> gpu_;
    bool hasScene_ = false;
    // 스트리밍(3MX)
    std::unique_ptr<asec::LodStreamer> streamer_;
    std::unordered_map<asec::LodStreamer::Key, std::vector<Gpu>> gpuNodes_;
    size_t gpuNodeBytes_ = 0;
    std::atomic<bool> repaintQueued_{false};
    FrameInfo lastFrame_;
    QByteArray glRenderer_;
    double lodBias_ = 2.0;  // >1 이면 더 세밀하게(설정 view/lodBias, 기본 2.0 — 2026-10-11)
    void paintStreaming(const QMatrix4x4& mvp);
    // 나눠 올리기: 준비된 노드를 peek 으로 빌려 버퍼·텍스처(밉맵 단계·행 띠)를 프레임마다 예산만큼 올리고, 다 올리면 take 로 상주 확정
    struct Staging {
        std::shared_ptr<void> prep;   // NodePrep
        std::vector<Gpu> parts;
        size_t part = 0; int stage = 0; size_t offset = 0; int level = 0; int row = 0;
        uint64_t lastSeen = 0;
    };
    std::unordered_map<asec::LodStreamer::Key, Staging> staging_;
    uint64_t frameNo_ = 0;
    size_t burstDone_ = 0, burstPeak_ = 0;
    bool stepStaging(Staging& st, size_t& budget, const class QElapsedTimer& t, double maxMs);   // true = 다 올림
    void freeStaging();
    void freeGpuNode(std::vector<Gpu>& v);
    void drawGpu(const Gpu& g);
    bool needUpload_ = false;
    std::unique_ptr<QOpenGLShaderProgram> prog_;
    HeightIndex hidx_;
    asec::Vec3 center_;  // 로컬 장면 중심
    asec::Box3 bounds_;  // 로컬
    asec::SrsInfo srs_;
    // 카메라(중심 기준 좌표)
    QVector3D target_;
    double mpp_ = 0.01, yaw_ = 0, pitch_ = 90;
    bool fitted_ = false;               // 맞춤 상태(fitAll 뒤 사용자가 확대 · 이동 · 돌리기 · 카메라 지정을 안 함) — 창 크기가 바뀌면 다시 맞춘다(B5)
    double zoomPending_ = 0;            // 남은 확대량(log, 음수 = 확대)
    QPointF zoomAnchor_;
    class QTimer* zoomTimer_ = nullptr;
    void applyZoomAt(const QPointF& at, double factor);
    QMatrix4x4 proj_, view_;
    // 단면선(로컬 XY)
    asec::SectionLine line_;
    bool hasLine_ = false;
    QString lineName_;
    std::vector<OtherSection> others_;
    bool syncOn_ = false;
    double syncS_ = 0;
    QImage planeImg_;
    asec::SectionLine planeLine_;
    double planeS0_ = 0, planeZ1_ = 0, planeRes_ = 0;
    bool planeOn_ = false, planeDirty_ = false;
    unsigned planeTex_ = 0;
    void drawSectionPlane();
    int drawStage_ = -1;  // -1 편집, 0 A 대기, 1 B 대기
    int dragHandle_ = -1; // 0 A, 1 B, 2 가운데(전체 이동)
    QPointF dragStartMouse_; asec::SectionLine dragStartLine_;
    bool panning_ = false, orbiting_ = false, orbitLift_ = false;
    QPoint lastMouse_;

    void freeGpu();
    void upload();
    struct TickLabel { QRectF rect; QString text; };
    QVector<TickLabel> tickLabels() const;   // 자리 = 눈금 중심에서 띠 반대쪽 16 px, 폭 = 글 폭 + 2(흰 테), 높이 = 글꼴 높이 + 2 — paintOverlay 와 tickLabelRects 가 같이 씀
    void updateMatrices();
    void makeMatrices(int w, int h, QMatrix4x4& proj, QMatrix4x4& view) const;   // 화면 행렬 식 한 곳(ortho · 회전 · 이동) — updateMatrices · contentFits(B5 검토 ⓒ)
    bool unprojectToRefZ(const QMatrix4x4& pv, const QPointF& sp, int w, int h, asec::Vec2& out) const;   // 화면 점 → 기준 높이 평면 교점(로컬) — screenToLocalXY · contentFits
    bool screenToLocalXY(const QPointF& sp, asec::Vec2& out) const;
    QPointF localToScreen(double x, double y, double z) const;
    double refZ() const;
    void paintOverlay(QPainter& p);
    void paintEmpty(QPainter& p);
    void paintLodCard(QPainter& p);
    asec::Vec2 lockAxis(const asec::Vec2& w, Qt::KeyboardModifiers m) const;
    void emitLine(bool final) { if (onLineChanged && hasLine_) onLineChanged(line_, final); }
};
