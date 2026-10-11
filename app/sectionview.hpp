// 단면 보기(Quick Section): 입면 영상 + 빨간 단면선 + 10 cm 레벨선·표고 라벨 + 거리축·축척 막대. 흰 바탕 보고서 도면 형식.
#pragma once
#include <algorithm>
#include <QImage>
#include <QWidget>
#include <functional>
#include "asec/engine.hpp"

struct SectionStyle {
    bool showImage = true, showLine = true, showLevels = true;
    bool showTicks = true;   // 거리 눈금(세로선 · 1 m · 5 m 굵게) — 입면 위(T10, D 키)
    bool depthFade = false;  // 입면 깊이 음영(먼 면일수록 옅게) — 계산 단계에서 적용, 내보내기도 같은 설정. 기본 끔(2026-10-10)
    double imageOpacity = 1.0;
    double lineWidthPx = 2.0;   // 화면 단면선 굵기(보기 메뉴 1.5 / 2 / 3 px). 인쇄는 0.35 mm 고정
    double levelMinorPt = 0.2, levelMajorPt = 0.5;   // 레벨선 굵기(pt, 디자인 v5 결정 L): 10 cm 선 · 50 cm(1 m 포함) 선. 내보내기는 이 값 그대로, 화면은 보이게 바닥값
    bool showBaseline = false;  // 기준선 EL(점선 + 「기준선 EL. 57.00 m」)
    double baselineEl = 0;      // 절대 표고(m)
    bool plotScaleBar = true;   // 그림 칸 안 축척 막대(도면은 표제란 띠에 따로 그림)
};

/// 단면 도면 한 장을 그리는 데 필요한 모든 것(스레드 사이 복사용 스냅숏)
// 화면 단면선 그리기 허용 오차(px): 꼭짓점이 그려진 선 중심에서 이만큼 안(눈에 안 보임). 내보내기는 솎지 않음
constexpr double kScreenProfileTolPx = 0.1;

/// 인쇄·조판 단면선 굵기(mm): 화면 선 굵기 2 px 가 0.35 mm. 1.5 → 0.26, 3 → 0.53 mm(사용자 요청 2026-10-11 「조판에서도 적용」)
inline double exportLineMm(double lineWidthPx) { return 0.35 * std::clamp(lineWidthPx, 1.0, 4.0) / 2.0; }

struct SectionDoc {
    asec::SectionResult r;
    SectionStyle st;
    QImage img;
    double imgS0 = 0, imgZ1 = 0, imgRes = 0.01;  // z 로컬
};
/// 화면 변환. ppm = 가로 픽셀/m, vex = 세로 과장(화면 보기만 — 내보내기·도면은 언제나 1)
struct SectionXf { double s0 = 0, zTop = 0, ppm = 100; QRectF plot; double vex = 1; double ppmZ() const { return ppm * vex; } };
struct SectionImgGeo { double s0 = 0, z1Local = 0, res = 0.01; };

/// 조판 확인(--sheet-check)이 글자 상자·글꼴·변환을 재는 기록. 화면 그리기에서는 비운다.
struct SheetPaintProbe {
    QRectF plot, image, frame, compass, legend;
    QVector<QRectF> labels;
    QStringList labelText;
    double fontPx = 0;
    double neighborPx = 0;
    double worldStep = 0;
    bool uniform = true;
    bool drewCutLine = false;
    double levelMinorPx = 0, levelMajorPx = 0;   // 실제로 쓴 레벨선 펜 굵기(painter 단위) — --sheet-check 가 pt 기대값과 비교
    int distTickCount = 0, distTickMajorCount = 0;   // 그린 거리 눈금 수(굵은 눈금 포함 · 굵은 눈금만)
    double distTickStepM = 0, distTickMinorPx = 0, distTickMajorPx = 0;   // 눈금 간격(m) · 실제로 쓴 펜 굵기(painter 단위, 레벨선과 같은 pt 값)
    // 조판 미리보기 마우스 연동(위치도 선 ↔ 단면): paintSheet 가 채움. 단위 = painter 픽셀(종이 원점 기준)
    double sectionS0 = 0, sectionPpm = 0, sectionLenM = 0;   // 단면 칸 왼쪽의 거리(m)·픽셀/m·A–A′ 길이(m)
    QRectF strip;                                            // 위치도 띠(그린 범위). 비어 있으면 위치도 없음
    double stripLineY = 0;                                   // 위치도 안 단면선(t=0)의 y
};

