#include "motion.hpp"

#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QPointer>
#include <QSettings>
#include <QVariantAnimation>
#include <cmath>

#ifdef Q_OS_WIN
// windows.h 를 끌어오면 min/max 매크로가 std::min 을 깨므로 함수 하나만 선언한다(user32 — Qt 가 이미 링크). SPI_GETCLIENTAREAANIMATION = 0x1042,
// pvParam = BOOL(TRUE = 애니메이션 켜짐) — Microsoft Learn 「SystemParametersInfoW」 · 「Client area animation parameter」
extern "C" __declspec(dllimport) int __stdcall SystemParametersInfoW(unsigned uiAction, unsigned uiParam, void* pvParam, unsigned fWinIni);
#endif

namespace motion {
namespace {
bool g_forcedOff = false;
bool osAnimationsOn() {
#ifdef Q_OS_WIN
    int on = 1;
    if (SystemParametersInfoW(0x1042, 0, &on, 0)) return on != 0;
#endif
    return true;
}
}  // namespace

const QList<Entry>& table() {
    static const QList<Entry> t = {
        {"kFrameMs", kFrameMs, false}, {"kHover", kHover, true}, {"kToggle", kToggle, true}, {"kTooltip", kTooltip, true}, {"kTab", kTab, true}, {"kRibbon", kRibbon, true},
        {"kChipIn", kChipIn, true}, {"kUndoIn", kUndoIn, true}, {"kCrossfade", kCrossfade, true}, {"kLoadingDelay", kLoadingDelay, true}, {"kTile", kTile, true},
        {"kZoom", kZoom, true}, {"kFit", kFit, true}, {"kVex", kVex, true}, {"kRowSelect", kRowSelect, true}, {"kRowNew", kRowNew, true}, {"kNoticeOpen", kNoticeOpen, true},
        {"kNoticeClose", kNoticeClose, true}, {"kSheetTab", kSheetTab, true}, {"kSaved", kSaved, true}, {"kStatusText", kStatusText, true},
        {"kSplash", kSplash, false}, {"kSplashEnd", kSplashEnd, false}, {"kSplashToWindow", kSplashToWindow, true},
    };
    return t;
}

QEasingCurve easing() { return QEasingCurve(QEasingCurve::OutCubic); }

void setForcedOff(bool off) { g_forcedOff = off; }

bool animationsOn() {
    if (g_forcedOff) return false;
    const QVariant v = QSettings().value(QStringLiteral("ui/animations"));
    if (v.isValid()) return v.toBool();   // 사용자가 정했으면 그대로
    return osAnimationsOn();               // 아니면 OS 설정을 따른다
}

// ---- 크로스페이드 겹침: 바뀌기 직전 그림(snapshot)을 위에 얹고 투명해진다. 마우스를 통과시킨다
namespace {
class FadeOverlay : public QWidget {
public:
    FadeOverlay(QWidget* parent, const QPixmap& snap) : QWidget(parent), snap_(snap) {
        setObjectName(QStringLiteral("fadeOverlay"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setGeometry(parent->rect());
    }
    void setAlpha(qreal a) { alpha_ = a; update(); }
protected:
    void paintEvent(QPaintEvent*) override { QPainter p(this); p.setOpacity(alpha_); p.drawPixmap(0, 0, snap_); }
private:
    QPixmap snap_;
    qreal alpha_ = 1.0;
};

// 진행 중인 fade 가 있으면 끝낸다(효과 · 위치를 원래대로)
void finishFades(QWidget* w) {
    for (auto* an : w->findChildren<QVariantAnimation*>(QStringLiteral("motionFade"))) { an->stop(); an->deleteLater(); }
    if (w->property("motionFading").toBool()) {
        w->setGraphicsEffect(nullptr);
        const QVariant b = w->property("motionBase");
        if (b.isValid()) w->move(b.toPoint());
        w->setProperty("motionFading", false);
    }
}
}  // namespace

void FadeStack::setCurrentIndex(int index) {
    if (index == currentIndex() || !isVisible() || !animationsOn()) { QStackedWidget::setCurrentIndex(index); return; }
    const QPixmap snap = grab();
    QStackedWidget::setCurrentIndex(index);
    for (auto* o : findChildren<QWidget*>(QStringLiteral("fadeOverlay"))) { o->hide(); o->deleteLater(); }   // 이전 겹침은 치운다(Q_OBJECT 없는 클래스는 이름으로 찾는다)
    auto* ov = new FadeOverlay(this, snap);
    ov->show(); ov->raise();
    auto* an = new QVariantAnimation(ov);
    an->setDuration(kTab); an->setStartValue(1.0); an->setEndValue(0.0); an->setEasingCurve(easing());
    QObject::connect(an, &QVariantAnimation::valueChanged, ov, [ov](const QVariant& v) { ov->setAlpha(v.toReal()); });
    QObject::connect(an, &QVariantAnimation::finished, ov, [ov] { ov->hide(); ov->deleteLater(); });
    an->start();
}

void FadeStack::setCurrentWidget(QWidget* w) { setCurrentIndex(indexOf(w)); }

bool FadeStack::fading() const {
    for (auto* o : findChildren<QWidget*>(QStringLiteral("fadeOverlay"))) if (o->isVisible()) return true;
    return false;
}

void fadeIn(QWidget* w, int durationMs, int slideDownPx) {
    if (!w || durationMs <= 0 || !animationsOn() || !w->isVisible()) return;
    finishFades(w);
    auto* eff = new QGraphicsOpacityEffect(w);
    eff->setOpacity(0.0);
    w->setGraphicsEffect(eff);
    const QPoint base = w->pos();
    w->setProperty("motionBase", base); w->setProperty("motionFading", true);
    auto* an = new QVariantAnimation(w);
    an->setObjectName(QStringLiteral("motionFade"));
    an->setDuration(durationMs); an->setStartValue(0.0); an->setEndValue(1.0); an->setEasingCurve(easing());
    QPointer<QWidget> pw(w);
    QObject::connect(an, &QVariantAnimation::valueChanged, w, [eff, pw, base, slideDownPx](const QVariant& v) {
        eff->setOpacity(v.toReal());
        if (pw && slideDownPx) pw->move(base.x(), base.y() - int(std::lround(slideDownPx * (1.0 - v.toReal()))));   // 위에서 slideDownPx 내려옴
    });
    QObject::connect(an, &QVariantAnimation::finished, w, [pw] { if (pw) finishFades(pw); });
    an->start();
}

void fadeOutAndHide(QWidget* w, int durationMs) {
    if (!w) return;
    if (durationMs <= 0 || !animationsOn() || !w->isVisible()) { finishFades(w); w->QWidget::setVisible(false); return; }   // setVisible 를 덮어 쓴 위젯(GuideBand)이 다시 이 함수를 부르지 않게 QWidget 쪽을 직접 부른다
    finishFades(w);
    auto* eff = new QGraphicsOpacityEffect(w);
    eff->setOpacity(1.0);
    w->setGraphicsEffect(eff);
    w->setProperty("motionBase", w->pos()); w->setProperty("motionFading", true);
    auto* an = new QVariantAnimation(w);
    an->setObjectName(QStringLiteral("motionFade"));
    an->setDuration(durationMs); an->setStartValue(1.0); an->setEndValue(0.0); an->setEasingCurve(easing());
    QPointer<QWidget> pw(w);
    QObject::connect(an, &QVariantAnimation::valueChanged, w, [eff](const QVariant& v) { eff->setOpacity(v.toReal()); });
    QObject::connect(an, &QVariantAnimation::finished, w, [pw] { if (pw) { finishFades(pw); pw->QWidget::setVisible(false); } });
    an->start();
}

bool fadingNow(const QWidget* w) { return w && w->property("motionFading").toBool(); }
}  // namespace motion
