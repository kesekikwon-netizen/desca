# 조판 탭 바로잡기와 잠금 확인 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 평면도·단면도 조판이 「사진은 칸을 가득, 숫자는 칸 밖, 휠은 간격만, 조판편집은 그림만」으로 동작하고, 그것을 명령줄 확인(`--sheet-check`)이 캡처와 기하 판정으로 잠근다.

**Architecture:** 평면도는 「왼쪽 화면 범위를 칸에 맞춰 줄이기」 대신 「축척 × 칸 크기 = 지상 창」을 그린다(맞춤 축척 10 단위로 시작). 단면도는 조판 확대를 그림 배율(`viewZoom`)이 아니라 변환(`SectionXf`의 ppm·s0·zTop)으로 바꿔, 작업 화면 단면 휠과 같은 길로 레벨선·숫자·영상 자르기를 다시 계산한다. 그리는 동안 확인용 기록(`SheetProbe`)을 모아 `main.cpp`가 판정한다.

**Tech Stack:** C++17, Qt 6.8.3 Widgets(MSVC 2022, `/utf-8`), Catch2(`asec_tests`), 빌드 `D:\des\build-msvc`(Ninja).

**Spec:** 시안 `research/ui-ref/board/section-viewer-design.html` 4장·8장, 요청 기록 `docs/intent/2026-10-06-plan-drawing-export.md`, 제품 규칙 `.cursor/rules/product-rules.mdc`.

**진단(코드를 읽어 확인, 2026-10-06):**
- 글자를 늘이던 `QPainter::scale`은 지금 소스에 없음.
- 평면도 `paintPlanSheet`는 왼쪽 화면 범위(`viewRectLocal`)를 칸에 「맞춰 줄여」 그림 → 칸과 비율이 다르면 좌우 빈 띠, 범위가 크면 표기 축척과 실제 크기가 어긋남. 확대는 `contain` 위로 못 커짐.
- 단면도 `paintSectionDoc(viewZoom)`은 그림만 키우고 레벨선·숫자 범위(`zVis0..zVis1`)와 간격(`sectionLevelPlan(ppmZ)`), 영상 자르기는 확대 전 기준 → 축소하면 위아래 선·숫자가 비고, 이동·확대 때 영상이 잘려 사라질 수 있음.

## Global Constraints

- 평면도와 단면도는 같은 바깥 도곽: 테두리, 표제란, 축척 막대, 진북 나침반, 범례 자리가 같다. 그림 칸만 다르다. → 두 조판 모두 `SheetSpec` 기본 여백(왼 17 · 오 15 · 위 9 · 아래 11 · 표제 띠 24 mm)을 쓴다.
- 평면도 그림 칸: 정사영상만. 단면선 없음. 나침반 필수.
- 축척: 10 단위, 상한 없음, 평면·단면 값은 각각 저장(`plansheet/denom`, `sheet/denom`). 위·아래 화살표 한 칸 = 10.
- 평면 범위 기본: 지금 왼쪽 화면. 모델 전체는 선택. 평면 GeoTIFF는 그대로.
- 좌표 = 메시 로컬 + SRSOrigin. 높이는 파일 값 그대로. 지오이드 재적용·자북·경계 GPS 만들어 넣기 금지.
- 글자에 가로·세로 배율을 따로 먹이지 않는다(`QPainter::scale(1, z)` 금지). 간격만 바뀐다.
- 조판은 메인 영역의 탭(`showSheetTab`). 별도 OS 창·전체화면 금지. 작업 화면 오른쪽 단면 휠 동작을 깨지 않는다.
- 커밋·푸시·배포(바탕화면 실행 파일 교체)는 사용자가 말할 때만. 그래서 각 Task 끝은 커밋 대신 「확인」.

## Review Focus