/// 보고서용 그리기(화면·내보내기 공용, 아무 스레드에서나 QImage 에 그릴 수 있음). ppm = 픽셀/m, ui = 글자·선 배율(DPI/96)
void paintSectionDoc(QPainter& p, const SectionDoc& d, const QRectF& area, const SectionXf& xf, double ui, const QImage& img, bool forExport,
                     const QString& footer, const SectionImgGeo* geo = nullptr, bool busy = false, bool titleRow = true,
                     double contentDxPx = 0, double contentDyPx = 0, double viewZoom = 1, double viewPanX = 0, double viewPanY = 0,
                     SheetPaintProbe* probe = nullptr, const QImage* prevImg = nullptr, const SectionImgGeo* prevGeo = nullptr, double newAlpha = 1.0);   // prevImg = 크로스페이드 중 아직 보이는 옛 영상(그 위에 새 영상이 newAlpha 로 섞여 들어온다)
/// 레벨선 간격(화면 4 px / 인쇄 0.5 mm 보다 촘촘하면 솎음). ui = DPI/96
asec::LevelPlan sectionLevelPlan(double ppm, double ui, bool forExport);
/// 내보내기용 크기·변환(여백 포함, 영상 픽셀 1:1 정렬)
QSize sectionExportLayout(const SectionDoc& d, double ppm, double ui, SectionXf& xf);

class SectionView : public QWidget {
public:
    explicit SectionView(QWidget* parent = nullptr);
    void setResult(const asec::SectionResult& r, const QImage& img, double imgS0, double imgZ1Local, double imgRes, bool keepView, bool crossfade = false);   // crossfade = 미리보기 → 최종: 영상만 motion::kCrossfade 동안 섞는다(잘린 선은 즉시)
    bool crossfadeActive() const;
    void clear();
    void setBusy(bool b) { busy_ = b; update(); }
    void setDrawingHint(bool on) { drawingHint_ = on; update(); }   // 그리는 중 「A′를 찍으면 단면이 여기에 나옵니다」 띠(화판 2)
    void setStyle(const SectionStyle& s) { st_ = s; update(); }
    const SectionStyle& style() const { return st_; }
    bool hasResult() const { return has_; }
    const asec::SectionResult& result() const { return r_; }
    void fit();
    /// 감사용: 단면선 전체(s 0–L)가 그림 칸 가로에 들어가는가(창을 줄여도 맞춤이 유지되는지 — B5)
    bool contentFits() const;
    void zoomBy(double f);
    /// 휠 확대/축소(커서 위치의 (s, z) 고정, 부드럽게 — 설정 view/smoothZoom)
    void wheelZoom(const QPointF& at, double notches);
    bool zoomAnimating() const { return zoomPending_ != 0.0; }
    /// 화면 점 → (거리 s, 절대 표고 z)
    bool screenToSZ(const QPointF& at, double& s, double& zAbs) const;
    const SectionXf& xf() const { return xf_; }
    double imgS0() const { return imgS0_; }
    double imgZ1() const { return imgZ1_; }
    double imgRes() const { return imgRes_; }
    const QImage& image() const { return img_; }

    using Xf = SectionXf;
    SectionDoc doc() const;

    std::function<void(double s, double zAbs, double X, double Y, bool valid)> onCursor;
    std::function<void()> onViewChanged;   // 확대·이동 후(정보 띠의 화면 축척)
    /// 화면 축척 1:N (논리 DPI 기준: 96 dpi 면 1 px = 0.2646 mm)
    double screenDenom() const;
    void setScreenDenom(double denom);
    /// 세로 과장(1·2·5·10, 화면만). 화면 가운데 높이를 유지
    void setVerticalExaggeration(double vex);
    double verticalExaggeration() const { return xf_.vex; }
    /// 1:1 로 맞췄을 때 보이는 높이(m) — 세로 과장 추천용
    double fitVisibleHeight() const;

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    asec::SectionResult r_;
    QImage img_;
    double imgS0_ = 0, imgZ1_ = 0, imgRes_ = 0.01;  // z 는 로컬
    bool has_ = false, busy_ = false, drawingHint_ = false;
    bool fitted_ = false;   // 맞춤 상태(fit 뒤 사용자가 확대 · 이동 · 축척 지정을 안 함) — 창 크기가 바뀌면 다시 맞춘다. 사용자 보기는 가운데 · 축척 유지(B5)
    SectionStyle st_;
    Xf xf_;
    double zoomPending_ = 0;
    QImage prevImg_; double prevS0_ = 0, prevZ1_ = 0, prevRes_ = 0.01, fadeAlpha_ = 1.0;   // 미리보기 → 최종 크로스페이드(영상만)
    class QVariantAnimation* fade_ = nullptr;
    QPointF zoomAnchor_;
    class QTimer* zoomTimer_ = nullptr;
    void applyZoomAt(const QPointF& at, double factor);
    bool panning_ = false;
    QPoint last_;
    QRectF plotRect(const QRectF& area, double ui) const;
};
