# Kerf 개발 규칙 — 절대 준수 (2026-10-09 사용자 지시 「모든 개발규칙을 절대 준수하라, 규칙을 지정하라」)

> 이 문서가 **단 하나의 규칙 목록**이다. 어떤 AI · 어떤 세션이든 코드를 한 줄 바꾸기 전에 읽고, 과제마다 아래 체크리스트를 채운다. 다른 문서(CLAUDE.md · DEV_PROGRESS §6 · DESIGN_SPEC · factory/project-profile.md)와 어긋나면 **이 문서가 이긴다**. 규칙을 못 지킬 상황이면 멈추고 사용자에게 알린다 — 몰래 어기지 않는다.

## R1. 절차(superpowers 스킬 고정 · context7)
- R1.1 모든 작업은 superpowers 스킬을 **실제로 Skill 도구로 부른 뒤** 한다: 새 기능 `brainstorming → writing-plans → executing-plans`, 코드 수정 `test-driven-development`, 버그 · 시험 실패 · 이상 동작 `systematic-debugging`, 끝났다고 말하기 전 `verification-before-completion`, 기능 하나 끝나면 `requesting-code-review`(cpp-reviewer · UI는 ui-reviewer), 브랜치 마무리 `finishing-a-development-branch`.
- R1.2 Qt · C++ · 라이브러리 API가 확실하지 않으면 **context7**(`resolve-library-id` → `query-docs`, Qt는 `/websites/doc_qt_io_qt-6_8`)로 확인한 뒤 쓴다. 지어내지 않는다. context7이 없으면 사용자에게 연결을 부탁하고 공식 문서(doc.qt.io)로 확인한다.
- R1.3 목적에 맞는 다른 스킬 · MCP · 훅(`desca:verify` · `ui-visual-check` · `cpp-build-fix` · `cpp-debug` · `cpp-test`)은 사용자 요청 없이도 쓴다.
- R1.4 계획이 있는 작업은 계획 문서(`docs/superpowers/plans/*.md`)의 과제 순서대로, 장부(`.superpowers/sdd/<plan>/progress.md`)에 과제마다 `complete` 줄과 모든 `Ruling:`을 적는다. 계획과 다르게 한 것은 반드시 Ruling(무엇을 · 왜 · 잘못이면 무엇을 잃는지).

## R2. 코드
- R2.1 C++17 · Qt 6 Widgets. **`Q_OBJECT` 금지**(moc · rcc 없이 빌드). 신호 연결은 람다, `qobject_cast` 대신 `static_cast`. 새 클래스도 같다.
- R2.2 계산은 Qt 없이 `core/`에. 계산을 바꾸거나 더하면 **`tests/`에 먼저 실패하는 시험**(Catch2)을 넣고 RED를 본 뒤 구현한다(R3.1).
- R2.3 색 · 크기 · 글꼴은 `app/theme.hpp` 토큰만. 새 색은 Strata `tokens.json`에 있는 값만 토큰으로 더한다. 단면선 `#FF0000`, 흙색 `#B5573A`는 한 화면에 주 단추 하나. **UI 의 사각 테두리는 모두 둥글게**(판 · 틀 8, 입력 · 단추 · 칩 6, 작은 칸 3–4; 한 변 선과 종이 위 도면은 제외 — 사용자 지시 2026-10-09 「앱 안에 UI의 사각형 선들은 전부 라운드 처리하라」, `--ui-audit square-borders=0`).
- R2.4 높이 · 좌표 값은 파일 값 그대로(지오이드 재적용 없음, 이름표만). 평면도 그림 칸에 단면선 없음 · 진북만. 축척 10단위. 조판은 탭.
- R2.5 `third_party/` · 빌드 출력 · 생성 파일(`generated/kerf_icons.hpp`)은 손대지 않는다. CMake 소스 목록은 손으로 적는다(glob 금지).
- R2.6 바꾼 줄만 고친다. 파일 전체 재포맷 금지(`.clang-format` 없음). 주변 코드의 이름 · 주석 · 모양을 따른다.
- R2.7 화면은 `docs/design/DESIGN_SPEC.md`(v5, BINDING)대로. 치수는 논리 픽셀, 아이콘은 SVG(`app/icons/`, dpr 1 · 1.5 · 2 · 3). 스펙과 다르게 하면 Ruling + 스펙 수정.
- R2.8 보이는 단추는 모두 아이콘 + 툴팁(이름 · 키). 같은 일을 하는 단추는 한 화면에 한 번(평면 머리 ↔ 단면 머리만 예외).

