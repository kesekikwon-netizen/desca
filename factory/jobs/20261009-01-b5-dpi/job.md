---
id: 20261009-01-b5-dpi
title: 디자인 v5 B5 해상도 · DPI 안정성(4K · 와이드 · 배율, Windows만)
status: merged            # queued | spec | build | verify | review | rework | needs-human | approved | merged | closed | rejected
rework_count: 0           # 최대 2. 넘으면 needs-human
ai_review: pass           # pending | pass | fail  (검토자 셋·UI 검토 결과 종합)
user_approval: approved   # pending | approved | rejected  (사용자가 /factory approve 로 말한 뒤에만 approved)
base_branch: feature/design-v2
base_sha: 0f7fbfed7455c0c6db308af386ae935b27d6f77e
branch: factory/20261009-01-b5-dpi
worktree: C:\dev\kerf-wt\20261009-01-b5-dpi
build_dir: C:\dev\kerf-wt\20261009-01-b5-dpi-build
head_sha: b378b3f38811375d08fc6fa19f7f75992c7e6184
last_verify: PASS (2026-10-09 23:54)
human_review_required: CI(`.github/workflows/ci.yml` Linux 작업 삭제 + 감사 판정 방식 변경, SKILL §7) · 화면/감사(`app/main.cpp` `app/mainwindow_ui.cpp` --ui-audit 기대값 추가, profile §8)
created: 2026-10-09 21:37
updated: 2026-10-09 23:54
---
# 20261009-01-b5-dpi — 디자인 v5 B5 해상도 · DPI 안정성(4K · 와이드 · 배율, Windows만)

## 요청 (사용자 말 그대로)
「추가로 해상도 모니터가 4k 와이드모니터 어떤 곳에서도 안정적이게 되어야한다.」(2026-10-09) · 「이것은 리눅스에 개발이 아니다 윈도우만을 대상으로 개발하라.」(2026-10-09) · 「지금 개발설정을 지켜가며 개발하라.」(2026-10-09) · 선택: B5 부터 팩토리로, CI 의 Linux 작업 삭제

## 상태 기록
| 때 | 상태 | 누가 | 메모 |
| --- | --- | --- | --- |
| 2026-10-09 21:37 | queued | factory | 작업 생성. 기준 feature/design-v2 @ 0f7fbfed7455c0c6db308af386ae935b27d6f77e |
| 2026-10-09 21:50 | spec | spec-writer | spec.md AC1–AC11. 브리프와 다른 판정 2(DEV_RULES R2.7 은 본 저장소에서 · CI 감사는 Start-Process 로 기다려 판정). 열린 질문 Q1(첫 실행 창 크기 1600×950 이 모니터보다 클 때) → 사용자에게 물음 |
| 2026-10-09 21:52 | build | factory | Q1 사용자 답 「이번에는 빼고 대기 목록에」 → backlog #2. builder 시작 |
| 2026-10-09 22:55 | verify | builder → factory | 커밋 7(b5640c0 RED → 30a8413 · c9b081e · 504e6c1 · 73493e2 · 066455a · 66737c5), builder 자체 V8 RESULT PASS(logs\20261009-224927). 판정 4(build.md). 주의: Task 2 커밋 메시지(BOM) 교정으로 자기 브랜치 역사 재적재(push 전 · factory 브랜치만). test-runner 독립 검증 시작 |
| 2026-10-09 23:20 | verify | test-runner | PASS — V8(8가지) · AC6(1.25 · 1.75) 재실행 모두 RESULT PASS, 회귀 3 · CI 모의(양성 · 음성) 통과, 이번 변경 탓 실패 없음. 미실행 4: AC3 수동(모니터 둘) · AC8 ③(push 전 Actions) · AC9(ui-reviewer 몫) · AC10(실제 화면 재현 · 사람 절차). 알린 것: 1280 창 단면 머리 겹침(감사 미검출) |
| 2026-10-09 23:22 | review | factory | 검토자 넷 시작(code · correctness · security · kerf-ui). AC10 실제 화면 감사는 팩토리가 직접 1회 재현(C:\dev\tmp\b5-ac10) → EXIT 0 RESULT ok |
| 2026-10-09 23:40 | needs-human | factory | 검토 결과: code pass(should 3 · note 4) · correctness pass(should 2 · note 6) · security pass(note 4) · **UI 충돌**: 1차 fail(blocking 2 — 1280 단면 머리 겹침 · 리본 1280 look=20≠스펙 32) vs 2차 pass(둘 다 전부터 있던 것: RED 실행에서 이미 min-width=1280, 머리 코드 불변; AC9 1280 루브릭 미충족은 사실). **막힌 것:** 전부터 있던 1280 좁은 창 문제를 이 작업에서 고칠지(rework) · 후속 작업으로 뺄지, 리본 1280 타일 기대값(스펙 32 vs 실제 20) 디자인 결정. 선택지는 사용자에게 AskUserQuestion |
| 2026-10-09 23:50 | review (승인 대기) | factory | 사용자 답: 「후속 작업으로 빼고 B5 는 승인 보고서로」 · 「스펙을 실제(1280 → 20)로 고친다」 → 1280 좁은 창 문제 + should 7건 = backlog #3c. review.md 종합 ai_review pass, approval.md 승인 보고서 작성. **사용자 승인 대기**(/factory approve 20261009-01-b5-dpi) |
| 2026-10-09 23:58 | approved | 사용자 → factory | 사용자 「승인 (추천)」 · 「병합까지 진행 (추천)」. 승인 SHA 66737c5. SKILL §5 병합 절차 시작(worktree 에 feature/design-v2 merge → 전체 검증 → 재승인 → 본 저장소 --no-ff). push 없음 |
| 2026-10-10 00:07 | merged | factory | 재승인(b378b3f) 뒤 본 저장소 feature/design-v2 에 --no-ff 병합 → **8cc67fb**. push 없음(사용자 말 때). GitHub Actions 는 push 뒤 `factory ci 8cc67fb -Wait`. worktree · 브랜치 삭제는 사용자 확인 뒤 |
| 2026-10-10 00:42 | merged(정리됨) | 사용자 → factory | 사용자 「지운다 (추천)」 → `git worktree remove C:\dev\kerf-wt\20261009-01-b5-dpi` · `git branch -d factory/20261009-01-b5-dpi`(was b378b3f) · 빌드 폴더 삭제. 로그(verify 원본)는 verify.md 요약만 남음 |

## 참조
- 명세 `spec.md` · 구현 요약 `build.md` · 검증 `verify.md` · 검토 `review.md` · 승인 `approval.md`
