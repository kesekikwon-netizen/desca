# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 먼저 할 일 (모든 세션 · 모든 계정)
- **개발 규칙은 [`docs/DEV_RULES.md`](docs/DEV_RULES.md)(절대 준수, 사용자 지시 2026-10-09)가 정본이다.** 코드를 바꾸기 전에 읽고, 과제마다 그 체크리스트를 장부에 채운다. 다른 문서와 어긋나면 그 문서가 이긴다.
- 지금 진행 중인 개발은 **Kerf 디자인 v5**다. 작업을 시작하기 전에 `docs/DEV_PROGRESS.md`를 끝까지 읽고 「다음 할 일」부터 이어서 한다.
- **단계 하나를 마칠 때마다 `docs/DEV_PROGRESS.md`를 고친다**(상태 표 · 기록 · 다음 할 일). 기록 없이 다음 단계로 가지 않는다.
- 화면 기준은 `docs/design/DESIGN_SPEC.md`(v5, 화판 `docs/design/boards/kerf-design-v5.html`). v4 `docs/design-v4/FINAL_PLAN.md` · `Buttons1/2.dc.html`은 v5에 없는 내용만 참고한다(어긋나면 v5).
- 끝났다고 말하기 전 · 커밋 전에 `/verify`(`desca:verify`, `.claude/skills/verify`)를 돌리고 그 결과를 보고한다.
- **개발 방식(사용자 지시 2026-10-09, DEV_RULES R1):** superpowers 스킬을 Skill 도구로 실제로 부른 뒤 한다 — 새 기능 `brainstorming → writing-plans → executing-plans`, 코드 수정 `test-driven-development`, 버그 `systematic-debugging`, 끝나기 전 `verification-before-completion`, 기능 하나 끝나면 `requesting-code-review`(cpp-reviewer · UI는 ui-reviewer), 브랜치 마무리 `finishing-a-development-branch`. Qt · C++ API가 확실치 않으면 **context7**(`resolve-library-id` → `query-docs`, Qt는 `/websites/doc_qt_io_qt-6_8`)로 확인한 뒤 쓴다 — 지어내지 않는다. 목적에 맞는 다른 skills · MCP · hooks(ui-visual-check · cpp-build-fix · /verify …)는 사용자 요청 없이도 쓴다.
- **해상도 · DPI(사용자 지시 2026-10-09):** 4K · 와이드 · 100–200 % 배율 어디서나 안정이어야 한다. 치수는 논리 픽셀, 아이콘은 dpr별로 굽는다. `--ui-audit`는 화면 변경마다 1920×1040, 단계 끝(B5 뒤로는 과제마다)에 1280×800 · 3440×1440 · 3840×2160까지 × `QT_SCALE_FACTOR` 1 · 2 모두 ok(DEV_RULES R3.3, 계획 Task B5).

@docs/DEV_PROGRESS.md

## 이 저장소의 규칙 (요약 — 자세한 것은 docs/DEV_RULES.md)
- C++17 · Qt 6 Widgets. `Q_OBJECT` 금지(moc 없이 빌드 — MinGW 교차 빌드 전제). 연결은 람다, `qobject_cast` 대신 `static_cast`.
- 계산은 Qt 없이 `core/`에, 바꾸면 `tests/`에 먼저 실패하는 시험. `third_party/` 수정 금지.
- 색 · 크기 · 글꼴은 `app/theme.hpp` 토큰만. 새 색은 Strata `tokens.json`(경로는 `docs/design/DESIGN_SPEC.md` 머리)에 있는 값만 토큰으로 더한다. 단면선 `#FF0000`, 흙색 `#B5573A`는 한 화면에 주 단추 하나.
- 높이는 파일 값 그대로(지오이드 재적용 없음). 평면도 그림 칸에 단면선 없음. 조판은 탭(별도 창 아님).
- 커밋은 DEV_RULES R5 범위만(지금은 `feature/design-v2`에 단계마다). **푸시 · 병합 · main 변경 · 배포는 사용자가 말할 때만.** 인덱스에 줄바꿈만 바뀐 파일이 대기 중이면 커밋하지 말고 먼저 알린다. `stash@{0}`(main 쪽 개발 설정 보관)는 건드리지 않는다.
- 사용자는 비개발자다. 한국어로, 실행해서 본 것만 보고한다.

## 명령 (이 PC: Windows 11 — 데스크톱 앱 세션 · 터미널 `claude` 모두 Windows, WSL 아님)
Bash 도구는 Git Bash, PowerShell 도구도 있다. cmake · ninja · clang-cl(LLVM)이 PATH에 있어 그대로 부른다(TEMP · PATH를 바꾸지 않은 cmd.exe에서도 빌드 성공, 2026-10-09 확인). 경로는 `C:/dev/...`처럼 슬래시로 쓰면 두 셸 모두에서 된다. 빌드 폴더 `C:\dev\kerf-v4c`는 정션 `C:\dev\desca`(→ 이 저장소)로 구성돼 있다 — 한글 경로로 직접 구성하면 리소스 컴파일러가 멈춘다. 처음 구성 · windeployqt · `z.dll` 복사 · 화면 캡처 명령은 DEV_PROGRESS §2에 있다.

