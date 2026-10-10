---
id: 20261010-01-narrow-1280
title: 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건
status: review            # queued | spec | build | verify | review | rework | needs-human | approved | merged | closed | rejected
rework_count: 3           # 최대 2. 넘으면 needs-human
ai_review: pending        # pending | pass | fail  (검토자 셋·UI 검토 결과 종합)
user_approval: pending    # pending | approved | rejected  (사용자가 /factory approve 로 말한 뒤에만 approved)
base_branch: feature/design-v2
base_sha: 308daac82e8e46189600488c209d43020e046cc2
branch: factory/20261010-01-narrow-1280
worktree: C:\dev\kerf-wt\20261010-01-narrow-1280
build_dir: C:\dev\kerf-wt\20261010-01-narrow-1280-build
head_sha: 02688be73475427ddd17ef34c4420bb0160997ea
last_verify: PASS (2026-10-10 10:24)
human_review_required: 화면 · 감사(`app/mainwindow_ui.cpp` --ui-audit 판정 추가 · 리본 1280 기대값 32 → 20 · DESIGN_SPEC 기대값 변경, profile §8). CI · 좌표 · 파일 형식 해당 없음
created: 2026-10-10 00:03
updated: 2026-10-10 10:35
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
| 2026-10-10 07:40 | review | builder → factory | rework 커밋 `c9214e2`(DESIGN_SPEC +3줄만). 확인: `git diff a2e2228..HEAD -- app core tests tools CMakeLists.txt app/CMakeLists.txt .github` 비어 있음(코드 불변) · AC13 키 9개 모두 §4 · §5 · §7 · §12 에 grep 됨 · `factory verify -UiAudit off` RESULT PASS(build 새 경고 0 · tests 51325 · asec-section 54). 감사 8조합은 코드가 같은 a2e2228 결과(07:30 PASS) 그대로. AC13 통과 → 검토 진행 중(넷) |
| 2026-10-10 08:20 | rework(2/2, 마지막) | factory | 검토: code fail(B1 홈 「… 기준」 글 사라짐) · UI fail(B1 1280 카드 칸 이름 줄임) · correctness pass(should 4) · security pass. 충돌(UI N6 ↔ code B1)은 캡처 증거로 code 쪽. builder 에게 blocking 2 + 같은 뿌리 should 5(감사 보강 · 정보 줄 빈 구간 지킴 · EPSG 값 줄임 · 머리 맞추기 한 번 · 메뉴 ▾) — review.md 종합 그대로. 다시 blocking 이면 needs-human |
| 2026-10-10 09:05 | verify → review(2차) | builder → factory | rework 2 커밋 5(`50abbd3` RED 감사 보강 → `a973c2a` 홈 · `4d5f979` 정보 줄 빈 구간 꼬리 · `70b46b4` 머리 맞춤 한 번 · `00c9a43` 메뉴 ▾ 구운 아이콘 + DESIGN_SPEC §4 · §7 · §12). 지적 7건 모두 고침(반론 0, code B1 은 검토자 대안 ② 채택 — 판정 build.md). builder 자체 V8 PASS(09:00) · 1280 ×1.25/×1.75 PASS(09:03) · icon-sheet · sheet-check · ctx-shot EXIT 0 · core/tests/CMake/CI diff 없음. 범위 밖 한 줄 `setAccessibleName("그릴 것")`(code note 1). test-runner 독립 검증 + 검토자 넷 재검토 동시 시작(검토는 코드만 읽고 검증은 빌드 폴더만 써서 서로 안 겹침) |
| 2026-10-10 09:31 | verify(2차, rework 2, HEAD 00c9a43) | test-runner | **PASS · 실패 0 · 미실행 5**. V8 RESULT PASS(8조합 `RESULT ok`, exit 0) · V2 RESULT PASS(×1.25 · ×1.75 `RESULT ok`, exit 0, V8 뒤에 순서대로) · 회귀 `--icon-sheet` · `--sheet-check` · `--ctx-shot` 모두 EXIT 0 · 긴 이름 1280 감사 ok · core/tests/CMake/CI diff 비어 있음 · 새 키(`carets=1` · `refit=1/0` · `gap-kept=1` · `cells-elided=0` · `home-clipped=0` · `sec-header-overlap=0` · `home-stale-children=0`) 10조합 모두 출력. AC1–AC10 · AC12–AC14 통과, 미실행 = AC11 루브릭(kerf-ui-reviewer) · 판별력 임시 변경 증명 4건(코드 수정 금지라 build.md 인용) · 전체 시험 · 실제 125/175 % · 3440/3840 실제 화면 · 목록에서 지우기 클릭 등 실제 화면 확인. 메모: ▾ 는 감사가 속성(`caretBaked`)만 믿고 캡처 확대로는 매우 작게(약 6×3.6 px) 보임 → 검토자 판단. verify.md 「검증 2차(rework 2, HEAD 00c9a43)」 |
| 2026-10-10 09:55 | review → 사용자 결정 대기 | factory | 재검토 넷 모두 pass(code should 3 · correctness should 2 · security note 3 · UI should 2, 충돌 없음) + test-runner 2차 PASS → `ai_review: pass`, 승인 보고서 approval.md. **자동 승인하지 않음**: ⓐ 1920 이상 홈 카드 칸 배치 바뀜(명세 §1 「1920 이상 모양 그대로」와 어긋남 — 앱 모양 결정이라 R1.5 예외) ⓑ ▾ 가 거의 안 보임(UI S1 미해결). 고치려면 수정 3회차 = 한도 초과라 사용자 허가 필요(SKILL §2) → AskUserQuestion |
| 2026-10-10 10:00 | rework(3, 사용자 허가) | 사용자 → factory | 사용자 답(AskUserQuestion) 「한 번 더 고친 뒤 병합 (추천)」 — 한도 2 를 넘는 3회차는 사용자 결정(SKILL §2). 범위는 ⓐ ⓑ 만: ⓐ 1920 이상 홈 카드 칸은 옛 2:1(내용 폭 규칙은 옛 비율로 줄임표가 생길 때만) + DESIGN_SPEC §7 「좁을 때만」 ⓑ ▾ 를 보이게(테 먼저 · 채움 나중, 단추 폭 그대로) + 감사를 픽셀 판정으로. 다른 should 는 후속(review.md 종합 2차). 끝나면 V8 · V2 + 2560x1440 감사 · 화면 재검토 → 승인 · 병합 |
| 2026-10-10 10:35 | verify → review(3차) | builder → factory | 회차 3 커밋 3(`97f5525` RED: `cells-mode` · `carets` 픽셀 판정 → `5526b5c` ⓐ 넉넉하면 폴더 2 : 나머지 1, 줄임표 생길 때만 내용 폭 → `02688be` ⓑ ▾ 8 × 5 · 테 먼저 · 펜 없이 채움 · 감사 `caret-ink`). builder V8 PASS(10:17) · 1280/2560 ×1.25/×1.75 PASS(10:22) · 2560 ×1 PASS(10:24) · 회귀 셋 ok. builder 는 보고 직전 문맥 한도로 멈췄으나 커밋 · build.md · verify.md 는 다 써 둠(worktree 깨끗). 팩토리 눈 확인: `home-1920.png` 2 : 1 · `caret-crop-1280.png` ▾ 가 리본 ▾ 와 같은 크기. **판정:** 재검토는 code + kerf-ui(이번 차분은 아이콘 그리기 · 홈 칸 폭 · 감사 판정 · 문서뿐 — 입력 · 파일 · 좌표 무관이라 security · correctness 는 2차 pass 유지) + test-runner 독립 검증 |

## 참조
- 명세 `spec.md` · 구현 요약 `build.md` · 검증 `verify.md` · 검토 `review.md` · 승인 `approval.md`
