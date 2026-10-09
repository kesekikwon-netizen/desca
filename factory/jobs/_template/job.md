---
id: {{ID}}
title: {{TITLE}}
status: queued            # queued | spec | build | verify | review | rework | needs-human | approved | merged | closed | rejected
rework_count: 0           # 최대 2. 넘으면 needs-human
ai_review: pending        # pending | pass | fail  (검토자 셋·UI 검토 결과 종합)
user_approval: pending    # pending | approved | rejected  (사용자가 /factory approve 로 말한 뒤에만 approved)
base_branch: {{BASE_BRANCH}}
base_sha: {{BASE_SHA}}
branch: {{BRANCH}}
worktree: {{WORKTREE}}
build_dir: {{BUILD_DIR}}
head_sha:                 # verify.ps1 가 채운다(마지막 검증 때의 worktree HEAD)
last_verify:              # verify.ps1 가 채운다: PASS|FAIL (시각)
human_review_required:    # 사람 검토 필수 영역에 해당하면 그 이유(좌표계·단위·수직 기준·수치 알고리즘 / 파일 형식·호환 / 원본 덮어쓰기·삭제 / 빌드·의존성·패키징·CI / 비밀정보·권한)
created: {{NOW}}
updated: {{NOW}}
---
# {{ID}} — {{TITLE}}

## 요청 (사용자 말 그대로)
{{REQUEST}}

## 상태 기록
| 때 | 상태 | 누가 | 메모 |
| --- | --- | --- | --- |
| {{NOW}} | queued | factory | 작업 생성. 기준 {{BASE_BRANCH}} @ {{BASE_SHA}} |

## 참조
- 명세 `spec.md` · 구현 요약 `build.md` · 검증 `verify.md` · 검토 `review.md` · 승인 `approval.md`
