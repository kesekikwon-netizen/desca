# 명세 — 20261009-01-b5-dpi 디자인 v5 B5 해상도 · DPI 안정성(4K · 와이드 · 배율, Windows만)

작성: spec-writer · 기준 커밋 0f7fbfed7455c0c6db308af386ae935b27d6f77e(`feature/design-v2` HEAD) · worktree `C:\dev\kerf-wt\20261009-01-b5-dpi`(확인: 브랜치 `factory/20261009-01-b5-dpi`, 미커밋 변경 없음)
근거 문서: `factory/project-profile.md` §6–§9 · `docs/DEV_RULES.md`(본 저장소 쪽 최신: R7.4 Windows만 · R7.5 포터블 구조) · `docs/design/DESIGN_SPEC.md` §2.1(BINDING) · 계획 `docs/superpowers/plans/2026-10-09-design-v5.md` Task B5(132–139행) · `.claude/skills/factory/SKILL.md` §6 「화면(app/) 변경의 고정 완료 기준」. 스킬: `superpowers:writing-plans` 를 불러 단계 계획을 §3-B 에 녹였다.

## 1. 목표 (1–3줄)
4K · 와이드 · 작은 창(1280×800)과 Windows 배율 100 · 125 · 150 · 175 · 200 % 어디서나 Kerf 화면이 잘리거나 흐리지 않게 한다(사용자 2026-10-09 「해상도 모니터가 4k 와이드모니터 어떤 곳에서도 안정적이게」). 그것을 `--ui-audit` 이 크기 넷 × 배율 둘에서 **판정**하게 하고, CI 는 Windows 작업만 남기되 4K · 배율 2 감사를 실제로 판정하게 한다(「윈도우만을 대상으로」 · 선택 「CI 의 Linux 작업 삭제」).

## 2. 변경 범위
- **바꾸는 파일(추정 — builder 가 원인을 보고 줄일 수 있음, 늘리면 build.md 판정):**
  | 파일 | 무엇을 |
  | --- | --- |
  | `app/main.cpp`(294행 근처) | `--size W×H` 가 그 크기로 열리게(원인은 builder 가 확인 — 후보: `MainWindow` 생성자 끝 `resize(1600, 950)`(`app/mainwindow_ui.cpp:1627–1628`) 와 `w.resize(W,H)` 순서 · 첫 표시 때 레이아웃이 최소 크기를 강제하는 것 · `QStackedWidget body_` 의 최소 폭(홈 페이지 포함)). 감사가 요청 크기를 알도록 `w.setProperty("auditSize", QSize(W, H))` |
  | `app/mainwindow_ui.cpp` | `uiAudit`(1822행~)에 판정 줄 3개 추가 · 고침(§3-A), 필요하면 시작 때 최소 폭 원인 고치기 |
  | `app/icons.hpp` · `app/icons.cpp` | 배율 목록 `{1, 1.25, 1.5, 1.75, 2, 3}` 을 한 곳(`kerf::iconDprs()`)에 두고 `icon` · `chipIcon` 이 쓰게(지금 `icons.cpp:19` `kDprs = {1,1.5,2,3}`), 머리 주석(3행) 고침 |
  | `app/ribbon.cpp`(197행) | 손으로 적은 `{1.0, 1.5, 2.0, 3.0}` 을 `kerf::iconDprs()` 로 |
  | `app/guideband.hpp` · `app/guideband.cpp` | `QEvent::DevicePixelRatioChange` 를 받으면 칩 아이콘(`icon_` 의 pixmap, 지금 `guideband.cpp:42` 에서 한 번만 구움)을 새 배율로 다시 굽는다 + 감사용 `iconCacheKey()` |
  | `app/sectionview.hpp` · `app/sectionview.cpp` | 「맞춤 상태」 표시 + `resizeEvent`(50행)에서 맞춤 상태면 다시 맞춤 + 감사용 `contentFits()` |
  | `app/planview.hpp` · `app/planview.cpp` | 「맞춤 상태」 표시 + `resizeEvent` 재정의(GL 없이도 불리는 길)에서 맞춤 상태면 `fitAll()`, 아니면 행렬만 갱신 + 감사용 `contentFits()` |
  | `.github/workflows/ci.yml` | `linux-core` 작업(19–33행)과 머리 주석 「- linux: …」(2행) 삭제, UI 감사 단계(73–82행)를 **기다려서 판정**하게 고치고 3840×2160 + `QT_SCALE_FACTOR=2` 실행 추가 |
  | `docs/design/DESIGN_SPEC.md` §2.1(40행) | 「아이콘은 dpr 1 · 1.5 · 2 · 3 네 장」 → 「dpr 1 · 1.25 · 1.5 · 1.75 · 2 · 3 여섯 장(Windows 기본 배율 125 · 150 · 175 · 200 %)」, 그리고 이번에 정한 규칙 세 줄(①`--size` 는 그 크기로 열림 · 창 최소 폭 ≤ 1280 ② 화면을 옮겨 배율이 바뀌면 한 번 구운 그림(떠 있는 안내 칩 아이콘)은 다시 굽는다 ③ 창 크기가 바뀌면 맞춤 상태인 평면 · 단면은 다시 맞추고, 사용자가 확대 · 이동 · 회전한 보기는 가운데와 축척을 지킨다) |
