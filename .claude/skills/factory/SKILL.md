---
name: factory
description: Kerf 소프트웨어 팩토리 — 개발 요청을 명세 → worktree 구현 → 실제 빌드·시험 → 독립 검토 → 수정(최대 2회) → 사용자 승인 대기로 돌린다. /factory <개발 요청> | status | next | rework <ID> | approve <ID>. 자동 push·병합·main 변경·배포 없음. Use when the user says /factory, 팩토리, or asks to run a feature request through spec→build→verify→review.
argument-hint: <개발 요청> | status | next | rework <ID> | approve <ID>
---
<!-- 목적: 이 저장소 전용 개발 흐름. 공통 규칙은 CLAUDE.md, 사실은 factory/project-profile.md, 역할 지침은 .claude/agents/*.md. -->
# Kerf 소프트웨어 팩토리

이 문서는 **스킬이 글로 해석하는 인터페이스**다. `/factory status` 같은 글은 셸 명령이 아니라 아래 표대로 이 스킬이 읽고 수행한다. 사용자는 비개발자다: 한국어로, 실행해서 본 것만 보고한다.

## 0. 먼저 읽는다 (매번)
1. `factory/project-profile.md` — 명령 · 시험 지도 · 허용오차 · 위험 모듈 · 환경 부족.
2. `factory/backlog.md` · `powershell -NoProfile -ExecutionPolicy Bypass -File factory/scripts/status.ps1` — 진행 중 작업.
3. 작업이 있으면 그 `factory/jobs/<ID>/job.md`(상태 · SHA) 와 단계 파일.
4. 디자인 v4 ⏸ 단계에 해당하는 요청이면(`docs/DEV_PROGRESS.md` §3) 시작하지 않고 보류로 적고 사용자에게 알린다.

## 1. 입력 해석
| 입력 | 하는 일 |
| --- | --- |
| `/factory <개발 요청>` | §3 queued 부터 시작. 진행 중 작업이 있으면 backlog 에만 적고 멈춘다(동시 작업 1개) |
| `/factory status` | `status.ps1` 결과 + 작업마다 「지금 상태 · 다음에 할 일 · 사용자가 할 일」 한 줄씩. 아무것도 바꾸지 않는다 |
| `/factory next` | 진행 중 작업이 없으면 backlog 첫 「대기」를 작업으로(§3). 있으면 그 작업의 다음 단계를 한 단계 진행 |
| `/factory rework <ID>` | 사용자 지시로 rework(§3 rework). 사용자의 말을 `build.md` 수정 회차에 그대로 적는다 |
| `/factory approve <ID>` | **사용자 승인**(§4). AI 검토 통과(`ai_review: pass`)와 다르다. 병합은 하지 않는다 |
| 「병합해」 「merge」 (approve 뒤 따로) | §5 병합 — 다시 검증 · 사용자 확인 · 직렬 |

## 2. 상태 흐름
`queued → spec → build → verify → review → approved → merged`
- `verify` 실패 또는 `review` blocking → `rework`(rework_count+1) → `verify` → `review`.
- rework_count 가 2를 넘게 되면 · 검토자끼리 결론이 다르면 · 환경 부족(도구 없음, 실제 모델 없음)으로 검증을 못 하면 → `needs-human`. 사람이 결정할 때까지 멈춘다.
- 상태를 바꿀 때마다 `job.md` 머리말 `status:` 와 본문 「상태 기록」 표에 한 줄(때 · 상태 · 누가 · 메모).

## 3. 단계 절차
### queued
1. 요청을 `factory/backlog.md` 표에 **사용자 말 그대로** 적는다.
2. `status.ps1` 로 진행 중 작업 확인. 있으면 「대기」로 두고 사용자에게 알리고 끝.
3. 없으면 `new-job.ps1 -Slug <영문-slug> -Title "<짧은 제목>" -Request "<사용자 말>"`. 출력의 ID · 기준 SHA · worktree 를 확인하고 backlog 의 그 줄에 작업 ID 를 적는다. 상태 `spec`.

### spec — `spec-writer` 에이전트
- Agent 도구로 `spec-writer` 를 부른다. 브리프에 넣을 것: 작업 ID · `factory/jobs/<ID>/` 경로 · worktree 경로 · 요청 원문 · 관련 파일 추정 · 「`factory/project-profile.md` §6–§8 을 읽고 완료 기준마다 시험 이름이나 확인 절차를 연결하라」.
- 결과 `spec.md` 를 읽고: 「사람 검토 필수 영역」 해당이면 `job.md` 의 `human_review_required:` 에 이유를 적는다. 「열린 질문」이 있으면 AskUserQuestion(추천 먼저)으로 닫은 뒤 진행. 허용오차가 기존 기준에 없으면 사용자 확인 전에는 build 로 가지 않는다.
- 상태 `build`.

### build — `builder` 에이전트(유일한 쓰기 역할)
- 브리프: 작업 ID · worktree 경로(**여기서만 편집 · 커밋**) · `spec.md` 전문 · 「시험 먼저(실패 확인) → 최소 구현 → `verify.ps1 -Id <ID>` 로 빌드 · 시험 → `build.md` 작성 → `factory/<ID>` 브랜치에 커밋」. 본 저장소(`C:\Users\권을\Documents\desc\desca`)는 읽기만.
- builder 가 명세와 다르게 한 것은 `build.md` 「판정」에 있어야 한다. 시험 기대값을 바꿨으면 근거가 있어야 하고, 근거가 「통과시키려고」면 rework 사유.
- 상태 `verify`.

### verify — `test-runner` 에이전트
- 브리프: 작업 ID · 「`verify.ps1 -Id <ID>` 를 돌리고(core/tests 변경이면 `-Full`, app/ 변경이면 ui-audit 포함) `verify.md` 의 완료 기준 표를 채우라. 실패는 기존 실패(profile §9)와 이번 변경의 실패로 나눠 적으라. 제품 코드는 고치지 말라」.
- PASS 이고 완료 기준이 모두 「통과」 또는 「수동(사용자 확인 예정)」 → 상태 `review`. FAIL → `rework`. 환경 부족으로 못 돈 기준이 있으면 그 기준은 「미실행(이유)」로 남기고 승인 보고서에 올린다 — 통과로 쓰지 않는다.

### review — 검토자 넷(읽기 전용, 병렬)
- 한 메시지에서 Agent 도구로 `code-reviewer` · `correctness-reviewer` · `security-reviewer` 를 동시에 부른다. `app/` 의 보이는 화면이 바뀌었으면 `kerf-ui-reviewer` 도(캡처는 먼저 `shot.ps1 -Exe <worktree-build>\app\SectionViewer.exe` 로 만들어 경로를 준다).
- 브리프: 작업 ID · worktree 경로 · `git -C <worktree> diff <base_sha>` 범위 · `spec.md` · `verify.md` · 「제품 코드를 고치지 말고 `review.md` 의 자기 절만 채우라. blocking/should/note 로 나누라」.
- 결과를 `review.md` 「종합」에 적는다: blocking 하나라도 → `rework`(builder 에게 blocking 목록을 그대로). 검토자끼리 결론이 다르면 → `needs-human`. 모두 pass → `ai_review: pass`, **승인 보고서**(§4 양식)를 `approval.md` 에 쓰고 사용자에게 보여 준다. 상태는 `review` 로 두고 **사용자 승인 대기**로 끝낸다.

### rework
- `rework_count` 를 1 올린다. 2를 넘으면 `needs-human`(요약: 시도한 가설 · 배운 것 · 선택지).
- builder 에게 실패 · blocking 목록을 그대로 넘긴다(검토자의 수정 제안을 그대로 받아쓰지 말고 근거를 확인하라고 적는다). 그 뒤 `verify` → `review` 를 다시 돈다.

### needs-human
- `job.md` 상태 기록에 「무엇이 막혔나 · 선택지」를 적고 사용자에게 AskUserQuestion(2–4 선택지, 추천 먼저). 답이 오기 전에는 아무것도 바꾸지 않는다.

## 4. 승인 (`/factory approve <ID>`)
전제: `ai_review: pass` 이고, `job.md` 의 `head_sha` 가 worktree 의 지금 HEAD 와 같고 미커밋 변경이 없다(`git -C <worktree> status --porcelain` 비어 있음). 다르면 먼저 `verify.ps1` 를 다시 돌리고 검토를 다시 받은 뒤 재승인을 요청한다.
1. 승인 보고서를 다시 보여 주고 사용자의 승인 말을 받는다(이 스킬 입력 자체가 승인 말이면 그 글).
2. `approval.md` 「사용자 승인」 표에 때 · 사용자 말 그대로 · 승인 SHA · 결과. `job.md` `user_approval: approved`, 상태 `approved`.
3. **여기서 멈춘다.** 병합 · push 는 하지 않는다. 사용자에게 「병합하려면 따로 말해 달라」고 한 줄.

**승인 보고서 양식**(`approval.md` 첫 절, 빠지는 항목 없이):
- 변경 요약과 변경 파일(`git diff --stat <base_sha>..<head_sha>`)
- 완료 기준별 검증 결과(AC 번호 · 통과/실패/미실행 · 근거 줄)
- 실패 · 미실행 항목과 이유
- 독립 검토 결과(검토자별 pass/fail · 받아들인 should/note)와 남은 위험
- 승인 대상 커밋 SHA(기준 → 변경)
- 되돌리기 방법(병합 전: 브랜치 미병합 · 병합 뒤: `git revert -m 1 <병합 커밋>`)

## 5. 병합 (사용자가 approve 뒤 따로 「병합」을 말했을 때만)
1. 전제 확인: 상태 `approved`, 다른 작업이 `merged` 진행 중이 아님(직렬), 본 저장소 `git status` 에 대상 브랜치 파일과 겹치는 미커밋 변경이 없음. 하나라도 어긋나면 멈추고 알린다.
2. worktree 에서 최신 대상 브랜치를 **merge**(rebase 금지 — 공유 가능 브랜치 역사 안 바꿈): `git -C <worktree> merge <base_branch>`. 충돌이면 `needs-human`.
3. `verify.ps1 -Id <ID> -Full` 다시 → PASS 가 아니면 멈춘다. `head_sha` 갱신, 승인 SHA 와 달라졌으니 사용자에게 **재승인**을 받는다(approval.md 「승인 뒤 변경」).
4. 재승인 뒤 본 저장소에서 `git merge --no-ff factory/<ID>`(대상 브랜치가 체크아웃돼 있어야 함 — 아니면 멈추고 알린다). 병합 커밋 SHA 를 `approval.md` 「병합」에, `job.md` 상태 `merged`.
5. push 는 하지 않는다(사용자가 말하면 DEV_PROGRESS §4 명령). worktree · 브랜치 삭제는 사용자 확인 뒤.

## 6. 검증 규칙 (모든 단계)
- 완료 기준마다 시험 이름 또는 확인 절차가 붙어야 한다. 없는 기준은 명세 미완.
- 기록은 **실제 실행한 명령 · 종료 코드 · 요약 · 로그 위치**. `verify.ps1` 가 그 형식으로 덧붙인다. 미실행 검증을 「통과」로 쓰지 않는다 — 「미실행(이유)」.
- 기존 실패(profile §9)와 이번 변경의 실패를 나눈다. 불안정 시험은 한 번 더 돈 결과를 같이 적는다.
- 관련 시험은 profile §6 지도에서 **실제 시험 이름을 `--list-tests` 로 확인한 뒤** 고른다. DXF · 좌표계 · 수직 기준 · 래스터 · 단면 · 도면 영역은 그 파일을 열어 기대값이 무엇을 재는지 읽고 고른다.
- 수치 허용오차는 profile §7 기존 기준. 없으면 이유 + 사용자 확인.
- 시험을 통과시키기 위해 기대값 · 단언 · 경고 설정을 바꾸지 않는다. 바꿔야 한다면 근거(계산 · 문서 · 사용자 확인)를 `build.md` 에.
- 수정 루프 최대 2회. 그 뒤는 `needs-human`.

## 7. 사람 검토 필수 영역 (자동으로 approved 로 가지 않음 — 보고서 맨 위에 표시)
좌표계 · 단위 · 수직 기준 · 수치 알고리즘(`core/src/engine.cpp` `section.cpp` `srs.cpp` `vdatum.cpp` `raster.cpp`) / 파일 형식 · 기존 데이터 호환(`dxf.cpp` `tiff.cpp` `pointcloud.cpp` `export.cpp` `sheet.cpp` `sheetexport.cpp`, QSettings 키) / 원본 파일 덮어쓰기 · 삭제 / 공통 빌드 설정 · 의존성 · 패키징 · CI(`CMakeLists.txt` `app/CMakeLists.txt` `.github/` `build-*.sh` `packaging/`) / 비밀정보 · 권한 설정(`.claude/settings*.json`, 훅).

## 8. 하지 않는 것
- push · main 직접 변경 · 자동 병합 · 배포 · 설치(`winget` `pip` 등) · 시스템 설정 변경.
- 본 저장소의 사용자 변경을 정리 · 삭제 · 리셋 · stash. `stash@{0}` 건드리지 않음. `git add -A` 금지(줄바꿈만 바뀐 파일).
- `third_party/` · `docs/design-v4/boards/` 수정. 디자인 v4 ⏸ UI 단계 착수.
- 사용자가 요청하지 않은 제품 기능 구현. 검토자가 제품 코드를 고치는 것.
- 실제 모델 · 실좌표 캡처를 저장소에 넣는 것(공개 저장소).

## 9. 기록 보존
`factory/jobs/<ID>/*.md` 는 Git(커밋은 사용자가 말할 때). 로그 · 빌드 결과 · 캡처는 `C:\dev\kerf-wt\<ID>-build\logs\` 에만 — 그래서 `verify.md` 요약 줄에 통과 수 · 기준값 줄 · RESULT · 종료 코드가 들어가야 한다. 디자인 v4 단계와 닿는 작업은 `docs/DEV_PROGRESS.md` 도 고친다.

## 10. 보고 (매 단계 끝, 한국어)
무엇을 했나 / 왜 / 어떻게 확인했나(명령 · 결과) / 직접 확인하는 방법 / 남은 위험과 다음 단계. 「끝」은 verify.md 블록과 review.md 종합이 있을 때만.
