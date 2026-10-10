# 독립 검토 — 20261010-02-msvc-c2026 MSVC 빌드 고침: theme.hpp QSS 원시 문자열 16 KB 아래로(C2026)

검토 대상: `factory/20261010-02-msvc-c2026` HEAD(verify.md 의 마지막 블록과 같은 SHA 여야 함). 검토자는 제품 코드를 고치지 않는다.
판정: **blocking**(고쳐야 함) · **should**(권장) · **note**(참고). blocking 이 하나라도 있으면 rework.

## code-reviewer (결함 · 유지보수성 · 동시성)
- 판정: **pass** (blocking 0 · should 0 · note 4) — 범위 `373c0e7..044ab95`, `app/theme.hpp` 6+/3−, worktree 미커밋 변경 없음(`git status --porcelain` 빈 출력)
- 확인한 것:
  - **매크로 펼침:** `C:\Qt\6.10.3\msvc2022_64\include\QtCore\qstring.h:1777` `#define QT_UNICODE_LITERAL(str) u"" str`, `:1789` `#define QStringLiteral(str) (QString(QtPrivate::qMakeStringPrivate(QT_UNICODE_LITERAL(str))))`. 인자 `R"(…)" R"(…)" R"(…)" R"(…)"` 는 문자열 리터럴 토큰 4개(쉼표 · 괄호는 리터럴 안이라 인자 분리에 영향 없음) → `u"" R"(…)" …` 로 펼쳐지고, 번역 6단계에서 접두사 있는 것 하나 + 없는 것들 = `u` 접두사 하나의 리터럴로 이어진다(C++11 [lex.string]). `qMakeStringPrivate(const char16_t (&)[N])` 에 그대로 맞는다. MSVC GREEN 빌드(build.md)가 실측 근거.
  - **내용 동일:** 일회성 스크립트(scratchpad, `python -I`)로 `git show 373c0e7:app/theme.hpp` 와 `044ab95` 의 `styleSheet` 첫 `QStringLiteral` 조각을 뽑아 이음 → 옛 1조각 20 241자 · 새 4조각 4 096 / 6 783 / 5 208 / 4 154자(비ASCII 0자라 바이트 = 자), 이은 결과 `equal True`. 각 조각은 `…}\n` 으로 끝나고 `\n…` 으로 시작 → 빈 줄 자리 그대로 복원. builder 의 AC3 출력(len 20241 · sha 같음)과 일치.
  - **`%CHECK%` · `%ARROW%` · 고대비:** 자리표시는 각각 3 · 2 번째 조각 안 한가운데이고 나눈 곳은 빈 줄뿐이라 잘림 없음. `s.replace` 는 실행 때 이어진 QString 에 걸리므로 영향 없음. 고대비 조각 · replace 줄은 diff 에 바뀐 덩어리가 없다.
  - **CRLF:** CI 체크아웃(autocrlf)에서 CR 이 세어진다고 쳐도 가장 큰 조각 6 783 + 약 80줄 < 8 000.
  - **다른 파일:** `app/` · `core/` 의 모든 `.cpp/.hpp` 에서 원시 문자열 본문과 일반 문자열 리터럴 중 2 000 바이트 넘는 것을 찾음 → `theme.hpp` 의 이 4조각 말고 없음(8 000 넘는 것 0). 같은 문제의 다른 자리 없음.
- 지적:
  - **note 1 — `app/theme.hpp:72` 주석의 단위:** MS 문서의 한도는 「16 380 단일 바이트 **문자**」, 유니코드는 「그 절반쯤」이다. 주석은 「8 000 바이트」로 적었는데 지금 QSS 는 ASCII 뿐이라 같고, 앞으로 비ASCII(한글 글꼴 이름 등)가 들어가도 UTF-8 바이트로 세면 자 수보다 커서 더 보수적이다 — 틀린 방향이 아니라 고칠 필요 없음. 「약 16 KB · u"" 는 절반」 설명은 문서 문구와 맞다.
  - **note 2 — Qt 6.8.3(CI) 헤더는 이 PC 에 없어 직접 확인하지 못함.** `C:\Qt` 에는 6.10.3 뿐. Qt 6 계열의 `QStringLiteral` 은 `u"" str` 펼침으로 알려져 있으나 6.8 파일 자체는 확인하지 못함 — 최종 근거는 push 뒤 CI Windows 작업(`factory.cmd ci <sha>`).
  - **note 3 — 검증 기록의 SHA:** verify.md 마지막 PASS 블록은 `373c0e7 (+미커밋 변경 있음)` 18:11 실행이고 커밋 `044ab95` 는 그 뒤에 만들어졌다. 지금 worktree 가 깨끗하고 diff 가 그 변경 하나뿐이라 실질 차이는 없어 보이나, 기록상 「검증한 SHA = 검토 SHA」는 아니다(review.md 머리의 요구). 조정자가 승인 전에 `factory verify` 를 `044ab95` 에서 한 번 더(빠른 것만이라도) 돌리면 깔끔하다.
  - **note 4 — 재발 방지는 주석 + MSVC 빌드뿐:** clang-cl 은 조각 한도가 없어 이 PC 빌드는 QSS 가 다시 커져도 통과한다(명세 §5 위험 3 과 같음). 조각별 `static_assert` 불가 이유는 명세 §4 가 맞게 설명. 받아들일 만함.

## correctness-reviewer (좌표 · 단위 · 수직 기준 · 계산 · 입출력 정확성)
- 판정: pass | fail
- 지적:

## security-reviewer (외부 입력 · 경로 · 파일 쓰기 · 비밀정보)
- 판정: pass | fail
- 지적:

## kerf-ui-reviewer (UI 변경이 있을 때만)
- 판정: pass | fail | 해당 없음
- 지적:

## 종합
- ai_review: pass | fail
- 검토 충돌(검토자끼리 다른 결론): 없음 | 있음 → needs-human
- 남은 위험(고치지 않고 받아들인 것과 이유):