```bash
# 빌드(바뀐 것만)
cmake --build C:/dev/kerf-v4c

# 단위 시험(Catch2 v3, 실행 파일 하나. ctest에는 `core` 하나로만 등록됨)
C:/dev/kerf-v4c/asec_tests.exe                         # 전체
C:/dev/kerf-v4c/asec_tests.exe "[gaps]"                # 태그 하나(--list-tags 로 목록)
C:/dev/kerf-v4c/asec_tests.exe "CoalescingWorker*"     # 이름으로(와일드카드)
C:/dev/kerf-v4c/asec_tests.exe "~CoalescingWorker*"    # CoalescingWorker 시험 4개를 모두 뺌(불안정한 건 1개. quickTest · CI Windows와 같음, 약 2초)

# GUI 없이 단면만 계산(합성 모델, 단계 0 고친 뒤 기대값: polylines=1 vertices=54)
# --out(과 모델 경로)은 ASCII만 — 한글이 든 경로(scratchpad 포함)를 주면 0xC0000409로 죽는다(2026-10-09 실측)
C:/dev/kerf-v4c/asec-section.exe C:/dev/kerf-synth/Synthetic.3mx 200002 450006 200014 450006 --out C:/dev/tmp/sec
```
```powershell
# 겹침 · 자리 규칙 감사(화면 없이, 한 번 약 1분). 실패면 EXIT=9, 결과는 DIR\ui-audit.txt 와 캡처. GUI exe라 Start-Process -PassThru + WaitForExit(시간)으로 돌리고 멈추면 죽인다(DEV_RULES R7.2). PowerShell 도구 timeout은 300000으로 준다
Remove-Item C:\dev\tmp\kerf-check -Recurse -Force -ErrorAction SilentlyContinue; $env:QT_QPA_PLATFORM='offscreen'; $p = Start-Process -FilePath C:\dev\kerf-v4c\app\SectionViewer.exe -ArgumentList 'C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\tmp\kerf-check --ui-audit C:\dev\tmp\ui --quit' -PassThru; $null = $p.Handle; if ($p.WaitForExit(240000)) { "EXIT=" + $p.ExitCode } else { $p.Kill(); 'TIMEOUT(죽임)' }
```
- **프로젝트 계약 `.claude/cpp-project.json`** — 위 명령의 기계용 사본. `/verify`(`desca:verify`) · `ui-visual-check`(`desca/`에서 실행, 캡처 `.claude/scripts/shot.ps1`, 장면 `work` · `draw` · `start`) · 전역 훅이 읽는다. 끝날 때 자동 빌드 · 빠른 시험(Stop 훅)과 시작 요약 훅은 **세션 작업 폴더에서 위쪽으로만** 계약을 찾는다 → `desc/`에서 연 세션에서는 돌지 않으니 `/verify`를 직접 돌린다. 캡처 명령에는 배율 인자가 없어 `ui-visual-check --scales 2`는 배율을 바꾸지 않는다 — 200 %는 `$env:QT_SCALE_FACTOR='2'`를 주고 찍는다. 명령이 바뀌면 이 파일과 거기를 같이 고친다.
- 합성 모델은 `asec-make-synthetic.exe C:\dev\kerf-synth`로 만든다(EPSG:5186, X 200000–200016 · Y 450000–450012). 위 12 m 선이 표준 시험 단면선이다.
- `CoalescingWorker: 끄는 동안의 미리보기는…`(`tests/test_schedule.cpp`)은 Windows 타이머 간격 탓에 **가끔** 실패한다. 고치지 말고 나머지가 통과하는지 본다.
- `[real]` 시험은 `ASEC_REAL_3MX`(실제 3MX 경로)가 없으면 건너뛴다(정상). 시험 대부분은 태그가 없어 이름으로 거른다.
- 앱 자동 실행은 항상 `--settings DIR`로 설정을 격리한다. SectionViewer는 GUI 실행 파일이라 PowerShell `&`는 끝을 기다리지 않는다 → `Start-Process -PassThru` + `WaitForExit(ms)`, 멈추면 Kill(R7.2). 옵션 전체는 `app/main.cpp`의 인자 해석(`s == "--…"`)이 정본이다(머리 주석 · README에는 `--sheet-check` 등이 빠져 있다). 판정 옵션과 실패 종료 코드: `--ui-audit DIR` 9 · `--sheet-check DIR` 8 · `--icon-sheet f.png` 10(모델 없이 `--quit`과) · `--undo-test` 7 · `--export-*` 4 · 열기 2 · 단면 3 · `--start-shot` 6. 캡처는 `--shot`(작업) · `--ctx-shot`(그리는 중) · `--start-shot`(파일 없이 홈).
- 포맷 · 린트 설정(.clang-format · .clang-tidy)은 없다. 바꾼 줄만 고치고 파일 전체를 다시 포맷하지 않는다.
- CI(`.github/workflows/ci.yml`): **Windows 작업 하나뿐**(Qt 6.8.3 + MSVC로 앱 전체 · 시험 `~CoalescingWorker*` · offscreen `--ui-audit` 1920×1040 ×1 과 3840×2160 ×`QT_SCALE_FACTOR=2` 를 끝까지 기다려 판정). Linux 작업은 B5(`8cc67fb`)에서 뺐다 — 사용자 지시 「윈도우만을 대상으로」(DEV_RULES R7.4). CI 초록불만 보고 끝이라 하지 말고 로컬 감사(8조합)를 적는다. 이 PC는 Qt 6.10.3 + clang-cl.

