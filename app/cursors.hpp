// 도구 커서(디자인 최종 G8 · 캔버스 「Trench 최종 디자인」 커서 화판): 도구를 든 동안(긋기 · 잘린 돌 · 윤곽)과 손잡이 위에서만 바뀌는 십자 + 단계 배지.
// 32×32 논리 px · 핫스폿 (16,16) · 선 Ink 1.5 + 바깥 흰 테 1(Ground) · 십자 안쪽 5 px 는 비어 있다(찍을 점이 안 가려짐).
// 배지(오른쪽 아래, 핫스폿에서 2 px): 흰 알약 14×12 · Ink 테 1 · 글자 9/700. 배율별로 굽고 캐시한다(iconDprs() 와 같은 배율).
// Q_OBJECT 없음 · 색은 theme.hpp 토큰만.
#pragma once
#include <QCursor>
#include <QPixmap>
#include <QString>

namespace kerf {
enum class ToolCursor {
    DrawA,     // 단면선 긋기 › A 찾는 중 — 십자 + 빨강 「A」
    DrawB,     // 단면선 긋기 › A′ 찾는 중 — 십자 + 빨강 「A′」. shift(축 맞춤) 이면 십자가 Ink2 · 굵게
    Hatch,     // 잘린 돌(H, 단면 화면) — 십자 + 빗금
    Outline,   // 윤곽(O, 평면) — 십자 + 점. shift(가장자리 붙음) 이면 점이 고리 ○
    Handle,    // 평면의 A · A′ · 가운데 손잡이 위 — 십자 + 빨간 고리(이 점을 끌 수 있음)
};
constexpr int kToolCursorCount = 5;
constexpr int kCursorLogical = 32;   // 논리 px
constexpr int kCursorHot = 16;       // 핫스폿(논리 px)

/// dpr 로 구운 커서 그림(devicePixelRatio = dpr, 크기 = 32 × dpr 물리 px). 시험 · --cursor-sheet 가 직접 본다
[[nodiscard]] QPixmap toolCursorPixmap(ToolCursor kind, qreal dpr, bool shift = false, bool highContrast = false);
/// 화면에 쓰는 QCursor(핫스폿 16,16 논리 px). 배율 · 종류 · 보조키 · 고대비별 캐시
[[nodiscard]] QCursor toolCursor(ToolCursor kind, qreal dpr, bool shift = false, bool highContrast = false);
/// --cursor-sheet: 모든 종류 × 배율 1 · 2 · 3 을 격자 PNG 로 쓰고 눈 검사(핫스폿 · 크기 · 중앙 틈 · 십자 · 배지/고리). 실패면 false(종료 코드 11)
bool writeCursorSheet(const QString& pngPath, QString* report);
}  // namespace kerf