- 모델 밖까지 그림 칸이 넓을 때: 정사영상이 없는 곳은 종이색(#F4F1EA)으로 채우고 흰 띠로 보이지 않는다 → Task 2 판정 `image-fills-plot`은 「그린 창 = 칸」으로 본다(모델 밖 종이색은 실패 아님).
- 3D로 많이 기울인 왼쪽 화면: `viewRectLocal`이 거대한 상자를 낼 수 있다 → Task 3에서 범위를 모델 상자로 자른 뒤 맞춤 축척 계산.
- 휠을 끝까지(8배·0.35배): 숫자 간격 단계가 1-2-5로 바뀌어도 숫자가 칸 안으로 들어오지 않는다 → Task 2 판정을 +3·−3에서 모두 돌린다.
- 조판편집으로 그림을 많이 옮겼을 때: 숫자는 칸 밖 여백에만, 도곽은 제자리 → 판정 `move=12mm`.
- 납작하거나 긴 단면: 단면도 확대에서 영상이 사라지지 않는다 → Task 4 판정을 도랑 단면선(A–A′ 2.52 m, 기복 12 cm)으로도 돌린다.

---

### Task 1: 평면 창 · 맞춤 축척 · 눈금 간격 (코어, 시험 먼저)

**Files:** Modify `core/include/asec/sheet.hpp`, `core/src/sheet.cpp` · Test `tests/test_sheet.cpp`

**Interfaces — Produces:**
- `double fitScaleDenom10(const SheetSpec& s, double wM, double hM);` — `wM × hM`(m)가 그림 칸에 다 들어가는 가장 작은 10의 배수 분모. 0·비수·음수는 10.
- `struct PlotWindow { double cx = 0, cy = 0, wM = 0, hM = 0; double x0() const; double x1() const; double yTop() const; double yBot() const; };` (가운데 ± 반폭)
- `PlotWindow plotWindow(const SheetLayout& L, double denom, double cx, double cy, double zoom, double dxMm, double dyMm);` — 창 = 칸 × denom/1000/zoom. 그림을 오른쪽(dxMm>0)으로 옮기면 창 가운데는 −dxMm·(denom/1000/zoom), 아래(dyMm>0)면 +dyMm·(…).
- `double tickStepForGap(double mPerMmPaper, double minGapMm);` — 1·2·5×10ⁿ 중 `step / mPerMmPaper ≥ minGapMm`인 가장 작은 값.

- [ ] **Step 1: 실패하는 시험** — `TEST_CASE("sheet: plan window fills the plot at a 10-step scale", "[sheet]")`

```cpp
SheetSpec s;                                   // A4 가로, 기본 여백 → 그림 칸 245 × 146 mm
SheetLayout L = layoutSheet(s, 1, 1);
CHECK(L.plotW == Approx(245)); CHECK(L.plotH == Approx(146));
CHECK(fitScaleDenom10(s, 24.5, 14.6) == 100);
CHECK(fitScaleDenom10(s, 24.6, 10.0) == 110);
CHECK(fitScaleDenom10(s, 26.9, 16.1) == 120);  // 16.1 m / 146 mm = 1:110.3
CHECK(fitScaleDenom10(s, 0, 0) == 10);
PlotWindow w = plotWindow(L, 100, 1000, 2000, 1.0, 0, 0);
CHECK(w.wM == Approx(24.5)); CHECK(w.hM == Approx(14.6));
CHECK(w.x0() == Approx(1000 - 12.25)); CHECK(w.yTop() == Approx(2000 + 7.3));
CHECK(plotWindow(L, 100, 1000, 2000, 2.0, 0, 0).wM == Approx(12.25));
PlotWindow m = plotWindow(L, 100, 1000, 2000, 1.0, 10, -5);
CHECK(m.cx == Approx(999.0)); CHECK(m.cy == Approx(1999.5));
CHECK(tickStepForGap(0.11, 28) == Approx(5));
CHECK(tickStepForGap(0.055, 28) == Approx(2));
CHECK(tickStepForGap(0.02, 28) == Approx(1));
```

- [ ] **Step 2: 실패 확인** — `cmake --build D:\des\build-msvc --target asec_tests` → 함수 없음으로 컴파일 실패
- [ ] **Step 3: 구현** — `fitScaleDenom10` = `max(10, ceil(fitDenominator(s, w, h) / 10 − 1e-9) × 10)`
- [ ] **Step 4: 통과 확인** — `asec_tests.exe "[sheet]"` → All tests passed

---

### Task 2: 조판 확인 장치 `--sheet-check` (지금 코드로 실패를 먼저 본다)

**Files:** Create `app/sheetprobe.hpp` · Modify `app/sheetexport.cpp`, `app/sectionview.cpp`, `app/mainwindow.hpp`, `app/main.cpp`

**Interfaces — Produces:**
- `struct SheetProbe { QRectF plot, image; std::vector<QRectF> outsideLabels; std::vector<double> labelPos; QStringList labelTexts; QRectF compass, legend, title; bool nonUniformText = false; int fontPx = 0; };` · `inline SheetProbe* g_sheetProbe = nullptr;` — 그리는 쪽은 `if (g_sheetProbe)`일 때만 채움. 바깥 숫자를 그릴 때 `p.worldTransform()`이 `m11 != m22` 또는 `m12, m21 != 0`이면 `nonUniformText = true`.
- `MainWindow`: `bool openSheetTab(bool plan);` · `QWidget* sheetPreview() const;`(objectName `sheetPreview`) · `int sheetScaleStep() const;`(열린 조판 축척 칸의 `singleStep()`)
- 명령줄 `--sheet-check DIR`: 평면·단면 각각 [확대 1 · 휠 +3 · 휠 −3 · 조판편집 이동 12 mm]를 `DIR/sheet_<kind>_<state>.png`로 찍고 줄마다 판정, 끝에 `sheet-check RESULT ok|FAILED`. 휠은 실제 `QWheelEvent`를 `sheetPreview()`에 보냄.

- [ ] **Step 1: 판정 규칙(시안 8장 C표)**
  - `image-fills-plot`(평면): `probe.image ⊇ probe.plot`(1 px 오차)
  - `labels-outside-plot`: 모든 `outsideLabels` ∩ `plot` 넓이 = 0
  - `text-uniform`: `!nonUniformText`
  - `spacing-grew`(휠 +3): 이웃 `labelPos` 간격 가운데값 > 확대 1의 값, `fontPx` 같음
  - `image-visible`·`cut-red-visible`(단면): 캡처의 칸 안 흰색 아닌 화소 > 5 %, 순수 빨강(R≥250, G·B≤8) > 0
  - `line-on-plan=0`(평면): 칸 안 순수 빨강 = 0
  - `chrome-fixed`(이동 12 mm): 나침반·범례·표제란 상자 그대로 · `label-values-changed`: `labelTexts` 다름
  - `scale-step`: 평면·단면 조판 모두 `sheetScaleStep() == 10`
- [ ] **Step 2: 빌드 후 지금 코드로 실행해 실패 확인**

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake --build D:\des\build-msvc --target SectionViewer
```
실행(`ProcessStartInfo.Arguments`, 모델 경로 따옴표): `SectionViewer.exe "E:\new project jeju1\Productions\Production_2\Scene\Production_2.3mx" --settings %TEMP%\sv-check --line 148083.065 98121.10 148084.595 98118.52 --back 0.5 --sheet-check %TEMP%\sv-check\out --log %TEMP%\sv-check\log.txt --quit`
Expected: `image-fills-plot=0` 등과 `RESULT FAILED`. 캡처를 열어 눈으로도 확인.

---

### Task 3: 평면도 = 축척 × 칸 크기의 지상 창

**Files:** Modify `app/sheetexport.cpp`(`planWindow`, `paintPlanSheet`, `writePlanSheetDxf`, `exportPlanSheet`, `buildPlanSheetDialog`, `defaultPlanSheetParams`), `app/mainwindow.hpp`(`PlanSheetParams`에 `double cx = 0, cy = 0;` 창 가운데·로컬 m)

**Consumes:** Task 1 세 함수, Task 2 `g_sheetProbe`

- [ ] **Step 1:** 평면 조판의 여백 덮어쓰기(왼 32 · 아래 16)를 지우고 단면과 같은 기본값.
- [ ] **Step 2:** 열 때: 범위(왼쪽 화면 / 모델 전체)를 모델 상자로 자르고 `cx, cy` = 가운데, 축척 = `fitScaleDenom10`. 「맞춤」 단추 = 같은 계산. 축척을 바꾸면 창만 다시(범위 그대로).
- [ ] **Step 3:** `paintPlanSheet(..., zoom, panXmm, panYmm)`: 창 = `plotWindow(L, denom, cx, cy, zoom, imgDxMm + panXmm, imgDyMm + panYmm)`. 칸을 종이색으로 칠하고 영상을 창 좌표로(칸으로 자름). 바깥 숫자: X 아래, Y 왼쪽·오른쪽, 간격 `tickStepForGap(denom/1000/zoom, 24)`, 글자 크기 고정. 확인 기록 채움.
- [ ] **Step 4:** 미리보기 영상은 창 가운데 기준 3배 넓이를 긴 변 1600 px로 렌더(확대·이동·0.35배 축소를 덮음). 창이 그 밖으로 나가면(축척 변경·조판편집 놓음) 다시 렌더.
- [ ] **Step 5:** 내보내기(PDF/PNG/TIFF/DXF)는 확대 1, `imgDx/Dy`만 반영한 창을 출력 해상도로. 나눠 붙이기는 범위를 창 크기로 나눔.
- [ ] **Step 6: 확인** — Task 2 명령 → `sheet-check plan …` 모두 1, 캡처에서 사진이 칸을 가득 채우고 좌표가 칸 밖.

---

### Task 4: 단면도 조판 확대 = 변환을 다시 계산

**Files:** Modify `app/sheetexport.cpp`(`paintSheet`), `app/sectionview.cpp`·`.hpp`(`paintSectionDoc`의 `contentDx/Dy`·`viewZoom`·`viewPan` 인자 제거)

- [ ] **Step 1:** `paintSheet(..., zoom, panXmm, panYmm)`: 칸 가운데를 고정점으로 `xf.ppm *= zoom`, `s0`·`zTop`을 가운데 유지로 다시 계산. `imgDxMm + panXmm`, `imgDyMm + panYmm`은 `s0 −= dx·m/mm`, `zTop += dy·m/mm`로 접어 넣음. 영상 자르기는 이 변환의 보이는 거리 범위로.
- [ ] **Step 2:** `paintSectionDoc`는 `xf`만 씀(작업 화면 호출 그대로). 표고·거리 숫자 상자를 확인 기록에.
- [ ] **Step 3: 확인** — Task 2 명령 → `sheet-check section …` 모두 1. 도랑 단면선(`--line 148081.4674 98119.9055 148082.7513 98117.7402 --back 2`)으로 한 번 더. `--wheel-test`의 `wheel-section … anchor-drift-px`가 지금 수준(1 px 안).

---

### Task 5: 미리보기 조작 정리 · 축척 화살표

**Files:** Modify `app/sheetexport.cpp`(`SheetPreview`, `scaleSpin`, `sheetEditButton`, `bindImageDrag`)

- [ ] **Step 1:** 휠 = 칸 안 확대(커서 아래 고정, 0.35–8배) · 왼쪽 끌기 = 미리보기 이동(저장 안 됨) · 조판편집 중 가운데 끌기 = 그림 이동(`imgDx/DyMm`, 저장) · 「닫기」 = 고정. 확대·이동은 mm로 그리기 함수에 넘김.
- [ ] **Step 2:** 축척 칸 두 개 `setSingleStep(10)` 유지(판정 `scale-step`).
- [ ] **Step 3: 확인** — Task 2 명령 전체 → `RESULT ok`, 캡처 8장을 열어 본 것만 보고.

---

### Task 6: 끝나기 전 확인과 기록

- [ ] `asec_tests.exe "[sheet]"`, `"[orbit]"` 통과
- [ ] `--sheet-check` RESULT ok, 캡처 8장 확인
- [ ] `--wheel-test`로 작업 화면 단면 휠 그대로
- [ ] `docs/intent/2026-10-06-plan-drawing-export.md` 「결과」에 실제 확인한 것만(커밋은 사용자 지시 때)