## 구조 (여러 파일을 읽어야 보이는 큰 그림)
- **두 층.** `core/`(정적 라이브러리 `archsection`, namespace `asec`)는 Qt 없는 C++17이다. 3MX 읽기 · LOD · 단면 자르기 · 레벨선 · 입면 영상 · 피킹 · 좌표계 · DXF/TIFF/LAS · 도면 배치 계산이 모두 여기 있다(MicroStation MDL 플러그인에서 다시 쓰려는 것). `app/`은 화면만 맡는다.
- **단면 파이프라인** `asec::computeSection`(`core/src/engine.cpp`): `MeshSource`(3MX = `TmxSource`, OBJ = `StaticSource`)에서 두께 띠(+1 mm 여유)와 겹치는 메시를 모은다 — 입면 영상은 최종 = 잎, 미리보기 = 해상도에 맞는 거친 LOD. 잘린 선은 미리보기에서도 단면 평면 ±1 mm 띠의 잎으로 자른다(`leafProfileAlways`, `--cut-check`가 확인). → `cutMesh` → `buildProfile`(`stitchSegments` · `cleanupPolylines`) · `snapEnds` → `renderElevation`(입면 영상, CPU Z 버퍼). 자르기는 두 패스다: 1패스는 스냅 없이 정확히, 1패스가 못 덮은 빈 구간만 2패스에서 1 mm 평면 스냅으로 메운다(OpenCTM 양자화 대응, DEV_PROGRESS 단계 0). 스냅을 1패스에 걸면 정밀도 시험이 깨진다.
- **단면 좌표계** `SectionFrame`: s = A에서의 수평 거리, d = 단면 평면에서의 깊이(+ = 보는 방향), z = 높이. 단면선 · 레벨선 · 입면 영상이 같은 (s, z)를 써서 정확히 겹친다.
- **스레드.** 단면 계산은 `asec::CoalescingWorker`(core, 자체 `std::thread`)가 한다. 끄는 동안의 미리보기는 마지막 요청만 남기고, 최종 요청은 실행 중인 계산을 취소한다. 결과는 `QMetaObject::invokeMethod(this, 람다, Qt::QueuedConnection)`로 GUI 스레드에 넘기고, 세대 번호(`ResultGate`)로 오래된 결과를 버린다. 평면 보기는 `asec::LodStreamer`가 작업 스레드에서 타일을 읽고, GUI 스레드가 프레임마다 그릴 · 올릴 · 뺄 노드를 받는다(GPU 예산 LRU).
- **좌표.** 실좌표 = 메시 로컬 + SRSOrigin(double). 3MX의 x = 동, y = 북이다(SRS의 공식 축 순서와 무관). `srs.hpp` · `vdatum.hpp`는 좌표계를 알아보고 이름표만 붙이며 높이 값은 바꾸지 않는다.
- **app 파일.** 화면 조립(리본 묶음 `buildRibbon` · `setRibbonContext` · 단면 목록 · 홈 `rebuildStartPage` · 되돌리기 · `uiAudit` · `iconSheet`)은 `mainwindow_ui.cpp`, 리본 위젯 자체(타일 크기 단계 · 좁으면 글자 숨김)는 `ribbon.cpp`, 떠 있는 안내 칩 · 떠 있는 단추(옛 도구 줄 대신)는 `guideband.cpp`, 열기 · 단면 요청 · 내보내기는 `mainwindow.cpp`, 도면 탭 · SVG/PDF 저장은 `sheetexport.cpp`(배치 계산은 core `sheet.hpp`), 평면 보기(OpenGL · `LodStreamer`)는 `planview.cpp`, 단면 보기는 `sectionview.cpp`.
- **아이콘은 SVG다.** `app/icons/lucide/` · `app/icons/kerf/`(kerf 쪽은 `make_kerf_svgs.py`가 만든다 — SVG 말고 스크립트를 고친다). 손으로 적은 목록 `app/icons/icons.cmake`가 configure 때 `generated/kerf_icons.hpp`로 굽고, `icons.cpp`의 `kerf::icon` · `chipIcon`이 그린다. `theme::icon(Ico)`는 `iconName(Ico)` 이름 표를 거치는 포장일 뿐이다. 새 아이콘 = SVG + `icons.cmake` 목록(+ `Ico`면 `iconName` · `kIcoCount`), 빠지면 `--icon-sheet` 종료 코드 10.
- **단면 그리기는 한 함수** `paintSectionDoc`(`sectionview.cpp`): 단면 화면(`forExport=false`) · 도면 미리보기와 도면 SVG/PDF(`paintSheet`) · 단면 PNG/TIFF(`exportSectionImage`) · 명령줄 캡처가 모두 이것을 부른다. 고치면 화면과 내보내기가 함께 바뀐다(`forExport`로 갈리는 곳 제외) — `--sheet-check` · `--cut-check`로 확인한다. 단면 DXF는 따로 간다(core `exportSectionDxf`).
- **`app/main.cpp`가 시험 장치다.** 캡처 · 내보내기 · 감사(`--shot` · `--ui-audit` · `--cut-check` · `--undo-test` …)는 모두 명령줄 옵션으로 앱을 몰아서 한다. 새 화면 판정이 필요하면 여기에 옵션을 더하고, 실패 때 0이 아닌 종료 코드를 돌려준다.
- **CMake 목록은 손으로 적는다(glob 아님).** core 소스와 시험 파일은 최상위 `CMakeLists.txt`(`archsection` · `asec_tests`), 앱 소스는 `app/CMakeLists.txt`에 더해야 빌드된다. 시험과 도구는 합성 3MX 생성기 `asec_synth`(`tools/synth.cpp`)를 링크한다.
- **앱 빌드 분기 둘**(`app/CMakeLists.txt`). ① `find_package(Qt6)`: 지금 개발 · CI가 쓰는 길, Svg · Sql 포함, .3sm 읽기(`KERF_HAS_3SM`, `sm3convert.cpp`). ② `ASEC_QT_WIN_DIR`(MinGW 교차, `build-win.sh`): Qt를 손으로 링크하고 Svg · Sql · .3sm이 없다. `icons.cpp`(QSvgRenderer)와 `sheetexport.cpp`(QSvgGenerator)가 Qt Svg를 조건 없이 쓰는데 ②의 include · 링크 목록에 QtSvg가 없어, ②는 지금 빌드되지 않는다(코드로 확인, 실행은 안 함). 새 Qt 모듈을 쓰면 두 분기를 모두 본다.