- **바꾸지 않는 것(명시):**
  - `core/` · `tests/` · `tools/` · `CMakeLists.txt` · `app/CMakeLists.txt` · `build-*.sh` · `packaging/` · `third_party/` · `generated/` — 계산 변경이 없다(새 Catch2 시험 없음, 판정은 `--ui-audit`). CMake 의 `ASEC_BUILD_APP` 옵션은 그대로 둔다(R7.4 는 Linux 를 「새로 만들거나 살리지 않는다」이지 옵션 삭제가 아님). 새 Qt 모듈 없음(`QEvent` 는 QtCore) → MinGW 분기 영향 없음.
  - `factory/scripts/verify.ps1` · `.claude/scripts/shot.ps1` — 이미 `-UiSizes` · `-UiScales` · `-Size` 가 있다.
  - **본 저장소 쪽에서 따로 고칠 것(worktree 에서 고치지 않음):** `CLAUDE.md`(「CI(…) Linux 는 …」 문장 · 「offscreen `--ui-audit` 단계는 지금 판정하지 않는다」 문장) · `docs/DEV_PROGRESS.md`(§1 · §3 · §4 · §5 B5 기록) · **`docs/DEV_RULES.md` R2.7 문구 「dpr 1 · 1.5 · 2 · 3」**(판정 아래) · `factory/project-profile.md`(§3 Linux 줄 · §4 CI 설명) · `.claude/skills/factory/SKILL.md` §5-6(「CI(Linux 코어·시험 · …)」) · 계획 문서 Task B5 체크 · 장부.
    - **판정(브리프와 다름):** 브리프는 DEV_RULES R2.7 을 이 작업 범위에 넣으라 했으나, 본 저장소에 `docs/DEV_RULES.md` 미커밋 변경(R7.4 · R7.5 두 줄, `git diff --stat` 확인)이 있다. `CLAUDE.md` · `DEV_PROGRESS.md` 를 빼는 것과 같은 이유(SKILL §5-1 병합 전제 「대상 브랜치 파일과 겹치는 미커밋 변경 없음」)로 R2.7 도 본 저장소에서 R7.4 · R7.5 와 함께 고친다. 잘못이면: 병합 뒤 DESIGN_SPEC(여섯 장)과 DEV_RULES(네 장)가 잠깐 어긋남 — 승인 보고서 「후속 일」에 올린다.
  - 홈 화면 앱 아이콘 56 px(`mainwindow_ui.cpp:1176`, dpr 2 고정 `QPixmap pm(112,112)`) · 홈 「작업 순서」 체크 표시(1334행, 홈을 다시 만들 때만 구움)의 화면 옮김 다시 굽기 — 범위 밖(§5 남은 위험).
  - 포터블 묶음 · 설치 · 배포 — 없음(R7.5). **제약:** 새 코드에 절대 경로(`C:\…`) · 레지스트리 · 설치 경로 의존을 넣지 않는다(설정은 지금처럼 `QSettings` + `--settings` 격리).
- **사람 검토 필수 영역 해당 여부: 해당** — ① 빌드·의존성·패키징·CI: `.github/workflows/ci.yml`(작업 삭제 + 판정 방식 변경, SKILL §7) ② profile §8 「디자인 토큰 · 화면」: `app/main.cpp` · `app/mainwindow_ui.cpp`(`--ui-audit` 기대값 추가). 좌표계 · 높이 · 파일 형식 · 단면 계산은 건드리지 않는다.

## 3. 완료 기준 ↔ 검증 방법 (기준마다 하나 이상)
기호: **V8** = `C:\Users\권을\Documents\desc\factory.cmd verify 20261009-01-b5-dpi -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2`(8가지). 감사 줄은 각 `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\<시각>\ui-audit-<크기>-x<배율>\ui-audit.txt` 에 있다. 모든 판정 줄은 실패면 끝에 `  FAIL` 이 붙고 `RESULT fail` · 종료 코드 9.

