// 단면 보기(Quick Section): 입면 영상 + 빨간 단면선 + 10 cm 레벨선·표고 라벨 + 거리축·축척 막대. 흰 바탕 보고서 도면 형식.
#pragma once
#include <QImage>
#include <QWidget>
#include <functional>
#include "asec/engine.hpp"

struct SectionStyle {
    bool showImage = true, showLine = true, showLevels = true;
    double imageOpacity = 1.0;
};

/// 단면 도면 한 장을 그리는 데 필요한 모든 것(스레드 사이 복사용 스냅숏)
struct SectionDoc {
    asec::SectionResult r;
    SectionStyle st;
    QImage img;
    double imgS0 = 0, imgZ1 = 0, imgRes = 0.01;  // z 로컬
};
struct SectionXf { double s0 = 0, zTop = 0, ppm = 100; QRectF plot; };
struct SectionImgGeo { double s0 = 0, z1Local = 0, res = 0.01; };

/// 보고서용 그리기(화면·내보내기 공용, 아무 스레드에서나 QImage 에 그릴 수 있음). ppm = 픽셀/m, ui = 글자·선 배율(DPI/96)
void paintSectionDoc(QPainter& p, const SectionDoc& d, const QRectF& area, const SectionXf& xf, double ui, const QImage& img, bool forExport,
                     const QString& footer, const SectionImgGeo* geo = nullptr, bool busy = false);
/// 내보내기용 크기·변환(여백 포함, 영상 픽셀 1:1 정렬)
QSize sectionExportLayout(const SectionDoc& d, double ppm, double ui, SectionXf& xf);

class SectionView : public QWidget {
public:
    explicit SectionView(QWidget* parent = nullptr);
    void setResult(const asec::SectionResult& r, const QImage& img, double imgS0, double imgZ1Local, double imgRes, bool keepView);
    void clear();
    void setBusy(bool b) { busy_ = b; update(); }
    void setStyle(const SectionStyle& s) { st_ = s; update(); }
    const SectionStyle& style() const { return st_; }
    bool hasResult() const { return has_; }
    const asec::SectionResult& result() const { return r_; }
    void fit();
    void zoomBy(double f);
    double imgS0() const { return imgS0_; }
    double imgZ1() const { return imgZ1_; }
    double imgRes() const { return imgRes_; }
    const QImage& image() const { return img_; }

    using Xf = SectionXf;
    SectionDoc doc() const;

    std::function<void(double s, double zAbs, double X, double Y, bool valid)> onCursor;

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
    bool has_ = false, busy_ = false;
    SectionStyle st_;
    Xf xf_;
    bool panning_ = false;
    QPoint last_;
    QRectF plotRect(const QRectF& area, double ui) const;
};
