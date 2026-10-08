# 개발 진행 기록 — Kerf 디자인 v4

> **이 파일이 개발 진행의 단 하나의 기준이다.** 어떤 AI(Claude Code · OpenCode/Muse Spark · 그 밖)든, 어떤 계정이든 작업을 시작하기 전에 이 파일을 끝까지 읽고, **단계 하나를 마칠 때마다 이 파일을 고친다.**
> 지시서: [`docs/design-v4/FINAL_PLAN.md`](design-v4/FINAL_PLAN.md) · 화판 원본: [`docs/design-v4/boards/`](design-v4/boards/) (단추 계약 = `Buttons1` · `Buttons2`, 아이콘 = `Icons`, 표시 = `Display`)
> 온라인 캔버스(소유자만 열림): https://claude.ai/artifact/VNPpTTVmqxoFG8eeZDWupE

## 0. 이어받는 법 (처음 온 AI는 이 순서로)
1. 이 파일 전체를 읽는다 → 「다음 할 일」을 확인한다.
2. `docs/design-v4/FINAL_PLAN.md`에서 그 단계 절만 읽는다(§4 절대 규칙은 매번 지킨다).
3. `git status` · `git diff HEAD --ignore-cr-at-eol --stat`로 아래 「바뀐 파일」과 실제가 같은지 본다. 다르면 먼저 사용자에게 알린다.
4. 빌드 · 실행으로 기준을 다시 확인하고(§2 명령), 단계를 진행한다.
5. 단계를 마치면 §3 표의 상태 · §4 기록 · §5 다음 할 일을 고친다. **기록 없이 다음 단계로 가지 않는다.**

## 1. 지금 상태 한눈에
| 항목 | 값 |
| --- | --- |
| 마지막 갱신 | 2026-10-09 · Claude Code (Opus) |
| 브랜치 | `feature/design-v2` (기준 커밋 `4422b4e` = `origin/kerf-portable-3sm`) |
| 커밋 | 사용자 허락(2026-10-09 「추천순으로 진행」) — **단계마다 커밋하고 `origin/feature/design-v2`에 푸시**한다. 해시는 `git log --oneline`으로 확인 |
| 바뀐 파일(내용) | `app/mainwindow_ui.cpp` · `app/sheetexport.cpp` · `app/theme.hpp` · 새 파일 `docs/DEV_PROGRESS.md` · `docs/design-v4/**` · `CLAUDE.md` |
| 빌드 | 성공 — `C:\dev\kerf-v4c` (§2) |
| 시험 | `asec_tests` 108 통과 · 1 실패(알려진 `CoalescingWorker`, test_schedule.cpp:39) · 1 건너뜀 |
| 최근 캡처 | `C:\dev\tmp\kerf-shots\work-v4b.png` (이 PC에만) |

## 2. 이 PC에서 빌드 · 실행 (검증된 명령)
- **한글 경로 함정:** 저장소가 `C:\Users\권을\…`에 있으면 CMake가 부르는 `rc.exe` · `llvm-rc`가 멈춘다(실측 2회). 그래서 **정션 `C:\dev\desca` → 저장소**를 만들어 그 경로로 빌드한다(이미 있음: `New-Item -ItemType Junction -Path C:\dev\desca -Target <저장소>`).
```powershell
$env:TEMP='C:\dev\tmp'; $env:TMP='C:\dev\tmp'; $env:Path = "C:\Program Files\LLVM\bin;$env:Path"
cmake -S C:\dev\desca -B C:\dev\kerf-v4c -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl "-DCMAKE_RC_FLAGS=/C 65001" -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/msvc2022_64
cmake --build C:\dev\kerf-v4c
C:\Qt\6.10.3\msvc2022_64\bin\windeployqt.exe --release --no-translations --no-system-d3d-compiler --no-system-dxc-compiler --no-compiler-runtime C:\dev\kerf-v4c\app\SectionViewer.exe
Copy-Item C:\dev\kerf-v4c\z.dll C:\dev\kerf-v4c\app\
C:\dev\kerf-v4c\asec_tests.exe
```
- 캡처: `SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\tmp\kerf-check --shot <out.png> --quit` (합성 모델은 `asec-make-synthetic.exe C:\dev\kerf-synth`)
- 다른 PC라면 FINAL_PLAN §3의 도구 설치 상태부터 확인한다.