| # | 완료 기준 | 검증 방법(시험 이름 · 명령 · 확인 절차) | 종류 |
| --- | --- | --- | --- |
| AC1 | **요청 크기 · 최소 폭 · 넘침 · 배율 적용**: 8가지 모두 새 줄 `ui-audit env size=WxH want=WxH scale=s dpr=d min-width=m overflow=0 icon-dpr-bad=0` 이 FAIL 없이 나온다. 판정 = ① `size` 가 `want` 와 가로 · 세로 모두 같음 ② `min-width`(= `minimumSizeHint().width()`) ≤ 1280 ③ `overflow`=0(보이는 리본 칩 · 「좌표 입력」 · 찾기 칸 · 배지 둘 · 문서 탭 구석 · 오른쪽 판이 창 밖으로 안 나감) ④ `qFuzzyCompare(dpr, scale)`(`QT_SCALE_FACTOR` 가 실제로 먹었는지 — ×2 실행이 가짜가 아니어야) | V8 의 env 줄 8개. **RED(구현 전):** 1280x800 에서 `size=1591x800 want=1280x800 … FAIL`(2026-10-09 실측값) | 자동 |
| AC2 | **아이콘 dpr 1.25 · 1.75**: 선 아이콘(`kerf::icon("search",18,theme::Hand)`)과 **지금 보이는 리본 칩 전부**(`b->icon()`, `ribbon.cpp:197` 의 「늘 켜짐」 칩 포함)가 d ∈ {1.25, 1.75} 에서 `QIcon::pixmap(QSize(n,n), d)` 로 **그 배율로 구운 장**을 낸다: 돌려받은 장의 `devicePixelRatio()`==d, 선 아이콘은 `kerf::glyph(name,18,ink,d)` 와 크기 · 픽셀이 같음. 어긋난 수가 `icon-dpr-bad` | AC1 의 env 줄 `icon-dpr-bad=0`(8가지). **RED:** 지금은 1.25 장이 없어 Qt 가 다른 배율 장을 준다(context7: 「requested devicePixelRatio might not match the returned one」) → `icon-dpr-bad≥2 FAIL` | 자동 |
| AC3 | **화면 옮김(dpr 바뀜) 다시 굽기**: `GuideBand` 가 `QEvent::DevicePixelRatioChange` 를 받으면 칩 아이콘을 `devicePixelRatioF()` 로 **다시 굽는다**(같은 아이콘 이름이라도 — `guideband.cpp:42` 의 이름 캐시를 이 길에서는 건너뜀). 호스트 크기가 바뀔 때(`place()`)도 구운 배율 ≠ 지금 배율이면 다시 굽는다(Qt 가 자식 위젯에 이 사건을 보내지 않는 경우의 안전판 — 아래 「확인하지 못함」) | 감사 5b 줄 `ui-audit guide-step2=1 … dpr-rerender=1 …`(8가지): `QApplication::sendEvent(guide_, QEvent(DevicePixelRatioChange))` 앞뒤 `GuideBand::iconCacheKey()` 가 달라야. **RED:** 지금은 처리 없음 → `dpr-rerender=0 … FAIL`. 실제 모니터 둘(배율 다름)이 있으면 창을 옮겨 칩 아이콘 선명도를 눈으로(없으면 「미실행(환경 부족)」) | 자동 + 수동 |
| AC4 | **창을 줄일 때 자동 맞춤 · 사용자 보기 유지**: (a) 맞춤 상태(`fit()` · `fitAll()` 직후, 사용자가 손대지 않음)에서 창을 1280×800 으로 줄이면 단면선 전체(s 0–L)가 단면 그림 칸 가로에, 장면 상자(XY)가 평면 화면 안에 들어간다 (b) 사용자가 확대 · 이동 · 회전 · 축척 지정을 했으면 창 크기가 바뀌어도 축척(단면 `xf().ppm` · 평면 `metersPerPixel()`)이 **그대로**(지금처럼 가운데 유지) (c) 줄인 창이 실제로 1280 이하 · 리본 타일 ≤ 32 · 글자 숨김 | 감사 2b 줄 `ui-audit narrow rule-at-1280 look=… labels=0 window-min-width=… actual-width=≤1280 actual-look=≤32 actual-labels=0 plan-fits=1 section-fits=1 zoom-kept=1`(8가지). **RED:** 1920 이상에서 `section-fits=0`, `plan-fits=0`(단, `PlanView::contentFits` 를 지금 크기로 재야 나온다 — §3-B Task 1 주의). `zoom-kept` 는 지킴 시험이라 처음부터 1(구현이 사용자 보기까지 다시 맞추면 0) | 자동 |
| AC5 | **고정 완료 기준(SKILL §6):** 8가지 모두 `ui-audit RESULT ok`, 빌드 새 경고 0, 빠른 시험 통과, asec-section 기준값 일치 | **V8 → `RESULT PASS`(exit 0)**, verify.md 의 build 줄 판정 PASS(WARN 아님) · ui-audit 줄 8개 PASS | 자동 |
| AC6 | **125 · 175 % 배율 전 과정**: 1920×1040 에서 `QT_SCALE_FACTOR` 1.25 · 1.75 로도 감사 전부 ok(AC1 의 `dpr=1.25`/`1.75` 포함) | `factory.cmd verify 20261009-01-b5-dpi -UiAudit on -UiSizes 1920x1040 -UiScales 1.25,1.75` → `RESULT PASS`. 이번 범위 밖 원인으로 다른 줄이 실패하면 고치지 말고 build.md 에 적고 needs-human(SKILL §2) | 자동 |
| AC7 | **회귀 없음**: 기존 감사 항목 전부 ok 유지(AC5 에 포함) · `--icon-sheet` · `--sheet-check` · `--ctx-shot` 그대로 · core/tests 변경 없음 | §4 「돌려야 하는 기존 시험」 명령 넷 + `git -C C:\dev\kerf-wt\20261009-01-b5-dpi diff --stat 0f7fbfe -- core tests tools CMakeLists.txt app/CMakeLists.txt` 가 비어 있음 | 자동 |
| AC8 | **CI**: ① `linux-core` 작업과 머리 주석 「- linux: …」 없음, 남은 작업은 `windows-app` 하나 ② UI 감사 단계가 (1920x1040, 배율 1) · (3840x2160, `QT_SCALE_FACTOR=2`) 두 번 돌고, 각각 **끝날 때까지 기다려**(`Start-Process -PassThru` + `WaitForExit(300000)`, 넘으면 Kill 후 실패 — DEV_RULES R7.2) 종료 코드 ≠ 0 이거나 `ui-audit.txt` 마지막 RESULT 줄이 `ui-audit RESULT ok` 가 아니면 `throw` ③ 실행마다 `--settings` · `--ui-audit` 폴더가 따로 | ① · ② 사람이 diff 를 읽음(사람 검토 필수) ② 로컬 모의 실행: 단계 스크립트를 경로만 worktree 빌드(`…-build\app\SectionViewer.exe`, `C:\dev\kerf-synth\Synthetic.3mx`, Qt `C:\Qt\6.10.3\msvc2022_64`)로 바꿔 PowerShell 에서 돌려 두 번 모두 exit 0 · RESULT ok, **그리고 모델 경로를 없는 파일로 바꾸면 단계가 실패(throw)** 하는 것을 본다(지금 단계가 판정하지 않는 것을 고쳤다는 증거). ③ GitHub Actions 결과는 사용자가 push 한 뒤 `factory.cmd ci <병합 커밋> -Wait`(SKILL §5-6) — 그 전에는 「미실행(push 전)」 | 수동 + 사람 |
| AC9 | **캡처에서 글자 잘림 · 겹침 없음, 아이콘 선명(SKILL §6 고정 기준)**: kerf-ui-reviewer 루브릭 항목 모두 4점 이상 | `powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\권을\Documents\desc\desca\.claude\scripts\shot.ps1 -Exe C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe -Scene <장면> -Size <크기> -Out C:\dev\tmp\b5-shots\<이름>.png`(PATH 앞에 `C:\Qt\6.10.3\msvc2022_64\bin`): `work` 1280x800 · 1920x1040 · 3440x1440 · 3840x2160 각 1장, `draw` 1280x800 1장, `start` 1280x800 1장, `$env:QT_SCALE_FACTOR='2'` 로 `work` 1920x1040 1장, `'1.25'` 로 `work` 1920x1040 1장(아이콘 흐림 확인). `start` 1280x800 그림의 픽셀 크기가 1280×800 인지(`[System.Drawing.Image]::FromFile(...)`.Width/Height) | 수동 |
| AC10 | **실제 화면(GL 이 뜨는 길)에서도 같은 판정**: offscreen 은 OpenGL 이 안 떠 평면 `resizeGL` 이 불리지 않으므로(근거 §5) 실제 화면에서 한 번 | CLAUDE.md 감사 명령에서 `$env:QT_QPA_PLATFORM='offscreen'` 을 빼고 worktree exe 로 `--size 1920x1040` 1회 → `EXIT=0` · `ui-audit RESULT ok`(`narrow … plan-fits=1 section-fits=1`). 사람 절차: 앱을 열어 합성 모델 · 표준 단면선 → 창 오른쪽 아래 모서리를 끌어 1280 근처까지 줄임 → 평면 · 단면 그림이 잘리지 않음, 그 뒤 휠로 확대하고 다시 창을 넓혀도 확대가 풀리지 않음 | 수동 |
| AC11 | **문서 · 포터블 제약**: DESIGN_SPEC §2.1 이 §2 표의 새 문구대로(「1 · 1.25 · 1.5 · 1.75 · 2 · 3」), `icons.cpp` 머리 주석도 같은 목록. 새 코드에 절대 경로 · 레지스트리 없음 | `git -C C:\dev\kerf-wt\20261009-01-b5-dpi diff 0f7fbfe -- docs/design/DESIGN_SPEC.md app/icons.cpp` 를 읽음 · `git -C … diff 0f7fbfe -- app` 의 `+` 줄에 `C:[\\/]` · `HKEY_` · `NativeFormat` 이 없음(Grep) | 수동 + 자동 |

