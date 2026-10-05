// 평면 보기: 위에서 본 정사(orthographic) 텍스처 메시 + 빨간 단면선·두께 띠(괄호선) 편집
#pragma once
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>
#include <functional>
#include <memory>
#include "asec/section.hpp"
#include "asec/tmx.hpp"

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
    bool hasScene() const { return !meshes_.empty(); }
    void clearScene();

    void setDrawMode(bool on);
    bool drawMode() const { return drawStage_ >= 0; }
    void setLine(const asec::SectionLine& l, bool has);
    const asec::SectionLine& line() const { return line_; }
    bool hasLine() const { return hasLine_; }
    void setBand(double front, double back);

    void fitAll();
    void topView();
    void zoomBy(double f);
    /// 현재 화면에 보이는 로컬 XY 범위(평면 기준)
    bool viewRectLocal(asec::Box3& out) const;
    const asec::Box3& bounds() const { return bounds_; }
    double metersPerPixel() const { return mpp_; }

    std::function<void(const asec::SectionLine&, bool final)> onLineChanged;
    std::function<void(double X, double Y, double Z, bool hasZ, bool valid)> onCursor;  // 실좌표
    std::function<void(bool)> onDrawModeChanged;
    std::function<void()> onOpenRequest;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    struct Gpu { GLuint vbo = 0, ibo = 0, tex = 0; GLsizei count = 0; bool hasTex = false; };
    std::vector<std::shared_ptr<DisplayMesh>> meshes_;
    std::vector<Gpu> gpu_;
    bool needUpload_ = false;
    std::unique_ptr<QOpenGLShaderProgram> prog_;
    HeightIndex hidx_;
    asec::Vec3 center_;  // 로컬 장면 중심
    asec::Box3 bounds_;  // 로컬
    asec::SrsInfo srs_;
    // 카메라(중심 기준 좌표)
    QVector3D target_;
    double mpp_ = 0.01, yaw_ = 0, pitch_ = 90;
    QMatrix4x4 proj_, view_;
    // 단면선(로컬 XY)
    asec::SectionLine line_;
    bool hasLine_ = false;
    int drawStage_ = -1;  // -1 편집, 0 A 대기, 1 B 대기
    int dragHandle_ = -1; // 0 A, 1 B, 2 가운데(전체 이동)
    QPointF dragStartMouse_; asec::SectionLine dragStartLine_;
    bool panning_ = false, orbiting_ = false;
    QPoint lastMouse_;

    void freeGpu();
    void upload();
    void updateMatrices();
    bool screenToLocalXY(const QPointF& sp, asec::Vec2& out) const;
    QPointF localToScreen(double x, double y, double z) const;
    double refZ() const;
    void paintOverlay(QPainter& p);
    void paintEmpty(QPainter& p);
    void emitLine(bool final) { if (onLineChanged && hasLine_) onLineChanged(line_, final); }
};
