---
id: 20261010-01-narrow-1280
title: 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건
status: rework            # queued | spec | build | verify | review | rework | needs-human | approved | merged | closed | rejected
rework_count: 1           # 최대 2. 넘으면 needs-human
ai_review: pending        # pending | pass | fail  (검토자 셋·UI 검토 결과 종합)
user_approval: pending    # pending | approved | rejected  (사용자가 /factory approve 로 말한 뒤에만 approved)
base_branch: feature/design-v2
base_sha: 308daac82e8e46189600488c209d43020e046cc2
branch: factory/20261010-01-narrow-1280
worktree: C:\dev\kerf-wt\20261010-01-narrow-1280
build_dir: C:\dev\kerf-wt\20261010-01-narrow-1280-build
head_sha: a2e222842d0f16e6ea3ba1472b565d81b0a61657
last_verify: PASS (2026-10-10 05:39)
human_review_required: 화면 · 감사(`app/mainwindow_ui.cpp` --ui-audit 판정 추가 · 리본 1280 기대값 32 → 20 · DESIGN_SPEC 기대값 변경, profile §8). CI · 좌표 · 파일 형식 해당 없음
created: 2026-10-10 00:03
updated: 2026-10-10 05:39
---
# 20261010-01-narrow-1280 — 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건

## 요청 (사용자 말 그대로)
(B5 검토 뒤 사용자 결정 2026-10-09) 「후속 작업으로 빼고 B5 는 승인 보고서로」 · 「스펙을 실제(1280 → 20)로 고친다」 · /goal 「완료까지 남은 개발작업을 플랜을 작게 나누고 나눠서 /factory 개발을 진행」 · 「단계마다 깃커밋해줘」

## 상태 기록
| 때 | 상태 | 누가 | 메모 |
| --- | --- | --- | --- |
| 2026-10-10 00:03 | queued | factory | 작업 생성. 기준 feature/design-v2 @ 308daac82e8e46189600488c209d43020e046cc2 |
| 2026-10-10 00:35 | spec | spec-writer | spec.md AC1–AC13(Task 1–10). 열린 질문 3(머리 줄이기 순서 · 정보 줄 숨기기 순서 · 세로 ×N 뒤 창 크기) → 사용자에게 물음. 사람 검토 필수(화면 · 감사) |
| 2026-10-10 00:40 | build | factory | 사용자 답 Q1 A(글 → 크게 → 세로 아이콘만 → 토글 메뉴 하나) · Q2 A(레벨선 안내 → 입면 뒤 → 점 수 → …) · Q3 사용자 보기 유지(fitted_=false). spec.md §7 에 적음. builder 시작 |
| 2026-10-10 05:30 | build(재개) | factory | 세션 재시작으로 builder 중단(worktree 미커밋 편집 4파일 +195, 커밋 0). 사용자 버그 보고(홈 겹침 · 「목록에서 지우기」 무효) → 원인 확정(`rebuildStartPage` 가 격자 안 위젯을 안 지움 + 두 번 그리기) · 재현 `home-dbg.png` → **spec §8 AC14 추가**(사용자 사전 허가 · 추천안: 새 작업 대신 #3c 에). builder 에게 재개 + AC14 메시지 |
| 2026-10-10 05:40 | verify | builder → factory | 커밋 9(8375cd3 RED → 1a031cd · 1538a7f · 28a2ba7 · c6a93a7 · f8cb170(AC14) · 52d6dc4 · 1b31c02 · a2e2228), builder 자체 V8 PASS(05:23) · 1280 ×1.25/×1.75 PASS(05:25), 판정 10(build.md). test-runner 독립 검증 시작 |
| 2026-10-10 07:30 | verify | test-runner | **FAIL(문서 1건)** — 자동 · 코드 기준 전부 통과(V8 8가지 · V2 1.25 · 1.75 모두 RESULT PASS exit 0, 회귀 icon-sheet · sheet-check · ctx-shot exit 0, core/tests/CMake/CI diff 없음, AC14 캡처 겹침 없음, 새 경고 0). 유일한 실패 = AC13: DESIGN_SPEC §12 에 새 키 `chip-dpr-bad=0` · `vex-fit=1` · narrow `probe/resized` 줄 없음(코드 재검증 불필요, 문서 한 줄 rework). 수동 1(AC11, kerf-ui-reviewer) · 판별력 임시 변경 증명 4건은 제품 코드 수정 금지라 build.md 기록 인용 · 전체 시험(CoalescingWorker 포함) 미실행(core/tests 변경 없음) |
| 2026-10-10 07:35 | rework(1/2) | factory | AC13 문서만 → builder 에게 DESIGN_SPEC §12 새 감사 키 줄(`sec-header-overlap=0` · `strip-clipped=0` · `plan-tick-overlap=0` · `chip-dpr-bad=0` · `vex-fit=1` · `home-stale-children=0` · narrow `probe/resized`) + Q3 「세로 ×N 바꾼 보기는 사용자 보기」 규칙 문장. **판정:** 코드는 a2e2228 그대로라 검토자 넷은 지금 코드 범위(308daac..a2e2228)로 동시에 시작 — rework 뒤 재검증은 문서 diff 확인 + build/tests(`-UiAudit off`)로(코드 불변 증거: `git diff a2e2228..HEAD -- app core tests` 비어 있음). 잘못이면: 문서 rework 가 코드를 건드리면 검토가 그 부분을 못 봄(그 경우 재검토) |

## 참조
- 명세 `spec.md` · 구현 요약 `build.md` · 검증 `verify.md` · 검토 `review.md` · 승인 `approval.md`