### 3-A. 감사 줄 형식(정본 — builder 는 이 이름 그대로 쓴다)
초안 `C:\dev\tmp\b5-red.patch`(8파일 · 63줄, 시험만)를 출발점으로 쓰되 아래가 정본이다. 초안과 다른 곳은 **굵게**.
- **env**(uiAudit 맨 앞 「0) 해상도 · DPI」): `ui-audit env size=%1x%2 want=%3x%4 scale=%5 dpr=%6 min-width=%7 overflow=%8[ [이름,…]] icon-dpr-bad=%9[  FAIL]`. `want` 는 `property("auditSize")`(없으면 판정 ①을 건너뜀). 판정 ①은 초안의 「≤ max(want,1280)」 이 아니라 **가로 · 세로 모두 같음**. **판정 ④ `qFuzzyCompare(devicePixelRatioF(), QT_SCALE_FACTOR 값)` 추가**(변수가 없으면 1). `icon-dpr-bad` 는 초안의 선 아이콘 비교 + **보이는 리본 칩 전부의 `pixmap(QSize(tile,tile), d).devicePixelRatio()==d`**.
- **guide-step2**(5b): 초안 그대로 `… dpr-rerender=%9 …` 를 `guide2` 판정에 더함.
- **narrow**(2b): 초안 그대로 `actual-width · actual-look · actual-labels · plan-fits · section-fits` + **`zoom-kept=%n`**: 1280 판정 뒤 `section_->zoomBy(1.4)` · `plan_->zoomBy(1/1.4)` → `ppm`(`section_->xf().ppm`) · `mpp`(`plan_->metersPerPixel()`) 기록 → `resize(keepSize)` → 두 값이 **그대로**면 1 → 이어서 `section_->fit()` · `plan_->fitAll()` 로 되돌림. 판정 `narrowOk` 에 `zoom-kept != 0` 을 더한다(결과 없는 경우 -1 = 판정 안 함).