## 3. 단계 표 (FINAL_PLAN §5 · §9)
상태: ⬜ 안 함 · 🟨 일부 · ✅ 끝(캡처 · 시험 근거 있음)

| 단계 | 내용 | 상태 | 메모 |
| --- | --- | --- | --- |
| −2 | 저장소 정리(줄바꿈만 바뀐 155개 대기 풀기) | ✅ | 2026-10-09 `git restore --staged .` → 대기 0, 내용 변화 없음 |
| −1 | 기준선 빌드 · 시험 | ✅ | §2. 108/110 |
| 0 | 잘린 선 빈 구간 원인 조사 | ⬜ | |
| 1 | `--ui-audit` | ⬜ | 원래 순서상 먼저였으나 눈에 보이는 리본부터 했다(아래 판정 기록) |
| 2 | 탭 넷 | ✅ | 파일 · 홈 · 보기 · 자료. 측정 탭 없앰 |
| 3 | 한 줄 리본 + 홈 3묶음 | 🟨 | 44 px · 아이콘 옆 글자 · 홈=단면/뒤 깊이/도면 · 목록 머리 「＋ 새 단면」 없앰. **남음:** 1280 px 좁은 창 글자 숨기기 |
| 3a | 아이콘 81개 | 🟨 | 리본은 18 px 기존 Ico. **남음:** Icons 화판 모양으로 theme.hpp `icon()` 다시 그리기, `--icon-sheet` |
| 4 | 단면 머리 켜기·끄기 · 축척 · 확대 | 🟨 | 토글 4 · 「세로 ×1」을 머리로, 평면 머리 「위에서」. **남음:** 축척 칸을 단면 머리로(지금 상태줄에 있음) |
| 5 | 정보 줄 · 상태줄 · 커서 | 🟨 | 정보 줄에서 길이 · 앞뒤 · 화면 축척 숨김. **남음:** 빈 구간 core 함수 + 「잘린 선 n줄 · 빈 구간」, 상태줄 축척 칸 빼기 |
| 6 | 오른쪽 판 읽기 전용 | 🟨 | 「이 단면으로 도면」 없앰 → 「좌표 입력 · 복사」. **남음:** 탭 없애기 확인, 연필 |
| 7 | 높이 배지 하나 | ✅ | 단면 머리 배지 숨김, 상태줄 배지만 |
| 8 | 그리는 동안 안내 한 곳 | ⬜ | 도구 줄 글 「단면선 그리기」(옛 이름)도 고칠 것 |
| 9 | 홈 화면 | ⬜ | |
| 10 | 목록 하나 | 🟨 | 조판 목록 머리 「단면 목록」으로. **남음:** 줄 모양 · 도면 상태 줄 · 고르기 칸 |
| 11 | 조판: 고르는 곳 하나 · 모드 칩 · 리본 접기 | 🟨 | 「작업으로」 단추 숨김, 문서 탭 줄 새 모양(조판 열릴 때만 보임). **남음:** 「도면 종류」 제거 · 모드 칩 · 리본 접기 |
| 12 | 도면 점검 | ⬜ | |
| 13 | 형식 · DXF 공간 | ⬜ | |
| 14 | 여러 장 저장 | ⬜ | |
| 15 | 빈 구간 표시 · E 키 | ⬜ | |
| 16 | 빈 상태 | ⬜ | |
| 16a | 새 키 · 문장 · 토큰 | ⬜ | |
| 17 | 문서 · 마무리 | ⬜ | |
| A1 | 단면 빗금(잘린 돌) | ⬜ | 확정: 사선 45° · 종이 위 1.0 mm |
| A2 | 단면 빗금 후보 | ⬜ | |
| A3 | 평면 윤곽 | ⬜ | 확정: SVG · DXF 층만 |

