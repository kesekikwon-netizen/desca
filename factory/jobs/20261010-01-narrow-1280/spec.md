# 명세 — 20261010-01-narrow-1280 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건

작성: spec-writer · 기준 커밋 308daac82e8e46189600488c209d43020e046cc2(worktree `C:\dev\kerf-wt\20261010-01-narrow-1280` 깨끗함 확인) · 2026-10-10

## 1. 목표 (1–3줄)
창 폭 1280(가운데 칸 ≈ 320–330 px)에서 단면 머리 · 정보 줄 · 평면 눈금 글 · 홈 한 줄 글이 겹치거나 반 토막 나지 않게 정해진 순서로 줄이고, 그것을 `--ui-audit` 판정으로 잠근다.
리본 1280 기대값은 사용자 결정(2026-10-09 「스펙을 실제(1280 → 20)로 고친다」)대로 스펙과 감사를 20 으로 맞추고, B5 검토의 should 7건(ⓐ–ⓖ)을 정리한다.
1920 이상 화면 모양은 바뀌지 않는다.

## 2. 변경 범위
- **바꾸는 파일:**
  | 파일 | 무엇을 | AC |
  | --- | --- | --- |
  | `app/mainwindow_ui.cpp` | 단면 머리 줄이기(`fitSectionHeader`) · 정보 줄 줄이기(`fitInfoStrip`) · 홈 한 줄 글 줄임표 + 툴팁 · 키 줄 · `uiAudit` 새 줄 · 좁은 창 블록(ⓔ) · 넘침 판정(ⓑ) · 칩 판정(ⓓ) | 1 2 4 6 7 8 9 |
  | `app/mainwindow.hpp` | 위 함수 · 멤버 선언 | 1 2 8 |
  | `app/planview.hpp` · `app/planview.cpp` | 눈금 글 간격 · 글 사각형 함수(그리기와 감사가 같이 씀), 행렬 도움 함수(ⓒ), `clearScene` 의 `fitted_=false`(ⓖ) | 3 9 |
  | `app/sectionview.cpp` | 세로 과장을 바꾼 뒤 맞춤 상태 처리(ⓐ, Q3) | 5 |
  | `app/icons.hpp` · `app/icons.cpp` | 칩 한 장 바로 그리기 `kerf::chipPixmap`(감사 기준 장, `chipIcon` 도 이것을 씀) | 6 |
  | `app/ribbon.hpp` · `app/ribbon.cpp` | `Ribbon::chipReference`(보이는 칩의 기준 장) | 6 |
  | `docs/design/DESIGN_SPEC.md` | §2.1 43행 · §3.3 · §12 142행 「1280 → 32」를 **20** 으로, §4 머리 · 정보 줄 줄이는 순서, §5 평면 눈금 글 규칙, §7 홈 줄임표, §12 새 감사 키 | 4 13 |
  | (필요할 때만) `app/theme.hpp` | 새 칸(「그릴 것」 메뉴 단추)에 QSS 가 꼭 필요할 때만 — 새 색 · 새 크기 없이 기존 `QWidget#viewTitle QToolButton` 규칙으로 되면 바꾸지 않는다 | 1 |