### 3-B. 단계 계획(`superpowers:writing-plans` — builder 는 `superpowers:test-driven-development` 로 Task 마다 RED → GREEN → 커밋)
**Global Constraints:** C++17 · Qt 6 Widgets · `Q_OBJECT` 금지(동적 속성 `setProperty` 는 moc 없이 됨) · 람다 연결 · 색 · 크기는 `theme.hpp` 토큰 · 바꾼 줄만 고침(재포맷 금지) · 치수는 논리 픽셀 · 앱 자동 실행은 `--settings` 격리 · 커밋은 `factory/20261009-01-b5-dpi` 에서 바꾼 파일 이름만(`git add -A` 금지) · 커밋 메시지 한국어 「무엇을 · 왜 · 확인」 + `Co-Authored-By` 줄.

**Review Focus**(시험이 덮지 못해 사람이 쓸 때 물릴 수 있는 것, 위에서부터):
1. 실제 화면(GL)에서 창 줄이기 — offscreen 은 `resizeGL` 이 안 불림 → AC10 실제 화면 감사 + 끌어서 줄이기.
2. 125 · 175 % 같은 소수 배율에서 18 px × 1.25 = 22.5 처럼 픽셀이 반으로 떨어짐 → AC2(`icon-dpr-bad`) · AC6(1.25 · 1.75 전 과정) · AC9(1.25 캡처).
3. 사용자가 휠로 확대한 직후(부드러운 확대 타이머가 돌기 전) 창 크기가 바뀜 → 휠은 **굴린 순간**(`wheelZoom`) 맞춤 상태를 끈다(`applyZoomAt` 에서만 끄면 타이머 전 resize 가 확대를 지움). `zoom-kept` 가 `zoomBy` 길을 잠근다.
4. 모니터를 옮겨 배율이 바뀜 — Qt 가 자식 위젯에 사건을 보내는지 확인 못 함 → AC3 의 `place()` 안전판 + 실제 모니터 둘이면 눈 확인.
5. CI 감사가 이제 실제로 판정함 → GitHub 러너(Qt 6.8.3)에서 처음으로 실패가 보일 수 있음 → AC8 로컬 모의 실행(양성 · 음성) + push 뒤 `factory ci`.

**Task 1 — 감사 판정 먼저(RED)** · Files: `app/main.cpp` · `app/mainwindow_ui.cpp`(uiAudit 0) · 5b · 2b) · `app/guideband.hpp/.cpp`(`qint64 iconCacheKey() const` = `icon_->pixmap().cacheKey()`) · `app/sectionview.hpp/.cpp`(`bool contentFits() const`) · `app/planview.hpp/.cpp`(`bool contentFits() const`)
- Produces: `MainWindow` 동적 속성 `"auditSize"`(QSize) · `GuideBand::iconCacheKey()` · `SectionView::contentFits()`(has_ 이고 `xf_.s0 ≤ 0 + 1e-6` 이고 `xf_.s0 + xf_.plot.width()/xf_.ppm ≥ L − 1e-6`) · `PlanView::contentFits()`(장면 상자 XY 가 보이는 범위 안, 1e-6 m).
- **주의(초안과 다름):** `PlanView::contentFits` 는 저장된 `proj_` 가 아니라 **지금 `width()` · `height()` · `mpp_` · `target_` · `yaw_` · `pitch_` 로** 보이는 범위를 잰다(`updateMatrices` 와 같은 식을 지역 행렬로). 초안처럼 `viewRectLocal`(=`proj_`)을 쓰면 offscreen 에서 `resizeGL` 이 안 불려 행렬이 옛 크기 그대로라 고치기 전에도 `plan-fits=1` 로 거짓 통과한다.
- Step: 위 3-A 대로 줄을 넣고 빌드 → `factory.cmd verify 20261009-01-b5-dpi -UiAudit on -UiSizes 1280x800,1920x1040 -UiScales 1` → **기대 FAIL**: 1280x800 `env size=1591x800 … FAIL`, 두 크기 `icon-dpr-bad≥2 FAIL` · `guide-step2=0 … dpr-rerender=0`, 1920x1040 `narrow … plan-fits=0 section-fits=0 … FAIL`, 감사 종료 코드 9. RED 줄을 build.md 에 붙인다. 기대와 다르게 GREEN 인 줄이 있으면(예: plan-fits=1) 판정식이 틀린 것 — 시험부터 고친다. 커밋 「시험: B5 해상도 · DPI 감사 판정(RED)」.

