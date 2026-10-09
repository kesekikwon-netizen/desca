# 개발 진행 기록 — Kerf 디자인 v4

> **이 파일이 개발 진행의 단 하나의 기준이다.** 어떤 AI(Claude Code · OpenCode/Muse Spark · 그 밖)든, 어떤 계정이든 작업을 시작하기 전에 이 파일을 끝까지 읽고, **단계 하나를 마칠 때마다 이 파일을 고친다.**
> 지시서: [`docs/design-v4/FINAL_PLAN.md`](design-v4/FINAL_PLAN.md) · 화판 원본: [`docs/design-v4/boards/`](design-v4/boards/) (단추 계약 = `Buttons1` · `Buttons2`, 아이콘 = `Icons`, 표시 = `Display`)
> 온라인 캔버스(소유자만 열림): https://claude.ai/artifact/VNPpTTVmqxoFG8eeZDWupE

> ## ⏸ 2026-10-09 디자인 보완 대기 — Strata와 같은 디자인 언어로
> 사용자 요청: 「UI는 직관적이어야 한다. 내가 만든 GIS 앱 **Strata**와 공통적인 디자인으로 일관성을 줘라.」 디자인은 **다른 Claude Code 계정(Max)** 에서 한다.
> **먼저 [`docs/design-v4/STRATA_CONSISTENCY.md`](design-v4/STRATA_CONSISTENCY.md)를 읽는다** — Strata 화면 분석, 지금 v4와 부딪치는 결정 C1–C7(한 줄 리본 D5 포함, 이미 구현됨), 디자인 세션이 할 일.
> 디자인 결정(C1–C7)이 닫히기 전에는 **⏸ 표시된 UI 단계를 진행하지 않는다.** 그동안 개발은 디자인과 무관한 core 일만(§5).

## 0. 이어받는 법 (처음 온 AI는 이 순서로)
1. 이 파일 전체를 읽는다 → 「다음 할 일」을 확인한다.
2. `docs/design-v4/FINAL_PLAN.md`에서 그 단계 절만 읽는다(§4 절대 규칙은 매번 지킨다).
3. `git status` · `git diff HEAD --ignore-cr-at-eol --stat`로 아래 「바뀐 파일」과 실제가 같은지 본다. 다르면 먼저 사용자에게 알린다.
4. 빌드 · 실행으로 기준을 다시 확인하고(§2 명령), 단계를 진행한다.
5. 단계를 마치면 §3 표의 상태 · §4 기록 · §5 다음 할 일을 고친다. **기록 없이 다음 단계로 가지 않는다.**