## R3. 검증(실행해서 본 것만 말한다)
- R3.1 **시험 먼저**: 새 기능 · 버그는 실패하는 시험(core = Catch2, app = `app/main.cpp` 명령줄 옵션 `--ui-audit` `--icon-sheet` `--ctx-shot` `--sheet-check` …)을 먼저 돌려 **RED를 본 뒤** 구현하고 GREEN을 본다. 시험을 통과시키려고 기대값 · 단언 · 경고 설정을 바꾸지 않는다.
- R3.2 커밋 전 · 끝났다고 말하기 전 `desca:verify`(= `/verify`) 순서대로: `cmake --build C:/dev/kerf-v4c` exit 0 · **새 경고 0**(기존 `_wfopen` · `mirrored`만) → `asec_tests "~CoalescingWorker*"` 전부 통과 → `core/` · `tests/` 바꿨으면 전체 `asec_tests` → `app/` 바꿨으면 `--ui-audit` **RESULT ok** → 보이는 화면이 바뀌면 캡처를 Read로 눈으로 확인.
- R3.3 화면 변경은 1920×1040에서 ok 이고, 단계 끝(B5 뒤로는 과제마다)에 1280×800 · 3440×1440 · 3840×2160 × `QT_SCALE_FACTOR` 1 · 2 모두 `RESULT ok`.
- R3.4 같은 문제로 **두 번** 고치기에 실패하면 멈추고 가설 · 배운 것 · 선택지를 보고한다. 세 번째 시도 전에 아키텍처를 의심한다.
- R3.5 `CoalescingWorker: 끄는 동안의 미리보기는…` 한 건은 알려진 Windows 타이머 불안정 — 그 외 실패는 모두 진짜.

## R4. 기록
- R4.1 단계(계획의 A · B · C …)를 마칠 때마다 `docs/DEV_PROGRESS.md` §1 표 · §3 상태 · §4 기록(무엇을 · 확인 명령과 결과 · 판정 · 남은 문제 · 커밋 해시) · §5 다음 할 일을 고친다. **기록 없이 다음 단계로 가지 않는다.**
- R4.2 사용자 지시는 받은 날짜와 **말 그대로** 기록한다(DEV_PROGRESS · 스펙 · 메모리).
- R4.3 보고는 한국어로, 머리말 **무엇을 했나 / 왜 / 어떻게 확인했나(명령 · 결과) / 직접 확인하는 방법 / 남은 위험과 다음 단계**. 전문용어는 처음 나올 때 한 줄로 풀어 쓴다. 「끝」은 R3.2가 모두 통과했을 때만.

## R5. Git
- R5.1 커밋은 사용자가 허락한 범위에서만: 지금은 브랜치 `feature/design-v2`에 **단계(과제)마다** 커밋(DEV_PROGRESS §1 「커밋」 허락, 2026-10-09). **푸시 · 병합 · main 변경 · 배포는 사용자가 말할 때만.**
- R5.2 `git add -A` 금지 — 바꾼 파일만 이름으로 더한다. 줄바꿈만 바뀐 파일은 올리지 않는다(`git diff HEAD --ignore-cr-at-eol --stat`로 확인). 다른 세션(팩토리)의 미커밋 변경과 섞인 파일은 커밋에서 빼고 알린다.
- R5.3 force push · 히스토리 재작성 · `reset --hard` · `stash@{0}` 손대기 금지. 사용자 파일은 지우지 않고 옮긴다.
- R5.4 커밋 메시지는 한국어 「무엇을 · 왜 · 확인」 + `Co-Authored-By: Claude Fable 5.1 <noreply@anthropic.com>`. 시험 기대값을 바꿨으면 `시험 변경: <이유>` 줄.
- R5.5 저장소는 **공개**다. 실좌표가 보이는 캡처 · 실제 모델 · 다른 앱(Strata) 캡처 · 비밀값은 올리지 않는다(합성 모델만).

## R6. 검토
- R6.1 기능(계획의 단계) 하나가 끝나면 **cpp-reviewer**에게 diff 범위를 주어 독립 검토를 받고, 화면이 바뀌었으면 **ui-reviewer**(ui-visual-check 루브릭, 항목 4점 이상)도. 지적은 근거를 확인한 뒤 이번 변경 안에서 고치거나 Ruling으로 남긴다. 검토자는 제품 코드를 고치지 않는다.
- R6.2 검토 결과(통과 · 고친 것 · 남긴 것)는 DEV_PROGRESS 기록과 보고에 적는다.

## R7. 안전 · 환경
- R7.1 소프트웨어 설치 · 시스템 · 환경 설정 변경은 사용자 허락 뒤. 비밀값은 환경 변수 이름으로만 부른다.
- R7.2 앱 자동 실행은 항상 `--settings <격리 폴더>`. GUI 실행 파일은 `Start-Process … -PassThru` + `WaitForExit(시간)`로 돌려 멈추면 죽인다(세션을 막지 않는다).
- R7.3 빌드는 정션 `C:\dev\desca` → `C:\dev\kerf-v4c`(한글 경로에서 rc가 멈춤). 명령은 `.claude/cpp-project.json`이 정본.

## 과제 체크리스트(장부에 붙여 넣고 채운다)
```
[ ] R1.1 스킬 부름: (brainstorming|writing-plans|executing-plans|TDD|systematic-debugging|verification-before-completion|requesting-code-review)
[ ] R1.2 context7로 확인한 API: (없음이면 「없음」)
[ ] R3.1 시험 먼저: RED 명령·결과 → GREEN 명령·결과
[ ] R3.2 verify: build exit · 새 경고 · asec_tests · --ui-audit RESULT · 캡처 확인
[ ] R2.7 스펙 §와 다른 점: (없음 | Ruling)
[ ] R5.2 커밋 파일 이름 목록(섞인 파일 제외)
[ ] R4.1 DEV_PROGRESS 갱신(단계 끝) · R6.1 검토(단계 끝)
```