**Task 2 — `--size` 요청 크기 · 최소 폭** · Files: `app/main.cpp` · (원인에 따라) `app/mainwindow_ui.cpp`
- Step: 원인을 `superpowers:systematic-debugging` 으로 확인(시작 직후 · 표시 직후 · 파일 연 뒤 `minimumSizeHint().width()` 를 찍어 1591 이 어디서 오는지) → 최소 수정 → 1280x800 env 줄 `size=1280x800 want=1280x800` GREEN, 다른 크기 그대로. 커밋.

**Task 3 — 아이콘 dpr 1.25 · 1.75 + 스펙 문구** · Files: `app/icons.hpp` · `app/icons.cpp` · `app/ribbon.cpp` · `docs/design/DESIGN_SPEC.md`
- Produces: `[[nodiscard]] QList<qreal> kerf::iconDprs();` → `{1.0, 1.25, 1.5, 1.75, 2.0, 3.0}`. `icon` · `chipIcon` · `Ribbon::chipIconCached` 가 이것만 쓴다.
- Step: 구현 → `icon-dpr-bad=0` GREEN. **소수 픽셀 주의:** `blank()` 는 `lround(px*dpr)`(18×1.25=22.5 → 23). Qt 가 23 을 22 로 줄여 돌려주면 픽셀 비교가 실패한다 — 그때는 비교를 느슨하게 하지 말고 돌려받은 크기를 build.md 에 적은 뒤 `blank()` 크기를 Qt 의 요청 크기와 맞추는 쪽으로 고치고 판정을 남긴다. `QIcon::pixmap(const QSize&, qreal, Mode, State)` 는 쓰기 전에 context7 로 다시 확인(아래 출처). DESIGN_SPEC §2.1 문구(§2 표). 커밋.

**Task 4 — 안내 칩 다시 굽기** · Files: `app/guideband.hpp` · `app/guideband.cpp`
- Produces: `protected: bool event(QEvent* e) override;` — `e->type() == QEvent::DevicePixelRatioChange` 면 아이콘을 `kerf::glyph(iconName_, 16, theme::Hand, devicePixelRatioF())` 로 **무조건** 다시 굽고 구운 배율 기억, 그다음 `QFrame::event(e)`. `place()` 는 구운 배율 ≠ `devicePixelRatioF()` 일 때만 다시 굽는다. 아이콘이 없으면(`iconName_` 빈 값) 아무것도 안 함.
- Step: `dpr-rerender=1` GREEN · `--ctx-shot … guide=1` 그대로. 커밋.

**Task 5 — 창 줄일 때 자동 맞춤** · Files: `app/sectionview.hpp/.cpp` · `app/planview.hpp/.cpp`
- Produces: 두 클래스에 `bool fitted_ = false;`. **켜는 곳:** `SectionView::fit()`(결과가 있을 때), `PlanView::fitAll()`(상자가 유효할 때 — `setScene` · `setStream` · `homeView` 가 이것을 부름). **끄는 곳(사용자가 바꾼 보기):** SectionView `wheelZoom`(굴린 순간) · `applyZoomAt` · `zoomBy`(→ `setScreenDenom` 포함) · 끌기 이동(`mouseMoveEvent` 의 panning), PlanView `wheelZoom`(굴린 순간) · `applyZoomAt` · `zoomBy` · `setCamera`(저장된 카메라 복원 `mainwindow_ui.cpp:1136` 포함) · `setViewPitch` · 끌기 이동 · 돌리기(`mouseMoveEvent` 의 panning · orbiting). **바꾸지 않는 곳:** `SectionView::setVerticalExaggeration`(fit 이 세로 과장을 반영하므로 맞춤 상태 유지) · `PlanView::topView` · 단면선 끌기 · 그리기.
- `SectionView::resizeEvent`(50행): `fitted_` 이면 `fit()`, 아니면 지금처럼 가운데 유지. `PlanView`: **`void resizeEvent(QResizeEvent* e) override`** 를 더해 `QOpenGLWidget::resizeEvent(e)` 를 먼저 부른 뒤 `fitted_` 이면 `fitAll()`, 아니면 `updateMatrices()`(GL 이 없는 offscreen 에서도 불리는 길 — `resizeGL` 만으로는 offscreen 감사가 아무것도 시험하지 못함). `resizeGL` 은 그대로 둬도 됨.
- Step: `section-fits=1 plan-fits=1 zoom-kept=1` GREEN(1920 이상). 커밋.

**Task 6 — CI** · Files: `.github/workflows/ci.yml`
- Step: AC8 ①–③ 대로 고침 → 로컬 모의 실행 양성(둘 다 exit 0 · RESULT ok) · 음성(없는 모델 → throw) 결과를 build.md 에. YAML 문법 검사기는 이 PC 에 없음(DEV_PROGRESS 기록) — 들여쓰기는 사람이 diff 로 본다. 커밋.

**Task 7 — 전체 검증 · 캡처** · V8 PASS(AC5) → AC6 실행 → §4 회귀 명령 → AC9 캡처 → AC10 실제 화면 → `build.md`(판정 · RED/GREEN 줄 · context7 출처). 커밋.