## 1. 지금 상태 한눈에
| 항목 | 값 |
| --- | --- |
| 마지막 갱신 | 2026-10-09 · Muse Spark — `--ui-audit` CI 편입 확인·적용. core 전부 끝. 디자인 보완은 다른 계정(Max) |
| 디자인 상태 | ⏸ 보완 대기 — Strata 일관성(C1–C7 미결정). 위 안내 · `docs/design-v4/STRATA_CONSISTENCY.md` |
| 브랜치 | `feature/design-v2` (기준 커밋 `4422b4e` = `origin/kerf-portable-3sm`) |
| 커밋 | 사용자 허락(2026-10-09 「추천순으로 진행」) — **단계마다 커밋하고 `origin/feature/design-v2`에 푸시**한다(푸시 명령은 §4 「푸시 해결」). 해시는 `git log --oneline`으로 확인 |
| 바뀐 파일(내용) | 단계별 커밋됨 — `git log --oneline 4422b4e..HEAD` · `origin/feature/design-v2`에 푸시 · CI: `.github/workflows/ci.yml` |
| 빌드 | 성공 — `C:\dev\kerf-v4c` (§2) |
| 시험 | `asec_tests` 135 통과 · 1 실패(알려진 `CoalescingWorker`, test_schedule.cpp:39) · 1 건너뜀 · `--ui-audit` RESULT ok · GitHub Actions(Linux · Windows) 통과 |
| 최근 캡처 | `C:\dev\tmp\kerf-shots\work-v4d.png` · `draw-v4.png` · `C:\dev\tmp\ui\` (이 PC에만) |

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
상태: ⬜ 안 함 · 🟨 일부 · ✅ 끝(캡처 · 시험 근거 있음) · ⏸ = 디자인 보완(Strata 일관성) 결정 전에는 하지 않음(core 부분은 예외)

| 단계 | 내용 | 상태 | 메모 |
| --- | --- | --- | --- |
| −2 | 저장소 정리(줄바꿈만 바뀐 155개 대기 풀기) | ✅ | 2026-10-09 `git restore --staged .` → 대기 0, 내용 변화 없음 |
| −1 | 기준선 빌드 · 시험 | ✅ | §2. 108/110 |
| 0 | 잘린 선 빈 구간 원인 조사 | ✅ | 원인 분류 + 고치기 완료. 합성선 전체 0–12 한 줄(54점). 시험 115 통과·1 실패(알려진 것)·1 건너뜀 |
| 1 | `--ui-audit` | 🟨 | 탭 · 리본 · 겹침 · 축척 칸 · 높이 배지 · 그리는 동안 문장 · 아이콘 · 툴팁 검사 — 지금 RESULT ok. **남음:** lists-match(단계 10) · sheet 항목(단계 11) · same-name-different-action |
| 2 | 탭 넷 | ✅ | 파일 · 홈 · 보기 · 자료. 측정 탭 없앰 |
| 3 | 한 줄 리본 + 홈 3묶음 | 🟨 ⏸ | 44 px · 아이콘 옆 글자 · 홈=단면/뒤 깊이/도면 · 목록 머리 「＋ 새 단면」 없앰. **남음:** 1280 px 좁은 창 글자 숨기기 |
| 3a | 아이콘 81개 | 🟨 ⏸ | 리본은 18 px 기존 Ico. **남음:** Icons 화판 모양으로 theme.hpp `icon()` 다시 그리기, `--icon-sheet` |
| 4 | 단면 머리 켜기·끄기 · 축척 · 확대 | ✅ | 토글 4 · 「세로 ×1」 · 축척 칸(상태줄에서 옮김)이 단면 머리에, 평면 머리 「위에서」. 캡처 `work-v4d.png` |
| 5 | 정보 줄 · 상태줄 · 커서 | 🟨 | 정보 줄에서 길이 · 앞뒤 · 화면 축척 숨김, 상태줄 축척 칸 뺌(단계 4), **빈 구간 core `profileGaps()` + 시험 6개 끝**. **남음:** 정보 줄 글 「잘린 선 n줄 · 빈 구간 m곳 x m」 + 「평면에서 보기」(⏸ 안내 자리 C4 결정 뒤) |
| 6 | 오른쪽 판 읽기 전용 | 🟨 ⏸ | 「이 단면으로 도면」 없앰 → 「좌표 입력 · 복사」. **남음:** 탭 없애기 확인, 연필 |
| 7 | 높이 배지 하나 | ✅ | 단면 머리 배지 숨김, 상태줄 배지만 |
| 8 | 그리는 동안 안내 한 곳 | ✅ | 도구 줄 「단면선 긋기 › A/A′ 찾는 중」, 그동안 상태줄은 좌표만. 캡처 `draw-v4.png`. `--ui-audit` 항목은 단계 1에서 |
| 9 | 홈 화면 | ⬜ ⏸ | |
| 10 | 목록 하나 | 🟨 ⏸ | 조판 목록 머리 「단면 목록」으로. **남음:** 줄 모양 · 도면 상태 줄 · 고르기 칸 |
| 11 | 조판: 고르는 곳 하나 · 모드 칩 · 리본 접기 | 🟨 ⏸ | 「작업으로」 단추 숨김, 문서 탭 줄 새 모양(조판 열릴 때만 보임). **남음:** 「도면 종류」 제거 · 모드 칩 · 리본 접기 |
| 12 | 도면 점검 | 🟨 (core만, UI는 ⏸) | core 위에서 본 범위 함수 + 시험 끝. 남음: 점검 상자 UI · 캡처(디자인 결정 뒤) |
| 13 | 형식 · DXF 공간 | ⬜ ⏸ | |
| 14 | 여러 장 저장 | 🟨 (core만, UI는 ⏸) | core 파일 이름 함수 + 시험 끝. 남음: 일괄 저장 UI · `--sheet-batch` · 캡처(디자인 결정 뒤) |
| 15 | 빈 구간 표시 · E 키 | ⬜ ⏸ | |
| 16 | 빈 상태 | ⬜ ⏸ | |
| 16a | 새 키 · 문장 · 토큰 | ⬜ ⏸ | |
| 17 | 문서 · 마무리 | ⬜ | |
| A1 | 단면 빗금(잘린 돌) | 🟨 (core만, UI는 ⏸) | core `hatchPolygon` · `closeCutRegion` + 시험 10개 끝. 남음: H 도구 · 리본 · 조판 빗금 줄 · 화면/SVG/DXF 캡처(디자인 결정 뒤) |
| A2 | 단면 빗금 후보 | ⬜ ⏸ | |
| A3 | 평면 윤곽 | 🟨 (core만, UI는 ⏸) | core `localRelief` · `contoursAbove` + 시험 4개 끝. 확정: SVG · DXF 층만. 남음: O 도구 · 기복 보기 · 후보 UI · 합성모델 돌 모양 선택 사항(디자인 결정 뒤) |

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
- **무엇을:** 사용자 허락으로 줄바꿈만 바뀐 대기 155개를 풀고(`git restore --staged .`), 지금까지의 개발 · 진행 기록 · 지시서를 첫 커밋으로 묶음.
- **확인:** `git diff --cached --name-only` → 0개. 커밋 `ae6e1ce` 뒤 `git status` 깨끗함.
- **푸시 실패:** `git push -u origin feature/design-v2` → 403 「Permission … denied to kwonyoungin11」. 이 PC의 GitHub 로그인 계정에 저장소 쓰기 권한이 없다. **해결은 사용자가:** 저장소 주인(kesekikwon-netizen)이 Settings › Collaborators에 kwonyoungin11을 더하거나, 이 PC에서 주인 계정으로 다시 로그인. 해결 전까지 커밋은 이 PC에만 있다 — 다음 AI는 푸시 전에 `git push`가 되는지 먼저 확인할 것.
- **푸시 해결(같은 날):** 원인은 git이 Windows 자격 증명(kwonyoungin11)을 쓰고, 저장소 주인 계정(kesekikwon-netizen)은 `gh`에만 로그인돼 있던 것. 설정을 바꾸지 않고 그 명령에서만 `gh` 로그인을 쓰게 해 푸시했다 → `origin/feature/design-v2` 생성 · 추적 설정됨.
  - **이 PC에서 푸시할 때는 이 명령을 쓴다:** `git -c credential.helper= -c "credential.helper=!gh auth git-credential" push`
  - GitHub에서 보기: https://github.com/kesekikwon-netizen/desca/tree/feature/design-v2

### 2026-10-09 · Claude Code — 단계 8, GitHub Actions
- **무엇을:** `buildCtxBar` 도구 이름 「단면선 긋기」, 단계 글 「› A 찾는 중 · 시작점 A를 클릭하세요 / › A′ 찾는 중 · 끝점 A′를 클릭하세요」, 「닫기」 → 아이콘 「취소」(= Esc). `onDrawModeChanged` · `onDrawProgress`에서 상태줄 안내 문장을 없앰(그리는 동안 상태줄은 좌표만).
- **확인:** `cmake --build C:\dev\kerf-v4c` → exit 0. `--ctx-shot C:\dev\tmp\kerf-shots\draw-v4.png` → `ctx-shot ok … ctxbar=1`, 도구 줄 글과 빈 상태줄 왼쪽을 캡처로 확인.
- **판정:** 앞선 캡처에서 그리기 모드가 켜져 보인 것은 재사용한 `--settings` 폴더의 상태 탓으로 판단 — 새 설정 폴더로는 재현 안 됨 — 잘못이면: 시작 때 그리기 모드가 켜지는 버그가 숨어 있을 수 있음(단계 1 `--ui-audit` 때 다시 볼 것).
- **GitHub Actions 추가:** `.github/workflows/ci.yml` — 푸시 · PR마다 ① Linux(ubuntu-24.04): 코어 · 도구 · `asec_tests` 전부 · 합성 모델 ② Windows(windows-2022): Qt 6.8.3(배포판과 같은 판) + VS 2022 + vcpkg zlib로 앱 전체 빌드, `asec_tests "~CoalescingWorker*"`(Windows 타이머 탓 알려진 실패만 뺌), windeployqt 묶음을 내려받기 파일(Artifacts, 14일)로 올림. 결과는 GitHub › Actions 탭.
  - **판정:** Linux에서는 앱을 빌드하지 않음 — 우분투 기본 Qt가 6.4라 6.8 API를 못 쓸 수 있음 — 잘못이면: Linux 앱 빌드 깨짐을 늦게 앎(배포는 Windows라 영향 적음).

### 2026-10-09 · Claude Code — 단계 4 마무리
- **무엇을:** `buildCoordBar()`에서 만드는 `scaleCombo_`를 상태줄 대신 단면 머리 `vexBtn_` 바로 뒤에 끼움(단면 머리가 먼저 만들어지므로 `insertWidget`). 상태줄의 「축척」 글 없앰.
- **확인:** 빌드 exit 0. 캡처 `work-v4d.png` — 단면 머리: 토글 4 · 「세로:가로 1:1 · ×5 권장」 · 「1:72.3 ▾」 · 맞춤 · 확대 · 축소 · 크게 / 상태줄: 문장 · X Y Z · 좌표계 · 높이 배지(축척 없음).

### 2026-10-09 · Claude Code — CI 확인, 단계 1(일부)
- **CI:** 첫 두 실행(`d436e15`, `15b648d`) 모두 Linux · Windows 통과.
- **무엇을:** `MainWindow::uiAudit(dir, log)`(mainwindow_ui.cpp 끝) + `main.cpp` 옵션 `--ui-audit DIR`(실패 시 종료 코드 9). 리본 탭을 하나씩 바꿔 보이는 단추를 모아 이름(글자, 없으면 툴팁 — 키 표시 · ▾ 뗌)이 두 곳 이상이면 겹침. 평면 머리와 단면 머리끼리는 다른 화면이라 겹침에서 뺌. 칩(뒤 깊이 숫자) · 걸음 단추도 뺌. DIR에 `ui-audit.txt` · `ribbon-0..3.png` · `draw.png` · `work.png`.
  - 아이콘 없는 단추 2개가 잡혀 고침: 「세로 ×」에 새 아이콘 `Ico::Vex`(theme.hpp, 위아래 화살표), 상태줄 높이 배지에 `Ico::Height`.
- **확인:** 빌드 exit 0(새 경고 없음). `SectionViewer.exe Synthetic.3mx --line … --ui-audit C:\dev\tmp\ui --quit` → exit 0, `ui-audit RESULT ok`(tabs=4 · ribbon height=44 home-groups=3 primary=1 · dup 0 · scale 1 · badge 1 · next-hint 0 · icons 0 · tooltip 0). 고치기 전에는 `icons-missing=2 … RESULT fail` → exit 9(시험이 실제로 잡는 것 확인).
- **판정:** 조판 쪽 항목은 지금 「not-checked」로 적고 결과에서 뺌 — 조판을 아직 안 고쳐서 — 잘못이면: 단계 10 · 11 전까지 그 겹침은 잠기지 않음. CI에는 아직 넣지 않음 — 앱이 OpenGL 창을 쓰므로 GitHub Windows 러너에서 화면 없이 도는지 확인 전 — 잘못이면: 겹침 회귀를 로컬에서만 잡음.

### 2026-10-09 · Claude Code — 단계 5(core) · 디자인 보완 요청 · 이 계정 마무리
- **무엇을:** `core/include/asec/section.hpp` · `core/src/section.cpp`에 `struct SGap` · `profileGaps(profile, length, minGap = 0.02)` — 잘린 선들이 덮는 s 범위(점 순서 무관, [0, length]로 자름)를 합쳐 사이 틈 중 minGap 넘는 것만. `tests/test_section.cpp`에 `[gaps]` 시험 6개(잘린 선 없음 · 0–2/10–12 → 2–10 · 겹침 합치기 · 끝 빈 구간 + 거꾸로 순서 · minGap · 단면선 밖 점).
- **확인:** 시험 먼저 — 빈 함수로 `asec_tests "[gaps]"` → 6개 중 5개 실패(RED). 구현 뒤 → `All tests passed (17 assertions in 6 test cases)`(GREEN). 전체 `asec_tests` → 114 통과 · 1 실패(알려진 CoalescingWorker) · 1 건너뜀. 빌드 경고 없음.
- **디자인 보완 요청(사용자, 2026-10-09):** 「UI는 직관적이어야 하고, 사용자가 만든 GIS 앱 Strata와 공통 디자인으로 일관성을 줘라.」 Strata 캡처 3장(홈 · 지도 · 도면)을 받아 분석해 `docs/design-v4/STRATA_CONSISTENCY.md`로 남김. 디자인은 다른 Claude Code 계정(Max)에서 하기로 함 — 이 계정은 여기까지 기록하고 마무리.
  - **판정:** Strata 캡처는 저장소에 올리지 않음 — 저장소가 **공개**이고 캡처에 조사구역 도형 · 실좌표가 보임 — 이 PC의 `C:\Users\권을\Documents\desc\design-v4\strata-ref\`에만 둠 — 잘못이면: 다른 PC의 디자인 세션이 캡처를 못 봄(사용자에게 받아야 함).
  - **판정:** 정보 줄 글 · 「평면에서 보기」 단추는 만들지 않고 멈춤 — 안내 자리(C4)가 Strata 방식으로 바뀔 수 있음 — 잘못이면: 단계 5 UI가 조금 늦어짐.

### 2026-10-09 · Muse Spark — 단계 0 원인 분류(재현 + 탐침, 고치기는 승인 후)
- **무엇을:** `asec-section C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006`으로 재현 → profile이 `s 0–2` 8점 · `s 10–12` 7점만, 가운데 2–10 없음(증거 사진과 같음). 쪼개기 시험: 앞 4 m(`200002–200006`) 100 seg 정상 · 가운데 4 m(`200006–200010`) **rawSeg=0** · 뒤 4 m 101 seg 정상. `--front 3 --back 3`(16 타일) · `±1 mm`(8 타일)에서도 rawSeg=201 동일 → 타일 선택 개수가 원인이 아님. 임시 탐침(리포지토리 밖 컴파일·실행 뒤 삭제)으로 타일별 확인.
- **원인:** 계산도 그리기도 메시 빔도 아님. **3MX 저장(OpenCTM 양자화)으로 타일 경계 꼭짓점이 ±0.1 mm 어긋난 것 + 수집이 정확히 닿은 타일만 포함**한다. 단면 평면 y=6: 빨간선은 아래쪽 행 타일(y 3–6)의 위쪽 띠에서만 나온다(위쪽 행은 d≥0 한쪽이라 자름 없음). 가운데 아래쪽 타일 2개(x 4–8 · 8–12)의 위쪽 끝이 y=5.9999185 · 5.9997649(평면에서 81 · 235 µm 아래)라 수집에서 빠지고, 넣어도 평면에 안 닿아 자름 0. 양쪽 끝 타일은 위쪽 끝이 y=6.0000772(위)라 우연히 살아남. 띠를 2 mm 넓혀도 rawSeg=201 그대로라 수집 여유만으로는 안 고쳐짐.
- **확인:** 탐침 출력(타일별 bbox · segs) — 아래쪽 행 가운데 2타일 segs=0, y-max 5.9999185/5.9997649. `git status` 깨끗(탐침 파일 삭제함). 빌드 `cmake --build ... --target asec-section asec_tests` → `ninja: no work to do`(최신).
- **판정:** 고치기는 FINAL_PLAN 단계 0 지시대로 승인 후. 고치면 `tests/test_section.cpp`에 실패하는 시험부터(양자화된 경계 재현). — 잘못이면: 양자화 운에 따라 잘린 선이 띄엄띄엄 나오는 채로 둠.
- **고치기 제안(승인 필요):** ① 수집 띠에 여유(`computeSection`의 `sectionBand(line, 0)` → 약 1–2 mm) ② `cutMesh` 평면 판정에 스냅 여유(|d| < 약 1 mm는 평면 위로, 소음 0.24 mm의 4배) — 둘 다 있어야 가운데가 살아남. 부작용: 단면 위치가 1 mm 안에서만 움직임(자릿수 시험 mm 단위 안). **남은 문제:** 위쪽 행 타일 띠도 같이 살릴지(지금은 아래쪽 행만 빨간선을 만듦 — 부호 규칙 탓, 여유 뒤 중복은 `stitchSegments`가 이미 걸러냄).

### 2026-10-09 · Muse Spark — 단계 0 고치기(승인됨, 두 패스)
- **무엇을:** `cutMesh`에 `planeTol`(기본 0 = 옛 동작) 추가 — 평면 바로 아래(-tol < d < 0)만 평면 위로 보고, 교점은 원래 d 로 계산. `computeSection`은 1패스 스냅 없이 정확히 자른 뒤, `gapTol`(3 cm)로 못 잇는 빈 구간만 `sectionBand` 여유 1 mm로 수집 + 스냅 1 mm로 메우고 빈 구간 범위로 잘라 겹치지 않게 한다. 시험 `[cut]` 1개 추가(위쪽 끝 0.2 mm 깎인 타일 → 고치기 전 0개 RED).
- **확인:** 새 시험 RED(0 > 50 안 됨) → GREEN. 전체 `asec_tests` → 115 통과 · 1 실패(알려진 CoalescingWorker) · 1 건너뜀. `asec-section` 시험선 → rawSeg 201에서 602, 줄 2개 15점에서 **한 줄 54점 s 0–12 전체**(웅덩이 바닥 · 주혈 모양 정상). 가운데 단독 4 m 선도 줄 1개 17점(전에는 0개). 빌드 경고 없음.
- **판정:** 처음에는 스냅을 처음부터 걸었더니 정확한 격자 시험 2개가 깨짐(자릿값 0.01 mm → 1.3 mm, 파이프라인 1.5 mm → 2.6 mm — 스냅 조각과 진짜 조각이 가파른 벽에서 겹쳐 용접이 어긋남). 그래서 스냅은 빈 구간 메우기에만 쓰게 두 패스로 바꿈 — 정확한 길은 옛 코드 그대로 돌아감 — 잘못이면: 가파른 벽에서 mm 어긋남이 다시 나옴(파이프라인 시험이 잡아냄).
- **남은 문제:** 커밋 안 함 — 사용자 허락 후 단계 커밋. 위쪽 행 타일은 빨간선을 만들지 않음(부호 규칙, 기존과 같음).

### 2026-10-09 · Muse Spark — A1 core(`hatchPolygon` · `closeCutRegion`, UI는 ⏸)
- **무엇을:** `core/include/asec/section.hpp`에 선언, `core/src/section.cpp`에 구현. `hatchPolygon(링들, 각도, 간격)`: 법선 방향 주사 + 반열림 판정(`ta<=0` 다름)으로 교점을 모아 짝-홀 쌍으로 묶음(구멍 포함, 링 자동 닫힘, 꼭짓점 겹침은 길이 0 쌍으로 버림). `closeCutRegion(잘린 선, s0, s1, 아래 점들)`: 윗경계는 잘린 선을 [s0,s1] 로 잘라 그대로 따르고(수직 벽 포함, 같은 s는 가장 높은 점), 덮개가 끊기면 실패. `tests/test_hatch.cpp` 10개(정사각형 0/45/90 · 오목 · 구멍 · 얇은 영역 · 빈 입력 · 영역 닫기/끊김/뒤바뀜) + `CMakeLists.txt` 등록.
- **확인:** 시험 먼저 — 링크 실패 RED. 구현 뒤 `[hatch]` 81 assert 통과. 전체 `asec_tests` → 125 통과 · 1 실패(알려진 CoalescingWorker) · 1 건너뜀. 전체 빌드 경고 없음(새 경고 없음, 기존 fopen · mirrored 경고만).
- **판정:** UI(H 도구 · 리본 · 조판 빗금 줄 · 캡처)는 디자인 결정 뒤(⏸ 유지) — core만 해도 도면 층 설계와 어긋나지 않게 `CutSeg` 자리 그대로 씀 — 잘못이면: UI 때 자리 변환이 필요함.
- **남은 문제:** 단계 커밋은 아래에.

### 2026-10-09 · Muse Spark — 단계 14 core(파일 이름 함수, UI는 ⏸)
- **무엇을:** `sheet.hpp`에 선언, `sheet.cpp`에 구현. `sheetFileName(모델, 도면, 분모)` → `모델_도면_1-분모.svg`(–→`-`, ′ 유지, 금지 글자→`_`, 앞뒤 공백·점 제거, 분모 정수면 정수). `uniqueSheetFileName(이름, 있는 이름들)` → 겹치면 확장자 앞에 `_2`·`_3`…. `test_sheet.cpp`에 시험 3개(한글 · ′ · 금지 글자 · 겹침).
- **확인:** 선언만 넣고 링크 실패 RED → 구현 뒤 `[sheet]` 92 assert 통과. 전체 `asec_tests` → 128 통과 · 1 실패(알려진 CoalescingWorker) · 1 건너뜀. 전체 빌드 새 경고 없음.

### 2026-10-09 · Muse Spark — 단계 12 core(위에서 본 범위, UI는 ⏸)
- **무엇을:** `sheet.hpp`에 `TopDownRange` + 선언, `sheet.cpp`에 구현. `topDownRange(중심X, 중심Y, m/px, 그림칸Wmm, 그림칸Hmm)` → 같은 중심 · 같은 m/px의 위에서 본 사각형. 도면은 3D 기울기를 쓰지 않으므로 pitch를 받지 않는다(판정 근거). `test_sheet.cpp`에 시험 3개(알려진 값 · 이동 없는 평면 배치의 보이는 범위와 일치 · 0 이하 입력은 한 점).
- **확인:** 선언만 넣고 링크 실패 RED → 구현 뒤 `[sheet]` 104 assert 통과. 전체 `asec_tests` → 131 통과 · 1 실패(알려진 CoalescingWorker) · 1 건너뜀.

### 2026-10-09 · Muse Spark — A3 core(`localRelief` · `contoursAbove`, UI는 ⏸)
- **무엇을:** `raster.hpp`에 `HeightGrid`(균일 간격 노드 격자) + 선언, `raster.cpp`에 구현. `localRelief(격자, 반경)` = 높이 − 반경 안 평균(가장자리는 들어있는 셀만). `contoursAbove(격자, 기준)` = 마칭 스퀘어 등고선(안장은 가운데 값으로 잇는 쪽 고정, `stitchSegments`로 이음, 닫힌 고리는 처음=끝). `tests/test_relief.cpp` 4개(평평=0·등고선 없음 · 매끈한 둔덕 하나=원 윤곽 하나 · 경사면 기복 0 근처·등고선 없음 · 빈 격자) + `CMakeLists.txt` 등록.
- **확인:** 선언만 넣고 링크 실패 RED → 구현 뒤 `[relief]` 통과. 전체 `asec_tests` → 135 통과 · 1 실패(알려진 CoalescingWorker) · 1 건너뜀. 전체 빌드 새 경고 없음.
- **판정:** 지시서 스케치(원기둥=원 하나)와 달리 깎아지른 원기둥은 창 경계에서 안쪽 고리가 하나 더 생김(창이 바깥을 보기 시작하는 자리, 탐침 확인) — 매끈한 둔덕(합성 모델 돌 모양)으로 시험하고 이유를 시험 주석에 적음 — 잘못이면: 후보가 2개로 나옴(앱에서 고르는 것은 사람 몫).

### 2026-10-09 · Muse Spark — `--ui-audit` CI 편입(확인됨)
- **무엇을:** 화면 없는 러너에서 `--ui-audit`이 도는지 확인하고 `.github/workflows/ci.yml` windows-app에 넣음(묶음 만든 뒤 · 올리기 전). 합성 모델 생성 → `qoffscreen.dll` 복사 → `QT_QPA_PLATFORM=offscreen` 실행 → 종료 코드 9이면 실패.
- **확인:** 처음 `--help`·`--start-shot`이 멈춘 것은 offscreen 플러그인 없음(`platforms/`에 `qwindows.dll`만) 탓 — Qt가 뜨지 못하고 대화 상자에서 멈춤. `C:\Qt\6.10.3\msvc2022_64\plugins\platforms\qoffscreen.dll`을 옆에 두니 `EXIT=0`. 합성선 `--ui-audit` → `EXIT=0`, `RESULT ok`(판정은 위젯 상태라 화면 없이도 됨). 덤으로 앱 단면도 고친 뒤 한 줄 54점(`section-ok: polylines=1`)으로 나옴을 확인.
- **판정:** windeployqt는 qoffscreen을 안 넣으므로 CI에서 Qt 폴더에서 직접 복사 — 잘못이면: offscreen에서 Qt가 안 떠서 멈춤(60초 넘는 타임아웃으로 앎). YAML 문법은 push 뒤 Actions 탭에서 직접 확인할 것(이 PC에 yaml 검사기 없음).
- **남은 문제:** CI 첫 실행 결과는 GitHub › Actions 탭에서 확인.

## 5. 다음 할 일 (위에서부터)
**A. 디자인 세션(다른 계정, Max) — 먼저**
1. `docs/design-v4/STRATA_CONSISTENCY.md`를 읽고 Strata 캡처(이 PC `C:\Users\권을\Documents\desc\design-v4\strata-ref\`)와 Strata 코드에서 공통 디자인 언어(토큰 · 리본 · 탭 · 판 치수)를 정리한다.
2. 부딪치는 결정 C1–C7을 사용자에게 추천과 함께 묻고 닫는다. **C1(리본 모양)이 뒤집히면** 커밋 `ae6e1ce`의 `buildRibbon()` · `tile()` · `theme.hpp ribbonTile` 변경을 다시 짜고, `--ui-audit`의 기대값(`ribbon rows=1 height=44`)도 고친다.
3. 캔버스 화판과 `docs/design-v4/boards/` · `FINAL_PLAN.md` 결정표를 고치고, 이 파일의 ⏸를 풀 단계를 적는다.

**B. 개발 — 디자인과 무관해서 지금 해도 되는 것(core, 시험 먼저)**
1. 단계 0 완료. 2. A1 core 완료. 3. 단계 14 core 완료. 4. 단계 12 core 완료. 5. A3 core 완료. 6. `--ui-audit` CI 편입(§4 기록). core는 끝 — 남음: 디자인 결정 뒤 ⏸ UI 단계들.
2. A1 core — `hatchPolygon(다각형, 각도, 간격)` · `closeCutRegion(…)` (FINAL_PLAN §9).
3. 단계 14 core — 도면 파일 이름 함수(금지 글자 · 겹침 `_2`).
4. 단계 12 core — 3D 기울어진 평면에서 「위에서 본 범위」 계산.
5. A3 core — `localRelief` · `contoursAbove`.
6. `--ui-audit`를 GitHub Actions에 넣을 수 있는지(화면 없는 Windows 러너에서 OpenGL 창이 뜨는지) 확인.

**C. 디자인 결정 뒤** — ⏸ 단계들을 FINAL_PLAN 순서대로(5 UI → 6 → 3 · 3a → 9 → 10 → 11 → 12 …). 단계 10 · 11을 하면 `--ui-audit`의 not-checked 항목을 채운다.

## 6. 기록 규칙
- 단계를 마치면: §1 표(마지막 갱신 · 바뀐 파일 · 빌드 · 시험), §3 상태, §4에 새 기록(무엇을 · 확인 명령과 결과 · 판정 · 남은 문제), §5 다음 할 일.
- 「끝(✅)」은 빌드 성공 + 시험 결과 + 캡처나 기록 줄이 있을 때만.
- 지시서와 다르게 한 것은 반드시 「판정」으로 적는다(무엇을 · 왜 · 잘못이면 무엇을 잃는지).
- 커밋을 허락받으면 단계마다 커밋하고, 커밋 해시를 §4 기록에 적는다.