## 함정
- **Linux · WSL git으로 보면 줄바꿈 탓에 거의 모든 파일이 수정됨(M)으로 보인다.** 이 PC의 Windows git(`core.autocrlf=true`)으로는 깨끗하다. 실제 변경은 `git diff HEAD --ignore-cr-at-eol --stat`로 본다. `git add -A` · `git add .`은 어디서든 금지 — 바꾼 파일만 이름으로 더한다(다른 세션의 미커밋 변경이 섞여 있음, R5.2). 푸시 명령은 DEV_PROGRESS §4 「푸시 해결」.
- offscreen 실행에는 앱 옆 `platforms\qoffscreen.dll`이 있어야 한다. windeployqt는 넣지 않는다(`C:\dev\kerf-v4c\app\platforms`에는 이미 있음).
- 저장소는 **공개**다. 실좌표가 보이는 캡처 · 실제 모델 · 다른 앱(Strata) 캡처를 커밋하지 않는다.
- 장부 `.superpowers/sdd/<계획>/progress.md`(DEV_RULES R1.4)는 `.gitignore` 대상이라 이 PC 본 체크아웃에만 있다(커밋 · push · worktree에 없음). 장부의 Ruling 중 남겨야 할 것은 DEV_PROGRESS 기록의 「판정」에도 적는다.

## 소프트웨어 팩토리 (`/factory`)
- `/factory` 스킬(`.claude/skills/factory/SKILL.md`)이 기능 요청을 명세 → worktree 구현 → 검증 → 검토 → 사용자 승인 대기로 돌린다. 절차 · 역할 · 검증 기준은 그 스킬과 `factory/project-profile.md`에, 실행기는 `factory.cmd`(`status | new … | verify <ID> | ci <sha>`). 브랜치 `factory/<ID>` · worktree `C:\dev\kerf-wt\<ID>`는 그 작업 것이다 — 본 체크아웃 작업에서 건드리지 않는다.
