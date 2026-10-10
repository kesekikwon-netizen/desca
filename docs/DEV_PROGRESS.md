# 개발 진행 기록 — Kerf 디자인 v5(구현 중) · v4 기록

> **이 파일이 개발 진행의 단 하나의 기준이다.** 어떤 AI(Claude Code · OpenCode/Muse Spark · 그 밖)든, 어떤 계정이든 작업을 시작하기 전에 이 파일을 끝까지 읽고, **단계 하나를 마칠 때마다 이 파일을 고친다.**
> 지시서: [`docs/design-v4/FINAL_PLAN.md`](design-v4/FINAL_PLAN.md) · 화판 원본: [`docs/design-v4/boards/`](design-v4/boards/) (단추 계약 = `Buttons1` · `Buttons2`, 아이콘 = `Icons`, 표시 = `Display`)
> 온라인 캔버스(소유자만 열림): https://claude.ai/artifact/VNPpTTVmqxoFG8eeZDWupE

> ## ▶ 2026-10-09 디자인 v5 확정 · 구현 시작
> 사용자가 결정 C1–C7(모두 추천대로)을 닫고 /goal로 「디자인 v5를 보면서 똑같이 개발 완성하라」고 지시했다. **지금 기준은 [`docs/design/DESIGN_SPEC.md`](design/DESIGN_SPEC.md)(화판 `docs/design/boards/kerf-design-v5.html`)이고, 구현 순서는 [`docs/superpowers/plans/2026-10-09-design-v5.md`](superpowers/plans/2026-10-09-design-v5.md)(A–I 단계)이다.** 아래 ⏸ 표시는 모두 풀렸다. 모든 작업에 superpowers 스킬(계획 → 시험 먼저 → 검토 → 끝나기 전 확인)과 context7을 쓴다.
>
> (옛 안내) ## ⏸ 2026-10-09 디자인 보완 대기 — Strata와 같은 디자인 언어로
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
| 마지막 갱신 | 2026-10-11 · Claude Code(Sonnet 5.5) — **T9 F1 창 고침(어두운 모드 바탕 · 표 = 실제 키 · F2 · Del 연결) 끝** → 다음 T10(G7) 입면 위 거리 눈금 + D 키 + 도면 SVG. 계획 `docs/superpowers/plans/2026-10-11-trench-final.md`. 검토는 전부 끝난 뒤 한 번(사용자 지시) |
| 디자인 상태 | ▶ v5 확정(2026-10-09, C1–C7 추천대로) · 구현 중 — 스펙 `docs/design/DESIGN_SPEC.md` · 계획 `docs/superpowers/plans/2026-10-09-design-v5.md` |
| 브랜치 | `feature/design-v2` (기준 커밋 `4422b4e` = `origin/kerf-portable-3sm`) |
| 커밋 | 사용자 허락(2026-10-09 「추천순으로 진행」) — **단계마다 커밋하고 `origin/feature/design-v2`에 푸시**한다(푸시 명령은 §4 「푸시 해결」). 해시는 `git log --oneline`으로 확인 |
| 바뀐 파일(내용) | 모두 커밋됨(단계마다 커밋 — 사용자 지시 2026-10-09 「단계마다 깃커밋해줘」). 로컬 `feature/design-v2` 는 `origin`(`0f7fbfe`)보다 앞섬 — **push 는 사용자 말 때**(또는 「자동 깃 커밋 푸시」 세션). `git log --oneline 0f7fbfe..HEAD` |
| 빌드 | 성공 — `C:\dev\kerf-v4c` (§2), 병합 `3c9b0d4` 뒤 다시 빌드 exit 0 |
| 시험 | 병합 뒤 `asec_tests "~CoalescingWorker*"` 51325 통과. 팩토리 통합 검증(`9a77d61`): 전체 51339 통과 · `--ui-audit` 1280×800 · 1920×1040 · 3440×1440 · 3840×2160 × 1 · 2 + 1280 · 2560 × 1 · 1.25 · 1.75 모두 RESULT ok · GitHub Actions: Windows 작업은 MSVC C2026 으로 실패 중(R0) |
| 최근 캡처 | `C:\dev\tmp\kerf-shots\b5-merged-work.png`(병합 빌드 1920×1040 실제 화면) · `C:\dev\tmp\b5-shots\`(work-1280 · 1920 · 1920-x1.25 · 1920-x2 · 3440 · 3840 · draw-1280 · start-1280) · offscreen 감사 `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\` (이 PC에만) |

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
| 1 | `--ui-audit` | 🟨 | v5 기대값(스펙 §12) + 검토 1 · 2차 항목 + sheet 항목 + square-borders. **남음:** same-name-different-action · lists-match 의 줄 모양(D3) |
| 2 | 탭 넷 | ✅ | 파일 · 홈 · 보기 · 자료. 측정 탭 없앰 |
| 3 | 리본(v5 C1 타일 리본) | ✅ | 묶음 7 · 타일 50 · 창 폭 따라 줄임 · 메뉴 칩 ▾ · 입면 칸 타일 가운데 · 찾기 142(검토 2차). 좁은 창 캡처 · 배율은 B5 |
| 3a | 아이콘 SVG(C7) | ✅ | Lucide 24 + Kerf 38, `--icon-sheet`, 같은 아이콘 다른 일 없음(`dup-icons=0`), 위에서 · LAS 그림 바꿈 |
| 4 | 단면 머리 켜기·끄기 · 축척 · 확대 | ✅ | 토글 4 · 「세로 ×1」 · 축척 칸(상태줄에서 옮김)이 단면 머리에, 평면 머리 「위에서」. 캡처 `work-v4d.png` |
| 5 | 정보 줄 · 상태줄 · 커서 | ✅ | 정보 줄 「잘린 선 n줄 · m점 · 빈 구간 없음/k곳 x m」(core `profileGaps`, `0f30c5a`), 상태줄 v5(C6) 한 문장 + 배지 둘. 「평면에서 보기」는 v5 에서 빠짐(안내 칩 C4) |
| 6 | 오른쪽 판 읽기 전용 | 🟨 | 「좌표 입력 · 복사」 + 그리는 중 상태(`0f30c5a`). **남음:** 스펙 §4 모양(2열 리더 · 「1 / 3」 · 쉼표 · 끝점 절) = 계획 D2 |
| 7 | 높이 배지 하나 | ✅ | 단면 머리 배지 숨김, 상태줄 배지만 |
| 8 | 그리는 동안 안내 한 곳 | ✅ | 도구 줄 「단면선 긋기 › A/A′ 찾는 중」, 그동안 상태줄은 좌표만. 캡처 `draw-v4.png`. `--ui-audit` 항목은 단계 1에서 |
| 9 | 홈 화면 | ✅ | v5 D1(`85802ec`) + 검토 반영(`0f30c5a`): 「높이」 칸 · 폴더 「C: › dev」 · 표 높이 |
| 10 | 목록 하나 | 🟨 ⏸ | 조판 목록 머리 「단면 목록」으로. **남음:** 줄 모양 · 도면 상태 줄 · 고르기 칸 |
| 11 | 조판: 고르는 곳 하나 · 모드 칩 · 리본 접기 | ✅ | v5 E3(`27ca154`) + E4: 고른 칸 전부 한 폴더에 `sheetFileName`(겹치면 `_2`) · `--sheet-batch` · 「n장 저장됨 · 폴더 열기」 |
| 12 | 도면 점검 | 🟨 (core만, UI는 ⏸) | core 위에서 본 범위 함수 + 시험 끝. 남음: 점검 상자 UI · 캡처(디자인 결정 뒤) |
| 13 | 형식 · DXF 공간 | ✅/— | 형식 칩은 E3(`27ca154`). DXF 모델/종이 공간 선택은 **v5 스펙에 없음 → 만들지 않음**(2026-10-10 #10 조사, 사용자 사전 허가 · 추천안). 스펙 §8 「빈 층 2 · 밑그림 잠금」 칸은 #5 |
| 14 | 여러 장 저장 | ✅ | core 이름 함수 + E4 UI(`saveCheckedSheets` · `--sheet-batch` 4장 ok) |
| 15 | 빈 구간 표시 · E 키 | ✅/— | 정보 줄 「빈 구간 k곳 x m」(`0f30c5a`) + 도면 점검 항목(E3). 띠 · 점선 · E 키는 **v5 스펙에 없음 → 만들지 않음**(#10 조사). 목록 줄 상태는 #3b |
| 16 | 빈 상태 | ✅/— | 홈 · 단면 화면 · 오른쪽 판 · 상태줄 · 배지 none 모두 있음. 도면 탭 단면 0개 카드는 **스펙에 없음 → 만들지 않음**(#10 조사) |
| 16a | 새 키 · 문장 · 토큰 | ✅/— | Ctrl+F · 상태줄 문장 · 기존 토큰 구현됨. 새 키 7개 · Ctrl+D/E 재해석 · 주의 토큰은 **스펙에 없음 → 만들지 않음**(#10 조사) |
| 17 | 문서 · 마무리 | ⬜ | |
| A1 | 단면 빗금(잘린 돌) | 🟨 (core만, UI는 ⏸) | core `hatchPolygon` · `closeCutRegion` + 시험 10개 끝. 남음: H 도구 · 리본 · 조판 빗금 줄 · 화면/SVG/DXF 캡처(디자인 결정 뒤) |
| A2 | 단면 빗금 후보 | — | **v5 스펙 §9 H 는 수동(두 점 → 아래 점 → Enter)뿐 → 후보 제안은 만들지 않음**(#10 조사, 2026-10-10) |
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

### 2026-10-09 · Claude Code — 개발 환경 계약(`.claude/`) · CLAUDE.md 명령을 Windows 기준으로
- **무엇을:** `/init`(전역 C++ 지시서 §6.7 「프로젝트 계약」 기준)으로 `.claude/cpp-project.json`(빌드 · 시험 · 빠른 시험 · 실행 · 캡처 명령의 단일 계약, `formatOnEdit:false` · `verifyOnStop:true`), `.claude/skills/verify/SKILL.md`(끝났다고 말하기 전 · 커밋 전 검증 절차), `.claude/scripts/shot.ps1`(장면 `work` · `draw` · `start` 캡처, `ui-visual-check`용)을 만듦. `CLAUDE.md` 「명령」 절을 WSL 기준에서 Windows(데스크톱 앱 · 터미널 `claude`) 기준으로 고침 — `cmake --build C:/dev/kerf-v4c` · `C:/dev/kerf-v4c/asec_tests.exe …`처럼 PowerShell · Git Bash 공통 명령. 터미널용 Claude Code 2.1.295를 이 PC에 설치(`~\.local\bin\claude.exe`).
- **확인:** `cmake --build C:/dev/kerf-v4c`(TEMP · PATH 그대로인 cmd.exe) → exit 0. `asec_tests "~CoalescingWorker*"` → 132 통과 · 1 건너뜀(Bash · PowerShell 모두, 2초). `asec-section` → polylines=1 vertices=54. `--ui-audit` → EXIT=0 `RESULT ok`. `shot.ps1` work · draw · start → PNG 3장(실제 화면 확인). 전역 `ui-visual-check`의 `visual.py capture --scenes work,start --themes light --scales 1` → OK 2장. 전역 세션 시작 훅이 계약 요약을 출력함.
- **판정:** `.clang-format` · `.clang-tidy`는 넣지 않음 — 저장소 규칙(바꾼 줄만 고침)과 전체 재포맷 위험 — 잘못이면: 자동 정렬 없이 손으로 모양을 맞춰야 함. 전역 디자인 토큰 감시 훅은 `ui/` · `gui/` 같은 폴더 이름만 보므로 `app/`에는 걸리지 않음(계약의 `ui.framework`는 기록용). 커밋하지 않음 — 사용자 지시 대기.

### 2026-10-09 · Claude Code — 소프트웨어 팩토리(`/factory`) 개발 설정 구축(설정만, 제품 변경 없음)
- **무엇을(사용자 지시 「개발설정만 담당하라」 · 「깃 액션까지 확인해줘」):** `.claude/skills/factory/SKILL.md`(입력 `/factory <요청>|status|next|rework <ID>|approve <ID>` 해석, 상태 흐름 queued→spec→build→verify→review→rework|needs-human→approved→merged, 수정 루프 2회, 사람 검토 필수 영역, 승인 보고서 양식, 병합 뒤 GitHub Actions 확인) · 에이전트 7개 `.claude/agents/`(spec-writer · builder(유일한 쓰기) · test-runner · code-reviewer · correctness-reviewer · security-reviewer · kerf-ui-reviewer — 도구 권한을 역할별로 제한) · `factory/project-profile.md`(확인된 사실 · 명령 · 시험 지도 · 허용오차 표 · 위험 모듈 · 기존 실패) · `factory/backlog.md` · `factory/jobs/`(README + `_template/` job · spec · build · verify · review · approval) · `factory/scripts/`(`new-job.ps1` worktree `C:\dev\kerf-wt\<ID>` + 브랜치 `factory/<ID>` + 기록 폴더, `verify.ps1` configure→build→tests→asec-section 기준값→ui-audit→ci(base) 를 `verify.md`에 기록, `status.ps1`, `ci-status.ps1` gh로 Actions 결과) · `CLAUDE.md` 끝에 10줄. 기준 브랜치 `feature/design-v2`.
- **확인:** `status.ps1` → exit 0. `new-job.ps1 -Slug smoke` → worktree `C:\dev\kerf-wt\20261009-01-smoke`(`4781e1d`) 생성, job.md 한글 정상(UTF-8). `verify.ps1 -Full -UiAudit on` → configure 6 s · 전체 빌드 14 s(새 경고 0, 기존 CRT deprecated만) · 시험 51314 assertions 통과 · `asec-section` polylines=1 vertices=54 · ui-audit RESULT ok(62 s) → **RESULT PASS exit 0**. `ci-status.ps1 -Sha de14dc2` → `completed success`(Windows 앱 · Linux 코어 둘 다), `-Sha 4781e1d` → `none`(push 안 됨, exit 3). 연기 시험 worktree · 브랜치 · 기록은 확인 뒤 삭제(아래).
- **판정:**
  - 시연 작업(README 보강 등)은 하지 않음 — 사용자 지시 「개발설정만」 — 잘못이면: spec-writer~reviewer 에이전트 브리프가 실전에서 한 번도 안 돌아 첫 작업 때 다듬어야 할 수 있음.
  - `CLAUDE.md`를 다시 쓰지 않고 끝에 절만 덧붙임 — 다른 세션(디자인 v5)이 같은 파일을 쓰고 있어 — 잘못이면: 「간결한 CLAUDE.md」 요구와 다름(원하면 따로 정리).
  - UI 검토자 이름을 `kerf-ui-reviewer`로 — 전역 `ui-reviewer`를 가리지 않으려고 — 잘못이면: 전역 `ui-visual-check` 스킬은 여전히 전역 검토자를 부름.
  - `.ps1`에 UTF-8 **BOM** 필수(PS 5.1은 BOM 없으면 CP949로 읽어 한글 리터럴이 깨짐, 실측). `gh run list --commit`은 전체 SHA만 받고, PS 5.1은 `"[]"`를 `$null`로 풀며 jq 식의 큰따옴표를 벗김 — 셋 다 스크립트에서 처리.
  - 전역 Stop 훅은 본 저장소(`kerf-v4c`)만 빌드하므로 worktree 검증은 `verify.ps1`만 믿는다.
- **추가(같은 날, 사용자 지시 셋 — 「적는 게 아니라 실제 개발에 쓸 수 있게」 · 「4K · 와이드 어디서나 안정」 · 「superpowers 스킬과 context7을 안정적으로」):**
  - 사용자가 `desc` 폴더에서 상대 경로 명령을 돌려 실패(`-File 'factory/scripts/status.ps1' 없음`) → **실행기 `factory.cmd`**(저장소 루트 + `desc\factory.cmd` 전달기, `factory\factory.ps1`이 명령 분배) — `factory status | new … | verify <ID> | ci <sha>`. `desc` · `desca` · `C:\`에서 PowerShell · cmd 모두 exit 0 확인. 모든 문서 · 에이전트 명령을 실행기로 통일.
  - `ci-status.ps1`(gh로 Actions 결과) + `verify.ps1`에 `ci(base)` 줄. `de14dc2` → success(Windows · Linux), 로컬 HEAD는 none(push 전). SKILL §5-6: push 뒤 병합 커밋 CI 확인.
  - `verify.ps1`에 `-UiSizes` · `-UiScales`(QT_SCALE_FACTOR) — 화면 변경은 1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × 배율 1 · 2 모두 `RESULT ok`가 고정 완료 기준(디자인 v5 세션이 CLAUDE.md에 적은 크기와 같음). backlog #1에 사용자 말 그대로.
  - `project-profile.md` §11 superpowers 단계 배정표(brainstorming · writing-plans · TDD · systematic-debugging · verification-before-completion · requesting/receiving-code-review · finishing-a-development-branch) + context7 규칙(Qt 6 ID `/websites/doc_qt_io_qt-6_8`, resolve → query 실제 응답 확인). builder · spec-writer에 Skill + context7 도구 열고 test-runner에 Skill.
  - 고친 버그: `.ps1` BOM, `Start-Process` 빈 인자, PS 5.1의 1원소 배열 풀림 · `"[]"`→`$null` · jq 큰따옴표 벗김, `gh --commit` 전체 SHA, gh는 저장소 폴더 안에서만, `$ErrorActionPreference='Stop'`이 gh stderr를 종료 오류로.
  - **`C:\dev\kerf-v4c`는 기준선이 아님:** 17:43 빌드(다른 세션의 아이콘 교체 미커밋 변경 포함)는 1920x1040에서도 `icons-missing=33 → fail`. 해상도 기준선은 깨끗한 HEAD `bc0aa32` worktree로 잼 → `factory verify … -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2` **RESULT PASS**: 8가지 모두 `ui-audit RESULT ok`(각 62–66 s), 빌드 새 경고 0, 빠른 시험 51300 통과, asec-section 기준값 일치. 측정 뒤 시험용 worktree · 브랜치 · 기록 삭제.
- **남은 문제:** ezdxf 없음(`dxf_audit.py` 못 돔) — 설치는 사용자 허락 뒤. 로컬 HEAD(`bc0aa32`)는 push 전이라 Actions 결과 없음. 커밋은 사용자 지시 대기.

### 2026-10-09 · Claude Code — 디자인 v5 A단계: 아이콘을 SVG로(C7) — 커밋 `bc0aa32` · `35be58d`
- **무엇을:** `app/icons/lucide/*.svg` 24개(Strata와 같은 Lucide, ISC 라이선스 동봉) + `app/icons/kerf/*.svg` 38개(Kerf 전용, 화판 7 그림 그대로, `make_kerf_svgs.py`). `app/icons/icons.cmake`가 손으로 적은 목록을 configure 때 `generated/kerf_icons.hpp`로 굽는다(rcc · moc 없이, 두 빌드 분기 공통). `app/icons.{hpp,cpp}`: `kerf::icon(이름, px, 먹색)` · `chipIcon(타일 · 아이콘 · 종류 · 색)`(상태 Normal · Active · Disabled × Off · On, dpr 1 · 1.5 · 2 · 3) · `hasIcon` · `iconNames`. `theme.hpp`의 코드 아이콘 150줄을 없애고 `iconName(Ico)` 이름 표 + `kerf::icon` 호출로. 토큰 `ActionHover` · `ChipPressed`(Strata tokens.json 값). `main.cpp --icon-sheet f.png`(모델 없이 돎, 빠진 이름 있으면 종료 코드 10).
- **확인:** 빈 구현으로 `--icon-sheet` → `EXIT=10 missing=31`(RED) → 구현 뒤 `EXIT=0 n=62 missing=0`(GREEN), `C:\dev\tmp\ui\icons.png`에서 타일 62개 확인. `cmake --build` exit 0(새 경고 0, 기존 `_wfopen` · `mirrored`만). `asec_tests "~CoalescingWorker*"` 51300 assertions 통과. `--ui-audit` RESULT ok. 실제 화면 캡처 `C:\dev\tmp\kerf-shots\v5-a2-icons.png`에서 리본 · 머리 · 배지 아이콘이 SVG로 바뀐 것 확인.
- **판정:** A2(그리기)와 A3(`--icon-sheet`)를 한 커밋으로 — `--icon-sheet`가 A2의 RED→GREEN 시험 장치라 — 잘못이면: 되돌릴 때 둘이 같이 돌아감. `CLAUDE.md` · 이 파일은 팩토리 세션(다른 창)의 미커밋 변경과 섞여 있어 코드 커밋에서 뺌 — 사용자 지시 때 한꺼번에 커밋. 사용자 추가 지시 둘을 기록함: ① 「superpowers 스킬 · context7을 안정적으로」 → `CLAUDE.md` 「먼저 할 일」 개발 방식 절 ② 「4K · 와이드 어디서나 안정」 → `DESIGN_SPEC §2.1` · 계획 Task B5.
- **남은 문제:** 리본은 아직 v4 모양(18 px 아이콘 옆 글자) — B단계에서 타일 리본으로. offscreen 캡처는 글꼴이 없어 글자가 □로 보임(앱 정상).

### 2026-10-09 · Claude Code — 디자인 v5 B단계(리본 C1 · 문서 탭 C2) · C단계(떠 있는 안내 C4 · 상태줄 C6) — 커밋 `070dce5` · `fa961ca` · `5fa3d88`
- **무엇을:** `app/ribbon.{hpp,cpp}` — Strata `KaBeginnerRibbon` 규칙을 `Q_OBJECT` 없이 옮김(묶음 이름 위 · 타일 칩 `kerf::chipIcon` · 크기 단계 50…34 → 32 → 32/24/20 글자 숨김 · 창 폭에 맞는 가장 큰 크기 · Tab/Enter). `buildRibbon()` 다시 씀: 탭 없이 **모델 · 단면 · 입면 · 보기 · 자료 · 내보내기 · 기타**, 글자 2–4자, 「도면」 흙색 주 단추, 오른쪽 끝 「좌표 입력」 + 「단면 찾기 Ctrl+F」(목록 거르기), 「더보기 ▾」(불투명도 · 단면선 굵기 · 매끈하게 · 영어 병기 · 정보 · 끝내기). `setRibbonContext(홈 · 단면 · 도면)`. 문서 탭 줄 36 px **홈 · 단면 · 도면(×)** 상시 + 구석 「모델 · 저장됨」. `app/guideband.{hpp,cpp}` — 평면 왼쪽 위 떠 있는 안내 칩 + 되돌리기 · 다시(리본 아래 도구 줄 삭제). 상태줄 32 px 셀(X · Y · Z) + 「수평 EPSG ▾」 · 「높이 ▾」 배지.
- **확인:** `--ui-audit` v5 기대값으로 고쳐 **RESULT ok**: `tabs=0 doctabs=홈,단면` · `ribbon height=118 groups=7 look=50 labels=1 chips-clay=1`(폭 근거 need50=1906) · `guide-band=1` · `status-badges=2` · dup 0 · icons 0. `--ctx-shot` guide=1. `asec_tests "~CoalescingWorker*"` 통과. 캡처 `C:\dev\tmp\kerf-shots\v5-b2-work.png` · `v5-b2-start.png` · `v5-c1-draw.png` · `v5-c2-work.png`(실제 화면).
- **판정:** ① 처음 감사에서 타일 32가 골라짐(need50 2185) → 입면 위젯 489→302 · 오른쪽 끝 320→278 · 묶음 여백 3 · 칩 바닥 타일+6(Strata +8)으로 1906. 잘못이면: 칩이 Strata 보다 2 px 좁음. ② 자료 「목록」→「목록 파일」(보기 「목록」과 겹침). ③ 좁은 창 감사는 창 최소 폭(판 넷 합 1729)에 막혀 `chooseLook` 규칙으로 판정 — 창 최소 폭 줄이기는 B5. ④ 평면 오른쪽 위 떠 있는 확대 묶음은 만들지 않음(머리와 겹침). ⑤ hatch · outline 칩은 꺼진 채(F · G에서 켬). ⑥ 「수평」 배지 글은 「수평 EPSG:5186 ▾」(원점 한글 이름 코드에 없음).
- **남은 문제:** 홈(D) · 도면 판(E) 아직 v4 모양. `CLAUDE.md` · 이 파일은 팩토리 세션 미커밋 변경과 섞여 커밋 보류.

### 2026-10-09 · Claude Code — 디자인 v5 D단계(홈 C3) · 개발 규칙 정본 `docs/DEV_RULES.md`
- **무엇을:** `rebuildStartPage()` 다시 씀(스펙 §7, Strata 홈 뼈대): 왼쪽 앱 아이콘 56 + 「Kerf | 발굴 평·단면」 + 설명 + 「모델 열기 Ctrl+O」 · 「최근 ▾」 | 이어서 작업 카드(먹색 테: 이름 명조 40 · 보조 줄 · 칸 6 · 오른쪽 흙색 「이어서 열기 →」 + 단면 썸네일(누르면 그 단면으로, `pendingSection_`)) | 「최근 모델 n」 + 찾기 칸 + 삼선표(모델 · 상태 · 수평 · 높이 · 단면 · 마지막 열림, 두 번 누르면 엶, 오른쪽 클릭 폴더 열기 · 목록에서 지우기) | 「작업 순서」 세 걸음(✓ · ● 다음 · 숫자) | 「처음 쓰는 키」 줄 + 「모든 키 F1」. 옛 리본 걸음 단추(`buildSteps` · `stepBtn_`) · 끌어 놓기 칸 삭제(창 전체가 받음). 사용자 지시 「모든 개발규칙을 절대 준수하라, 규칙을 지정하라」 → **`docs/DEV_RULES.md`**(R1 절차 · R2 코드 · R3 검증 · R4 기록 · R5 Git · R6 검토 · R7 안전 + 과제 체크리스트), CLAUDE.md 맨 위에서 가리킴.
- **확인:** 빌드 exit 0(새 경고 0). `--start-shot` → `C:\dev\tmp\kerf-shots\v5-d1-home.png` 눈으로 확인(흙색은 「이어서 열기」 하나, 작업 순서 ✓✓●). `--ui-audit` RESULT ok. `asec_tests "~CoalescingWorker*"` 통과. cpp-reviewer(B · C diff) · ui-reviewer(홈 · 작업 · 그리기 캡처) 독립 검토 요청함(결과는 다음 기록에).
- **판정:** 홈 「모델 열기」 · 「최근」은 리본 모델 묶음과 같은 일(Strata 홈도 「새 조사 · 조사 열기」가 리본과 겹침) — 홈은 감사 대상 밖. 「도면」 칸은 저장 기록(`sheets` 메타)이 생기기 전까지 「—」.
- **남은 문제:** 도면 판(E) · 레벨선 pt(E1–E2) 다음.

### 2026-10-09 · Claude Code — 디자인 v5 E1 · E2(레벨선 0.2 / 0.5 pt) — 커밋 `a368b9d` · `e9116b1`
- **무엇을:** core `sheet.hpp`: `LineWeights{0.2, 0.5}` · `ptToMm` · `clampLineWeightPt`(0.05–3.0, NaN→0.2) · `dxfLineWeight(pt)`(DXF 허용 굵기 370 으로 반올림: 0.2 pt → 9, 0.5 pt → 18). `DxfExportOptions` · `DxfParams` · `SheetParams` · `SectionStyle` 에 `levelMinorPt / levelMajorPt`, QSettings `sheet/levelMinorPt · levelMajorPt`. `paintSectionDoc` 는 내보내기 · 도면 미리보기에서 `ptToMm(pt) × px/mm` 굵기(1 m 선도 0.5 pt), 작업 화면은 바닥값 0.75 / 1.0 / 1.25 px. DXF 레이어 LEVEL_10CM 9 · LEVEL_50CM 18 · LEVEL_1M 18. `SheetPaintProbe.levelMinorPx/MajorPx` + `--sheet-check level-pt` 판정 줄.
- **확인:** 시험 먼저 — `asec_tests "[pt]"` 링크 실패 · `--sheet-check` `level-pt … ok=0 RESULT fail`(EXIT 8) → 구현 뒤 `[pt],[dxf]` 25/25 · `level-pt minor=0.564 expect=0.564 major=1.411 expect=1.411 ok=1 RESULT ok`(EXIT 0) · 전체 `asec_tests` 139 통과 · 1 건너뜀 · `--ui-audit` ok · `--export-dxf` 레이어 370 = 9 / 18 / 18 · 빌드 새 경고 0.
- **판정:** 1 m 선도 0.5 pt(사용자 말 「50 cm 선 0.5 pt」에 포함) — 잘못이면 pt 칸 하나 더(E3). 도면 미리보기는 종이 기준 그대로(낮은 배율에서 10 cm 선 희미) — WYSIWYG. DXF 는 허용 굵기 반올림(형식 한계).
- **남은 문제:** E3 에서 판에 pt 칸 둘 노출 · 미리보기 눈 확인.

### 2026-10-09 · Claude Code — B · C · D 검토 반영(cpp-reviewer 7건 · ui-reviewer 8항목) — 커밋 `0f30c5a`
- **검토 결과(R6.2):** cpp-reviewer: Critical 0 · Important 7(I1 켜고 끄기 두 곳 · I2 홈에서 그리기 지속 · I3 키 캡 재생성 · I4 도면 탭 잔류 · I5 거르기 풀림 · I6 감사 고정값 · I7 아이콘 캐시) → **7건 모두 고침**(I7 은 캐시 비움까지, QIconEngine 은 미룸). ui-reviewer: 8항목 모두 4점 미만 → 글 잘림 · 겹침 · Strata 뼈대 · 상태 · 접근성 지적을 고침(아래), 미룬 것은 판정.
- **무엇을:** `updateEnabled()` 한 곳(문맥 · 모델 · 작업 중 · 그리는 중), `discardSheetTab()`, 안내 칩 라벨 재사용 + 좁으면 도구 이름 단계만 → 숨김 → 줄임표 + 되돌리기 칸 아래, 판 폭 348 · 272 고정, 머리 · 정보 줄 가로 Ignored(창 최소 폭 1763 → 1280), 배지 24 · 「높이 KVD1964 ▾」 · 회색 none, 홈 「높이」 칸 · 폴더 · 표 높이, 오른쪽 판 그리는 중 + 아이콘, 단면 화면 안내 띠, 정보 줄 「잘린 선 …」, 상태줄 한 문장, 「세로 ×1 ▾」, Tab 초점 테, 같은 아이콘 정리(dup-icons), 나침반 오른쪽 아래 「진북」, 눈금 숫자 흰 테, 화면 레벨선 8 px 규칙, 문구 띄어쓰기.
- **확인:** `--ui-audit` 새 항목 5개 RED(EXIT 9) → 전부 ok(EXIT 0, offscreen · 실제 화면). 빌드 새 경고 0. `asec_tests "~CoalescingWorker*"` 51325 통과. `--icon-sheet` EXIT 0. 캡처 `rv-home.png` · `rv-work.png` · `ui-screen/draw.png` · `draw2.png`: 칩 · 되돌리기 아래 · 안내 띠 · 오른쪽 판 그리는 중 · 나침반 「진북」 · 축척 막대 눈으로 확인(감사 `grab()` 캡처에는 GL 위 그림이 안 찍힘 — `shot.ps1` 캡처로 봄).
- **판정:** 장부 `Task R1` Ruling 9개(안내 칩 줄이기 순서 · 판 폭 고정 · Ignored · 배지 22+테 · 감사 글꼴 허용 · 2b 순서 · 그리는 중 띠 · 「저장됨」 · 미룬 지적 목록). 미룬 지적은 계획에 **D2(오른쪽 판 §4) · D3(목록 줄)** 로 더함.
- **남은 문제:** 재검토(cpp · ui) 결과 대기. 커밋 push 전(사용자 지시 때).

### 2026-10-09 · Claude Code — 검토 2차 반영(cpp C2 · I3 · M13, ui 8항목) + 사용자 지시 「UI 사각형 선 전부 라운드」
- **사용자 지시(말 그대로, 2026-10-09):** 「앱안에 ui의 사각형 선들은 전부 라운드 처리하라」 → 스펙 §2.1 · DEV_RULES R2.3 에 규칙으로 적음.
- **검토 결과(R6.2):** cpp-reviewer(`a368b9d..0f30c5a`): Critical 2(C1 저장 중 `embeddedSave_` 파괴 UAF · C2 끼운 조판의 `accepted` 지역 변수 참조) · Important 3(I1 나침반 눌림 자리 · I2 `updateHeader` 가 그리는 중 판 덮음 · I3 도면 문맥에서 그리기 지속) · Minor 13 → **C · I 전부 + Minor 1 · 2 · 3 · 5 · 6 · 7 · 8 · 12 · 13 고침**, 나머지 Minor 는 판정. ui-reviewer: 7항목 3점 → 우선 지적(글자 체계 · 그리는 중 상태 셋 · 꺼짐 모양 · 접근성 · 표고 (m) · 아이콘 · 판 머리 · 여백) 고침.
- **무엇을:** `saveEmbeddedSheet` 가 함수 복사본을 부름 + 저장 창 뒤 모델 바뀌면 저장 안 함(C1), 끼운 조판은 `accepted` 안 씀(C2), `PlanView::compassCenter()` 한 곳(I1, 감사 `compass-hit`), `updateHeader` 가 그리는 중이면 정보 줄만 쓰고 돌아감(I2), `setRibbonContext` 가 단면 문맥을 떠나면 도구 내려놓음(I3), 빈 구간 길이 = 결과 선(M1), `--sheet-check` 레벨선 켬(M2), 리본 DXF 도 pt 설정(M3), 모델 바뀌면 찾기 칸 비움(M5), 조판 목록 지연 호출 가드(M6), 감사가 상태줄 · 활성 보기 되돌림(M7), 초점 이유 Active · Popup 은 이전 값(M8), 높이 이름 `heightDatumShort()` 한 곳(M12), 감사 `sheet-discard`(M13). UI: 글자 체계 11 · 12 · 13 · 20(단면 머리 `viewTitleText` 13 · 오른쪽 판 이름 명조 20 · 「진북」 11 · 홈 빈 상태 20), 안내 칩 단계 이름은 폭이 되면 보임 + 세로선 같이 숨김 + 새 키 캡 라벨 `ensurePolished`(칩 25 px 좁아 문장 잘림 실측 → 고침), A′ 단계 판 「A 찍음 · 끝점 A′를 찍으세요」, 꺼짐 QSS(좌표 입력 · 찾기 · 홈 단추), 모든 단추 Tab 초점 테(앱 전체 이벤트 필터) · 목록 · 표 · 배지 TabFocus, 「표고 (m)」 잘림 밖으로, 축척 막대 격자선과 띄움, 빈 상태 글 「단면선 긋기」, Z 셀 빈 글, 위에서(사각 + 화살) · LAS(구름 + 점) 아이콘, 메뉴 칩 ▾ 굽기, 작업 순서 숫자 + 흙색 토큰, 홈 빈 상태 주 단추 하나(왼쪽 「모델 열기」 흙색 · 카드 단추 · 칸 숨김) · 「열어 본 모델이 여기에 쌓입니다」 · 찾기 돋보기, 네 판 32 px 머리 + 여백 16, 입면 칸 타일 가운데(`elev-center-dy=1`), 나침반 · 축척 막대 여백 16. 둥글기: viewFrame 8(둥근 마스크) · chip 6 + 사이 2 px · smallRow · 메뉴 목록 · QMenu · continueCard · thumb · 입력 칸 6 · 체크 3 · 썸네일 6 · 종이 미리보기 테두리 4. 찾기 칸 150 → 142(need50 1916).
- **확인:** 시험 먼저 — 감사 항목 `ribbon-context`(elev-center-dy=8) · `compass-hit=0` · `draw-step2-note=0` · `square-borders=7` RED → 고친 뒤 전부 ok(offscreen · 실제 화면 RESULT ok, 실제 화면 창 최소 폭 1527 → 1275). 빌드 새 경고 0. `asec_tests "~CoalescingWorker*"` 51325 통과. `--icon-sheet` EXIT 0. 캡처 `ui-screen/draw2.png` · `r3-chip.png`(문장 전체 + Enter Esc, 되돌리기 오른쪽) · `r3-corner.png`(둥근 틀) · `r3-sheet-right.png`(둥근 칩) · `kerf-shots/r3-home.png`(주 단추 하나 · 둥근 카드) 눈 확인.
- **판정:** 장부 `Task R2` · `Task R3` Ruling(종이 위 도면은 둥글기 제외 · 한 변 선 제외 · QMenu 둥글기는 창 모서리 바탕색 · 「A」 글자색 #FF0000 유지 · 자동 맞춤(autoFit) · 창 최소 폭 강제는 B5 · dxfLineWeight 위치 유지 · QIconEngine 미룸).
- **남은 문제:** E4(여러 장 SVG + `--sheet-batch`) · B5 · D2 · D3 · F · G · H · I. push 는 사용자 지시 때.

### 2026-10-09 · Claude Code — 디자인 v5 E4(여러 장 SVG 저장 · `--sheet-batch`)
- **무엇을:** `MainWindow::saveCheckedSheets(dir, base, log)` — 조판 목록에서 고른 칸(평면 1 + 단면 n) 전부를 한 폴더에 저장. 이름 core `sheetFileName(모델, 도면, 분모)` + `uniqueSheetFileName`(겹치면 `_2`). 평면도는 마지막 평면 조판 설정(없으면 기본), 단면은 판의 설정(용지 · 항목 · 형식 · pt)에 도면마다 자기 축척(저장된 값 → 없으면 맞춤, 지금 연 도면은 판의 값). 단면마다 `selectSection` + `computeNow`(최종), 끝나면 고른 단면으로 되돌림. 저장 상태는 QSettings 모델별 `sheet/<이름>/denom · paper · saved` → 목록 줄 글. 판 「n장 SVG 저장」 · 목록 「내보내기」가 고른 칸이 둘 이상이면 폴더를 묻고 묶음 저장, 끝나면 판 바닥 「n장 저장됨 · 폴더 열기」(링크). `main.cpp --sheet-batch DIR`: 전부 고르고 SVG 로 저장, 수가 안 맞으면 종료 코드 9.
- **확인:** 시험 먼저 — 빈 구현으로 `--sheet-batch` → `saved=0 expected=4 RESULT fail`(EXIT 9) → 구현 뒤 `Synthetic_평면도_1-90.svg · A-A′_1-50 · B-B′_1-50 · C-C′_1-50` 4장 `RESULT ok`(EXIT 0, 합성 모델 + `--extra-line` 2개). 빌드 새 경고 0. `--ui-audit` RESULT ok. `--sheet-check` ok. `asec_tests "~CoalescingWorker*"` 통과.
- **판정:** 「n장 저장됨 · 폴더 열기」 줄은 묶음 저장(폴더 대화상자) 뒤에만 보여 캡처로 확인하지 못함 — 코드 경로 + `--sheet-batch` 결과로 갈음 — 잘못이면: 글 모양을 눈으로 못 봄(다음 실제 저장 때 확인). DXF · PDF · PNG · TIFF 묶음도 같은 함수(형식 칩대로) — SVG 만 `--sheet-batch` 로 판정.
- **남은 문제:** B5(해상도 · DPI) → D2 · D3 → F · G · H · I. 사용자 화면의 「no Qt platform plugin」 대화상자: `C:\dev\desca-build\app\platforms` 에 `qoffscreen.dll` 이 없어 offscreen 실행이 뜨지 못한 것(이 PC 의 다른 빌드 폴더) — 복사해 둠.

### 2026-10-09 ~ 10 · Claude Code — 디자인 v5 B5(해상도 · DPI)를 팩토리 작업 `20261009-01-b5-dpi` 로 — 병합 `8cc67fb`
- **사용자 지시(말 그대로, 2026-10-09):** 「이것은 리눅스에 개발이 아니다 윈도우만을 대상으로 개발하라.」 → DEV_RULES R7.4 · 「최종 완성이 되면 포터블로 다른컴퓨터에서도 실행될 구조로 개발할것이며 포터블로 전환은 사용자 요청이없으면 하지마라.」 → R7.5 · 「지금 개발설정을 지켜가며 개발하라. 니맘대로하지말고」 → B5 부터 **팩토리**로(AskUserQuestion 선택) · 「단계마다 깃커밋해줘」 → R5.1 · /goal 「남은 개발작업을 플랜을 작게 나누고 나눠서 /factory 개발을 진행」 → backlog #3c–#9(계획 Task 하나 = 작업 하나).
- **무엇을(팩토리 작업, worktree `C:\dev\kerf-wt\20261009-01-b5-dpi`, 기준 `0f7fbfe`):** spec-writer 명세 AC1–AC11 → builder 커밋 7(`b5640c0` RED 감사 줄 → `30a8413` `--size` 요청 크기(홈 한 줄 라벨 넷이 최소 폭 1591 을 밀어내던 것, `Ignored`) → `c9b081e` 아이콘 배율 1 · 1.25 · 1.5 · 1.75 · 2 · 3(`kerf::iconDprs()` 한 곳) → `504e6c1` 안내 칩 `QEvent::DevicePixelRatioChange` 다시 굽기 + `place()` 안전판 → `73493e2` `fitted_`: 창 크기 바뀌면 맞춤 상태의 평면 · 단면만 다시 맞춤, 사용자 확대 · 이동은 축척 유지 → `066455a` CI Linux 작업 삭제 + 감사를 `Start-Process` 로 기다려 판정 + 3840×2160 ×2 → `66737c5` env 줄 실제 화면 판정 범위) → test-runner 독립 검증 PASS → 검토자 넷 → 사용자 승인(`66737c5`) → worktree 에 `feature/design-v2` merge(`b378b3f`) → 전체 검증 `-Full` + 8조합 PASS → 재승인 → 본 저장소 `--no-ff` 병합 **`8cc67fb`**. 기록 전부 `factory/jobs/20261009-01-b5-dpi/`(spec · build · verify · review · review-code · review-correctness · approval).
- **확인:** `--ui-audit` 새 판정 줄 RED(1280 `size=1591x800` · `icon-dpr-bad=2` · `dpr-rerender=0` · `plan-fits=0 section-fits=0`, EXIT 9) → GREEN. V8(1280×800 · 1920×1040 · 3440×1440 · 3840×2160 × `QT_SCALE_FACTOR` 1 · 2) 8가지 모두 `RESULT ok`, 1920 ×1.25 · ×1.75 ok, 실제 화면(offscreen 아님) 1920 `EXIT=0 RESULT ok`(`plan-fits=1 section-fits=1 zoom-kept=1`). 전체 `asec_tests` 51339 통과(CoalescingWorker 포함). `--icon-sheet` missing=0 · `--sheet-check` ok · `--ctx-shot` guide=1. CI 로컬 모의: 양성 2 exit 0 · 음성(없는 모델) throw. 병합 뒤 본 저장소 빌드 exit 0 · 빠른 시험 51325 통과 · 실제 화면 캡처 `C:\dev\tmp\kerf-shots\b5-merged-work.png` 눈 확인.
- **검토(R6.2):** code pass(should 3 · note 4) · correctness pass(should 2 · note 6) · security pass(note 4) · UI 1차 fail(blocking 2: 1280 단면 머리 「북쪽을 봄」·토글 겹침 · 리본 1280 타일 20 ≠ 스펙 32) / 2차 pass(둘 다 전부터 있던 것 — RED 실행에서 이미 `min-width=1280`, 머리 코드 불변) → **needs-human → 사용자 결정:** 「후속 작업으로 빼고 B5 는 승인 보고서로」 · 「스펙을 실제(1280 → 20)로 고친다」 → backlog **#3c**(1280 좁은 창 + should 7건 + 스펙 §2.1 · §12 1280 → 20).
- **판정(Ruling):** ① B5 를 본 체크아웃 executing-plans 가 아니라 팩토리로 — 사용자 지시 — 본 체크아웃의 B5 RED 코드는 `C:\dev\tmp\b5-red.patch` 로 빼고 `app/` 은 HEAD 로 되돌림. ② CI Linux 작업 삭제 — 사용자 선택(R7.4) — Linux 에서 core 가 깨져도 모름(배포 대상 아님). ③ 열린 질문 Q1(첫 창 1600×950 > 모니터) → backlog #2 — 사용자 선택. ④ builder 판정 4(build.md): 실제 화면에서는 env ①·④ 판정 제한(150 % 모니터에서 1040 → 940, dpr = OS 배율 × 변수) — offscreen 8조합은 스펙 그대로. ⑤ builder 가 Task 2 커밋 메시지(BOM) 교정으로 자기 브랜치 역사를 재적재(push 전 · factory 브랜치만, diff 동일) — 공유 브랜치 아님. ⑥ 검토자가 Write 도구가 없어 `review.md` 에 직접 썼고 code · correctness 는 상세를 `review-code.md` · `review-correctness.md` 로 남김 — 기록으로 보존.
- **남은 문제:** push 전(GitHub Actions 미확인 — 기준 `0f7fbfe` 의 Windows 작업은 failure, 원인 미조사). worktree `C:\dev\kerf-wt\20261009-01-b5-dpi` · 브랜치 `factory/20261009-01-b5-dpi` 삭제는 사용자 확인 뒤. 1280 좁은 창 문제 · 리본 스펙 20 · should 7건 → #3c. 홈 앱 아이콘 56 px · 「작업 순서」 체크 표시는 화면 옮김 때 다시 굽지 않음(범위 밖).

### 2026-10-10 · Claude Code — 디자인 v6 「전체 채택」 → 스펙 변경분 + 구현 계획(문서만, 코드 변경 없음)
- **사용자 결정(말 그대로 선택, 2026-10-10):** `/design`으로 만든 v6 제안 캔버스(https://claude.ai/artifact/6qRaVBkEjWawkAHeA7hxjk, 소유자만 열림 — 개요·변경표 + 화면 5장)를 보고 AskUserQuestion 에서 **「전체 채택」**. 이어서 「스펙 v6 작성 계획」을 승인하며 **「변경분 문서 + 계획」**(v5 본문 덮어쓰지 않음) 선택.
- **무엇을:** `docs/design/DESIGN_SPEC_v6.md`(v5 대비 변경분, 결정 C8–C16: 진행 표시 · 도구 막대 · Ctrl+K 명령 검색 · 판 접기 · 오른쪽 판 조용한 단추 · 도면 판 재배치 · 막혔을 때 안내 한 모양 · 작은 추가 · 글자 12 px, 감사 기대값, v5 위치 대응표, 열린 질문 3) + `docs/superpowers/plans/2026-10-10-design-v6.md`(Task V1–V12: 뒤집기 없는 것 V1–V5 먼저 → 뒤집는 것 V6–V11 → 문서 합침 V12, 작업마다 팩토리 작업). `CLAUDE.md` 에 한 줄 안내.
- **확인:** 문서만 바꿈 — 빌드 · 시험 대상 아님. `git status --short`: 새 파일 2개 + 이 파일 + `CLAUDE.md` + 다른 세션(팩토리 `20261010-01-narrow-1280`)의 `factory/jobs/…` 파일들(건드리지 않음). 문서 속 경로 · 절 번호는 `Grep`으로 대조.
- **판정(Ruling):** ① v5 본문 `DESIGN_SPEC.md` 를 덮어쓰지 않고 변경분 문서로 — 진행 중인 팩토리 작업 `20261010-01-narrow-1280`(#3c)이 v5 §2.1 · §12 를 고치고 v5 본문으로 검증 중 — 잘못이면: 문서가 두 개라 읽는 사람이 겹쳐 읽어야 함(V12 에서 합치고 변경분 삭제). ② 화판 원본은 저장소에 넣지 않고 링크만 — 소유자만 열리는 링크, `support.js` 없이는 안 그려짐 — 잘못이면: 링크가 사라지면 그림이 없음(스펙 글에 치수 · 문구를 다 적어 둠). ③ 스펙을 먼저 쓰고 코드는 안 바꿈 — 뒤집는 v5 결정(한 동작 한 자리 · 판 폭 고정 · 읽기 전용 · 11 px · 단면 찾기 칸 · 도면 판 구성)이 많아 사용자 확인 뒤에만 — 잘못이면: 없음(코드 변경 없음).
- **남은 문제:** 팩토리 대기 목록에 v6 작업(V1–V11)은 아직 안 넣음 — #3c 병합 뒤 사용자 확인 받고 넣는다. 대기 #3(D2)은 V7 과 합칠 것. 열린 질문(자동 저장 시각 근거 · 연습 모델 크기 · 걸음 누름 ↔ Ctrl+Tab) = 스펙 §13.

### 2026-10-10 · Claude Code — #3c 1280 좁은 창 · 홈 겹침 버그를 팩토리 작업 `20261010-01-narrow-1280` 으로 — 병합 `3c9b0d4`
- **무엇을:** 창 폭 1280 에서 단면 머리 · 정보 줄 · 평면 눈금 글 · 홈 한 줄 글을 정해진 순서로 줄이고(사용자 답 Q1 A · Q2 A · Q3) `--ui-audit` 판정으로 잠금, 리본 1280 기대값 20, B5 검토 should 7건, **사용자 보고 버그(홈에 옛 위젯이 겹치고 「목록에서 지우기」가 안 됨, AC14)** 고침. 수정 3회: ① 문서 키 ② 홈 「<모델> 기준」 사라짐 · 칸 이름 줄임표 · 빈 구간 꼬리 · 머리 맞춤 한 번 · ▾ ③(사용자 허가) 1920 이상 카드 칸 옛 2 : 1 · ▾ 8 × 5 보이게 + 감사 픽셀 판정.
- **확인:** 독립 검증 3차 13조합 RESULT ok · 통합 `9a77d61` 전체 시험 51339 · V8 + 1280/2560 × 1 · 1.25 · 1.75 RESULT PASS · 회귀 `--icon-sheet` · `--sheet-check` · `--ctx-shot` EXIT 0 · 긴 이름 1280 · 1920 ok. 검토 code · correctness · security · kerf-ui 모두 pass. 캡처 `C:\dev\tmp\n1280-rework3\`.
- **판정:** 2차 재검토 뒤 1920 이상 칸 배치 변경(명세 §1 과 어긋남)과 ▾ 안 보임은 자동으로 넘기지 않고 사용자에게 물음(R1.5 예외) → 「한 번 더 고친 뒤 병합」. 남은 should · note 는 대기 R1b.
- **남은 문제:** push 전. worktree · 브랜치 삭제는 사용자 확인 뒤. 기록 `factory/jobs/20261010-01-narrow-1280/`.

### 2026-10-10 · Claude Code — R0 MSVC 빌드 고침(팩토리 `20261010-02-msvc-c2026`) — 병합 `5331124`
- **무엇을:** `app/theme.hpp` 의 QSS 원시 문자열(20241 바이트 한 조각 — MSVC C2026)을 이미 있는 빈 줄 셋에서 `)" R"(` 로 나눠 4조각(4096 · 6783 · 5208 · 4154)으로, 규칙 주석(조각 ≤ 8000 바이트 · 빈 줄에서 나눔 · 출처 MS 문서 C2026).
- **확인:** RED = VS 2022 생성기 빌드 exit 1(C2026 하나). GREEN = `--clean-first` exit 0 · error 0(기존 C4551 경고만) · clang-cl `-Full` 51339 통과 · 감사 8조합 RESULT ok · QSS 바이트 같음(20241 · 343) · 캡처 work/start/draw 해시 같음. code-reviewer pass.
- **판정:** correctness · security · UI 검토는 해당 없음(화면 · 좌표 · 입력 무관). GitHub Actions 초록불은 push 뒤에만 확인 가능(Qt 6.8.3 헤더는 이 PC에 없음).

### 2026-10-10 · Claude Code — 입면이 하얗게 보임(깊이 음영) → 「깊이 음영」 체크 칸 · 기본 끔
- **사용자 보고 · 지시(말 그대로):** 「현재 앱의 입면이 보이지 않는다 단면선도 중요하지만 입면이 보이지 않아 약간 하얗게 나온다. 모든개발은 슈퍼파워skills 를 사용하여 개발을 하라.」 · 「모든테스트는 "C:\Users\권을\Downloads\Production_1.3sm" 이것으로하라」 · 「깊이 음영 체크를 만들라.」 → AskUserQuestion 답 「단면 머리, 입면 바로 위」 · 「기본 끔」.
- **원인(systematic-debugging):** 고장 아님 — 깊이 음영(`kDepthFadeStrength` 0.55, 기준 깊이 max(뒤, 2 m))이 기본으로 켜져 뒤쪽 면을 흰색 쪽으로 섞음. 실제 모델(뒤 5 m) 입면 평균 밝기 켬 143 · 끔 91, 색 진하기 15.9 · 22.4. 끄는 곳은 단면 머리 넷째 아이콘뿐이라 못 찾음.
- **무엇을:** 단면 머리 넷째 아이콘 토글 → 글자 있는 체크 칸 「☐ 깊이 음영」(`depthFadeCheck`, 아이콘 `fade`, 같은 QAction 에 양방향 묶음) · 기본값 끔(`section/depthFade` 기본 false, `SectionStyle` · `ViewState` 기본 false) · `--ui-audit depth-fade check label bound default-off` · 스펙 §4 · §12 한 줄씩.
- **확인:** RED — 새 감사 줄 `check=0 label=0 bound=0 default-off=0 FAIL`(실제 · 합성 모델 EXIT 9) → GREEN: 감사 행렬 1280×800 · 1920×1040 · 3440×1440 · 3840×2160 × 1 · 2 를 합성 · 실제 모델 둘 다 — 16조합 모두 `depth-fade check=1 label=1 bound=1 default-off=1`(1280 = 「그릴 것 ▾」 메뉴, 나머지 = 머리), 합성 8조합 `RESULT ok`. 빠른 시험 51325 통과. 회귀 `--undo-test` · `--sheet-check`(합성 · 실제) · `--ctx-shot`(실제) · `--icon-sheet` 모두 EXIT 0. 실제 모델 캡처 `C:\dev\tmp\elev-bug\after-*.png`(이 PC만 — 실좌표) 눈 확인: 입면이 원래 흙색 · 나무 색, 체크하면 바램.
- **판정:** ① 체크 칸에 아이콘도 붙임 — 감사 「글자 있는 단추는 아이콘」 규칙 — 잘못이면: 칸 폭 약 20 px 더 씀. ② offscreen 1920 감사에서 단면 머리 단계 1 → 3(「크게」 숨김 · 세로 아이콘만), 실제 화면 1920 캡처는 단계 1 그대로(글꼴 차이) · 2560 은 0 — 잘못이면: 1920 에서 「크게」가 숨을 수 있음(리본 「보기」에 같은 일). ③ 깊이 음영 세기(0.55)는 그대로 — 요청은 체크와 기본값뿐.
- **남은 문제(이번에 안 고침, 기록):** (a) 명령줄 `--depth-fade off` 는 단면선을 긋기 전에 토글을 바꿔 단면 방향이 뒤집혀 찍힘(시험 장치) — 설정 INI 로 우회. (b) 실제 모델에서만 감사 실패 둘(변경 전 RED 실행에도 있음): `home-clipped`(1280 홈 「C: › Users › 권을」 · 「EPSG:5183」 칸 잘림) · `icons-missing=2`(좌표계 확인 알림 띠 「높이 기준 지정…」 · 「자세히」). 이어서 과제 ②(더보기의 영상 불투명도 · 단면선 굵기 → 리본 입면 아래 줄, R4 일부).

### 2026-10-10 · Claude Code — 더보기의 영상 불투명도 · 단면선 굵기 → 리본 입면 아래 줄(R4 일부)
- **사용자 지시(말 그대로):** 「ui더보기안에 이것은 밖으로 빼라. … 이위치 밑에 넣어라」(그림: 더보기의 영상 불투명도 · 단면선 굵기 / 리본 입면 묶음) → 설계 승인 「진행」(좁은 창은 더보기 안에 — 추천안).
- **무엇을:** 리본 입면 칸을 두 줄로 — 위 줄(깊이 칩 · 뒤 · 앞)은 타일 가운데 그대로(`ribbonCenterHeight`), 아래 줄 「영상 불투명도」 슬라이더(가로 Ignored · 최소 56) · 「단면선 굵기」 칩 1.5 · 2 · 3(28 px). `Ribbon::setLookHook` — 크기를 고를 때 여백 계산 전에 불러, 두 줄이 칩 높이에 안 들어가면(글자 숨김 크기) 아래 줄을 숨기고 더보기 안의 같은 칸을 보임. 값은 하나: `opacity_`(정본) ↔ `opacityMenu_`, 굵기 칩 전부는 `setSectionLineWidth`. 홈 · 도면 문맥에서는 입면 칸과 같이 꺼짐. 감사 `elev-row2`, 스펙 §3 표 · §12, backlog R4 「일부 끝」.
- **확인:** RED — `elev-row2 … FAIL`(1920 wide=1 · 1280 wide=0, 합성 · 실제). 첫 구현에서 시험이 아래 줄 최소 폭 330 > 위 줄 320 을 잡음(1920 타일 50 → 48, 1280 최소 폭 1290) → 칩 32 → 28 · 띄움 6 → 2 로 314. GREEN: 1280 · 1920 · 2560 · 3440 · 3840 × 1 · 2 × 합성 · 실제 20회 모두 `elev-row2 … sync=1`(넓으면 `shown=1 more=0`, 1280 은 `shown=0 more=2`) · `need50=1916 need20=1280` 그대로 · `elev-center-dy=1` · `depth-fade` 통과, 합성 10회 `RESULT ok`. 실제 화면(offscreen 아님) 1920 감사도 `elev-row2` 통과(`row-min=300`). 빠른 시험 51325 · `--undo-test` · `--sheet-check` · `--ctx-shot` · `--icon-sheet` · `--sheet-batch` EXIT 0. 캡처 `C:\dev\tmp\elev-bug\task2-*.png` 눈 확인.
- **판정:** ① 좁은 리본(글자 숨김 — 창 폭 약 1680 미만)에서는 두 칸이 더보기 안에 남음 — 리본 높이를 늘리지 않으려고(사용자 승인한 추천안) — 잘못이면: 좁은 창에서는 여전히 더보기를 열어야 함. ② 아래 줄 메뉴 → 줄 방향 칩 시험은 메뉴 칸이 보이는 크기(1280)에서만 — 넓은 창에서는 숨긴 QWidgetAction 칸의 click() 이 먹지 않음(실측). ③ 이름(「영상 불투명도」 · 「단면선 굵기」)은 줄이지 않고 칩 폭 · 띄움만 줄임.
- **남은 문제:** R4 나머지(평활 · 정보 · 끝내기 · 더보기 삭제). 실제 모델 감사의 기존 실패 둘(`home-clipped` · 알림 띠 `icons-missing=2`)은 그대로 — 알림 띠는 좌표계 이름표 문제(아래 R6)를 고치면 사라질 것.

### 2026-10-11 · Claude Code — 최종 디자인 캔버스 「Trench 최종 디자인」(이름 Kerf → Trench · 아이콘 · 여는 모션 · 화면 넷 · 부품 · 커서 · AI 작업 지시서)
- **사용자 지시(말 그대로, 2026-10-10 밤):** 「이제 최종 디자인과 최종 개발을 완료를 할 디자인 최종을 만들어보자 지금까지 사용자의 요청과 수정했던것을 통합 가장 이상적인 앱을 개발하자. 이것은 아이콘과 커서 첫화면의 모션그래픽 앱안의 디자인을 전체 합쳐 마지막 디자인을 만드는 것이다」 · 「각 디자인 ui 버튼들의 상호작용과 사용자의 편의성 폰트 색 모션그래픽 아이콘 커서까지 … 사용자가 놓친것을 ai에게 디자인과함께 작업지시 개발진행법 세부적인 디테일 디자인 ui의 밋밋한 디자인의 보완」 · 「현재 처음시작 로딩 모션그래픽 다른것으로 구상 … 아이콘 다른것으로 구상 깊이 생각 이름도 다른것으로 깊이생각하라」 · 「백그라운드 멈춰라 끝나지도않았는데 리뷰를 하나」(검토는 전부 끝난 뒤 한 번).
- **결정:** 앱 이름 **Trench(트렌치)**(AskUserQuestion, 추천안 선택 — Sectum · 켜 · Plumb 중). 디자인 기준 = 앱 코드 토큰 그대로(`app/theme.hpp`, 새 색 없음). 아이콘 「T는 트렌치의 단면이다」(크림 타일 · 흙 띠 · T 모양 빈 자리 · 빨간 단면선), 여는 모션 「T를 판다」 1.0 초 — 사용자 「묻지 말고 깊이 생각해 하나로」에 따라 AI가 정함(다른 안 셋은 폐기).
- **무엇을:** 디자인 캔버스 https://claude.ai/artifact/St8M2cwJYYfPW9gcNDshRQ (소유자만 열림) — 화판 16: 0줄 개요 · 토큰 · 이름/아이콘/모션(최종) / 1줄 단추 · 칩 · 칸 상태와 상호작용 · 아이콘 62 · 커서 5 / 2줄 여는 화면(움직임, ↻ 다시 재생) · 모션 시간표 / 3줄 홈 · 단면 작업 · 긋는 중 · 도면 탭(2560×1440, 실제 모델 Production_1 그림) / 4줄 F1 · Notice 3종 · 빈 상태 · 툴팁 · 밋밋함 보완 8 / 5줄 AI 작업 지시서(G0–G20, 8걸음, 완료 정의, 화판 → 코드 대응) · 놓친 결정 20(추천 17 · 물음 3: Illustrator 버전 · A2/A1 · Strata로 넘길 파일). 캔버스에 올린 정사영상 · 입면 조각은 비공개 아티팩트에만(저장소에는 안 넣음).
- **확인:** 캔버스 publish 25판까지 모두 성공(파일 17). 화판 내용은 코드가 아니라 빌드 · 시험 대상 아님. 이 기록 · backlog 한 줄만 커밋.
- **판정(Ruling):** ① 디자인 범위 = v5 뼈대 + 2026-10-10 결정(Q0–Q6 · A1–A14 · R-목록) + v6 중 V4만 + 오늘 둘(깊이 음영 · 입면 아래 줄) — 어긋나면 캔버스가 이김(R23 문서 합침 때 DESIGN_SPEC 본문으로). ② 「기타」 묶음은 「보기」로 합쳐 묶음 6(점검 P2 추천) — 잘못이면: 「단축키」 자리만 되돌림. ③ 커서는 도구를 든 동안만 십자 + 단계 배지(5종) — 잘못이면: 배지 없는 OS 십자로 되돌림. ④ 코드 검토(3f43c5b · e3abc49)는 사용자 지시로 중단 — G0–G20 끝에 한 번. ⑤ 이름 바꾸기는 코드 작업 G0(창 제목 · 설정 이전 · .ico · 여는 화면)에서 — 저장소 · 폴더 이름(`desca` · `kerf-*`)은 바꾸지 않음(사용자가 말할 때).
- **남은 문제:** 다음은 G0부터(Guide 화판 순서). 좌표계 5183 → 5187(R6 = G1)은 사용자가 물은 것 — G0 바로 뒤. 캔버스 질문 셋(18 · 19 · 20)은 R13 · 용지 · 자료 정리 전까지 답을 받는다.

### 2026-10-11 · Claude Code(Sonnet 5.5, advisor Opus 5.5) — T1 (G0a): 사용자에게 보이는 이름 Kerf → Trench
- **사용자 지시:** /goal 「작업지시서와 trench 최종 디자인을 보고 맞춰가며 개발디자인을 보완하라 … 슈퍼파워skills … 매단계 커밋하고 다음으로」. 계획 `docs/superpowers/plans/2026-10-11-trench-final.md`(T1–T22), 장부 `.superpowers/sdd/2026-10-11-trench-final/progress.md`.
- **시험 먼저(RED):** `--ui-audit` 새 줄 `app-name title=… display=… home=…` — 창 제목 · 표시 이름 · 홈 이름 라벨이 `Trench` 이고 `Kerf` 가 없어야 ok. 변경 전 합성 모델 `title="Synthetic.3mx — Kerf 1.2.1" … FAIL`(EXIT 9).
- **무엇을:** 창 제목 3곳 · 표시 이름(`window-title … appended=0` 규칙 유지) · 정보 창 · 홈 이름 라벨 · 단면 PNG 바닥글 · SVG/PDF 제작자 · `app.rc`(회사 · 설명 · 제품 이름) · `읽어보기.txt` · README 머리 · 내보낸 파일 안의 설명 글(GeoTIFF citation · 단면 GeoTIFF 설명 · LAS 생성 소프트웨어 · 기본 citation = `Trench …`). 형식 식별자(`ExcavSection.sections` 단면 목록 · `ExcavSection section image`)는 파일 호환 때문에 그대로.
- **같이 고친 것(회귀):** 홈 이름 「Trench」가 「Kerf」보다 길어 1280 에서 이어서 작업 카드 칸이 줄임표 → `home-clipped` FAIL(합성 전부). 원인 = 부제 라벨의 QSS `border-left`+`padding-left` 가 sizeHint 에 여백을 더해(offscreen 에서 이름 240 px) 왼쪽 칸이 420 → 459. 해결 = 가름선을 라벨 테두리가 아니라 별도 `QFrame#homeSep`(1×24)으로, 부제를 `elideLab`(가로 Ignored · 줄임표 + 툴팁)로.
- **확인:** 합성 모델 감사 10조합(1280 · 1920 · 2560 · 3440 · 3840 × 배율 1 · 2) 모두 `RESULT ok` · `app-name` 통과 · 빠른 시험 51325 통과 · `--sheet-check` · `--icon-sheet` · `--undo-test` EXIT 0. 실제 모델(Production_1.3sm)은 `app-name` 통과, `RESULT fail` 은 기존 두 가지(① 좌표계 5183 알림 띠 → T4(G1)에서 없어짐 ② 홈 긴 폴더 · EPSG 칸 줄임 `home-clipped` → **T4 로도 안 없어짐**, 별도 결함으로 backlog R6b). 실제 화면 캡처 `C:\dev\tmp\elev-bug\t1-shots\`(Batang 에서 「Trench | 발굴 평·단면」 정상).
- **판정(Ruling):** ① 내부 식별자 유지(`SectionViewer.exe` · QSettings `ExcavSection/SectionViewer` · 캐시 경로) — 조직 · 앱 이름이 설정 위치와 3sm 변환 캐시(135 MB)를 정함, 바꾸면 최근 · 단면 목록이 사라져 보임 — 잘못이면: 폴더에 옛 이름이 보임. 캔버스의 「설정 이전」 문장 둘을 이에 맞게 고침. ② 버전은 1.2.1 그대로(릴리스 결정) — 캔버스의 「1.3」을 1.2.1 로 고침. ③ `app.rc` 의 `FILEVERSION` · 설치 파일 이름 · `app.manifest` 이름은 그대로.
- **남은 문제:** 여는 화면 · 홈 아이콘 · .ico 는 아직 옛 그림 → T2 · T3. 캔버스 「여는 화면」 화판의 `rench` 글자 크기가 게시본에서 15 px 로 바뀌어 있음(내 원본은 230 px — 캔버스 편집기에서 바뀐 것으로 보여 되돌리지 않고 사용자에게 알림).

### 2026-10-11 · Claude Code(Sonnet 5.5) — T2 (G0b): 앱 아이콘 「T는 트렌치의 단면」 — 한 SVG 에서 앱 · .ico 모두
- **시험 먼저(RED):** 새 옵션 `--app-icon DIR`(모델 · 창 없이 도는 판정, 실패 종료 12) — 16 · 24 · 32 · 48 · 256 PNG 를 쓰고 모서리 투명 · 지표 빨간 선 · T 모양 빈 자리(크림) · 흙 띠 색을 픽셀로 잰다. 그리기 전(빈 그림 stub)에는 `n=5 bad=5 … RESULT fail`(EXIT 12).
- **무엇을:** `app/icons/app/trench.svg`(> 32 px, 띠 넷 · 선 2.2) · `trench-small.svg`(17–32, 띠 둘 · 선 2.6) · `trench-tiny.svg`(≤ 16, 슬롯 8.5–15.5 · 선 3.2) — 명시 색, `clipPath` 없이 「크림 슬롯을 띠 위에 덮는」 방식(QtSvg 호환), `icons.cmake` 에 더함. `kerf::appPixmap(px, dpr)`(둥글기 변의 23 % 합성 마스크 + 안쪽 1 px 가는 테 — 크림 바탕에서도 타일 윤곽이 보임) · `kerf::appIcon()`(16 · 24 · 32 · 48 · 64 · 128 · 256 × 1 · 2) · `kerf::writeAppIcons`. 창 아이콘 · 정보 창 · 홈 56 px 타일이 이것을 씀(옛 손그림 흙색 타일 삭제). `packaging/assets/make_app_ico.py`(Pillow)가 앱이 그린 PNG 를 `app.ico`(5 크기) · `app_256.png` · `app_64.png` 로 묶음 — 도형을 파이썬에서 다시 그리지 않는다(각 ico 장이 앱 그림과 픽셀 동일함을 확인). 옛 `make_icons.py` 는 기록용 표시.
- **확인:** GREEN `app-icon n=5 bad=0 RESULT ok`(EXIT 0) · ico 5 크기 모두 앱 렌더와 픽셀 동일 · exe 에 실제로 박힘(`app.rc` touch 뒤 재빌드, 탐색기용 32 px 추출이 새 아이콘) · `--icon-sheet n=65 missing=0` · 합성 모델 감사 10조합 `RESULT ok`(마지막 변경 뒤 4조합 재확인) · 빠른 시험 51325 통과 · 홈 캡처 2560(Batang)에서 「T」 모양 확인(`C:\dev\tmp\elev-bug\t2-shots\`).
- **판정(Ruling):** ① 앱 아이콘 SVG 는 `icons.cmake` 목록(`kAll`)에 들어가 `--icon-sheet` 에도 보인다(65개) — 별도 목록을 만들 만큼의 이득이 없음. ② 설치 프로그램 그림(`header.bmp` · `wizard.bmp` · `app_1024.png`)은 옛 Kerf 그림 그대로 — 설치판은 사용자 요청 때만 만든다(R7.5). ③ 어두운 작업 표시줄에서의 윤곽은 크림 타일 자체가 충분히 밝아 따로 테를 두지 않음.
- **남은 문제:** 여는 화면은 아직 옛 그림 → T3.

### 2026-10-11 · Claude Code(Sonnet 5.5) — T3 (G0c): 여는 화면 「T를 판다」 1.0 초
- **시험 먼저(RED):** `--splash-shot` 판정을 「네 장(t = .12 · .40 · .70 · 1.0) · `kOpeningSplashMs == 1000`」으로 고치고 상수를 지금 길이 5000 으로 두어 `frames=4 total-ms=5000  FAIL`(EXIT 6) 확인 → 상수를 1000 으로 + 새 장면.
- **무엇을:** `MainWindow::kOpeningSplashMs = 1000`(한 곳 — `OpeningSplash::run` · `--splash-live` · 판정이 같은 값). `paintOpeningSplash` 다시 씀(캔버스 Splash 화판): 타일 160 px 떠오름 → 지표선 → 트렌치가 파 내려감(크림이 위에서 아래로) → 빨간 단면선이 벽면을 따라(길이 46 격자, 앞에서부터) → 「rench」(명조, 「h」 키 = 타일 높이, 기준선 = 타일 아래 모서리, 글꼴이 넓으면 오른쪽 여백 안으로 줄임) → 부제 · 상태 글(.30 「잠시 후…」 · .85 「준비 완료」) · 진행 막대 1 초 시계. 끝 장면(t ≥ .85)의 타일은 `kerf::appPixmap`(앱 아이콘)을 그대로 그림. `OpeningSplash`: Esc · 클릭 = 건너뜀(끝 장면 0.3 초 뒤 닫힘), Windows 「애니메이션 끔」(`SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION = 0x1042)` — Microsoft Learn 로 확인, windows.h 대신 함수 하나만 선언해 min/max 매크로 충돌 없음)이면 처음부터 끝 장면 0.3 초, `--splash-live` 는 120 · 400 · 700 · 1000 ms 에 창을 직접 찍음, `--splash-esc N`(Esc 건너뜀 시험).
- **확인:** GREEN `splash-shot … frames=4 total-ms=1000`(EXIT 0, 실제 창 플랫폼 — 글자 정상) · 네 장 눈 확인(`C:\dev\tmp\elev-bug\t3\sheet.png`: T 타일 + rench 가 Trench 로 읽힘) · `--splash-live` 실제 창이 1 002 ms 에 끝 장면, 프로세스 전체 1.1 s · `--splash-esc 100` 이 530 ms · `--splash-esc 400` 이 707 ms 에 닫힘(Esc + 300 ms 부근) · 파일로 열 때는 여는 화면 없음(코드 조건 그대로 `file.isEmpty()`) · 합성 감사 10조합 `RESULT ok` · 빠른 시험 51325 · `--sheet-check` · `--icon-sheet` · `--app-icon` · `--undo-test` EXIT 0.
- **판정(Ruling):** ① 키 · 클릭 입력은 앱 시험 장치로만 확인(Esc 는 `keyPressEvent` 직접 호출) — 실제 키보드 · 마우스는 사용자가 한 번 눌러 봐야 함(플랫폼이 SplashScreen 창에 초점을 주는지는 Windows 에서 처음 한 번 확인 필요). ② 진행 막대는 실제 준비와 무관한 1 초 시계 — 준비가 더 걸리면 열기가 늦어질 뿐 막대는 끝에 머묾(옛 5 초도 같았음). ③ 상수 `kOpeningSplashMs` 가 5000 → 1000 이라 옛 `--splash-shot` 의 t = .2 · .45 · .7 중간 장면 이름이 바뀜(`_t12 · _t40 · _t70`).
- **남은 문제:** 없음(T3 끝). 다음 T4(G1) 좌표계 — advisor 먼저.

### 2026-10-11 · Claude Code(Sonnet 5.5, advisor Opus 5.5) — T4 (G1 · R6): 좌표계 — 투영 값이 이름을 이긴다 · 상태줄 배지 「수평 EPSG:5187 · 동부원점(GRS80)」
- **원인(사용자 질문 「5187 · 5186 이라는 이상한 좌표계가 왜 나오나」):** Production_1.3sm 의 SRS 원문 이름은 「Korea 2000 / East Belt」(= EPSG:5183)인데 투영 값은 가산 북 600000 · 중앙자오선 129°(= East Belt **2010**, EPSG:5187). 옛 순서(WKT ID → REMARK → **이름** → 매개변수)가 이름만 보고 5183 으로 정한 뒤 「매개변수가 내장 값과 다르다」 경고 띠를 띄웠다.
- **시험 먼저(RED):** `tests/test_srs.cpp` 3개 — 실제 SRS 원문(좌표 없는 정의만)으로 ① 이름 5183 · 값 5187 → `horizontalEpsg == 5187` · `horizontalHow == "매개변수"` · `horizontalNameEpsg == 5183` · 경고 0 · 툴팁에 5183 · 5187 · `SrsInfo::epsg() == 5187` · 원점(200000, 600000)의 위경도 38° · 129°(1e-6) ② 거꾸로(이름 5187 · 가산 북 500000 → 5183) · 이름과 값이 같으면 이름 그대로 · 표의 어느 것과도 안 맞으면 이름 번호 + 경고 1 ③ `horizontalShortKo()`(5187 동부원점 · 5186 중부원점 · 5185 서부원점 · 32652 UTM 52N(WGS84) · 5174 번호만 · 빈 값). 구현 전 `SRS:*` 15 assertion 실패. 앱 쪽은 감사 줄 `status-badge … srs-static=… srs-text=…` 를 먼저 바꿔 `caret=1,1 srs-static=0 … FAIL`(EXIT 9) 확인.
- **무엇을:** core `readHorizontal` — 이름으로 번호를 찾은 경우에만(WKT ID · REMARK 로 정한 번호는 그대로) 값이 그 번호와 다르고 내장 표의 다른 번호와 같으면 값을 따른다(`SrsDesc::horizontalNameEpsg` 기록). 툴팁 한 문장 「이름은 EPSG:5183(…)이지만 투영 값(가산 동 · 북 · 중앙자오선)이 EPSG:5187 과 같아 값을 따랐습니다. 좌표는 값 그대로 계산합니다.」 새 `SrsDesc::horizontalShortKo()`("EPSG:5187 · 동부원점(GRS80)", 내장 표에 있는 번호만 원점 이름 · 타원체, 없으면 번호만). 앱: 상태줄 수평 배지는 **표시만**(누름 · ▾ · Tab 초점 없음 — 사용자 결정 「좌표계는 모델마다 자동 · 작게 표시만」), QSS `[static="true"]` 로 마우스를 올려도 안 바뀜, 도면 글 `sp.srsLabel` 도 같은 한 줄. 높이 배지는 T8 에서 지우므로 그대로(▾).
- **확인:** `SRS:*` 10 케이스 151 assertion 통과 · 전체 `asec_tests` 51371 중 실패 1 = 알려진 `CoalescingWorker: 끄는 동안의 미리보기…` 뿐(`~CoalescingWorker*` 51360 통과) · 합성 감사 1280×800 ×1 · 1920×1040 ×1 · 3840×2160 ×2 `RESULT ok`(`srs-text=수평 EPSG:5186 · 중부원점(GRS80)`) · **실제 모델 Production_1.3sm** 1920: 배지 `수평 EPSG:5187 · 동부원점(GRS80)` · `icons-missing=0`(알림 띠 사라짐) · `--sheet-check` · `--icon-sheet` · `--undo-test` EXIT 0 · 실제 창 캡처 `C:\dev\tmp\elev-bug\t4\status.png`(배지 글 · ▾ 없음 확인).
- **판정(Ruling):** ① REMARK · WKT ID 로 정한 번호는 건드리지 않음(명시 선언은 존중, 이름만 약한 단서) — 잘못이면: ID 와 값이 다른 파일은 경고 띠가 계속 뜸. ② 경고(알림 띠)는 지우지 않음 — 표의 어느 것과도 안 맞는 진짜 불일치 · 지리 좌표계는 계속 알려야 함. 이 모델의 띠만 사라짐. ③ 원점 이름은 내장 표에 있는 번호만(5174 등은 번호만) — 이름으로 번호를 짐작하지 않음. ④ 높이 배지 · 지정 창 삭제는 T8(G5) — 이번에는 수평 배지만.
- **남은 문제:** 실제 모델 감사의 `home-clipped`(1280 에서 홈 표의 긴 폴더 「C: › Users › 권을」 · 「EPSG:5187」 칸 줄임)는 T4 와 무관한 기존 결함 → backlog R6b(T8 뒤 다시 재서 고침). T5(G2) 다음.

### 2026-10-11 · Claude Code(Sonnet 5.5) — T5 (G2 · R3): 좌표 입력 · 단면 찾기 · 영어 병기 삭제
- **사용자 결정(2026-10-10 확인 ③):** 좌표 입력(리본 오른쪽 끝 · 오른쪽 판 「좌표 입력… Enter」 · 그리는 중 Enter) · 단면 찾기(칸 · Ctrl+F) · 영어 병기를 지운다.
- **시험 먼저(RED):** `--ui-audit` 새 줄 `removed ribbon-corner=%1 coord-entry=%2 find-box=%3 bilingual=%4`(`ribbonCorner` 단추 수 · 동작 `coord` + 「좌표 입력」 글자 단추 · `ribbonSearch` 칸 + `QKeySequence::Find` 단축키 · 동작 `lang` — 모두 0 이어야 ok). 지우기 전 `ribbon-corner=1 coord-entry=3 find-box=2 bilingual=1  FAIL`(EXIT 9).
- **무엇을:** 리본 오른쪽 끝 묶음(`setCorner` · 좌표 입력 단추 · 단면 찾기 칸) · Ctrl+F · `filterSectionList` · 홈/모델 닫기의 찾기 칸 비움 · 오른쪽 판 「좌표 입력…」 단추 · 동작 `coord`(Return) · `coordEntryDialog` · `dlgCoordEntry` · `PlanView::finishDrawAt` · `--coord-shot` · 동작 `lang` · `bilingual_` · `retranslate` · `labels_`(동작에는 아무 일도 안 하던 장치) · `Ico::Lang` · QSS `ribbonCorner` · `ribbonSearch` · F1 키 표의 「Enter (그리는 중) A/A′ 좌표 직접 입력」 · README · 읽어보기의 영어 병기 문장. 오른쪽 판의 「복사」 단추는 남김(T7 에서 판째 없앰).
- **같이 바뀐 것(결과):** 오른쪽 끝 270 px 가 비어 리본 폭 예산이 need50 1916 → 1644 · need32 1682 → 1410 · need20 1280 → 1008 이 되었고, 1280 에서 창 폭에 들어가는 가장 큰 크기가 20 → **24** 가 됐다(`narrow rule-at-1280 look=24`). 감사 기대 `kNarrowTile` 와 스펙 §2.1 · §3.3 을 실측값으로 고침(2026-10-09 사용자 결정 「스펙을 실제로 고친다」와 같은 방식).
- **같이 고친 것(칩):** 그리는 중 A′ 단계 안내 칩의 「Enter」 키 캡과 「숫자 = 길이」 문장은 지운 좌표 입력을 가리키고 실제로는 아무 일도 안 해(길이를 치는 처리 없음) 함께 지움 — 감사 `guide-step2 keys=Esc`(고치기 전 `keys=Enter+Esc … FAIL`), 스펙 §5 문장 정정. v6 의 「길이 숫자 입력 칸」(`DESIGN_SPEC_v6.md` §…)은 별도 계획 기능이라 건드리지 않음.
- **확인:** 합성 감사 1280 · 1920 · 2560 · 3440 · 3840 × 배율 1 · 2 열 가지 모두 `RESULT ok`(칩 고친 뒤 1920 ×1 · 1280 ×1 · 3840 ×2 재확인) · 실제 모델 Production_1.3sm 1920: `removed … 0` · `guide-step2=1 keys=Esc` · `icons-missing=0`, `RESULT fail` 은 기존 `home-clipped` 하나뿐(backlog R6b) · `~CoalescingWorker*` 51360 통과 · `--sheet-check` · `--icon-sheet` · `--undo-test` · `--ctx-shot` EXIT 0 · 실제 창 캡처 `C:\dev\tmp\elev-bug\t5\`(work · draw · start, 리본 오른쪽 끝 비어 있음 · 칩 「Shift = 축 맞춤 [Esc]」).
- **판정(Ruling):** ① 1280 리본 타일 20 → 24 는 사용자 요청(삭제)의 필연적 결과 — 창 폭에 맞는 가장 큰 크기를 고르는 규칙은 그대로, 틀리면: 1280 타일이 기대보다 큼(읽기 쉬워짐). ② 정밀 좌표로 단면선을 만드는 길이 이제 없다(평면에서 클릭만) — 사용자 결정 그대로, 필요해지면 새 요청. ③ 동작 이름표 영어(`en`)는 툴팁 괄호에만 남김(영어 병기 토글이 아니라 툴팁 안의 표기).

### 2026-10-11 · Claude Code(Sonnet 5.5) — T6 (G3 · R4 나머지): 리본 정리 — 묶음 6 · 더보기 삭제 · 평활 → 입면 아래 줄 · 점군 하나로
- **사용자 결정(2026-10-10 확인 ③):** 평활은 입면 아래 줄(선 굵기 옆 켜기 · 끄기), 정보는 F1 창 아래 한 줄, 끝내기는 Ctrl+Q · 창 닫기만, 더보기 삭제. 캔버스 G3: 기타 → 보기 합침 · 점군 하나로.
- **시험 먼저(RED):** `--ui-audit` 줄 `ribbon … groups=6 more=0`(실제 묶음 수 · 「더보기」「기타」 칩 · 동작 `more`) · `elev-row2 … smooth=1 menu-chip=…` · `ribbon-tidy points-chips=1 xyz-las-chips=0 keys-in-view=1 quit-ctrl-q=1 about-in-keys=1`. 고치기 전 `groups=7 more=2 … FAIL` · `smooth=0 menu-chip=0  FAIL` · `points-chips=0 xyz-las-chips=2 keys-in-view=0 … about-in-keys=0  FAIL`(EXIT 9).
- **무엇을:** 묶음 「기타」 삭제(단축키 칩은 「보기」 맨 끝) · 동작 `more` · 더보기 메뉴 삭제(정보 · 끝내기 항목도) · 입면 아래 줄에 「평활」 켜기 · 끄기 칩(같은 동작 `smooth`) · 줄 이름표를 사용자 말대로 「불투명도」 · 「선 굵기」로 줄임 · 새 칩 **「입면 ▾」**(`Ico::Sliders` = Lucide `sliders-horizontal`, 새 SVG)는 글자 숨김 크기에서만 보이고 같은 셋(불투명도 · 선 굵기 · 평활)을 메뉴로 연다 — `Ribbon::lookWidths` 가 `narrowOnly` 칩을 보이는 크기에서만 센다 · 「점군 ▾」 칩 하나(메뉴: 점군 XYZ · 점군 LAS, 동작 `points`) · F1 창(`keysDialog()` 로 분리 — 자동화 · 감사도 씀) 맨 아래 「Trench 1.2.1 · 프로그램 정보」 줄 + 표에 「Ctrl+Q 끝내기」 행.
- **같이 바뀐 것(결과):** 리본 폭 예산 need50 1644 → **1521** · need32 1410 → **1296** · need20 1008 → 973. 1280 에서 창 폭에 들어가는 가장 큰 크기 24 → **32(글자 숨김)**. 글자 보이는 32 는 1296 이라 1280 에 16 px 모자람. 감사 `narrow rule-at-1280` 은 상수 대신 규칙(창 폭에 들어가는 가장 큰 크기) + 바닥 24(`kNarrowFloor`)로 판정하고 값은 출력에 적는다. 스펙 §2.1 · §3.1 · §12 · §3.3 정정.
- **확인:** 합성 감사 1280 · 1920 · 2560 · 3440 · 3840 × 배율 1 · 2 열 가지 모두 `RESULT ok` + 1920 ×1.25 · ×1.75 ok · 실제 모델 Production_1.3sm 1920 은 기존 `home-clipped` 하나뿐(R6b) · `~CoalescingWorker*` 51360 통과 · `--sheet-check` · `--icon-sheet`(새 SVG 포함) · `--undo-test` · `--ctx-shot` EXIT 0 · 캡처 `C:\dev\tmp\elev-bug\t6\`(ribbon 1920: 묶음 여섯 · 입면 아래 줄 「불투명도 ▬ 선 굵기 1.5 2 3 평활」 · 점군 ▾) · 감사 narrow 캡처에서 「입면 ▾」 칩(아이콘 + ▾) 확인.
- **판정(Ruling):** ① 줄 이름표를 「영상 불투명도」 · 「단면선 굵기」에서 「불투명도」 · 「선 굵기」로 줄임(사용자 말 「선굵기」) — 아래 줄이 위 줄(320)보다 넓어지면 줄을 보이고 숨길 때 리본 폭 계산이 달라져 크기가 오락가락(첫 구현에서 336 → 1296/1312 갈림 확인) — 틀리면: 이름표가 짧아 처음엔 덜 설명적(툴팁에 전체 이름). ② 「입면 ▾」 칩은 캔버스 「입면 아래 줄 → 입면 ▾ 안」 그대로 — 더보기 삭제로 좁은 창에서 불투명도 · 굵기 · 평활에 닿는 유일한 길. ③ 끝내기는 Ctrl+Q 와 창 닫기뿐(사용자 결정) — 틀리면: 마우스만 쓰는 사람이 닫기 단추로만 끝냄.
- **남은 문제:** 없음(T6 끝). 다음 T7(G4) 오른쪽 판 삭제 → 목록 아래 읽기 전용 수치.

### 2026-10-11 · Claude Code(Sonnet 5.5) — T7 (G4 · R5): 오른쪽 판 삭제 → 단면 목록 아래 읽기 전용 수치
- **사용자 결정(2026-10-10):** 「선택한 단면의 이 표시들도 단면 목록 밑에 확인용도로만 표시」 — 오른쪽 판(272)을 없애고 수치는 목록 아래에 읽기 전용으로. (옛 계획 D2 「오른쪽 판 §4 모양」 · 대기 #3 은 이 작업으로 대체)
- **시험 먼저(RED):** `--ui-audit` 줄 `inspector=0 list-readout rows≥9 read-only=1 under-list=1`(판 개체 `inspector` 수 · 목록 아래 칸 `listReadout` 안의 보이는 값 줄 수 · 단추 · 입력 칸 0 · 판 안에 있음) · `drawing-readout=1`(그리는 중 A′ 단계에 길이 「7.43 m」 · 방위 · A · 커서 줄). 고치기 전 `inspector=1 list-readout rows=0 read-only=0 under-list=0  FAIL` · `drawing-readout=0  FAIL`(EXIT 9).
- **무엇을:** `buildInspector` · 오른쪽 판 · 「복사」 단추 삭제. 단면 목록 판(348) 목록 아래에 `listReadout`: 머리 「선택한 단면 · 1 / 3」(11) · 이름 명조 20 + 보는 쪽(메모가 있으면 앞에) · 2열 줄(이름 12 · 값 모노 12 오른쪽 정렬): 길이 · 깊이(툴팁 「지표 → 바닥」) · 방위 · (그리는 중) 보는 쪽 · 앞 / 뒤 · 지표 EL. · 바닥 EL. · 잘린 선 n줄 · m점 · A · A′(그리는 중에는 「커서」) · 수평 EPSG:5187 · 동부원점(GRS80) · 맨 아래 「계산한 값이라 여기서 고칠 수 없습니다…」. 값이 없는 줄은 숨김. 좌표는 천 단위 띄어 쓰기 + X · Y 두 줄(캔버스 Work 화판). `updateHeader` 끝이 `updateReadout()`, `updateCtx` 가 `readoutDrawing()` 을 부름. 분할 `{348, 520, 520}`(평면 · 단면이 272 + 분할선만큼 넓어짐), 창 최소 폭 1263 → **1096**.
- **확인:** 합성 감사 1280 · 1920 · 2560 · 3440 · 3840 × 배율 1 · 2 열 가지 `RESULT ok` + 1920 ×1.25 · ×1.75 ok · 실제 모델 Production_1.3sm 은 기존 `home-clipped` 하나뿐(R6b) · `~CoalescingWorker*` 51360 통과 · `--sheet-check` · `--icon-sheet` · `--undo-test` · `--ctx-shot` · `--sheet-batch` EXIT 0 · 실제 창 캡처 `C:\dev\tmp\elev-bug\t7\`(work: 목록 아래 수치 열 줄 · 오른쪽 판 없이 평면 · 단면이 넓어짐 / draw: 「새 단면 · 그리는 중」 + 길이 · 방위 · 보는 쪽 · 앞 / 뒤 · A · 커서 · 수평).
- **판정(Ruling):** ① 「복사」 단추를 지움(읽기 전용 · 값은 마우스로 끌어 선택해 복사) — 사용자 결정 「확인용도로만」 — 틀리면: 한 번에 복사하는 단추를 다시 만들 수 있음. ② 좌표 구분은 캔버스(띄어 쓰기)를 따름 — 스펙 v5 §4 의 쉼표 안과 다름, 틀리면: 구분 글자만 바꿈. ③ 이름 옆 보조 글이 긴 경우(그리는 중 안내)는 두 줄로 접힘 — 목록 폭 348 고정이라 한 줄에 못 넣음.
- **남은 문제:** 없음(T7 끝). 다음 T8(G5) 높이 UI 삭제(advisor 먼저 — 표고 숫자 불변).

### 2026-10-11 · Claude Code(Sonnet 5.5, advisor Opus 5.5) — T8 (G5 · R7): 높이 UI 삭제 — 표고 숫자는 byte 그대로
- **사용자 결정(2026-10-10):** 「높이 기준은 여기서 지정할 필요가 없다 — iTwin Capture 에서 이미 높이 보정이 되어 들어온다」 · 확인 답 「앱 · 도면 어디에도 안 씀(상태줄 높이 배지 · 지정 창 · 도면 범례 높이 줄 삭제, 표고 숫자는 파일 그대로)」.
- **기준선 먼저(advisor):** 고치기 전 현재 빌드로 합성 · 실제 모델의 내보내기를 떠 둠(`C:\dev\tmp\t8b-A` · 반복 `t8b-B` 로 결정성 확인 — 같은 입력이면 PDF 시각 말고는 byte 같음 · 높이 지정 `egm96` 을 저장된 설정으로 심은 `t8b-seeded`). 심은 지정값은 옛 빌드에서 plan.tif · points.las · section.dxf · section.json · section.svg · section.tif · sheet.dxf · sheet.png · 조판 SVG 셋을 **조용히 바꿈**(저장된 지정값이 열 때 자동 적용되던 길) — 이 길을 막는 것이 이번 작업의 핵심.
- **시험 먼저(RED):** `--ui-audit height-ui=N status-badges=N srs-plain=N`(높이 동작 · 높이 글 단추 · 라벨 · 홈 표 머리 · F1 표의 「높이 기준」 합계가 0, 상태줄에 테두리 단추 0, 수평 좌표계는 단추가 아닌 글) · `--sheet-check height-lines=N`(조판 SVG 의 표제란 · 범례에서 「높이」 글) · core Catch2: 높이 안내는 `heightWarnings` 로 따로(`warnings` 에 안 섞임) · `horizontalTooltipKo()` · `horizontalLabelKo()`. 고치기 전 `height-ui=5 status-badges=2 srs-plain=0  FAIL` · `height-lines=2` · Catch2 10 assertion 실패.
- **무엇을(core):** `SrsDesc::heightWarnings` · `SrsReport::heightWarnings`(수직 기준 모름 · metadata 높이 불일치 · iTwin 3D 승격 안내를 `warnings` 에서 뗌 — 앱은 이것을 알림 띠에 올리지 않는다) · `SrsDesc::horizontalTooltipKo()`(높이 줄 · 「높이 기준 지정」 안내 없음) · `SrsReport::horizontalTooltipKo` · `SrsDesc::horizontalLabelKo()`(「수평 EPSG:5187 · 동부원점(GRS80)」 / 「좌표계 없음(로컬 좌표)」 …). `asec-info` 는 높이 안내를 「경고(높이)」 줄로 계속 출력. core 의 `VDatum` · `applyHeightDeclaration` 은 남김(MDL 재사용 · 시험).
- **무엇을(앱):** 상태줄 높이 배지 · 숨겨 둔 단면 머리 높이 배지 · 동작 `height` · 「높이 기준 지정」 창(`dlgHeightDatum` · `heightDatumDialog` · `setHeightDeclaration`) · 알림 띠의 「높이 기준 지정…」 단추 · 되돌리기의 `heightDecl` · `--height-datum` · `--datum-shot` · 열 때 저장된 모델별 / 마지막 지정값 읽기 · 열림 상태 문장의 높이 안내 · Z 칸 툴팁의 높이 줄 · 홈 카드 「높이」 칸(칸 6 → 5) · 최근 표 「높이」 열(6 → 5) · 홈 메타 `height`/`heightState` 쓰기 · F1 표의 「높이 기준」 글 · 도면: 범례 높이 줄(줄 4 → 3) · 표제란의 높이 글 · PDF/SVG 제목의 높이 글 · 조판 결과 문장 · 도면 점검의 높이 항목과 「지정…」 링크 · 판 툴팁 · 단면 화면 / 내보내기 머리 글의 「/ 높이 …」(수평만). 수평 좌표계는 테두리 없는 작은 글 `#statusSrs`(캔버스 Work 화판 그대로 — 상태줄 배지 0). 설정에 예전에 저장된 값은 지우지 않고 읽지만 않음.
- **확인:** 합성 감사 1280 · 1920 · 2560 · 3440 · 3840 × 배율 1 · 2 열 가지 + 1920 ×1.25 · ×1.75 + **실제 모델 Production_1.3sm `RESULT ok`**(그동안 남던 `home-clipped` 도 높이 칸이 빠져 폭이 남으면서 사라짐 — 감사 R6b 닫음) · 전체 `asec_tests` · `--sheet-check height-lines=0` · `--icon-sheet` · `--undo-test` · `--ctx-shot` EXIT 0. **내보내기 byte 비교:** 고친 뒤(`t8a-after`)와 심은 지정값 있는 고친 뒤(`t8a-seeded`)가 모든 파일에서 **같음**(저장된 지정값은 이제 아무 영향 없음). 기준선(A)과 비교: real/points.las · .xyz, synth/plan.tif · plan.tfw · points.las · points.xyz · profile.csv · section.dxf · section.json · section.tfw · section_image.png · sheet.dxf · sheet_image.png **byte 같음**. 달라진 것은 높이 글이 빠진 그림뿐 — 조판 SVG 셋 · section.svg · sheet.png · section.tif(GeoTIFF 태그는 줄 오프셋 · 바이트 수 말고 같고, 화소 차이는 머리 글 줄 하나 `(4165, 40)–(5097, 72)` 안).
- **판정(Ruling):** ① core 높이 해석 · `applyHeightDeclaration` 은 지우지 않음(MDL 재사용 · 시험 · asec-info) — 앱에서만 쓰지 않는다. ② 저장된 높이 지정값은 지우지 않고 읽지 않음(「사용자 설정 값은 지우지 않음」) — 틀리면: 예전에 지정해 둔 사람의 내보내기 파일 높이 이름표가 바뀜(값은 그대로). ③ 수평 좌표계는 배지가 아니라 글 — 캔버스 Work 화판의 상태줄이 글이라서(advisor 지적) — 틀리면 테두리 배지로 되돌림(QSS 만). ④ 높이 안내를 core 에서 `heightWarnings` 로 뗌(앱이 높이를 다루지 않아 알림 띠에 올리면 고칠 길 없는 안내가 되므로). ⑤ `Ico::Height` · `kerf/height.svg` 는 안 쓰이지만 T13(제거 목록)에서 한꺼번에 지움.
- **남은 문제:** 없음(T8 끝). 다음 T9(G6) F1 창 고침 · 표 = 실제 키.

### 2026-10-11 · Claude Code(Sonnet 5.5) — T9 (G6 · R8): F1 창 고침 — 어두운 바탕에 어두운 글 · 표 = 실제 키
- **사용자 보고(2026-10-10 캡처):** F1 단축키 창 본문이 어두워 글이 안 보임. 확인 ③ 「프로그램 정보는 F1 창 아래 한 줄」(T6 에서 줄은 이미 넣음).
- **원인:** 이 PC 는 윈도우 「앱 어두운 모드」(`AppsUseLightTheme = 0`) — Qt 가 시스템 어두운 팔레트를 따라 QSS 가 없는 표 · 창이 어두운 바탕이 되고, 앱 글자색(어두운 색)이 그 위에 올라가 읽히지 않았다.
- **시험 먼저(RED):** `--ui-audit keys-table-match=%1 rows two-col contrast color-scheme missing=[…] extra=[…]` — 표의 키 집합과 실제 연결된 키 집합(동작 단축키 · `QShortcut`, 아직 안 켜진 H · O 도구 제외)이 같아야 하고 · 두 열 · 표 그림에서 바탕 / 글 대비 ≥ 4.5 · 색 체계 Light. 표에 F2 · Del 을 적자 `extra=[Del,F2]  FAIL`(EXIT 9), 색 체계 `not-light`.
- **무엇을:** `keyRows()` 한 곳(키 글 · 하는 일 · 실제 키들)에서 F1 창 표를 만들고(23줄, 두 열 4칸) 감사가 같은 표를 읽음 · 표 QSS `#keysTable`(흰 바탕 · 잉크 글 · 둥글기 8) · `main()` 에서 `setColorScheme(Light)`(Qt 6.8 에서 생긴 API — 문서로 확인, CI 6.8.3 · 이 PC 6.10.3 모두 가능) · 단면 목록 메뉴에만 적혀 있던 **F2(이름 · 메모) · Del(지우기)** 를 작업 영역 단축키로 연결 · 「닫기 Esc」(영어 「Close」 대신) · 스크롤바 없음.
- **확인:** 합성 감사 10조합 + 1920 ×1.25 · ×1.75 + 실제 모델 `RESULT ok` · **실제 Windows 창 플랫폼에서도 `RESULT ok`**(`color-scheme=light` — 시스템은 어두운 모드인데 앱은 밝게, `contrast=18.4`) · 실제 창 캡처 `C:\dev\tmp\ui__keys.png`(두 열 표 · 글 읽힘 · 정보 줄 · 닫기 Esc) 눈 확인 · `~CoalescingWorker*` 통과 · `--sheet-check` · `--icon-sheet` · `--undo-test` EXIT 0.
- **판정(Ruling):** ① 앱 전체를 밝은 색 체계로 고정(어두운 모드 지원 없음 — 스펙에 어두운 테마 없음, `ui-visual-check` 도 `--themes light` 만) — 틀리면: 어두운 모드를 쓰는 사람도 밝은 앱을 봄. ② F2 · Del 은 「고른 단면」에 걸었고 단면 목록에 초점이 없어도 작업 영역 안이면 먹음(목록이 Tab 초점만 받아 클릭 뒤에는 초점이 안 가므로) — Del 은 확인 창을 거침. ③ Ctrl+Tab(탭 순환) · D(거리 눈금)는 아직 없는 기능이라 표에 안 적음(D 는 T10 에서 걸며 표에 더함).
- **남은 문제:** 없음(T9 끝). 다음 T10(G7) 입면 위 거리 눈금(세로) + D 키 + 도면 SVG.

## 5. 다음 할 일 (위에서부터)
계획 `docs/superpowers/plans/2026-10-09-design-v5.md` 순서대로, 장부 `.superpowers/sdd/2026-10-09-design-v5/progress.md` 에 과제마다 complete · Ruling. 규칙은 `docs/DEV_RULES.md`.
- **사용자가 직접 확인할 것(E4–E7, 1분 — 화면 조작 도구가 Kerf 창을 못 잡아 미실행):** 바탕화면 Kerf 로 합성 모델이나 실제 모델을 열고 ① 단면선 긋기(S) 중 평면 왼쪽 위 떠 있는 되돌리기 · 다시를 눌러 반응하는지 ② F11 로 전체화면 ↔ 원래 크기 ③ Ctrl+P 로 도면 탭을 연 뒤 Esc 로 무엇이 일어나는지 ④ 단면 머리 축척 칸에 「1:30」 을 쳐 Enter 하면 축척이 바뀌는지. 안 되는 것만 알려 주면 대기 목록에 넣는다.
0. **(2026-10-10) 최종 점검 끝 · 사용자 결정 Q0–Q6 받음(모두 추천안, Q0 = 「모델마다 다르다」) → 정본 대기 목록 R0–R23(`factory/backlog.md` 맨 위 표).** 순서: R1(#3c) → R0(MSVC) → 최종 점검 worktree E4–E7 → R2 기준 화면 → R3–R9 화면 정리 · 좌표계 · 높이 · F1 · 눈금 → R10 제거 25 → R11–R15 도면(진북 화살 · DXF · 층 · 범례 · 표제란) → R16–R17 H · O → R18 저장 실패 안내 → R19–R20 기준 화면 다듬기 · 첫 창 → R21–R22 도구 · CI → R23 문서. (아래는 결정 전 기록)
   **(결정 전) 최종 점검 → 사용자 결정 대기.** 사용자 지시 「워크트리를 만든후 최종 점검과 안되는것을 확인해야한다 … 모든정리가 필요하다」 → `docs/REVIEW_2026-10-10.md`(의도 79 · 기능 157 · 도면 관례 44 · 실행 점검 E1–E14 · 제거 후보 28 · 대기 목록 R0–R23 · 질문 Q0–Q6). 같은 날 사용자 결정(좌표계 5186/5187 자동 · 표시만, 높이 지정 삭제, 좌표 입력 · 단면 찾기 · 더보기 · 영어 병기 삭제, 불투명도 · 선 굵기 · 평활을 입면 아래로, 오른쪽 판 → 목록 아래 읽기 전용, F1 창 고침, 입면 거리 눈금, 기준 모니터 TFG39Q14V 2560×1440)은 `factory/backlog.md` 2026-10-10 절. 순서: #3c 마무리 · 병합 → **R0 MSVC 빌드 고침(결정과 무관)** → 최종 점검 worktree 에서 E4–E7 → Q0–Q6 답대로 R1– 재개.
1. ~~E3 · E4 · B5~~ 끝(B5 = 팩토리 `20261009-01-b5-dpi`, 병합 `8cc67fb`). #3c(1280 좁은 창 — 단면 머리 · 정보 줄 · 눈금 · 리본 기대값 20 + B5 검토 should 7건 + 홈 겹침 버그 AC14) 진행 중(rework 2/2).
   - **B5 는 팩토리 작업 `20261009-01-b5-dpi` 로 진행 중**(사용자 지시 2026-10-09 「지금 개발설정을 지켜가며 개발하라. 니맘대로하지말고」 + 선택 「B5 부터 팩토리로」 · 「CI 의 Linux 작업 삭제」). 명세 `factory/jobs/20261009-01-b5-dpi/spec.md`(AC1–AC11), worktree `C:\dev\kerf-wt\20261009-01-b5-dpi`, 브랜치 `factory/20261009-01-b5-dpi`(기준 `0f7fbfe`). 본 체크아웃에 있던 B5 RED 감사 코드는 `C:\dev\tmp\b5-red.patch` 로 빼고 `app/` 은 HEAD 로 되돌렸다 — **이 작업이 끝날 때까지 본 체크아웃의 `app/` 을 고치지 않는다.** 기준선(고치기 전) 감사: 8가지 모두 ok, 단 `--size 1280x800` 이 1591×800 으로 열림(새 판정으로 잡음). 사용자 지시 둘을 규칙으로: DEV_RULES R7.4(Windows 만) · R7.5(포터블 구조 유지, 전환은 요청 때만). 열린 질문 Q1(첫 창 1600×950 > 모니터)은 backlog #2. 병합 뒤 본 체크아웃에서 고칠 문서: `CLAUDE.md` CI 문장 · DEV_RULES R2.7 「dpr 1 · 1.5 · 2 · 3」 → 여섯 장 · `factory/project-profile.md` §3 Linux 줄 · SKILL §5-6. 다른 세션 미커밋: `CLAUDE.md` · `.claude/cpp-project.json` · `.claude/skills/verify/SKILL.md`(팩토리).
2. **남은 단계는 모두 팩토리 작업으로**(사용자 /goal 2026-10-09 「남은 개발작업을 플랜을 나눠서 /factory 개발을 진행한다면?」) — 나눈 목록과 순서는 `factory/backlog.md` #3–#10: #3 D2 + D3(오른쪽 판 · 목록 줄) → #4 F1 + F2(잘린 돌 core + H 도구) → #5 F3(빗금 SVG · DXF 층) → #6 G1(윤곽 O 도구) → #7 G2(윤곽 층) → #8 H1 + H2(EPSG 5187 · 높이 의심) → #9 I1(문서 · 정리, 사용자가 본 저장소 문서를 커밋한 뒤) · #2(첫 창 크기) · #10(v4 잔여 13 · 15 · 16 · 16a · A2 — 스펙 대조 뒤 사용자 결정). 동시 1개, 작업마다 승인 보고서 → 사용자 승인 → 「병합」 말 뒤 병합. 팩토리 검토(검토자 넷)가 R6.1 의 cpp · ui 검토를 대신한다 — 2차 지적 중 미룬 것(장부 `Task R2` Ruling)은 #3 명세에 넣는다.
2a. **디자인 v6(2026-10-10 사용자 「전체 채택」)** — 스펙 `docs/design/DESIGN_SPEC_v6.md`, 계획 `docs/superpowers/plans/2026-10-10-design-v6.md`(V1–V12). **사용자 확인 받음(2026-10-10 「추천 순서로 진행」) → 대기 목록 #11–#22 로 넣음.** 전체 순서: #3c → #8(좌표계 지정) → #11–#15(V1–V5) → #16–#21(V6–V11, 스펙 본문 먼저) → #4 · #4b · #5 · #6 · #7(잘린 돌 · 윤곽) → #3b → #2 → #22(V12 + I1). 충돌 해소: 1280 타일 20 유지 · 높이 의심 자동 판단 없음(「그것만 하라」) · E 키 · 빈 구간 안내는 v6 대로. 상세 `factory/backlog.md` 「진행 규칙」. v6 조사 기록 `C:\dev\tmp\v6-inventory.md`(이 PC).
3. 각 작업 병합 뒤 이 파일 §1 · §3 · §4 · §5 를 본 저장소에서 고친다(R4.1). push 는 사용자 지시 때(명령 §4 「푸시 해결」).

## 6. 기록 규칙
- 단계를 마치면: §1 표(마지막 갱신 · 바뀐 파일 · 빌드 · 시험), §3 상태, §4에 새 기록(무엇을 · 확인 명령과 결과 · 판정 · 남은 문제), §5 다음 할 일.
- 「끝(✅)」은 빌드 성공 + 시험 결과 + 캡처나 기록 줄이 있을 때만.
- 지시서와 다르게 한 것은 반드시 「판정」으로 적는다(무엇을 · 왜 · 잘못이면 무엇을 잃는지).
- 커밋을 허락받으면 단계마다 커밋하고, 커밋 해시를 §4 기록에 적는다.
