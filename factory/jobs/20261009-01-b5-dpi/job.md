---
id: 20261009-01-b5-dpi
title: 디자인 v5 B5 해상도 · DPI 안정성(4K · 와이드 · 배율, Windows만)
status: verify            # queued | spec | build | verify | review | rework | needs-human | approved | merged | closed | rejected
rework_count: 0           # 최대 2. 넘으면 needs-human
ai_review: pending        # pending | pass | fail  (검토자 셋·UI 검토 결과 종합)
user_approval: pending    # pending | approved | rejected  (사용자가 /factory approve 로 말한 뒤에만 approved)
base_branch: feature/design-v2
base_sha: 0f7fbfed7455c0c6db308af386ae935b27d6f77e
branch: factory/20261009-01-b5-dpi
worktree: C:\dev\kerf-wt\20261009-01-b5-dpi
build_dir: C:\dev\kerf-wt\20261009-01-b5-dpi-build
head_sha: 66737c515c4deac0b4ba08fbc4ff75af9f0252eb
last_verify: PASS (2026-10-09 23:00)
human_review_required: CI(`.github/workflows/ci.yml` Linux 작업 삭제 + 감사 판정 방식 변경, SKILL §7) · 화면/감사(`app/main.cpp` `app/mainwindow_ui.cpp` --ui-audit 기대값 추가, profile §8)
created: 2026-10-09 21:37
updated: 2026-10-09 23:00
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

## 참조
- 명세 `spec.md` · 구현 요약 `build.md` · 검증 `verify.md` · 검토 `review.md` · 승인 `approval.md`
