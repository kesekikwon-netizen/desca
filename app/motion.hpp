// 움직임(디자인 최종 G9 · 캔버스 「모션」 화판): 움직임은 상태가 바뀌었음을 알리는 신호이지 장식이 아니다.
//  - 모든 시간 · 곡선은 이 파일 한 곳의 상수(숫자를 코드 여기저기에 적지 않는다). 곡선은 OutCubic 하나.
//  - UI 전환은 300 ms 를 넘지 않는다(여는 화면 1 초는 UI 전환이 아님). 바운스 · 스프링 · 저절로 움직이는 그림 없음.
//  - Windows 「애니메이션 효과」가 꺼져 있거나(SystemParametersInfo) 설정 ui/animations 가 꺼져 있으면 전부 즉시(시간 0).
//  - 자동화(--ui-audit · --shot · 내보내기 …)는 늘 움직임 없이 돈다(setForcedOff).
// Q_OBJECT 없음.
#pragma once
#include <QEasingCurve>
#include <QList>
#include <QStackedWidget>

namespace motion {
// ---- 시간(ms). 이름 = 무엇이 움직이나(캔버스 표). 한 곳에서만 정한다
constexpr int kFrameMs = 16;           // 자체 타이머 프레임(GL 보기 확대 보간)
constexpr int kHover = 100;            // 마우스 위 · 놓음(바탕색만)
constexpr int kToggle = 120;           // 켜짐 ↔ 꺼짐(바탕색 보간)
constexpr int kTooltip = 80;           // 툴팁 나타남(fade, 커서 아래 12 px)
constexpr int kTab = 120;              // 탭 전환(홈 ↔ 단면 ↔ 도면) 작업 영역 크로스페이드
constexpr int kRibbon = 150;           // 리본 묶음 흐림 ↔ 또렷
constexpr int kChipIn = 160;           // 안내 칩이 위에서 8 px 내려오며 fade in(사라질 때는 fade out 만)
constexpr int kUndoIn = 100;           // 되돌리기 칸(도구 들 때)
constexpr int kCrossfade = 150;        // 단면 미리보기 → 최종: 영상만 크로스페이드(잘린 선은 즉시)
constexpr int kLoadingDelay = 300;     // 「최종 계산 중…」 상자 · 진행 막대는 이 시간 뒤에야(그 안에 끝나면 안 보임)
constexpr int kTile = 120;             // 평면 LOD 타일 fade in
constexpr int kZoom = 180;             // 확대 · 축소 단추 · 키 — 축척 보간
constexpr int kFit = 240;              // 맞춤(F) · 위에서(T) — 가운데 · 축척 · 기울기 보간
constexpr int kVex = 200;              // 세로 과장 ×N — 세로 축척만 보간
constexpr int kRowSelect = 100;        // 목록 줄 고름
constexpr int kRowNew = 160;           // 새 단면 줄 펼침(높이 0 → 48 + fade)
constexpr int kNoticeOpen = 160;       // 알림 아래로 펼침
constexpr int kNoticeClose = 120;      // 알림 닫힘 fade
constexpr int kSheetTab = 150;         // 도면 탭 열기 · 종이 fade
constexpr int kSaved = 150;            // 「n장 저장됨 · 폴더 열기」 fade in
constexpr int kStatusText = 120;       // 상태줄 문장 글자 크로스페이드
constexpr int kSplash = 1000;          // 여는 화면 전체 길이(UI 전환이 아님)
constexpr int kSplashEnd = 300;        // 건너뜀 · 애니메이션 끔: 마지막 장면을 보이는 시간
constexpr int kSplashToWindow = 150;   // 여는 화면 → 창 크로스페이드

struct Entry { const char* name; int ms; bool ui; };   // ui = UI 전환(300 ms 를 넘으면 안 됨)
/// 위 상수 전부의 표(감사 · 문서) — 상수를 더하면 여기에도 더한다(감사가 짝을 센다)
const QList<Entry>& table();
/// 곡선: OutCubic(cubic-bezier .33 1 .68 1) 하나
QEasingCurve easing();
/// 움직임을 써도 되는가: 강제 끔(자동화)이 아니고, 설정 ui/animations(없으면 Windows 「애니메이션 효과」)가 켜져 있을 때
bool animationsOn();
void setForcedOff(bool off);
/// 움직임이 꺼져 있으면 0, 아니면 base
inline int ms(int base);

// ---- 부품
/// 탭 전환을 크로스페이드로 하는 스택(QStackedWidget 대체). 모양이 바뀌기 직전 그림을 겹쳐 놓고 kTab 동안 투명해진다. 움직임이 꺼져 있으면 즉시
class FadeStack : public QStackedWidget {
public:
    explicit FadeStack(QWidget* parent = nullptr) : QStackedWidget(parent) {}
    void setCurrentIndex(int index);        // QStackedWidget 슬롯을 가린다 — 이 형으로 부르는 곳은 전부 크로스페이드
    void setCurrentWidget(QWidget* w);
    bool fading() const;
};
/// 이미 보이는 위젯을 투명 → 불투명으로(위에서 slideDownPx 내려오며). 끝나면 효과를 지운다. 꺼져 있으면 아무것도 안 한다
void fadeIn(QWidget* w, int durationMs, int slideDownPx = 0);
/// 위젯을 투명하게 만든 뒤 숨긴다(사라질 때 fade out 만). 꺼져 있으면 바로 숨긴다
void fadeOutAndHide(QWidget* w, int durationMs);
/// 지금 이 위젯에서 fade 가 진행 중인가(감사용)
bool fadingNow(const QWidget* w);

inline int ms(int base) { return animationsOn() ? base : 0; }
}  // namespace motion
