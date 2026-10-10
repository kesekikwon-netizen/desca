# 승인 기록 — 20261010-01-narrow-1280 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건

## 승인 보고서 (AI 검토 통과 뒤 스킬이 작성, 사용자에게 보여 준 것 — 2026-10-10 09:55)
> **사람 검토 필수 영역:** 화면 · 감사(`--ui-audit` 판정 추가 · 리본 1280 기대값 32 → 20 · DESIGN_SPEC 기대값 변경). 좌표 · 파일 형식 · CI 해당 없음.
> **사용자 결정 필요 2건(아래 「실패 · 미실행」 ⓐ ⓑ):** 고치려면 수정 회차가 한도 2 를 넘어 사용자 허가가 필요하다(SKILL §2) → 자동 승인하지 않음.

- 변경 요약: 창 폭 1280 에서 단면 머리(제목 → 「크게」 → 세로 아이콘만 → 토글 메뉴 하나) · 정보 줄(레벨선 안내 → 입면 뒤 → 점 수 → 「잘린 선 n줄」 → 계산 상태, 빈 구간 꼬리는 늘 보임) · 평면 눈금 글 · 홈 한 줄 글이 겹치거나 반 토막 나지 않게 정해진 순서로 줄이고 `--ui-audit` 판정(`sec-header-overlap` · `strip-clipped` · `gap-kept` · `plan-tick-overlap` · `home-clipped` · `cells-elided` · `home-stale-children` · `carets` · `refit` · `chip-dpr-bad` · `vex-fit` · narrow probe)으로 잠금. 리본 1280 기대값 20(사용자 결정). B5 검토 should 7건(ⓐ–ⓖ). **홈 겹침 · 「목록에서 지우기」 무효 버그(AC14, 사용자 보고) 고침** — `rebuildStartPage` 가 격자 안 옛 위젯을 지움(`home-stale-children` 25 → 0).
- 변경 파일(`git diff --stat 308daac..00c9a43`): `app/mainwindow_ui.cpp` +517 · `app/planview.cpp` · `app/planview.hpp` · `app/mainwindow.hpp` · `app/icons.cpp` · `app/icons.hpp` · `app/ribbon.cpp` · `app/ribbon.hpp` · `app/sectionview.cpp` · `app/sheetexport.cpp`(1줄) · `docs/design/DESIGN_SPEC.md` — 11 files, +651 −105. core · tests · CMake · CI 변경 없음.
- 완료 기준별 검증 결과(verify.md 「검증 2차」, test-runner 09:22 · 09:24): AC1–AC10 · AC12–AC14 **통과**(13) · AC11 루브릭 **통과**(kerf-ui-reviewer 2차: 1280 캡처 8항목 모두 4 이상). V8(1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × 1 · 2) `RESULT PASS` exit 0 — 8조합 `ui-audit RESULT ok` · 빌드 새 경고 0 · 빠른 시험 51325 통과 · asec-section `polylines=1 vertices=54`. V2(1280 × 1.25 · 1.75) `RESULT PASS`. 회귀 `--icon-sheet` · `--sheet-check` · `--ctx-shot` EXIT 0. 긴 모델 이름 1280 감사 ok.
- 실패 · 미실행 항목:
  - ⓐ **[사용자 결정] 1920 이상 홈 카드 칸 배치가 바뀜** — 전: 폴더 2 : 나머지 1 / 지금: 폴더 칸이 남는 폭을 전부 받고 나머지 다섯은 오른쪽에 내용 폭으로 몰림(1920 에서 약 600 px 빔, 기준 모니터 2560 에서 더 큼). 명세 §1 「1920 이상 화면 모양은 바뀌지 않는다」와 어긋남(code · UI · correctness 같은 결론). 글은 다 읽히고 겹침 없음.
  - ⓑ **[사용자 결정] 좁은 단면 머리의 ▾ 가 거의 안 보임** — 그리는 순서 탓(채운 뒤 바탕색 테)에 약 2.8 × 1.7 논리 px 점. 감사 `carets` 는 속성만 봐서 통과. → 「구현됐으나 눈에 안 보임 — 미해결」.
  - 미실행(통과로 쓰지 않음): 실제 125 · 175 % 모니터 · 3440 · 3840 실제 화면(이 PC 150 % 하나 — offscreen 으로 갈음) · 판별력 임시 변경 증명 4건 재실행(build.md 인용) · 전체 시험(CoalescingWorker 포함, core/tests 변경 없음) · 「목록에서 지우기」 클릭 · 「그릴 것」 메뉴 연 모양의 실제 화면(감사 키로 간접).
- 독립 검토 결과와 남은 위험: code pass(should 3) · correctness pass(should 2) · security pass(note 3) · kerf-ui pass(should 2), 충돌 없음 — review.md 「종합 2차」. 후속으로 넘길 것: 감사 `refit == 1` · `gap-kept` 숨김 판정 · `fitInfoStrip` 입력 비교 · 감사 실패 줄 라벨 글 · 단면 오른쪽 끝 「A」(전부터). 받아들인 것: 범위 밖 `setAccessibleName("그릴 것")` 한 줄(화면 · 동작 불변).
- 승인 대상 커밋 SHA: `00c9a43844c46e13ea445379e75576e8ceebc498` (기준 `308daac82e8e46189600488c209d43020e046cc2` → 변경 `00c9a43`)
- GitHub Actions: 기준 커밋 `ci(base)` = none(WARN exit 3 — 기준 커밋이 push 되지 않음). 통과로 쓰지 않는다. 참고: 지금 GitHub Windows 작업은 MSVC C2026(`theme.hpp`) 으로 빌드부터 실패 — 대기 R0 에서 고침.
- 되돌리기 방법: 병합 전 = 브랜치 `factory/20261010-01-narrow-1280` 를 병합하지 않음(worktree 삭제) · 병합 뒤 = `git revert -m 1 <병합 커밋>`

## 사용자 승인 (사용자가 /factory approve 20261010-01-narrow-1280 라고 말한 뒤에만)
| 때 | 사용자 말(그대로) | 승인 SHA | 결과 |
| --- | --- | --- | --- |

## 승인 뒤 변경
- 승인 뒤 코드가 바뀌면 여기 적고 다시 검증 · 다시 승인(위 표에 새 줄).

## 병합 (사용자가 따로 「병합」을 말한 뒤에만)
- 최신 대상 브랜치와 통합한 커밋 · 다시 돈 검증(verify.md 블록 시각):
- 병합 커밋 SHA · 때:
- GitHub Actions(push 뒤, `factory ci <병합 커밋> -Wait` 결과 줄 그대로):
