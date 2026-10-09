---
id: 20261010-01-narrow-1280
title: 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건
status: spec              # queued | spec | build | verify | review | rework | needs-human | approved | merged | closed | rejected
rework_count: 0           # 최대 2. 넘으면 needs-human
ai_review: pending        # pending | pass | fail  (검토자 셋·UI 검토 결과 종합)
user_approval: pending    # pending | approved | rejected  (사용자가 /factory approve 로 말한 뒤에만 approved)
base_branch: feature/design-v2
base_sha: 308daac82e8e46189600488c209d43020e046cc2
branch: factory/20261010-01-narrow-1280
worktree: C:\dev\kerf-wt\20261010-01-narrow-1280
build_dir: C:\dev\kerf-wt\20261010-01-narrow-1280-build
head_sha:                 # verify.ps1 가 채운다(마지막 검증 때의 worktree HEAD)
last_verify:              # verify.ps1 가 채운다: PASS|FAIL (시각)
human_review_required: 화면 · 감사(`app/mainwindow_ui.cpp` --ui-audit 판정 추가 · 리본 1280 기대값 32 → 20 · DESIGN_SPEC 기대값 변경, profile §8). CI · 좌표 · 파일 형식 해당 없음
created: 2026-10-10 00:03
updated: 2026-10-10 00:03
---
# 20261010-01-narrow-1280 — 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건

## 요청 (사용자 말 그대로)
(B5 검토 뒤 사용자 결정 2026-10-09) 「후속 작업으로 빼고 B5 는 승인 보고서로」 · 「스펙을 실제(1280 → 20)로 고친다」 · /goal 「완료까지 남은 개발작업을 플랜을 작게 나누고 나눠서 /factory 개발을 진행」 · 「단계마다 깃커밋해줘」

## 상태 기록
| 때 | 상태 | 누가 | 메모 |
| --- | --- | --- | --- |
| 2026-10-10 00:03 | queued | factory | 작업 생성. 기준 feature/design-v2 @ 308daac82e8e46189600488c209d43020e046cc2 |
| 2026-10-10 00:35 | spec | spec-writer | spec.md AC1–AC13(Task 1–10). 열린 질문 3(머리 줄이기 순서 · 정보 줄 숨기기 순서 · 세로 ×N 뒤 창 크기) → 사용자에게 물음. 사람 검토 필수(화면 · 감사) |

## 참조
- 명세 `spec.md` · 구현 요약 `build.md` · 검증 `verify.md` · 검토 `review.md` · 승인 `approval.md`
