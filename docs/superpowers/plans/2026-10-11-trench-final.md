# Trench 최종 디자인 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans (이 세션에서 직접 — 사용자 /goal 2026-10-11 「매 단계 커밋하고 다음으로 진행」). 작업마다 TDD(`superpowers:test-driven-development`), 끝나기 전 `superpowers:verification-before-completion`. 복잡한 판단(좌표계 · 도면 정확성 · 형식)은 advisor(Opus 5.5)에게 묻는다.

**Goal:** 캔버스 「Trench 최종 디자인」의 작업 지시서(G0–G20)를 코드로 옮겨 앱 이름 · 아이콘 · 여는 화면 · 화면 구성 · 커서 · 모션 · 도면을 최종 디자인과 같게 한다.

**Architecture:** 기존 두 층(core Qt 없음 / app Widgets) 그대로. 디자인 값은 `app/theme.hpp` 토큰 + 새 `app/motion.hpp` · `app/cursors.{hpp,cpp}` · `app/notice.{hpp,cpp}`. 모든 화면 변화는 `--ui-audit` 판정 줄을 먼저 RED 로 만든 뒤 고친다. 내부 식별자(`SectionViewer.exe` · QSettings `ExcavSection/SectionViewer` · 캐시 경로)는 바꾸지 않는다(Ruling 1).

**Tech Stack:** C++17 · Qt 6.10(이 PC, CI 6.8) Widgets · Catch2 · PowerShell 감사 스크립트 · Python(Pillow, 아이콘 포장만).

**Spec:** 캔버스 https://claude.ai/artifact/St8M2cwJYYfPW9gcNDshRQ (「Guide」 화판 = 작업 순서, 나머지 화판 = 값) · `docs/design/DESIGN_SPEC.md`(v5 + 오늘 추가) · `docs/REVIEW_2026-10-10.md` §9 · §10-0(사용자 결정 Q0–Q6) · 어긋나면 **캔버스가 이긴다**. 정본 대기 목록 `factory/backlog.md`(R-번호 ↔ G-번호 대응은 캔버스 Guide).

## Global Constraints

- 색 · 크기 · 글꼴은 `app/theme.hpp` 토큰만. 새 색 없음(여는 화면 흙 띠 `#D9B99B` · `#8F4229` 는 아이콘 · 여는 화면 전용).
- `Q_OBJECT` 금지(람다 연결 · `static_cast`). 계산은 `core/` 에 시험 먼저. `third_party/` 수정 금지. 바꾼 줄만 고친다.
- 글자 ≥ 12 px(키 캡 · 단위 11), 대비 ≥ 4.5:1, 둥글기 규칙(칸 6 · 판 8 · 칩 12.5), 흙색은 한 화면에 하나.
- 모든 시험 · 캡처는 **실제 모델 `C:\Users\권을\Downloads\Production_1.3sm`**(앞 0 · 뒤 5 m, 단면선 `194879.1388 574010.5874 194891.8146 573993.4063`) + 합성 모델 둘 다. 감사 행렬 = 1280×800 · 1920×1040 · 2560×1440 · 3440×1440 · 3840×2160 × 배율 1 · 2. 도구: 스크래치패드 `matrix.ps1` · `audit.ps1`(없으면 `factory verify` 또는 DEV_PROGRESS §2 명령).
- 커밋: `feature/design-v2`, 작업 하나 = 커밋 하나(바꾼 파일 이름으로만, `git add -A` 금지), 메시지 한국어 + `Co-Authored-By` 줄. **`design-v4/` · `dg/` · `image*.png` · `factory/jobs/20261010-03-audit-tighten/build.md` 는 올리지 않는다**(R1b 가 다른 세션에서 진행 중). push · 병합 · 배포는 사용자가 말할 때만.
- 코드 검토자 에이전트는 **G0–G20 전부 끝난 뒤 한 번**(사용자 지시). 중간에는 감사 · 시험 · 캡처 눈 확인만.
- Qt · Win32 API 는 쓰기 전에 context7(`/websites/doc_qt_io_qt-6_8`) 또는 Microsoft Learn MCP 로 확인한다.
- 줄바꿈: 파일의 기존 줄바꿈(CRLF)을 유지(스크립트로 고칠 때 `newline=""`).

## Review Focus