## 4. 단계별 기록 (새 기록은 맨 아래에 덧붙인다)
### 2026-10-09 · Claude Code — 단계 −1, 2, 3(일부), 4(일부), 5(일부), 6(일부), 7, 10(일부), 11(일부)
- **무엇을:** `buildRibbon()` 다시 씀 — 탭 넷, `pages_` 44 px, 묶음 이름을 앞에, 오른쪽 끝 주 단추. `tile()`은 아이콘 18 px + 글자 옆(옛 44 px 타일 그림 함수 `tileIcon` 삭제). 동작 이름: 「단면선 긋기」 · 「새 단면」 · 「잘린 선」. 단면 머리에 `viewToggles`(image · line · levels · fade, 같은 QAction) + `vexBtn_` 이동. 평면 머리에 `top` 단추. 정보 줄의 길이 · 앞뒤 · 화면 축척 글은 숨김(위젯은 갱신 코드 때문에 남김). 단면 머리 `heightBadge_` 숨김. 오른쪽 판 「이 단면으로 도면」 → 「좌표 입력… · 복사」. 목록 오른쪽 클릭 메뉴에 아이콘 · 「이 단면의 도면」 · 「방향 반전」. 문서 탭 줄 `docTabs` 스타일 + 처음엔 숨김. 조판 「작업으로」 단추 숨김. 조판 목록 머리 「단면 목록」.
- **확인:** `cmake --build C:\dev\kerf-v4c` → exit 0. `asec_tests` → 108 통과 · 1 실패(알려진 것) · 1 건너뜀. 캡처 `work-v4b.png`에서 탭 넷 · 한 줄 리본 · 머리 토글 · 판 아래 단추 확인.
- **판정(Ruling):**
  - 커밋하지 않음 — 사용자 허락 없음 · 인덱스에 줄바꿈 대기 155개 — 잘못이면: 단계별 되돌림 지점이 없음.
  - 빌드 경로를 정션 `C:\dev\desca` + `llvm-rc /C 65001`로 — 한글 경로에서 rc 멈춤 — 잘못이면: 빌드 명령만 바뀜.
  - `--ui-audit`(단계 1)보다 리본(2 · 3)을 먼저 — 사용자가 「실제 적용」을 바로 보기를 원함 — 잘못이면: 중복 잠금 시험이 늦어짐(다음에 바로 만든다).
  - 「평행 이동」은 지금처럼 누르면 메뉴(±0.1 · ±1 m) — 기존 동작 유지 — Buttons1 B4 「누르면 0.1 m」와 다름, 나중에 맞출 것.
- **남은 문제:** 그리기 도구 줄 글 「╱ 단면선 그리기」(옛 이름, `buildCtxBar`). 캡처에서 그리기 모드가 켜진 채 찍힘(원인 확인 전). 축척 칸이 아직 상태줄에 있음.

### 2026-10-09 · Claude Code — 단계 −2, 첫 커밋 · 푸시
- **무엇을:** 사용자 허락으로 줄바꿈만 바뀐 대기 155개를 풀고(git restore --staged .), 지금까지의 개발 · 진행 기록 · 지시서를 첫 커밋으로 묶어 origin/feature/design-v2에 푸시.
- **확인:** git diff --cached --name-only → 0개. 커밋 뒤 git status 깨끗함.

## 5. 다음 할 일 (위에서부터)
1. 단계 8 — 도구 줄 글을 「단면선 긋기 › A 찾는 중 / A′ 찾는 중」으로, 그리는 동안 상태줄 「다음:」 숨김. 캡처에서 그리기 모드가 켜진 이유 확인.
2. 단계 4 남은 것 — 상태줄 축척 칸을 단면 머리로.
3. 단계 1 — `--ui-audit` 만들기(FINAL_PLAN 기대값).
4. 단계 5 — 빈 구간 core 함수(시험 먼저) + 정보 줄 글.
5. 이후 FINAL_PLAN 순서대로.

## 6. 기록 규칙
- 단계를 마치면: §1 표(마지막 갱신 · 바뀐 파일 · 빌드 · 시험), §3 상태, §4에 새 기록(무엇을 · 확인 명령과 결과 · 판정 · 남은 문제), §5 다음 할 일.
- 「끝(✅)」은 빌드 성공 + 시험 결과 + 캡처나 기록 줄이 있을 때만.
- 지시서와 다르게 한 것은 반드시 「판정」으로 적는다(무엇을 · 왜 · 잘못이면 무엇을 잃는지).
- 커밋을 허락받으면 단계마다 커밋하고, 커밋 해시를 §4 기록에 적는다.
