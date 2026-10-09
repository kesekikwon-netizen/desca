# factory/jobs — 작업 기록 형식

작업 하나 = 폴더 하나 `factory/jobs/<ID>/`. ID 는 `YYYYMMDD-NN-slug`(예 `20261009-01-readme-cli`). `_template/` 은 양식이라 상태 표에서 뺀다.

| 파일 | 누가 쓰나 | 내용 |
| --- | --- | --- |
| `job.md` | new-job.ps1 · verify.ps1 · 스킬 | 머리말(YAML 비슷한 `key: value`)에 상태 · 기준 SHA · 변경 SHA · worktree · 검토/승인 플래그, 본문에 사용자 요청 원문과 상태 기록 표 |
| `spec.md` | spec-writer | 목표 · 범위 · **완료 기준마다 검증 방법** · 회귀 시험 계획 · 허용오차 근거 · 열린 질문 |
| `build.md` | builder | 바꾼 파일 · 커밋 · 명세와 다르게 한 판정 · 기대값 변경 근거 · 수정 회차 |
| `verify.md` | verify.ps1(블록 덧붙임) · test-runner(완료 기준 표) | 실제 실행 명령 · 종료 코드 · 요약 · 로그 파일 이름. 기존 실패와 이번 실패 구분 |
| `review.md` | 검토자 넷 · 스킬(종합) | 결함 · 정확성 · 보안 · UI 지적과 blocking/should/note, 종합 판정, 검토 충돌 |
| `approval.md` | 스킬 · 사용자 말 | 승인 보고서, 사용자 승인 줄(말 그대로 · SHA), 승인 뒤 변경, 병합 기록 |

## 상태 흐름
`queued → spec → build → verify → review → (rework → verify → review …) → approved → merged`
갈라지는 곳: `verify` 실패 또는 `review` blocking → `rework`(최대 2회), 2회 넘거나 검토 충돌 · 환경 부족 → `needs-human`.
`approved` 는 **사용자가** `/factory approve <ID>` 라고 말한 뒤에만. AI 검토 통과는 `ai_review: pass` 로만 표시한다.
`merged` 는 사용자가 따로 「병합」을 말하고, 최신 대상 브랜치와 통합한 뒤 검증을 다시 돌려 통과했을 때만.

## 무엇을 Git 에 넣나
- 넣는다: 이 폴더의 `.md` 전부(승인 근거). 커밋은 사용자가 말할 때(저장소 규칙).
- 넣지 않는다: 빌드 폴더 `C:\dev\kerf-wt\<ID>-build\`(로그 포함), 캡처 PNG, 실제 모델 · 실좌표가 보이는 것(저장소는 공개).
- 그래서 `verify.md` 의 요약 줄에 **통과 수 · 기준값 줄 · RESULT 줄 · 종료 코드**가 들어가야 한다. 로그가 지워져도 승인 근거가 남게.

## worktree 정리
병합되거나 닫힌 작업의 worktree 는 사용자 확인 뒤 `git worktree remove C:\dev\kerf-wt\<ID>` 와 빌드 폴더 삭제. 브랜치 `factory/<ID>` 삭제도 사용자 확인 뒤.