## 4. 회귀 시험 계획
- **새로 쓸 시험(파일 · 이름 · 먼저 실패해야 하는 이유):** Catch2 새 시험 없음(core 변경 없음). 앱 판정 = `app/mainwindow_ui.cpp` `MainWindow::uiAudit` 의 줄 셋(§3-A):
  - `ui-audit env …` — 지금 코드에 없는 판정. 1280 요청이 1591 로 열림(실측) · 1.25 장 없음 → RED.
  - `ui-audit guide-step2 … dpr-rerender=` — `GuideBand` 에 `DevicePixelRatioChange` 처리가 없음 → RED.
  - `ui-audit narrow … plan-fits= section-fits= zoom-kept=` — `SectionView::resizeEvent` · `PlanView::resizeGL` 이 축척을 그대로 둬 줄이면 잘림 → RED(fits 둘). `zoom-kept` 는 지킴 시험(처음부터 1).
  - RED 보는 법: Task 1 명령(크기 둘 × 배율 1)이 `RESULT FAIL`, 해당 ui-audit 행 `exit=9`. 고친 뒤 V8 이 PASS.
- **돌려야 하는 기존 시험(이름 · 태그)** — worktree 빌드 exe = `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe`, PATH 앞에 `C:\Qt\6.10.3\msvc2022_64\bin`, 자동 실행은 `Start-Process -PassThru` + `WaitForExit(240000)`(R7.2), 설정은 새 폴더:
  1. `asec_tests.exe "~CoalescingWorker*"`(V8 의 tests(quick) 줄 — 140건 중 CoalescingWorker 4건 제외, `--list-tests` 로 확인). core 변경이 없으므로 전체 시험은 필요 없음(바뀌면 verify 가 스스로 전체로 감).
  2. `asec-section` 기준값 `polylines=1 vertices=54`(V8 의 asec-section 줄).
  3. `SectionViewer.exe --icon-sheet C:\dev\tmp\b5\icons.png --quit` → 종료 코드 0 · `missing=0`(아이콘 목록이 안 깨짐).
  4. `SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\tmp\b5\s1 --sheet-check C:\dev\tmp\b5\sheet --quit` → 종료 코드 0 · `RESULT ok`(`paintSectionDoc` 은 안 바꾸지만 단면 화면 크기 · 맞춤이 바뀌므로).
  5. 같은 모델 · 선으로 `--ctx-shot C:\dev\tmp\b5\draw.png --settings C:\dev\tmp\b5\s2 --quit` → `ctx-shot ok: … guide=1`(main.cpp:743 형식).
  6. 기존 감사 항목(tabs · ribbon · ribbon-context · dup · context-home · filter-persists · scale-controls · status-badge · compass-hit · draw-step2-note · next-hint · guide-band · sheet · sheet-discard · lists-match · square-borders · icons-missing)은 V8 의 `RESULT ok` 에 포함.
- **수치 허용오차:**
  - `contentFits` 경계 비교 `1e-6 m` — profile §7 「단면 끝점 s 좌표 · 실좌표 변환 1e-6 m」(`tests/test_pipeline.cpp:38` `Approx(0).margin(1e-6)` · `tests/test_cutplace.cpp:38` `< 1e-6`)과 같은 값 · 같은 단위(m). 맞춤 상태의 여유는 단면 L 의 1.5 % · 평면 20 px + 4 % 라 이 값은 경계 처리일 뿐 판정을 좌우하지 않는다.
  - `zoom-kept`: 축척 값은 **같아야 함**(허용오차 없음 — 맞춤 상태가 아니면 resize 길이 `ppm` · `mpp` 를 다시 계산하지 않는다).
  - 아이콘: 크기 · 픽셀 **완전히 같음**. dpr ↔ `QT_SCALE_FACTOR`: Qt `qFuzzyCompare`(부동소수 같음 판정, 새 영역 허용오차 아님).
  - 그 밖은 픽셀 · 참/거짓 판정. 새 허용오차 없음 → 사용자 확인 불필요.