- 옛 설정(최근 파일 · 단면 목록)이 이름을 바꾼 뒤에도 그대로 보인다 → 설정 키를 안 바꾸고 감사에서 `QSettings` 경로 불변을 확인.
- 창 제목이 표시 이름으로 끝나지 않으면 Qt 가 「 - 표시 이름」을 덧붙인다(1.2.0 신고) → `window-title … appended=0` 유지.
- 여는 화면이 느린 PC 에서 1 초보다 길어져 마지막 장면에 머무를 때 멈춘 것처럼 보이지 않는다 → 「앱을 준비하고 있습니다…」 문구 유지.
- `.ico` 를 바꿔도 Ninja 가 리소스를 다시 안 빌드한다 → exe 아이콘이 실제로 바뀐 것을 확인.
- 16 px 아이콘이 4 K · 배율 200 % 에서 흐려지지 않는다 → 작은 변형 + dpr 별 렌더.

---

### Task 1 (G0a): 사용자에게 보이는 이름 Kerf → Trench

**Files:**
- Modify: `app/main.cpp`(표시 이름 · 머리 주석), `app/mainwindow.cpp`(창 제목 3곳 · 단면 PNG 바닥글 · 정보 창 · 여는 화면 글은 Task 3 에서), `app/mainwindow_ui.cpp`(홈 이름 라벨 · 창 제목 · `uiAudit` 새 줄), `app/sheetexport.cpp`(SVG · PDF 제작자 2곳), `app/sm3convert.cpp`(임시 폴더 이름 주석만 — 경로는 그대로), `packaging/app.rc`(CompanyName · FileDescription · ProductName), `packaging/installer.nsi`, `packaging/읽어보기.txt`, `README.md` 머리, `core/src/{export,pointcloud,sheet,tiff}.cpp` 의 소프트웨어 표시 글(있으면)
- Test: `app/mainwindow_ui.cpp` 의 `uiAudit` — 새 판정 줄 `ui-audit app-name title=… display=… home=…`

**Interfaces:**
- Produces: 사용자에게 보이는 앱 이름은 모두 `Trench`. 버전 `kVersion`(1.2.1) · 실행 파일 이름 · QSettings 이름 · 캐시 경로 불변. 이후 작업은 이 이름만 쓴다.