- **바꾸지 않는 것(명시):** `core/` · `tests/` · `tools/` · `CMakeLists.txt` · `app/CMakeLists.txt`(계산 없음 — 아래 R2.2 판정) · `.github/workflows/ci.yml`(CI 의 1920 ×1 · 3840 ×2 감사 안에 좁은 창 블록이 이미 돈다) · `app/main.cpp`(새 명령줄 옵션 없음) · `paintSectionDoc` · `app/sheetexport.cpp` · `app/guideband.*` · SVG · `app/icons/icons.cmake`(`lucide/layers.svg` 는 이미 목록에 있음) · `CLAUDE.md` · `docs/DEV_PROGRESS.md` · `docs/DEV_RULES.md` · `factory/`(본 저장소에서) · `docs/superpowers/plans/2026-10-09-design-v5.md`(27 · 122 · 137행의 「look=32」는 끝난 과제 B3 · B5 기록 — 병합 뒤 본 저장소에서 주석을 달지 판단) · 첫 창 크기(backlog #2) · 포터블 · 설치(R7.5).
  - B5 검토에서 이번 요청 밖으로 남는 것(손대지 않음): 저장된 카메라 복원이 맞춤 상태를 끄는 것(review-code should 2 · correctness N4), `CoalescingWorker` CI 제외(review-code should 1, CI 영역), 나침반이 맞춤 뒤 장면 모서리에 걸침(UI N2), 안내 칩 툴팁(UI N4), `SectionView::contentFits` 가 저장된 `xf_.plot` 을 씀(correctness N2), yaw ≠ 0 에서 `PlanView::contentFits` 과대 판정(correctness S2).
  - **R2.2 판정(계산은 core):** 눈금 숫자 간격 고르기는 **화면 글자 배치 규칙**(입력이 Qt 글꼴로 잰 글 폭 픽셀)이고 도면 · DXF · MDL 플러그인에서 다시 쓰지 않으므로 `app/planview.cpp` 안 작은 함수로 둔다. core 의 `labelStepCm`(`core/src/section.cpp:378`)은 단계가 {0.5, 1, 5, 10} m 라 {1, 2, 5, 10} m 에 쓰지 않는다. 결과는 감사 `plan-tick-overlap` 이 그린 사각형 그대로 잰다. 잘못이면: 같은 규칙을 다른 화면에서 다시 쓸 때 core 로 옮겨야 함.
  - **ⓕ 범위 판정:** 홈 카드의 모델 이름(`heroName`, 명조 40)도 같은 원인(한 줄 글이 창 최소 폭을 밀거나 잘림)이라 함께 줄임표로 한다 — `review-code.md` blocking 1(종합에 빠짐, 「이름이 길면 창 최소 폭이 1280 을 넘음」).
- **사람 검토 필수 영역 해당 여부:** **해당 — 화면 · 감사**(profile §8 「디자인 토큰 · 화면」: `app/mainwindow_ui.cpp` 의 `--ui-audit` 판정 추가 · 리본 1280 기대값을 32 → 20 으로 바꿈 · `docs/design/DESIGN_SPEC.md` 기대값 변경). SKILL §7 의 다섯 영역(좌표 · 단위 · 수치 알고리즘 / 파일 형식 / 원본 덮어쓰기 / 빌드 · CI / 비밀정보)에는 해당 없음.
- **외부 API(context7 `/websites/doc_qt_io_qt-6_8`, 2026-10-10 확인):**
  - `QFontMetrics::elidedText(text, mode, width, flags)` — 「width 보다 넓으면 "..." 이 든 글을, 아니면 원래 글을 돌려준다. width 는 픽셀」. `Qt::TextElideMode`: 「ElideMiddle 은 보통 URL 에, ElideRight 는 그 밖의 글에 알맞다」 → 경로 · 파일 · 모델 이름 = `Qt::ElideMiddle`, 문장 = `Qt::ElideRight`. 출처: context7 질의 「QFontMetrics::elidedText …」.
  - `QRect::right()` — 「역사적 이유로 left() + width() − 1 을 돌려준다」 → ⓑ 의 `r.right() > width()` 는 1 px 넘침을 놓친다(`>=` 가 맞다). 출처: context7 질의 「QRect::right() …」.
  - `QWidget::minimumSizeHint` — 「QLayout 은 minimumSize() 가 정해졌거나 크기 정책이 Ignore 가 아니면 위젯을 이것보다 작게 줄이지 않는다」, `sizeHint` = 권장 크기, `toolTip` 속성(`setToolTip`). 출처: context7 질의 「QWidget sizeHint and minimumSizeHint …」. → 머리 · 정보 줄 부모는 가로 `Ignored` 라(`mainwindow_ui.cpp:428` · `:475`) 자식 최소 폭 합보다 좁아질 수 있고, 그때 자식이 짓눌리거나(라벨) 고정 폭 위젯(축척 칸 84)이 옆 칸을 덮는다(캡처 `work-1280.png`).
  - **확인하지 못함(builder 가 쓰기 전에 context7 로):** 숨긴 위젯이 `QLayout::minimumSize()` 에서 빠지는지(`QWidgetItem::isEmpty`), `QMenu::addAction(QAction*)` 로 넣은 같은 동작의 체크 상태가 단추 · 키와 함께 바뀌는지, `QLayout::activate()` 뒤 바로 잰 최소 폭이 맞는지.

## 3. 완료 기준 ↔ 검증 방법 (기준마다 하나 이상)
기호: **V8** = `C:\Users\권을\Documents\desc\factory.cmd verify 20261010-01-narrow-1280 -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2`(8가지) · **V2** = `… verify 20261010-01-narrow-1280 -UiAudit on -UiSizes 1280x800 -UiScales 1.25,1.75`(2가지). 감사 줄은 `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\<시각>\ui-audit-<크기>-x<배율>\ui-audit.txt`. 판정 줄은 실패면 끝에 `  FAIL`, 전체 `RESULT fail` · 종료 코드 9. 줄 형식의 정본은 §3-A.

| # | 완료 기준 | 검증 방법(시험 이름 · 명령 · 확인 절차) | 종류 |
| --- | --- | --- | --- |
| AC1 | **단면 머리**: 단면 칸이 어떤 폭이든(최소 320 — `SectionView` 최소 폭, `sectionview.cpp:18`) 머리(`secTitleBar_`)의 보이는 자식이 ① 서로 겹치지 않고 ② 머리 밖으로 안 나가고 ③ 자기 최소 폭보다 짓눌리지 않는다. 폭이 모자라면 §3-C 표(Q1 답)의 순서로 줄인다. 제목(줄임표 가능) · 축척 칸 · 맞춤 · 확대 · 축소는 늘 보이고, 켜기 · 끄기 넷(입면 영상 · 잘린 선 · 레벨선 · 깊이 음영)은 상자나 「그릴 것」 메뉴 단추로 늘 닿는다 | V8 + V2 의 `ui-audit sec-header-overlap=0 kept=1 toggles-reachable=1 at=…`(측정 상태 `run · 1280w · 1280d · 1280L · 1280v · probe` — §3-A). **RED(Task 1, 구현 전):** 1280 측정이 `sec-header-overlap>0 … FAIL`(캡처 `C:\dev\tmp\b5-shots\work-1280.png` 의 토글 짓눌림 · 축척 칸이 확대 아이콘을 덮음). 그 밖 감사 `dup-icons=0` · `icons-missing=0` · `tooltip-missing=0` · `scale-controls=1` 유지 | 자동 |
| AC2 | **정보 줄**: `infoStrip` 의 글이 반 토막 나지 않는다(라벨 `width() >= sizeHint().width()`, 겹침 · 밖으로 나감 없음). 모자라면 §3-C 표(Q2 답)대로 칸을 숨기고, 남은 글은 줄임표 + 툴팁(전체 글). 계산 상태 글(`stripState_` — 「✓ 최종(잎)」 · 「미리보기 → 최종 계산 중…」 · 「단면선을 긋는 중 — …」)은 숨기지 않는다 | V8 + V2 의 `ui-audit strip-clipped=0 state-kept=1 elide-tip-missing=0 at=…`(상태 `run · 1280w · 1280d · 1280p · probe`). **RED:** 1280 측정 `strip-clipped>0 … FAIL`(「입면 뒤 0–3.(」 「레벨선 10 cn」 「단면선을 긋는 중 — 단」) | 자동 |
| AC3 | **평면 눈금 글**: 단면선 위 m 숫자 간격 = {1, 2, 5, 10} m 중 `간격 × (화면 px/m) ≥ 가장 넓은 숫자 글 폭 + 8` 인 가장 작은 값(없으면 10). 눈금 선(0.5 m · 1 m)은 그대로. 숫자 글 사각형끼리 겹치지 않는다 | V8 + V2 의 `ui-audit plan-tick-overlap=0 step=… px-per-m=… at=…`(상태 `run · 1280w`). **RED:** 1280 에서 `plan-tick-overlap>0 … FAIL`(지금은 px/m ≈ 17 에서도 1 m 마다 — 「1 m2 m3 m」). 실제 화면 1920 캡처는 지금처럼 1 m 마다(AC11) | 자동 + 수동 |
| AC4 | **리본 1280 = 타일 20**: DESIGN_SPEC §2.1 43행 「창 폭 1280(글자 숨김 타일 32)」 · §12 142행 「창 폭 1280에서 `look=32 labels=0`」을 20 으로(사용자 말 그대로 · 날짜를 적음, R4.2), §3.3 에 「1280 에서는 20」. 감사 narrow 판정을 `nk.tile <= 32` 에서 **`== 20`**(상수 `kNarrowTile = 20`, 주석에 스펙 절)으로 | V8 + V2 의 `ui-audit narrow rule-at-1280 look=20 labels=0 … actual-look=20 actual-labels=0 …`. 지금 동작을 잠그는 것이라 RED 가 없다 → **잠금 확인:** 상수를 임시로 24 로 두고 1280x800 ×1 감사가 `narrow … FAIL`(종료 코드 9)인 것을 본 뒤 되돌린다(커밋하지 않음, build.md 에 명령 · 결과). 근거: B5 실측 `need20=1280`(창 최소 폭 = 리본 `need20`) · `need32=1682` | 자동 + 문서 |
| AC5 | **ⓐ 세로 과장 뒤 맞춤 상태**(Q3 답): 추천 「끈다」 — 맞춤 상태에서 세로 과장을 바꾸면 그 보기를 사용자 보기로 보고(`fitted_=false`), 그 뒤 창 크기가 바뀌어도 가로 축척(`xf().ppm`)이 그대로. (Q3 이 「바로 맞춤」이면: 세로 과장을 바꾸는 순간 `fit()` — 바꾼 직후의 ppm 이 같은 크기에서 새로 `fit()` 한 ppm 과 같음) | V8 + V2 의 `ui-audit vex-fit=1 mode=keep`(또는 `mode=fit`). 측정: 맞춤 상태 → 세로 ×10(합성 모델에서 맞춤이 높이로 정해지는 값) → ppm 기록 → 창 크기 바꿈 → 비교. **RED:** 지금은 `fitted_` 가 켜진 채라 크기를 바꾸면 `fit()` 이 다시 잡아 `vex-fit=0 … FAIL`(review.md code should 1 · correctness S1 의 재현) | 자동 |
| AC6 | **ⓓ 칩 배율 판정을 픽셀로**: 보이는 리본 칩 전부가 d ∈ {1.25, 1.75} 에서 `icon().pixmap(QSize(t,t), d, Normal, Off)` 로 **그 배율로 바로 구운 장**(`Ribbon::chipReference` → `kerf::chipPixmap`)과 크기 · 픽셀이 같다. 어긋난 수 = `chip-dpr-bad`(선 아이콘 `icon-dpr-bad` 와 따로) | V8 + V2 의 env 줄 `icon-dpr-bad=0 chip-dpr-bad=0`. **RED(판별력 증명):** `kerf::chipIcon` 의 배율 반복에서 1.25 만 임시로 빼면 `chip-dpr-bad>0 … FAIL`(옛 판정은 같은 조건에서 통과했다 — B5 build.md RED `icon-dpr-bad=2` 가 선 아이콘뿐) → 되돌려 0. 임시 변경은 커밋하지 않음 | 자동 |
| AC7 | **ⓔ 좁은 창 시험이 1280×800 실행에서도 실제로 시험한다**: 실행 크기가 1280 이하이면 좁은 창 블록이 먼저 1600×1000(`probe`)으로 바꾼 뒤 1280 으로 줄이고, 확대 지킴(`zoom-kept`)도 1280 ↔ probe 사이 실제 크기 변화로 잰다. 줄에 `probe=WxH resized=1` | V8 + V2 의 narrow 줄 `… plan-fits=1 section-fits=1 zoom-kept=1 probe=1600x1000 resized=1`(1280 실행) · `probe=<실행 크기> resized=1`(그 밖). **판별력 증명:** `SectionView::resizeEvent` 의 맞춤 길(`sectionview.cpp:59`)을 임시로 끄면 1280x800 ×1 감사가 `section-fits=0 … FAIL`(B5 코드에서는 같은 조건에 통과 — correctness should 2) → 되돌림 | 자동 |
| AC8 | **ⓕ 홈 한 줄 글**: 홈의 줄바꿈 없는 글(끌어 놓기 안내 · 카드 보조 줄 · 표 도움말 · 최근 표 파일 이름 · 경로 · 카드 칸 값 · 카드 모델 이름 `heroName`)은 폭이 모자라면 줄임표(경로 · 이름 = 가운데, 문장 = 끝) + 툴팁(전체 글). 키 줄(「처음 쓰는 키」)은 줄임표 대신 **뒤 키부터 숨김**(「모든 키 F1」은 남김). 최근 모델이 있는 홈에서도 창 최소 폭 ≤ 1280 | ① V8 + V2 의 `ui-audit home-clipped=0 elide-tip-missing=0 min-width=≤1280`(감사가 연 합성 모델이 최근 목록에 든 홈, 1280). ② 긴 이름 감사 1회(§4 회귀 7) → `home … min-width≤1280`(review-code blocking 1 재현 입력). **RED:** ① offscreen 1280 에서 키 줄 라벨이 짓눌려 `home-clipped>0 … FAIL` 예상 — 안 나오면 사실을 build.md 에 ② 는 고치기 전 `min-width>1280 FAIL` 예상(리뷰 추정, 미실행) | 자동 + 수동 |
| AC9 | **ⓑ ⓒ ⓖ 코드 정리**: ⓑ env 넘침 판정 `r.right() >= width() \|\| r.bottom() >= height() \|\| r.left() < 0 \|\| r.top() < 0`(`mainwindow_ui.cpp:1857`) ⓒ `PlanView` 의 ortho · 회전 · 이동 식과 화면 → 기준 높이 교차 식을 도움 함수 하나씩으로(`updateMatrices` · `contentFits` · `screenToLocalXY` 가 같이 씀 — 같은 식이 두 번 적힌 곳 없음) ⓖ `PlanView::clearScene` 에서 `fitted_ = false` | ⓑ diff + V8 · V2 모두 `overflow=0`(1 px 넘친 위젯이 드러나면 이름이 `[…]` 에 찍힘 — 좁은 창 범위면 고치고, 밖이면 needs-human). ⓒ diff 읽기(`proj.ortho(` · `view.rotate(` 가 `planview.cpp` 에 한 번만) + **판별력 증명:** `PlanView::resizeEvent` 의 `if (fitted_) fitAll();` 을 임시로 끄면 1920x1040 ×1 감사 `plan-fits=0 … FAIL` → 되돌림. ⓖ diff 한 줄(검토자 확인) | 자동 + 사람 |
| AC10 | **고정 완료 기준(SKILL §6 · DEV_RULES R3.3)**: `--ui-audit` 이 1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × 배율 1 · 2 에서 모두 RESULT ok **그리고** 1280x800 × 1.25 · 1.75 도 ok(UI 검토 권고). 빌드 새 경고 0(기존 `_wfopen` · `mirrored` 만), 빠른 시험 통과, asec-section 기준값 일치 | **V8 → `RESULT PASS`(exit 0)** · **V2 → `RESULT PASS`**. verify.md 의 build 줄 PASS(WARN 아님) · tests(quick) · asec-section `polylines=1 vertices=54` · ui-audit 줄 10개 PASS | 자동 |
| AC11 | **캡처에서 글자 잘림 · 겹침 없음(SKILL §6)**: 1280 의 작업 · 그리는 중 · 최근 모델이 있는 홈(합성 + 긴 이름)에서 kerf-ui-reviewer 루브릭(전역 8항목) 모두 4 이상 — B5 AC9 미충족분. 1920 이상은 모양 변화 없음(머리 단계 0: 「북쪽을 봄」 · 토글 넷 · 「세로 ×1 ▾」 글 · 정보 줄 전체 글 · 눈금 1 m 마다) | §4 회귀 8 의 캡처를 `C:\dev\tmp\n1280-shots\` 에. 1920 작업 · 그리는 중은 B5 캡처 `C:\dev\tmp\b5-shots\work-1920.png` 와 나란히 본다(같은 PC · 같은 150 % 조건 — 픽셀 = 논리 × 1.5, 1920 은 높이 940 으로 잘림). 3440 · 3840 은 이 화면에 잘려 판정 불가 → offscreen 감사로 갈음(B5 와 같음) | 수동 |
| AC12 | **회귀 없음**: 기존 감사 항목 전부 ok(AC10 에 포함) · `--icon-sheet` · `--sheet-check` · `--ctx-shot` 그대로 · core/tests/CMake/CI 변경 없음 | §4 「돌려야 하는 기존 시험」 1–6 + `git -C C:\dev\kerf-wt\20261010-01-narrow-1280 diff --stat 308daac -- core tests tools CMakeLists.txt app/CMakeLists.txt .github` 가 비어 있음 | 자동 |
| AC13 | **문서**: DESIGN_SPEC §2.1 · §3.3 · §12(1280 → 20, 사용자 말 · 날짜) · §4(단면 머리 · 정보 줄 줄이는 순서 — Q1 · Q2 답) · §5(평면 눈금 글 규칙) · §7(홈 줄임표 · 키 줄) · §12(새 키 `sec-header-overlap=0 · strip-clipped=0 · plan-tick-overlap=0 · home-clipped=0 · chip-dpr-bad=0 · vex-fit=1`, narrow `probe`) | `git -C … diff 308daac -- docs/design/DESIGN_SPEC.md` 를 사람이 읽음 · `+` 줄에 `C:[\\/]` · `HKEY_` 없음(Grep) | 수동 + 자동 |

### 3-A. 감사 줄 형식(정본 — builder 는 이 이름 그대로 쓴다)
도움 판정(파일 안 `static` 함수로):
- **겹침 수 `overlapCount(QWidget* bar)`** = (a) 보이는 자식 위젯(`bar` 의 직계 자식, `isVisibleTo(bar)`) 두 개의 `geometry()` 가 `intersects` 하는 쌍 수 + (b) `!bar->rect().contains(child->geometry())` 인 자식 수 + (c) 짓눌린 자식 수(QLabel 은 `width() < sizeHint().width()`, 그 밖은 `width() <` (`minimumWidth() > 0 ? minimumWidth() : minimumSizeHint().width()`)).
- **잘린 글 수 `clippedLabels(QWidget* root, int* tipMissing)`** = `root` 아래 보이는 QLabel 중 `!wordWrap()` 이고 `width() < sizeHint().width()` 인 수. `tipMissing` = 동적 속성 `fullText`(전체 글, 서식 없는 글)가 있고 보이는 글이 그것과 다른데 `toolTip() != fullText` 인 수.
- 측정 상태 이름: `run`(실행 크기, 작업 화면) · `1280w`(1280, 작업) · `1280d`(1280, 그리는 중 — `plan_->setDrawMode(true)`) · `1280L`(1280, 제목을 40자 긴 이름 「트렌치 3호 북벽 서쪽 끝 A–A′ (2026-10-09 다시 그음)」으로) · `1280v`(1280, 세로 ×10) · `1280p`(1280, 가장 긴 정보 줄 글: 상태 「단면선 정확 · 배경 미리보기 → 최종 계산 중…」 + 잘린 선 「잘린 선 12줄 · 1234점 · 빈 구간 12곳 10.25 m」) · `probe`(probe 크기 ≠ 실행 크기일 때만). 시험 글을 넣은 상태는 재고 나서 `updateHeader()` 로 되돌린다.

줄(모두 기존 줄 뒤, `RESULT` 앞):
- **env**(바뀜): `ui-audit env size=%1x%2 want=%3x%4 scale=%5 dpr=%6 min-width=%7 overflow=%8[ [이름,…]] icon-dpr-bad=%9 chip-dpr-bad=%10[ screen-avail=WxH][  FAIL]` — ⓑ 넘침 식은 AC9. `chip-dpr-bad` 는 옛 「칩 `devicePixelRatio()==d`」 판정을 **대신**한다(AC6). `envOk` 에 `chipDprBad == 0` 을 더함.
- **sec-header**(새): `ui-audit sec-header-overlap=%1 kept=%2 toggles-reachable=%3 at=<상태>:<단계>:<n>,…[  FAIL]` — `%1` = 상태별 `overlapCount(secTitleBar_)` 합. `kept` = 모든 상태에서 제목 라벨 · `scaleCombo_` · 맞춤 · 확대 · 축소(아이콘 이름 `square-dashed` · `zoom-in` · `zoom-out`)가 보임. `toggles-reachable` = 모든 상태에서 토글 상자가 보이거나, 「그릴 것」 메뉴 단추가 보이고 그 메뉴에 `action("image")` · `action("line")` · `action("levels")` · `action("fade")` 넷이 다 있음. `<단계>` = `secHeaderStep_`(Task 1 에서는 늘 0). FAIL = `%1>0 || !kept || !toggles`.
- **strip**(새): `ui-audit strip-clipped=%1 state-kept=%2 elide-tip-missing=%3 at=<상태>:<단계>:<n>,…[  FAIL]` — `%1` = 상태별 `overlapCount(infoStrip_) + clippedLabels(infoStrip_)` 합. `state-kept` = 글이 있는 상태에서 `stripState_` 가 늘 보임. FAIL = `%1>0 || !state-kept || %3>0`.
- **plan-tick**(새): `ui-audit plan-tick-overlap=%1 step=%2 px-per-m=%3 at=<상태>:<step>:<n>,…[  FAIL]` — 상태 `run` · `1280w`. `n` = `plan_->tickLabelRects()` 중 서로 `intersects` 하는 쌍 수. `step` · `px-per-m` 은 1280w 값. 단면선이 없으면 `not-checked`(-1, 판정 안 함). FAIL = `%1>0`.
- **home**(새): `ui-audit home-clipped=%1 elide-tip-missing=%2 min-width=%3[  FAIL]` — 1280 에서 `showStart(true)` + pump 뒤 `clippedLabels(startPage_)` · 그때 창 `minimumSizeHint().width()`, 재고 `showStart(false)` + pump. FAIL = `%1>0 || %2>0 || %3>1280`.
- **vex-fit**(새): `ui-audit vex-fit=%1 mode=keep|fit ppm=%2/%3[  FAIL]` — Q3 답에 따른 한 모드. keep: 맞춤 상태(probe 크기) → `section_->setVerticalExaggeration(10)` → ppm A → `resize(1280,800)` → ppm B, `A == B` 면 1(허용오차 없음). fit: 세로 ×10 직후 ppm A 와 같은 크기에서 `section_->fit()` 한 뒤의 ppm F 가 같으면 1. 재고 나면 세로 과장 1 · `fit()` 으로 되돌림. 결과 없으면 -1.
- **narrow**(바뀜): `ui-audit narrow rule-at-1280 look=%1 labels=%2 window-min-width=%3 actual-width=%4 actual-look=%5 actual-labels=%6 plan-fits=%7 section-fits=%8 zoom-kept=%9 probe=WxH resized=0|1[  FAIL]` — 판정 `nk.tile == kNarrowTile && !nk.labels && ak.tile == kNarrowTile && !ak.labels && narrowW <= 1280 && planFits != 0 && secFits != 0 && zoomKept != 0 && resized == 1`(`kNarrowTile` 은 Task 2 에서, Task 1 동안은 옛 `<= 32` 유지).

좁은 창 블록 순서(정본, `uiAudit` 맨 끝 — offscreen 은 크기를 되돌려도 판 폭이 안 돌아오므로 다른 항목 뒤에, `mainwindow_ui.cpp:2129` 주석):
1. `keepSize = size()`; `run` 상태 측정(머리 · 정보 줄 · 눈금). `probe = keepSize.width() > 1280 ? keepSize : QSize(1600, 1000)`. `keepSize.width() <= 1280` 이면 `resize(probe)` + pump(250).
2. `resize(1280, 800)` + pump(250) → `resized` = 창 크기가 실제로 바뀌었는가. 리본 · `plan-fits` · `section-fits` · `1280w`(머리 · 정보 줄 · 눈금) · `1280L` · `1280p` · `1280d`(그리기 켬 → 재고 → 끄고 선 되살림) · home · `narrow.png`.
3. `section_->zoomBy(1.4)` · `plan_->zoomBy(1/1.4)` → ppm · mpp 기록 → `resize(probe)` + pump → `zoom-kept` · `probe` 상태 측정 → `section_->fit()` · `plan_->fitAll()`.
4. vex-fit(모드대로 재고) → **두 모드 모두** 세로 ×10 인 채 `resize(1280,800)` + pump 에서 `1280v` 머리 측정 → 세로 과장 1 · `fit()` 으로 되돌림.
5. 지금 크기 ≠ `keepSize` 면 `resize(keepSize)` + pump → `work.png`.

### 3-B. 단계 계획(`superpowers:writing-plans` — builder 는 `superpowers:test-driven-development` 로 Task 마다 RED → GREEN → 커밋)
**Goal:** 1280 좁은 창에서 머리 · 정보 줄 · 눈금 · 홈 글이 겹치거나 잘리지 않게 하고, 감사로 잠근다. **Architecture:** 줄이기는 `GuideBand::place()`(`guideband.cpp:75`)와 같은 「차례대로 줄이기」를 크기 바뀜 사건(앱 전체 필터 `MainWindow::eventFilter`, `mainwindow_ui.cpp:323` — 이미 Resize 를 받음)에서 부른다. 판정은 `uiAudit` 의 규칙(겹침 · 잘림 · 사각형)이라 offscreen 글꼴(한글 □, 폭 약 2배)에서도 같은 뜻. **Tech Stack:** C++17 · Qt 6.10.3 Widgets(문서 6.8) · clang-cl. **Spec:** 이 파일.

**Global Constraints:** `Q_OBJECT` 금지(동적 속성 `setProperty` · 가상 함수 재정의는 됨) · 람다 연결 · `static_cast` · 색 · 크기는 `theme.hpp` 토큰만(새 색 없음) · 사각 테두리는 둥글게(R2.3, `square-borders=0`) · 보이는 단추는 아이콘 + 툴팁(R2.8) · 바꾼 줄만(재포맷 금지) · 치수는 논리 픽셀 · 앱 자동 실행은 `--settings` 새 폴더 + `Start-Process -PassThru` + `WaitForExit`(R7.2) · 커밋은 `factory/20261010-01-narrow-1280` 에 바꾼 파일 이름만(`git add -A` 금지), 메시지 R5.4 형식 · 임시 변경(판별력 증명)은 커밋하지 않고 되돌린 뒤 build.md 에 명령 · 결과 · **Q1–Q3 답을 받은 뒤 시작**.

**Review Focus**(시험이 안 덮으면 쓸 때 물릴 것 → 각 줄의 시험을 담당 Task 에 넣었다):
1. 사용자가 단면 이름을 길게 바꿈(목록 두 번 누르기) → 제목 줄임표(머리 마지막 단계). 시험: `1280L` 상태(Task 1 측정 · Task 3 GREEN).
2. 세로 과장 ×5 · ×10 으로 「세로 ×10 ▾」 글이 길어지고 상태 색이 바뀜 → 머리 다시 맞춤. 시험: `1280v` 상태(Task 1 · 3).
3. 계산 중 · 빈 구간이 있는 단면처럼 정보 줄 글이 가장 길 때. 시험: `1280p` 상태(Task 1 · 4).
4. 발굴 모델 파일 이름은 보통 길다 → 홈 카드 이름이 창 최소 폭을 밈. 시험: 긴 이름 복사본 감사(§4 회귀 7, Task 1 RED · Task 6 GREEN).
5. 125 · 175 % 배율과 한글 글꼴 폭 차이(offscreen □ vs 맑은 고딕). 시험: V2(Task 10) + 1280 실제 화면 캡처(Task 10).

**Task 1 — 감사 판정 먼저(RED)** · Files: `app/mainwindow_ui.cpp`(uiAudit env ⓑ · 좁은 창 블록 ⓔ · 새 줄 sec-header · strip · plan-tick · home · vex-fit — §3-A), `app/mainwindow.hpp`(`QWidget* infoStrip_ = nullptr;` · `int secHeaderStep_ = 0, stripStep_ = 0;` — `buildSectionFrame` 의 지역 `strip` 을 이 멤버로), `app/planview.hpp/.cpp`
- Produces: `int PlanView::tickLabelStepM() const;`(Task 1 에서는 늘 1 = 지금 동작) · `QVector<QRectF> PlanView::tickLabelRects() const;`(지금 그리는 자리 그대로: s = k·step m(1 ≤ s < L − 1e-4)마다 중심 `c − n·16`, 폭 = `QFontMetricsF(theme::monoFont(11)).horizontalAdvance(글) + 2`(흰 테 1 px 양쪽), 높이 = 글꼴 높이 + 2; `planview.cpp:773–781` 의 그리기가 이 사각형 중심에 글을 그리게 바꿈 — 보이는 결과는 그대로). `hasLine_` 아니거나 화면 길이 < 80 px 면 빈 목록.
- Step 1: §3-A 대로 줄 · 블록 · 도움 판정을 넣고 빌드. `1280L` · `1280p` 의 시험 글은 이 Task 에서는 라벨에 바로 `setText` 하고(줄이기 함수가 아직 없음), Task 3 · 4 가 `setSecTitleText` · `setStripTexts` 길로 바꾼다 — 재는 법 · 판정은 그대로(약하게 하는 변경 아님).
- Step 2: `factory.cmd verify 20261010-01-narrow-1280 -UiAudit on -UiSizes 1280x800,1920x1040 -UiScales 1` → **기대 FAIL**: 두 실행 모두 `sec-header-overlap>0` · `strip-clipped>0` · `plan-tick-overlap>0`(1280w) · `vex-fit=0` FAIL, `home-clipped>0` FAIL(안 나오면 기록), 종료 코드 9. GREEN 이어야 하는 줄: env(`overflow=0`) · narrow(`probe=1600x1000 resized=1` 1280 실행, `plan-fits=1 section-fits=1 zoom-kept=1`). 기대와 다른 줄이 있으면 판정식부터 고친다.
- Step 3(ⓔ 판별력): `sectionview.cpp:59` 맞춤 길을 임시로 끄고 1280x800 ×1 만 다시 → `section-fits=0 … FAIL` 확인 → 되돌림.
- Step 4(Review Focus 4): 긴 이름 감사(§4 회귀 7) → `home … min-width=` 값 기록(예상 >1280).
- Step 5: RED 줄을 build.md 에. 커밋 「시험: 1280 좁은 창 감사 판정(RED) — 머리 · 정보 줄 · 눈금 · 홈 · 세로 과장 · 넘침 1 px · 1280 실행 probe」.

**Task 2 — 리본 1280 = 20(스펙 + 잠금)** · Files: `docs/design/DESIGN_SPEC.md`(§2.1 43행 · §3.3 · §12 142행), `app/mainwindow_ui.cpp`(narrow 판정)
- Produces: `constexpr int kNarrowTile = 20;   // DESIGN_SPEC §2.1 · §12 — 사용자 결정 2026-10-09 「스펙을 실제(1280 → 20)로 고친다」`.
- Step: 문구 · 판정 바꿈 → 1280x800 ×1 감사 `narrow … look=20 … actual-look=20` 통과(이 줄만) → 상수 24 로 임시 → `narrow … FAIL` → 되돌림. 커밋 「스펙: 창 폭 1280 리본 타일 20 + 감사 잠금」.

**Task 3 — 단면 머리 줄이기(Q1)** · Files: `app/mainwindow_ui.cpp`(`buildSectionFrame` · `updateHeader` · `updateVexUi` · `eventFilter`), `app/mainwindow.hpp`, `docs/design/DESIGN_SPEC.md` §4
- Produces: `void MainWindow::fitSectionHeader();`(지금 `secTitleBar_->width()` 에 들어가는 **가장 작은 단계**를 §3-C 머리 표 순서로 고르고 `secHeaderStep_` 에 둠; 재진입 막기 플래그; 크기 판단은 단계를 적용한 뒤 `secTitleBar_->layout()->activate()` 다음 `minimumSize().width() <= secTitleBar_->width()`) · `void MainWindow::setSecTitleText(const QString& full);`(`secTitleFull_` 에 두고 `fitSectionHeader()` — `updateHeader` 와 감사 `1280L` 이 이 길로) · 멤버 `QString secTitleFull_; QWidget* secToggles_; QToolButton *secTogglesMenu_, *secMaxBtn_;` · 파일 안 `static void setElidedText(QLabel* l, const QString& full, Qt::TextElideMode mode, int width);`(줄인 글 = `fontMetrics().elidedText(full, mode, width)`, 동적 속성 `fullText`=full, 줄였으면 툴팁 = full 아니면 빈 툴팁 — Task 4 · 6 도 씀).
- 「그릴 것」 메뉴 단추: `QToolButton` 아이콘만, `kerf::icon("layers", 16, theme::Hand)` + 동적 속성 `iconName`="layers"(감사 dup-icons 용), `InstantPopup`, 툴팁 「그릴 것 켜기 · 끄기 — 입면 영상 · 잘린 선 · 레벨선 · 깊이 음영 (I · L · V)」, 메뉴에 같은 `QAction` 넷. 처음에는 숨김.
- 부르는 곳: 앱 전체 필터의 `QEvent::Resize` 에서 `o == secTitleBar_` 일 때, `updateHeader()` 끝(그리는 중 일찍 돌아가는 길 포함), `updateVexUi()` 끝.
- Step: 구현 → Task 1 명령 → `sec-header-overlap=0 kept=1 toggles-reachable=1`(1280L · 1280v 포함) + `dup-icons=0 icons-missing=0 tooltip-missing=0 scale-controls=1` 유지. DESIGN_SPEC §4 에 순서 한 줄. 커밋.

**Task 4 — 정보 줄 줄이기(Q2)** · Files: `app/mainwindow_ui.cpp`, `app/mainwindow.hpp`, `docs/design/DESIGN_SPEC.md` §4
- Produces: `void MainWindow::fitInfoStrip();`(§3-C 정보 줄 표, `stripStep_`) · `void MainWindow::setStripTexts(const QString& stateRich, const QString& cutFull, const QString& cutShort);`(멤버에 두고 `fitInfoStrip()` — `updateHeader` 의 두 길과 감사 `1280p` 가 이 길로). `cutShort` = 「잘린 선 n줄 · 빈 구간 없음」 / 「잘린 선 n줄 · 빈 구간 k곳 x m」(점 수만 뺌).
- 주의: `stripState_` 는 서식 글(RichText, `mainwindow_ui.cpp:480 · 590`) — 줄일 때만 서식 없는 글(`fullText`)을 잘라 넣고, 안 줄일 때는 지금 서식 그대로(1920 모양 변화 없음).
- Step: 구현 → `strip-clipped=0 state-kept=1 elide-tip-missing=0`(1280p · 1280d 포함). 커밋.

**Task 5 — 평면 눈금 글 간격** · Files: `app/planview.cpp`, `app/planview.hpp`(필요하면), `docs/design/DESIGN_SPEC.md` §5
- Produces: 파일 안 `static int pickTickLabelStep(double pxPerM, double labelPx);` → {1, 2, 5, 10} 중 `step * pxPerM >= labelPx + 8` 인 가장 작은 값, 없으면 10. `tickLabelStepM()` 이 이것을 씀(`pxPerM` = 화면 단면선 길이 px ÷ L, `labelPx` = 가장 큰 m 값 글 「%1 m」 의 폭). 눈금 선 반복(0.5 m)은 그대로.
- Step: 구현 → `plan-tick-overlap=0`, 1280w `step=2` 근처(값은 기록), `run` 1920 은 실제 화면에서 1(AC11). 커밋.

**Task 6 — 홈 한 줄 글** · Files: `app/mainwindow_ui.cpp`(`rebuildStartPage` · `eventFilter`), `docs/design/DESIGN_SPEC.md` §7
- 줄임표 대상과 방식: `drop`(1194) · `ccSub`(1210) · `help`(1318) = `Qt::ElideRight`; 카드 이름 `heroName`(1209) · 최근 표 `nm`(1290) · `pth`(1291) · 카드 칸 값 `vl`(1228, 폴더 칸) = `Qt::ElideMiddle`, 나머지 칸 값 = `ElideRight`. 대상 라벨은 가로 `QSizePolicy::Ignored` + 동적 속성 `elideMode` → 앱 전체 필터의 Resize 에서 `setElidedText(l, fullText, mode, l->contentsRect().width())` 다시.
- 키 줄 `#homeKeys`: Resize 때 들어가지 않으면 키 묶음을 **뒤에서부터**(Ctrl+Z → Ctrl+P → O → H → 1–5 → S) 숨김, 「처음 쓰는 키」 머리와 「모든 키 F1」은 남김.
- Step: 구현 → `home-clipped=0 elide-tip-missing=0 min-width≤1280` + 긴 이름 감사 `min-width≤1280`. 커밋.

**Task 7 — 세로 과장 뒤 맞춤 상태(ⓐ, Q3)** · Files: `app/sectionview.cpp`(`setVerticalExaggeration`, 467–484)
- keep(추천): 결과가 있고 그림 칸이 유효하면 `fitted_ = false`(지금 가로 축척 · 세로 가운데 맞추기는 그대로). fit: 같은 조건에서 `xf_.vex = v` 뒤 `fit()`.
- Step: `vex-fit=1` GREEN · `section-fits=1 zoom-kept=1` 유지. 커밋.

**Task 8 — 칩 배율 판정을 픽셀로(ⓓ)** · Files: `app/icons.hpp/.cpp`, `app/ribbon.hpp/.cpp`, `app/mainwindow_ui.cpp`(env)
- Produces: `[[nodiscard]] QPixmap kerf::chipPixmap(const QString& name, int tile, int glyphPx, ChipKind kind, const ChipColors& c, QIcon::Mode mode, QIcon::State state, qreal dpr, bool caret = false);`(한 장 — `chipIcon` 의 반복이 이것을 불러 그리는 길이 하나) · `QPixmap Ribbon::chipReference(const QAbstractButton* b, qreal dpr) const;`(칩이 아니면 빈 pixmap; `forceOn` 칩은 `Shown` · `On` 모양, 메뉴 칩은 ▾ 포함 — `chipIconCached` 와 같은 인자).
- Step 1: env 의 칩 판정을 픽셀 비교(`chip-dpr-bad`)로 → 빌드 → `chip-dpr-bad=0`.
- Step 2(판별력): `chipIcon` 반복에서 1.25 만 임시로 뺌 → `chip-dpr-bad>0 … FAIL` → 되돌림. 커밋.

**Task 9 — 평면 행렬 도움 함수 + clearScene(ⓒ ⓖ)** · Files: `app/planview.hpp`, `app/planview.cpp`
- Produces(private): `void makeMatrices(int w, int h, QMatrix4x4& proj, QMatrix4x4& view) const;`(지금 `updateMatrices` 식: ortho ±w·mpp/2 · ±h·mpp/2 · ±4·diag, 회전 −(90−pitch) X · −yaw Z, −target 이동) · `bool unprojectToRefZ(const QMatrix4x4& pv, const QPointF& sp, int w, int h, asec::Vec2& out) const;`(지금 `screenToLocalXY` 의 역투영 + `refZ()` 교차, 로컬 double + `center_`). `updateMatrices` = `makeMatrices(width(), height(), proj_, view_)`, `screenToLocalXY` · `contentFits` 가 둘을 씀. `clearScene` 에 `fitted_ = false;`.
- Step: 빌드 → `plan-fits=1` → `resizeEvent` 의 `if (fitted_) fitAll();` 임시로 끔 → 1920x1040 ×1 `plan-fits=0 … FAIL` → 되돌림. 커밋.

**Task 10 — 전체 검증 · 캡처 · 기록** · V8 PASS(AC10) → V2 PASS → §4 회귀 1–8 → AC11 캡처 → `build.md`(판정 · RED/GREEN 줄 · 판별력 증명 넷 · context7 출처 · 「확인하지 못함」). 커밋.

### 3-C. 줄이는 순서 표(Q1 · Q2 의 추천안 — 답이 다르면 이 표만 바꾼다)
**단면 머리(Q1 추천 A)** — 단계 k 는 1..k 를 모두 적용:
| 단계 | 하는 일 | 근거 |
| --- | --- | --- |
| 0 | 모두 보임(1920 · 실제 화면) | 지금 모양 |
| 1 | 「북쪽을 봄」(`secFacing_`) 숨김 | 오른쪽 판 「보는 쪽」에 같은 글 |
| 2 | 「크게」(이 보기만 크게) 숨김 | 리본 「보기」 묶음 평면 · 단면 칩이 같은 일 |
| 3 | 「세로 ×1 ▾」 → 아이콘만(`Qt::ToolButtonIconOnly`, 상태 색 · 툴팁 그대로) | 툴팁에 이름 · 키 X |
| 4 | 켜기 · 끄기 상자(122 px) 숨김 + 「그릴 것」 메뉴 단추(아이콘 `layers`) 보임 | 깊이 음영은 키가 없어 숨기기만 하면 마우스로 못 닿음 |
| 5 | 제목 「A–A′ 단면」 줄임표(끝, 툴팁 전체) — 남는 폭에 맞춤 | 긴 단면 이름(Review Focus 1) |
늘 보임: 제목 · 축척 칸(84 그대로 — 「1:1000」 이 읽혀야 함) · 맞춤 · 확대 · 축소.

**정보 줄(Q2 추천 A):**
| 단계 | 하는 일 | 근거 |
| --- | --- | --- |
| 0 | 모두 | 지금 모양 |
| 1 | 「레벨선 10 cm · 숫자 50 cm」(`stripLevels_`) 숨김 | 고정 범례 — 단면 화면 눈금이 보여 줌 |
| 2 | 「입면 뒤 0–3.00 m」(`stripBand_`) 숨김 | 리본 「뒤」 칸 · 오른쪽 판 「앞 / 뒤」에 같은 값 |
| 3 | 「잘린 선 …」을 짧은 글(점 수 뺌, 툴팁 전체) | 빈 구간 정보는 지킴(오른쪽 판에는 빈 구간이 없음) |
| 4 | 남은 글(`stripState_` · `stripCut_`) 줄임표 + 툴팁 | 계산 상태는 숨기지 않음 |

## 4. 회귀 시험 계획
- **새로 쓸 시험(파일 · 이름 · 먼저 실패해야 하는 이유):** Catch2 새 시험 없음(core 변경 없음 — §2 R2.2 판정). 앱 판정 = `app/mainwindow_ui.cpp` `MainWindow::uiAudit` 의 새 줄(§3-A):
  - `sec-header-overlap` · `strip-clipped` — 부모가 가로 `Ignored` 라 1280 에서 자식이 짓눌리거나 겹침(캡처로 확인) → RED.
  - `plan-tick-overlap` — 숫자를 1 m 마다 고정 36 px 칸에 씀(`planview.cpp:773–781`) → px/m ≈ 17 에서 겹침 → RED.
  - `home-clipped` — offscreen 1280 에서 키 줄 라벨이 짓눌림(B5 실측 키 줄 1495 > 1184) → RED 예상. 긴 이름 감사의 `min-width` — 카드 이름이 최소 폭을 밈(review-code blocking 1 추정) → RED 예상.
  - `vex-fit` — `fitted_` 가 켜진 채 → RED.
  - 판정을 굳히기만 하는 넷(`look == 20` · `chip-dpr-bad` · `probe/resized` · ⓒ 정리)은 RED 가 생기지 않으므로 **임시 변경으로 판별력을 한 번씩 보인다**(AC4 · AC6 · AC7 · AC9 — 커밋하지 않음).
  - RED 보는 법: Task 1 Step 2 명령이 `RESULT FAIL`, 해당 ui-audit 행 `exit=9`.
- **돌려야 하는 기존 시험(이름 · 태그)** — exe = `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe`, PATH 앞에 `C:\Qt\6.10.3\msvc2022_64\bin`, 설정은 새 폴더, `Start-Process -PassThru` + `WaitForExit(240000)`:
  1. `asec_tests.exe "~CoalescingWorker*"` — V8 의 tests(quick) 줄(2026-10-10 `--list-tests` 로 136건 확인, CoalescingWorker 4건 제외). core 변경 없음 → 전체는 불필요.
  2. `asec-section` 기준값 `polylines=1 vertices=54`(V8 의 asec-section 줄, profile §4).
  3. `SectionViewer.exe --icon-sheet C:\dev\tmp\n1280\icons.png --quit` → 종료 코드 0 · `icon-sheet n=62 missing=0 … RESULT ok`(`kerf::chipIcon` 을 바꾸므로).
  4. `SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\tmp\n1280\s1 --sheet-check C:\dev\tmp\n1280\sheet --quit` → 0 · `sheet-check RESULT ok` · `level-pt … ok=1`.
  5. 같은 모델 · 선 `--ctx-shot C:\dev\tmp\n1280\draw.png --settings C:\dev\tmp\n1280\s2 --quit` → `ctx-shot ok: … guide=1`.
  6. 기존 감사 항목(env · tabs · ribbon · ribbon-context · dup · dup-icons · context-home · filter-persists · scale-controls · status-badge · compass-hit · draw-step2-note · guide-step2(`dpr-rerender=1`) · next-hint · guide-band · sheet · sheet-discard · lists-match · square-borders · icons-missing · narrow)은 V8 · V2 의 `RESULT ok` 에 포함.
  7. **긴 이름 감사(Review Focus 4 · AC8 ②):** `Copy-Item C:\dev\kerf-synth C:\dev\tmp\n1280-long -Recurse; Rename-Item C:\dev\tmp\n1280-long\Synthetic.3mx Synthetic_Excavation_Area3_TrenchA_LongName.3mx` → `$env:QT_QPA_PLATFORM='offscreen'` 로 `SectionViewer.exe C:\dev\tmp\n1280-long\Synthetic_Excavation_Area3_TrenchA_LongName.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\tmp\n1280\s-long --ui-audit C:\dev\tmp\n1280\ui-long --quit` → `home … min-width=≤1280` · `RESULT ok`. 복사본이 안 열리면(타일 경로) 「미실행(이유)」.
  8. **캡처(AC11)** — `powershell -NoProfile -ExecutionPolicy Bypass -File C:\Users\권을\Documents\desc\desca\.claude\scripts\shot.ps1 -Exe <위 exe> -Scene <work|draw> -Size <크기> -Out C:\dev\tmp\n1280-shots\<이름>.png`: `work-1280` · `draw-1280` · `work-1920` · `draw-1920`. 최근 모델이 있는 홈(shot.ps1 은 설정을 지우므로 직접): ① `SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --settings C:\dev\tmp\n1280\s-home --quit` ② `SectionViewer.exe --start-shot C:\dev\tmp\n1280-shots\home-1280.png --size 1280x800 --settings C:\dev\tmp\n1280\s-home --quit`, 긴 이름 복사본으로 같은 두 줄 → `home-1280-long.png`. ①이 최근 목록에 넣는지(`addRecent`, `mainwindow.cpp:334`)는 builder 가 그림으로 확인.
- **수치 허용오차:** 새 허용오차 없음 → 사용자 확인 불필요.
  - `contentFits` 경계 `1e-6 m` 그대로 — profile §7 「단면 끝점 s 좌표 · 실좌표 변환」(`tests/test_pipeline.cpp:38` `Approx(0).margin(1e-6)` · `tests/test_cutplace.cpp:38` `< 1e-6`, 2026-10-10 줄 확인).
  - `zoom-kept` · `vex-fit`: 축척 값이 **같아야 함**(허용오차 없음 — 맞춤 상태가 아니면 크기 바뀜이 ppm · mpp 를 다시 계산하지 않고, `fit()` 은 같은 입력에 같은 값).
  - 겹침 · 잘림: 정수 픽셀 `QRect::intersects` · `contains` · `width() >= sizeHint().width()` — 참/거짓. 칩 · 아이콘: 크기 · 픽셀 완전히 같음.
  - 눈금 글 사이 8 px 는 허용오차가 아니라 디자인 값(요청 원문 「간격 × px/m ≥ 글 폭 + 8」). 글 높이 16 · 폭 ≥ 12 px 이면 어느 방향 단면선에서도 이 규칙이 사각형 겹침을 막는다(손계산: 겹치려면 dx < w 이고 dy < h 여야 하는데 간격 d ≥ w + 8 이면 dy ≥ √(16w + 64) ≥ 16).

## 5. 위험 · 되돌리기
- **위험:**
  1. offscreen 은 한글이 □(폭 약 2배)라 1920 에서도 줄이기 단계가 들어갈 수 있다 — 판정은 규칙이라 무방하지만, 「1920 이상 모양 그대로」는 **실제 화면 캡처(AC11)로만** 확인된다.
  2. 크기 바뀜 사건 안에서 보이기를 바꾸면 레이아웃이 다시 돌아 같은 함수가 되풀이될 수 있다 → 재진입 막기 플래그. 머리(32) · 정보 줄(26)은 높이 고정 · 가로 `Ignored` 라 크기 고리는 없다.
  3. `stripState_` 서식 글 줄이기(Task 4 주의) — 줄일 때 「✓」 초록색이 사라질 수 있음(좁을 때만).
  4. 감사가 끝 블록에서 홈으로 갔다 오고 · 세로 ×10 · 창 크기를 여러 번 바꾼다 → 모두 되돌린 뒤 `work.png`. 되돌리기를 빠뜨리면 CI 캡처 · 다음 줄이 흐트러진다.
  5. Q3 「끈다」면: 세로 ×10 뒤 창을 많이 줄이면 단면 양끝이 잘릴 수 있다(맞춤 단추 · 두 번 클릭으로 다시 맞춤) — 의도된 동작으로 승인 보고서에 적는다.
  6. 「그릴 것」 메뉴는 같은 `QAction` 을 쓰므로 키 I · L · V · 체크 상태가 그대로여야 한다(확인하지 못함 — §2).
  7. 긴 이름 3MX 복사본이 안 열릴 수 있다(타일 상대 경로 — review-code 추정) → 미실행으로 남기고 heroName 은 코드 · 합성 감사로만 확인.
- **되돌리기:** 브랜치 `factory/20261010-01-narrow-1280` 를 병합하지 않으면 끝. 병합 뒤라면 `git revert -m 1 <병합 커밋>`.

## 6. 열린 질문 (사용자에게 물을 것 — 답을 받은 뒤 build)
- **Q1. 좁은 창에서 단면 머리를 어떤 순서로 줄일까요?**
  - (추천) A: ① 「북쪽을 봄」 숨김 → ② 「크게」 숨김 → ③ 「세로 ×1 ▾」 아이콘만 → ④ 켜기 · 끄기 넷을 「그릴 것 ▾」 메뉴 단추 하나로 접기 → ⑤ 단면 이름 줄임표. 결과: 1280 에서도 이름 · 축척 · 맞춤 · 확대 · 축소가 보이고, 켜기 · 끄기는 한 번 더 눌러 메뉴에서 고릅니다(키 I · L · V 그대로).
  - B(요청에 적힌 예): ① 「북쪽을 봄」 숨김 → ② 「세로」 아이콘만 → ③ 축척 칸 좁힘(84 → 72) → ④ 「크게」와 켜기 · 끄기 넷 숨김. 결과: 단추 수는 적지만 좁을 때 「깊이 음영」은 키가 없어 마우스로 켤 수 없고, 「1:1000」 같은 축척 글이 잘릴 수 있습니다.
- **Q2. 좁은 창에서 단면 위 정보 줄은 무엇부터 숨길까요?**
  - (추천) A: ① 「레벨선 10 cm · 숫자 50 cm」 안내 → ② 「입면 뒤 0–3.00 m」 → ③ 「잘린 선 …」에서 점 수만 빼고 → ④ 그래도 모자라면 끝에 「…」 + 마우스를 올리면 전체 글. 결과: 계산 상태와 「빈 구간 있음/없음」은 늘 보입니다.
  - B: ① 「입면 뒤」 → ② 「레벨선」 → ③ 「잘린 선 …」 통째로 숨김. 결과: 더 깔끔하지만 좁은 창에서는 빈 구간 정보가 사라집니다(오른쪽 판에는 빈 구간이 안 나옴).
- **Q3. 「세로 ×N」 과장을 바꾼 뒤 창 크기를 바꾸면 어떻게 할까요?**
  - (추천) 「사용자가 바꾼 보기로 본다」: 지금처럼 가로 축척을 그대로 두고, 창을 줄이거나 넓혀도 가로 축척이 갑자기 바뀌지 않습니다. 대신 창을 많이 줄이면 단면 양끝이 잘릴 수 있고 「맞춤」으로 다시 맞춥니다.
  - 「바로 다시 맞춘다」: ×N 을 고르는 순간 위아래까지 다 들어가게 다시 맞춥니다. 창 크기를 바꿔도 늘 전체가 보이지만, 고르는 순간 단면이 가로로 좁아집니다(합성 모델 ×2 에서 화면 축척 약 1:285 → 1:508).

## 7. 열린 질문 닫힘(사용자 답, 2026-10-10 00:40 · AskUserQuestion)
- **Q1 단면 머리 줄이기 순서 → A(추천)**: 「북쪽을 봄」 글 숨김 → 「크게」 숨김 → 「세로 ×1 ▾」 아이콘만 → 켜기·끄기 넷을 「그릴 것 ▾」 메뉴 단추 하나로 접기 → 단면 이름 줄임표. 사용자 말: 「A: 글 → 크게 → 세로 아이콘만 → 토글을 메뉴 하나로 (추천)」
- **Q2 정보 줄 숨기기 순서 → A(추천)**: 「레벨선 10 cm · 숫자 50 cm」 안내 → 「입면 뒤 0–3.00 m」 → 「잘린 선 …」 의 점 수 → 그래도 모자라면 끝에 「…」 + 툴팁 전체 글. 계산 상태 · 빈 구간 정보는 늘 보임. 사용자 말: 「A: 레벨선 안내 → 입면 뒤 → 점 수 → 끝에 … (추천)」
- **Q3 세로 ×N 뒤 창 크기 → 사용자가 바꾼 보기로 본다(가로 축척 유지)**: `setVerticalExaggeration` 은 `fitted_ = false`. 사용자 말: 「사용자가 바꾼 보기로 본다 — 가로 축척 유지 (추천)」
- (같은 자리) 끝난 B5 worktree · 브랜치 삭제: 「지운다 (추천)」 → 팩토리가 정리(이 작업과 무관).