## 5. 위험 · 되돌리기
- **위험:**
  - **offscreen 에서는 평면 GL 이 안 뜬다**(근거: `C:\dev\tmp\ciaudit.log` `stream: first-draw-ms=-1 idle-ms=60000 nodes=0` — 감사 1회 62–66 s 중 60 s 는 `main.cpp` 의 첫 그림 기다림, 실제 화면 `C:\dev\tmp\audit.log` 는 `first-draw-ms=103`). 그래서 평면 맞춤은 `resizeEvent` 길에 두고 판정은 지금 크기로 재야 한다(§3-B Task 1 · 5). 실제 화면 확인은 AC10 뿐.
  - **CI 가 처음으로 실제 판정**: 지금 CI 감사 단계는 GUI exe 를 `&` 로 불러 기다리지 않아 판정하지 않는다(CLAUDE.md 「로그에 `ui-audit RESULT` 없이 6초 만에 통과」). 브리프의 「`$LASTEXITCODE` 로 판정」은 이 때문에 동작하지 않으므로 `Start-Process` 로 바꾼다(판정). push 뒤 GitHub 러너(Qt 6.8.3 · MSVC · 글꼴 다름)에서 지금까지 숨어 있던 실패가 보일 수 있다 — 그때는 되돌리지 말고 원인을 따로 작업으로.
  - **Qt 판 차이:** `QIcon::pixmap(size, dpr)` 는 Qt 6.8 에서 크기 해석이 바뀌었다(context7 주). 이 PC 6.10.3 · CI 6.8.3 둘 다 6.8 이상이지만 `icon-dpr-bad` 가 CI 에서만 다를 수 있다.
  - **본 저장소와 같은 파일:** `app/mainwindow_ui.cpp` 등 B5 대상 파일을 본 저장소 v5 세션이 동시에 고치면 병합 충돌. 사용자 선택 「B5 부터 팩토리로」에 따라 이 작업이 끝날 때까지 본 저장소는 이 파일들을 고치지 않는다(지금 본 저장소 `app/` 미커밋 변경 없음, `git status` 확인).
  - **남은 위험(범위 밖):** 홈 앱 아이콘 56 px · 「작업 순서」 체크 표시는 화면을 옮겨도 다시 굽지 않음(흐릴 수 있음) · 첫 실행 기본 창 크기 1600×950(`mainwindow_ui.cpp:1628`)이 작은 모니터보다 클 수 있음(열린 질문 Q1) · DEV_RULES R2.7 문구는 병합 뒤 본 저장소에서.
- **확인하지 못함:** Qt 가 창의 배율이 바뀔 때 `DevicePixelRatioChange` 를 **자식 위젯**(GuideBand)에까지 보내는지 — context7 문서에는 「QWindow 가 받는다」만 있음(설치된 헤더에 `QWidgetWindow::handleDevicePixelRatioChange()` 비공개 선언만 확인). 그래서 AC3 에 `place()` 안전판을 넣었다. 실제 모니터 둘로 옮겨 보는 시험은 이 PC 에 모니터가 몇 대인지 몰라 미정.
- **되돌리기:** 브랜치 `factory/20261009-01-b5-dpi` 를 병합하지 않으면 끝. 병합 뒤라면 `git revert -m 1 <병합 커밋>`.

## 6. 열린 질문 (사용자에게 물을 것)
- **Q1. 처음 켤 때 창 크기가 모니터보다 크면 화면 안으로 줄일까요?** 지금 첫 실행 크기는 1600×950 고정이라 1366×768 같은 작은 노트북 화면에서는 창이 화면 밖으로 나갈 수 있습니다. 이번 감사는 `--size` 로 크기를 정해서 열기 때문에 이 경우를 시험하지 못합니다.
  - (추천) 이번 작업에는 넣지 않고 다음 작업 목록(backlog)에 따로 올린다 — 이번 작업은 검증할 수 있는 것만 담아 빨리 끝내고, 그 일은 작은 모니터 확인 방법과 함께 따로 한다.
  - 이번 작업에 넣는다 — 「첫 실행 크기 = 1600×950 과 화면 사용 가능 영역 중 작은 쪽」 한 줄이 늘고, 확인은 코드 읽기와 사람 확인으로만 한다.
  - **닫힘(2026-10-09 21:52, 사용자 답 AskUserQuestion):** 「이번에는 빼고 대기 목록에」 → `factory/backlog.md` #2. 이번 작업 범위 밖.

---
**외부 API 확인(context7 · 헤더):**
- `QEvent::DevicePixelRatioChange`(=222, Qt 6.6+): 출처 context7 `/websites/doc_qt_io_qt-6_8` 「QEvent::Type DevicePixelRatioChange — The devicePixelRatio has changed for this widget's or window's underlying backing store」(브리프에서 확인) + 「QWindow::devicePixelRatio … The QWindow instance receives an event of type QEvent::DevicePixelRatioChange when the device pixel ratio changes」(이번 질의 「DevicePixelRatioChange delivered to QWidget children…」). `QWidget::changeEvent` 문서의 사건 목록에는 없음 → `event()` 재정의로 받는다. 헤더 `C:\Qt\6.10.3\msvc2022_64\include\QtCore\qcoreevent.h:291`.
- `QIcon::pixmap(const QSize&, qreal devicePixelRatio, Mode, State)`(Qt 6.0+): 출처 context7 `/websites/doc_qt_io_qt-6_8` 질의 「QIcon::pixmap … devicePixelRatio … how it picks」 — 「The requested devicePixelRatio might not match the returned one. This delays the scaling of the QPixmap until it is drawn later on」 · 「since Qt 6.8 it's the device independent size」. 헤더 `qicon.h:205`. **어느 장을 고르는지(가장 가까운 배율 규칙)는 문서로 확인하지 못함** — builder 는 쓰기 전에 다시 확인하고 Task 3 의 실측 크기로 판단한다.
- `QOpenGLWidget::resizeEvent(QResizeEvent*)` 는 protected override 로 재정의 가능 — 헤더 `C:\Qt\6.10.3\msvc2022_64\include\QtOpenGLWidgets\qopenglwidget.h:74`(context7 미질의, 헤더로 확인).