- [ ] **Step 1: 감사 줄을 먼저 쓴다(RED).** `uiAudit` 에 `app-name`: 창 제목(`windowTitle()`) · `QApplication::applicationDisplayName()` · 홈 이름 라벨(`objectName=heroName`) 글이 모두 `Trench` 를 포함하고 `Kerf` 를 포함하지 않으면 ok. 실패 줄에 `  FAIL`.
- [ ] **Step 2: 빌드 + 합성 모델 감사 실행 → `app-name … FAIL`(EXIT 9) 확인.**
- [ ] **Step 3: 위 Files 의 글을 바꾼다.** 코드 식별자 · 설정 키 · 캐시 경로 · 파일 이름 · 문서 안 옛 기록(DEV_PROGRESS 옛 항목)은 바꾸지 않는다. `core/` 의 `ExcavSection` 글이 파일 안에 쓰이는 값(GeoTIFF · LAS · DXF · SVG 헤더)이면 같은 길이 규칙(LAS 생성 소프트웨어 32 바이트)을 지켜 `Trench` 로, 시험 기대값이 바뀌면 커밋에 `시험 변경:` 줄.
- [ ] **Step 4: 빌드 · 빠른 시험(`asec_tests "~CoalescingWorker*"`) · 감사 행렬 → `app-name … ok`, `window-title … appended=0`.**
- [ ] **Step 5: 캔버스 두 곳 고침**(Brand 화판 글 · Missed #1: 「설정 이전」 → 「내부 이름은 그대로, 보이는 이름만 Trench」, 버전 1.3 → 현재 버전) — 캔버스가 코드와 모순되지 않게.
- [ ] **Step 6: 커밋** `이름 Kerf → Trench(보이는 글만)` + DEV_PROGRESS 기록 + backlog G0 행을 G0a/b/c 로 쪼갬.

### Task 2 (G0b): 앱 아이콘 「T는 트렌치의 단면」 — 한 SVG 에서 앱 · .ico 모두

**Files:**
- Create: `app/icons/app/trench.svg`(큰 것, 24 격자, 명시 색), `app/icons/app/trench-small.svg`(32 px 이하: 띠 둘 · 선 2.6), `packaging/make_app_ico.py`
- Modify: `app/icons/icons.cmake`(두 파일 추가), `app/icons.{hpp,cpp}`(이름 → 색 바꾸기 없이 그리기 `kerf::appIcon(int px, qreal dpr)`), `app/main.cpp`(`--app-icon DIR`, `QApplication::setWindowIcon`), `app/mainwindow_ui.cpp`(홈 왼쪽 위 56 px 아이콘), `app/mainwindow.cpp`(정보 창), `packaging/assets/app.ico`
- Test: `--app-icon DIR` 판정(`app-icon n=5 missing=0`, 종료 12), `--icon-sheet` 회귀

**Interfaces:**
- Consumes: Task 1 의 이름.
- Produces: `QIcon kerf::appIcon()`(16 · 24 · 32 · 48 · 256, dpr 1–3 픽스맵 포함) — Task 3(여는 화면)이 같은 SVG 로 큰 타일을 그린다.

- [ ] **Step 1: `--app-icon` 판정을 먼저 쓴다(RED)** — 다섯 크기 PNG 를 DIR 에 쓰고 하나라도 비면 종료 12. 아이콘 파일이 없으니 지금은 FAIL.
- [ ] **Step 2: SVG 두 개를 쓴다.** 값은 캔버스 Brand 화판의 작도: 24 격자, 지표 y=8, 슬롯 x 9–15 · 바닥 y=19, 띠 `#E3DACC`(8–12.5) `#D9B99B`(12.5–16.5) `#B5573A`(16.5–21) `#8F4229`(21–24), 바탕 `#FAF9F5`, 빨간 선 `#FF0000` 폭 2.2 · 둥근 이음. 작은 변형: 띠 둘(`#D9B99B` 8–14, `#B5573A` 14–24), 선 2.6, 16 px 용은 슬롯 8.5–15.5 · 선 3.2.
- [ ] **Step 3: `appIcon` · `setWindowIcon` · 홈 56 px · 정보 창에 연결.**
- [ ] **Step 4: `make_app_ico.py`**(Pillow): `--app-icon` 이 낸 PNG 를 모아 `app.ico`(256 · 48 · 32 · 24 · 16) — 도형을 파이썬에서 다시 그리지 않는다.
- [ ] **Step 5: 확인** — `--app-icon` ok, `--icon-sheet` ok, 8조합 감사 ok, exe 아이콘을 실제로 바뀐 리소스로 다시 빌드(`app.rc` touch)해 탐색기용 크기 PNG 로 눈 확인, 16 · 32 px dpr 1 · 2 캡처.
- [ ] **Step 6: 커밋.**

### Task 3 (G0c): 여는 화면 「T를 판다」 1.0 초

**Files:**
- Modify: `app/mainwindow.cpp`(`paintOpeningSplash` 다시 씀), `app/main.cpp`(`OpeningSplash`: 1000 ms 상수 한 곳 · Esc/클릭 건너뜀 · Windows 애니메이션 끔이면 마지막 장면 300 ms · `--splash-shot` t = .12 · .40 · .70 · 1.0 · `--splash-live` 1000 ms)
- Test: `--splash-shot` 판정 줄 `splash-shot frames=4 total-ms=1000`

**Interfaces:**
- Consumes: `kerf::appIcon` SVG(Task 2).
- Produces: `constexpr int kSplashMs = 1000`(main.cpp 한 곳).

- [ ] **Step 1: 판정 줄을 먼저 쓴다(RED)** — 지금은 `frames=3 total-ms=5000`.
- [ ] **Step 2: 장면**(캔버스 Splash 화판): 타일 160 px at (110, 180), 땅 `#E3DACC` 53–83 · `#D9B99B` 83–110 · `#B5573A` 110–140 · `#8F4229` 140–160, 시간표 지표선 0–.12 → 파기 .12–.50(OutCubic) → 단면선 .45–.85 → 「rench」 .55–.80(24 px 왼쪽에서 들어옴, 명조 230, 기준선은 `QFontMetrics` 로 타일 아래 모서리에 맞춤) → 부제 .80–1.0, 아래 띠 72 · 진행 막대 3 px `#B5573A`, 상태 글 「앱을 준비하고 있습니다」 → 「잠시 후 화면이 열립니다」(.30) → 「준비 완료」(.85), 아래 글 「원본 파일은 바꾸지 않습니다 · Esc = 건너뜀」 · 「© 2026 Trench · Qt 6.8」.
- [ ] **Step 3: Win32 `SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION)`** 사용 전에 Microsoft Learn 으로 확인, `Q_OS_WIN` 가드.
- [ ] **Step 4: 확인** — `--splash-shot` 네 장 눈 확인(offscreen 은 글자가 □ 이므로 실제 화면 `--splash-live`), 실제 모델 파일로 열 때 여는 화면이 없는 것, 8조합 감사.
- [ ] **Step 5: 커밋.**

---

## 이후 작업 (캔버스 Guide 화판 G1–G20 — 각 작업 시작 때 이 형식으로 펼친다)

| 작업 | 범위 | 완료 기준 | 사람 검토 |
| --- | --- | --- | --- |
| T4 = G1 (R6) | `core/src/srs.cpp` 매개변수가 이름을 이김 · 상태줄 배지 글 · 알림 띠 삭제 | Catch2 East Belt WKT(FN 600000) → 5187 · 경고 0 · 좌표 불변 1e-6 / 실제 모델 감사 `icons-missing=0` | **advisor 먼저**(좌표계) |
| T5 = G2 (R3) | 좌표 입력 · 단면 찾기 · 영어 병기 삭제 | `ribbon-corner=0 coord-entry=0 find-box=0 bilingual=0` · `dup 0` | |
| T6 = G3 (R4 나머지) | 평활 → 입면 아래 줄 · 정보 → F1 · 끝내기 Ctrl+Q · 더보기 삭제 · 기타 → 보기 · 점군 하나로 | `ribbon groups=6 more=0` · need50 기록 | |
| T7 = G4 (R5) | 오른쪽 판 삭제 → 목록 아래 수치 | `inspector=0 list-readout rows≥9` | |
| T8 = G5 (R7) | 높이 UI 삭제 | `status-badges=0 height-ui=0` · 표고 SVG 바이트 동일 | advisor |
| T9 = G6 (R8) | F1 창 · 표 = 실제 키 | `keys-table-match=1` | |
| T10 = G7 (R9) | 거리 눈금 + D 키 + 아이콘 `dist-ticks` · 도면 SVG | `dist-ticks …` · `--sheet-check` | |
| T11 = G8 | 커서 5종 `app/cursors.{hpp,cpp}` | `--cursor-sheet`(종료 11) · `cursor stage=…` | |
| T12 = G9 | 모션 상수 `app/motion.hpp` · 전환 | `motion … max-ms=300` | |
| T13 = G10 (R10) | 제거 · 숨김 25 | 행마다 grep · 감사 | |
| T14 = G11 (R11) | 도면 글 · 진북 화살(수렴각 core) · 「해발 표고」 | Catch2 수렴각 · `--sheet-check compass` | **advisor 먼저** |
| T15 = G12 (R12) | DXF 단위 · 단면선 0.35 mm | Catch2 `$INSUNITS` | advisor |
| T16 = G13 (R13) | SVG 층 나눔 · 밑그림 잠금 | `--sheet-check svg-layers=…` | **advisor 먼저** |
| T17 = G14 · G15 (R14 · R15) | 범례 기호 · 표제란 조사명 · 유구 번호 | `legend-items=4` · `title-fields=2` | |
| T18 = G16 (R18) | 저장 일부 실패 Notice | `saved=2 failed=1 notice-save-fail=1` | |
| T19 = G17 | 밋밋함 보완 8(그림자 3단 · 틀 안쪽 그림자 · 명조 이름 · 숫자 셀 · 빈 상태 모티프) | 캡처 루브릭 + 8조합 | |
| T20 = G18 (R2 · R19 · R20) | 기준 화면 2560 · 첫 창 | `-UiSizes …2560x1440,2293x912` | |
| T21 = G19 (R16 · R17) | 잘린 돌 H · 윤곽 O | 옛 #4–#7 기준 | |
| T22 = G20 (R21 · R22 · R23) | 한글 경로 · CI · 문서 합침 | `[cli]` 3 · Grep 대조 | |

**검토 한 번(전부 끝난 뒤):** code · correctness · security · UI 네 검토자 → 지적 반영 → 사용자 승인.
