# 구현 요약 — 20261010-01-narrow-1280 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건

작성: builder · worktree `C:\dev\kerf-wt\20261010-01-narrow-1280` · 브랜치 `factory/20261010-01-narrow-1280` · 기준 `308daac` · 2026-10-10

스킬: `superpowers:test-driven-development`(Skill 도구로 부름, Task 마다 RED → GREEN → 커밋). 사용자 답(spec §7): Q1 A · Q2 A · Q3 사용자 보기 유지(keep).

## 바꾼 것
| 파일 | 무엇을 | 왜(완료 기준 번호) |
| --- | --- | --- |
| `app/mainwindow.hpp` | `infoStrip_` · `secHeaderStep_` · `stripStep_` 멤버 | AC1 AC2(감사가 단계를 읽음) |
| `app/mainwindow_ui.cpp` | 감사 도움 판정 `overlapCount` · `clippedLabels`(파일 안 static), env 넘침 `>=`(ⓑ), 좁은 창 블록을 §3-A 순서(run → probe → 1280 → zoom → vex)로 다시 씀 + 새 줄 `sec-header` · `strip` · `plan-tick` · `home` · `vex-fit`, narrow 줄 `probe=WxH resized=` | AC1 AC2 AC3 AC5 AC7 AC8 AC9 |
| `app/planview.hpp` · `app/planview.cpp` | `tickLabelStepM()` · `tickLabelRects()` · 내부 `tickLabels()`(그리기와 감사가 같은 사각형) — `paintOverlay` 의 숫자 글이 이 사각형 가운데에 | AC3 |

(Task 2 이후는 아래 커밋 표와 함께 덧붙인다)

## 커밋
| SHA | Task | 메시지 |
| --- | --- | --- |
| `8375cd3` | 1 | 시험: 1280 좁은 창 감사 판정(RED) — 단면 머리 · 정보 줄 · 평면 눈금 · 홈 · 세로 과장 · 넘침 1 px · 1280 실행 probe · 홈 다시 그리기 |
| `1a031cd` | 2 | 스펙: 창 폭 1280 리본 타일 20 + 감사 잠금(kNarrowTile == 20) |
| `1538a7f` | 3 | 단면 머리 좁은 창 줄이기(스펙 §4 · Q1 A) |
| `28a2ba7` | 4 | 정보 줄 좁은 창 줄이기(스펙 §4 · Q2 A) |
| `c6a93a7` | 5 | 평면 눈금 숫자 간격(스펙 §5) |
| `f8cb170` | 6 | 홈 한 줄 글 줄임표 · 키 줄 숨김 · 홈 다시 그리기 잔류 위젯 고침(AC8 · AC14) |
| `52d6dc4` | 7 | 세로 과장을 바꾸면 사용자 보기로(fitted_ = false, Q3) |
| `1b31c02` | 8 | 감사 칩 배율 판정을 픽셀 비교로(chipPixmap · chipReference → chip-dpr-bad) |
| `a2e2228` | 9 | 평면 행렬 도움 함수 둘 + clearScene fitted_ = false |
HEAD `a2e2228`(worktree 깨끗, 미커밋 없음). Task 10 은 검증 · 캡처 · 기록뿐이라 커밋 없음. 바꾼 파일(기준 대비): `app/mainwindow.hpp` · `app/mainwindow_ui.cpp` · `app/planview.hpp` · `app/planview.cpp` · `app/sectionview.cpp` · `app/icons.hpp` · `app/icons.cpp` · `app/ribbon.hpp` · `app/ribbon.cpp` · `app/sheetexport.cpp` · `docs/design/DESIGN_SPEC.md`.

## 전체 검증(Task 10)
- **V8** `factory.cmd verify 20261010-01-narrow-1280 -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2` → **RESULT PASS**(exit 0, verify.md 05:23 블록, HEAD `a2e2228`, 로그 `logs\20261010-051346`):
```
[PASS] build exit=0 3s — 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만)
[PASS] tests(quick) exit=0 3s — assertions: 51325 ¦ 51325 passed
[PASS] asec-section exit=0 1s — … polylines=1 vertices=54 …
[PASS] ui-audit 1280x800 ×1 exit=0 66s — ui-audit RESULT ok
[PASS] ui-audit 1280x800 ×2 exit=0 66s — ui-audit RESULT ok
[PASS] ui-audit 1920x1040 ×1 exit=0 66s — ui-audit RESULT ok
[PASS] ui-audit 1920x1040 ×2 exit=0 66s — ui-audit RESULT ok
[PASS] ui-audit 3440x1440 ×1 exit=0 66s — ui-audit RESULT ok
[PASS] ui-audit 3440x1440 ×2 exit=0 67s — ui-audit RESULT ok
[PASS] ui-audit 3840x2160 ×1 exit=0 66s — ui-audit RESULT ok
[PASS] ui-audit 3840x2160 ×2 exit=0 69s — ui-audit RESULT ok
[WARN] ci(base) exit=3 — CI 308daac none(push 전)
```
- **V2** `… -UiSizes 1280x800 -UiScales 1.25,1.75` → **RESULT PASS**(verify.md 05:25 블록): `ui-audit 1280x800 ×1.25 … RESULT ok` · `×1.75 … RESULT ok`. (첫 V2 시도는 V8 과 동시에 돌려 `z.dll` 복사가 잠긴 파일에 막혀 감사 전에 멈춤 — 기록 없음, 단독으로 다시 돌린 것이 위 결과.)
- AC10: 빌드 새 경고 0 · 빠른 시험 51325 통과 · asec-section 기준값 일치 · ui-audit 10 줄 모두 PASS → **통과**.

## RED → GREEN 줄(그대로)
### Task 1 — 감사 판정 먼저(RED)
명령: `C:\Users\권을\Documents\desc\factory.cmd verify 20261010-01-narrow-1280 -UiAudit on -UiSizes 1280x800,1920x1040 -UiScales 1` → **RESULT FAIL**(두 ui-audit 행 exit=9). 로그 `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-044530\`(첫 RED 는 `20261010-014457`, 그 뒤 AC14 줄 + probe 맞춤을 더해 다시 돌림)

1280x800 ×1:
```
ui-audit env size=1280x800 want=1280x800 scale=1 dpr=1.00 min-width=1280 overflow=0 icon-dpr-bad=0
ui-audit sec-header-overlap=40 kept=1 toggles-reachable=1 at=run:0:6,1280w:0:6,1280L:0:6,1280p:0:6,1280d:0:6,probe:0:4,1280v:0:6  FAIL
ui-audit strip-clipped=50 state-kept=1 elide-tip-missing=0 at=run:0:8,1280w:0:8,1280L:0:8,1280p:0:8,1280d:0:4,probe:0:6,1280v:0:8  FAIL
ui-audit plan-tick-overlap=38 step=1 px-per-m=17.4 at=run:1:19,1280w:1:19  FAIL
ui-audit home-clipped=66 elide-tip-missing=0 min-width=1280  FAIL
ui-audit home-stale-children=25  FAIL
ui-audit vex-fit=0 mode=keep ppm=30.2589/15.2659  FAIL
ui-audit narrow rule-at-1280 look=20 labels=0 window-min-width=1280 actual-width=1280 actual-look=20 actual-labels=0 plan-fits=1 section-fits=1 zoom-kept=1 probe=1600x1000 resized=1
ui-audit RESULT fail
```
1920x1040 ×1:
```
ui-audit sec-header-overlap=32 kept=1 toggles-reachable=1 at=run:0:2,1280w:0:6,1280L:0:6,1280p:0:6,1280d:0:6,1280v:0:6  FAIL
ui-audit strip-clipped=40 state-kept=1 elide-tip-missing=0 at=run:0:4,1280w:0:8,1280L:0:8,1280p:0:8,1280d:0:4,1280v:0:8  FAIL
ui-audit plan-tick-overlap=21 step=1 px-per-m=17.4 at=run:1:2,1280w:1:19  FAIL
ui-audit home-clipped=62 elide-tip-missing=0 min-width=1280  FAIL
ui-audit home-stale-children=25  FAIL
ui-audit vex-fit=0 mode=keep ppm=43.2039/15.2659  FAIL
ui-audit narrow rule-at-1280 look=20 labels=0 window-min-width=1280 actual-width=1280 actual-look=20 actual-labels=0 plan-fits=1 section-fits=1 zoom-kept=1 probe=1920x1040 resized=1
ui-audit RESULT fail
```
- offscreen 글꼴(한글 □, 폭 약 2배)이라 1920 의 `run` 상태에서도 머리 2 · 정보 줄 4 · 눈금 2 가 겹친다(spec §5 위험 1 — 규칙 판정이라 무방, 실제 화면은 캡처로).
- **긴 이름 감사(§4 회귀 7, Step 4):** `C:\dev\tmp\n1280-long\Synthetic_Excavation_Area3_TrenchA_LongName.3mx --size 1280x800 --ui-audit` → `EXIT=9`, `ui-audit home-clipped=21 elide-tip-missing=0 min-width=2621  FAIL`(review-code blocking 1 재현 — 복사본은 정상으로 열림).
### Task 2 — 리본 1280 = 20(스펙 + 잠금)
- 1280x800 ×1 감사(`C:\dev\tmp\n1280\ui-t2-*`): `ui-audit narrow rule-at-1280 look=20 labels=0 window-min-width=1280 actual-width=1280 actual-look=20 actual-labels=0 plan-fits=1 section-fits=1 zoom-kept=1 probe=1600x1000 resized=1`(FAIL 없음).
- **잠금 확인:** `kNarrowTile = 24` 로 임시 → 같은 감사 → `… resized=1  FAIL`(EXIT=9) → 20 으로 되돌림(커밋하지 않음).

### Task 3 — 단면 머리 줄이기(GREEN)
- 1280x800 ×1 감사(`C:\dev\tmp\n1280\ui-t3-*`): `ui-audit sec-header-overlap=0 kept=1 toggles-reachable=1 at=run:5:0,1280w:5:0,1280L:5:0,1280p:5:0,1280d:5:0,probe:3:0,1280v:5:0`(RED 40 → 0). `dup-actions=0 · dup-icons=0 · icons-missing=0 tooltip-missing=0 · scale-controls=1` 유지. offscreen(□ 글꼴 2배)이라 1280 은 5단계(제목 줄임표), probe 1600 은 3단계 — 실제 화면 1920 은 캡처(AC11)로.

### Task 4 — 정보 줄 줄이기(GREEN)
- 1280x800 ×1 감사(`C:\dev\tmp\n1280\ui-t4-*`): `ui-audit strip-clipped=0 state-kept=1 elide-tip-missing=0 at=run:3:0,1280w:3:0,1280L:3:0,1280p:4:0,1280d:4:0,probe:2:0,1280v:3:0`(RED 50 → 0). 1280p(가장 긴 글) · 1280d(그리는 중)만 4단계(줄임표), 그 밖 1280 은 3단계(점 수 뺌).

### Task 5 — 평면 눈금 숫자 간격(GREEN)
- 1280x800 ×1 감사(`C:\dev\tmp\n1280\ui-t5-*`): `ui-audit plan-tick-overlap=0 step=5 px-per-m=17.4 at=run:5:0,1280w:5:0`(RED 38 → 0). offscreen 모노 글꼴이 넓어 1280 은 5 m 간격(실제 화면 1280 은 캡처로, 1920 은 1 m — AC11).

### Task 6 — 홈 한 줄 글 · 키 줄 · 홈 다시 그리기(AC8 · AC14, GREEN)
- 1280x800 ×1 합성(`C:\dev\tmp\n1280\ui-t6-*`): `ui-audit home-clipped=0 elide-tip-missing=0 min-width=1280` · `ui-audit home-stale-children=0`(RED 66 · 25 → 0).
- 긴 이름 복사본(`ui-t6long-*`, `Synthetic_Excavation_Area3_TrenchA_LongName.3mx`): `home-clipped=0 elide-tip-missing=0 min-width=1280`(RED 2621 → 1280) · `home-stale-children=0`.
- `sheetexport.cpp:951`(도면 점검 상자)의 같은 `takeAt(0)` 패턴도 `MainWindow::clearLayout` 로 — 거기는 하위 레이아웃 없이 줄 위젯만 담아 전부터 결함은 없었음(한 곳으로 모음).

### Task 7 · 8 · 9 — 세로 과장 뒤 맞춤 상태(ⓐ) · 칩 픽셀 비교(ⓓ) · 행렬 도움 함수 + clearScene(ⓒ ⓖ) — 한 빌드 · 한 감사로 GREEN, 커밋은 셋
- 1280x800 ×1 감사(`C:\dev\tmp\n1280\ui-t789-051028`, **EXIT=0 · RESULT ok**):
```
ui-audit env size=1280x800 want=1280x800 scale=1 dpr=1.00 min-width=1280 overflow=0 icon-dpr-bad=0 chip-dpr-bad=0
ui-audit vex-fit=1 mode=keep ppm=30.2589/30.2589
ui-audit narrow rule-at-1280 look=20 labels=0 window-min-width=1280 actual-width=1280 actual-look=20 actual-labels=0 plan-fits=1 section-fits=1 zoom-kept=1 probe=1600x1000 resized=1
ui-audit RESULT ok
```
- `--icon-sheet`(chipIcon 이 chipPixmap 을 부르게 바뀜): `icon-sheet n=62 missing=0 … RESULT ok`(EXIT=0).
- **판별력 증명(AC6 · AC9, 임시 변경 둘을 한 빌드에, 1920x1040 ×1 감사 `C:\dev\tmp\n1280\ui-disc89-*`):** ① `chipIcon` 반복에서 `if (d != 1.25)` 로 1.25 장만 뺌 → `env … icon-dpr-bad=0 chip-dpr-bad=21  FAIL`(보이는 칩 21개 — 옛 판정은 같은 조건에 통과했음, B5 RED 는 선 아이콘 2 뿐) ② `PlanView::resizeEvent` 의 `if (fitted_) fitAll();` 을 `if (false && …)` 로 끔 → `narrow … plan-fits=0 section-fits=1 zoom-kept=1 … FAIL`. 둘 다 되돌리고 다시 빌드(`git status` 깨끗, 커밋하지 않음).

- **ⓔ 판별력(Step 3):** `sectionview.cpp:59` 의 `if (fitted_ && has_) { fit(); return; }` 를 `if (false && …)` 로 임시로 끄고 1280x800 ×1 → 처음에는 `section-fits=1`(1280 실행은 처음부터 1280 에 맞춰져 있어 1280 → probe → 1280 을 오가도 다시 맞춤 없이 들어맞음) → 좁은 창 블록 1단계에서 probe 로 키운 뒤 `plan_->fitAll(); section_->fit();` 를 명시(「맞춤 상태(probe)」 전제, 판정 1) → `… plan-fits=1 section-fits=0 zoom-kept=1 probe=1600x1000 resized=1  FAIL`. 되돌린 뒤 `section-fits=1`(위 1280 줄). 임시 변경은 커밋하지 않음(`git diff -- app/sectionview.cpp` 0줄 확인).

## 명세와 다르게 한 것 (판정)
1. **좁은 창 블록 1단계에서 probe 로 키운 뒤 `plan_->fitAll(); section_->fit();` 를 명시** — 무엇을: spec §3-A 1단계는 `resize(probe)` 만 적음. 왜: 1280 실행은 처음부터 1280 에 맞춰져 있어 맞춤 길(`sectionview.cpp:59`)을 꺼도 1280 → 1600 → 1280 에서 `section-fits=1`(ⓔ 판별력 없음). spec 4단계의 「맞춤 상태(probe 크기)」 전제를 1단계에서 명시한 것 — 판정을 약하게 한 것이 아니라 전제를 만든 것(끈 뒤 `section-fits=0 FAIL` 확인). 잘못이면: probe 에서 사용자가 확대한 보기를 가정하는 시험은 없어짐(zoom-kept 가 따로 잰다).
2. **Task 7 · 8 · 9 를 한 빌드 · 한 1280 감사로 GREEN 을 보고 커밋은 셋으로** — 왜: 서로 다른 파일(sectionview · icons/ribbon/감사 env · planview)이고 판정 줄도 다르다(vex-fit · chip-dpr-bad · plan-fits). 잘못이면: 한 Task 의 GREEN 이 다른 Task 코드에 기댔을 가능성 — 판별력 증명 둘(①②)로 각 줄이 제 코드에 반응함을 확인했다.
3. **AC14 `home-stale-children` 를 좁은 창 블록의 home 측정 안에서 잼**(`showStart(true)` 두 번 → `sendPostedEvents(DeferredDelete)` → 직접 자식 중 어느 레이아웃도 관리하지 않는 보이는 위젯 수) — 왜: 홈을 보였다 숨기는 횟수를 줄이려고(감사 끝에서 크기를 되돌리는 블록과 같은 곳). 잘못이면: 없음(판정식은 §8 그대로).
4. **`app/sheetexport.cpp:951` 을 바꿈**(spec §2 「바꾸지 않는 것」에 있으나 §8 AC14 가 「같은 takeAt 패턴은 같은 도움 함수로」 라고 뒤에 적음) — `MainWindow::clearLayout(lay_)` 한 줄. 그 상자는 줄 위젯만 담아 전부터 결함은 없었음(코드 읽기). 잘못이면: 도면 점검 상자가 지워진 뒤 숨김이 한 번 더 들어가는 것뿐.
5. **홈 줄임표 대상을 spec Task 6 목록보다 넓게**(카드 칸 이름 `cap`, 썸네일 안내 · 썸네일 이름, 작업 순서 「… 기준」) — 왜: offscreen(□ 글꼴 2배)에서 그 라벨들도 `home-clipped` 에 잡혔다(RED 66). 모두 같은 규칙(elideLab). 잘못이면: 짧은 글에 Ignored 정책이 붙어 레이아웃이 조금 달라질 수 있음(1920 캡처로 확인).
6. **키 줄의 키 묶음을 QWidget 하나로 감쌈**(전에는 QHBoxLayout) — 뒤부터 숨기려면 위젯이어야 한다. 모양은 같다(여백 0 · 간격 8).
7. **정보 줄 4단계(줄임표)에서 상태 글을 PlainText 로** — RichText 라벨의 sizeHint 가 글 폭과 1 px 다를 수 있어 「폭 < sizeHint」 판정이 흔들린다. 「✓」 초록색은 그때만 사라짐(spec §5 위험 3, 1280 가장 긴 글에서만).
8. **「그릴 것 ▾」 단추는 QSS 를 더하지 않음**(기존 `QWidget#viewTitle QToolButton` 규칙 + Qt 기본 메뉴 표시) — spec §2 「theme.hpp 는 꼭 필요할 때만」. 잘못이면: ▾ 모양이 `vexBtn` 과 조금 다름(캡처로 봄).
9. **`plan-tick` 의 px-per-m 을 `1 / metersPerPixel()` 로 적음** — 감사는 위에서 본 화면(yaw 0 · pitch 90)이라 같은 값. 잘못이면: 돌린 평면에서 기록값만 다름(판정은 사각형 겹침으로 따로).
10. **`tickLabelRects()` 의 사각형은 옛 그리기(폭 36 고정)보다 글 폭 + 2 로 좁음** — spec Task 1 지시 그대로(「폭 = 글 폭 + 2」). 보이는 글 위치(가운데)는 같다.

## 검증 방법 변경
- 시험 기대값을 바꿨다면 그 근거(계산 · 문서 · 사용자 확인):
  - 리본 1280 기대값 32 → 20: 사용자 결정 2026-10-09 「스텍을 실제(1280 → 20)로 고친다」(spec §1 · §7, B5 review.md 종합 ②). Task 2 에서 적용.

## context7 출처(`/websites/doc_qt_io_qt-6_8`, 2026-10-10)
- `QWidgetItem::isEmpty()` — 「위젯이 숨겨져 있으면 true」(QLayoutItem::isEmpty 재구현) → 숨긴 자식은 레이아웃 최소 폭에서 빠진다(spec §2 「확인하지 못함」 ①).
- `QAction` 상세 — 「메뉴 · 도구 모음에 더한 동작은 UI 를 자동으로 동기화한다(Bold 단추를 누르면 Bold 메뉴 항목도 체크)」 → 「그릴 것」 메뉴에 같은 QAction 넷을 넣으면 체크 상태 · 키가 함께 바뀐다(「확인하지 못함」 ②).
- `QFontMetrics::elidedText(text, mode, width, flags)` — 「width(픽셀)보다 넓으면 "..." 이 든 글, 아니면 원래 글」. `Qt::ElideMiddle` 은 URL 류, `Qt::ElideRight` 는 그 밖의 글.
- `QLayout::activate()` 뒤 바로 잰 최소 폭(「확인하지 못함」 ③): 문서 질의로 확답을 얻지 못함 → 구현은 `activate()` 뒤 `minimumSize()` 를 쓰되, 줄임표 폭은 되풀이(최대 4회)로 맞춰 1 px 오차에 기대지 않는다. 감사가 실제 겹침 · 잘림을 재므로 틀리면 FAIL 로 드러난다.

## 캡처(실제 화면, 이 PC 150 % — 픽셀 = 논리 × 1.5) `C:\dev\tmp\n1280-shots\`
| 파일 | 조건 | 눈으로 본 것 |
| --- | --- | --- |
| `work-1280.png` | `shot.ps1 -Scene work -Size 1280x800` | 단면 머리 5단계: 제목 「A–A′ …」(툴팁 전체) · 「그릴 것」 메뉴(layers) · 세로 아이콘만 · 「1:218 ▾」 · 맞춤 · 확대 · 축소 — 겹침 · 짓눌림 없음. 정보 줄 2단계 「✓ 최종(잎) … 잘린 선 1줄 · 54점 · 빈 구간 없음」 잘림 없음. 평면 눈금 「2 m 4 m 6 m 8 m 10 m」 겹침 없음 |
| `draw-1280.png` | `-Scene draw -Size 1280x800` | 정보 줄 「단면선을 긋는 중 — 단면은 A′를 찍은 뒤 계산됩니다」 전체, 안내 칩 「… Shift …」 줄임표(스펙 §5 순서 ③), 오른쪽 판 「새 단면 · 그리는 중」 |
| `work-1920.png` · `draw-1920.png` | `-Size 1920x1040`(화면 높이 탓 940 으로 잘림, B5 와 같음) | 머리 0단계(「A–A′ 단면 북쪽을 봄」 · 토글 넷 · 「세로 ×1 ▾」 · 「1:87.5」 · 맞춤 · 확대 · 축소 · 크게), 정보 줄 전체(잘린 선 · 입면 뒤 0–3.00 m · 레벨선 10 cm · 숫자 50 cm), 눈금 1 m 마다 — B5 `C:\dev\tmp\b5-shots\work-1920.png` 와 나란히 같은 모양 |
| `home-1280.png` | 합성 모델을 연 뒤 `--start-shot --size 1280x800`(같은 `--settings`) | 카드 이름 「Synthetic」, 칸 「폴더 C: › de…f-synth」 · 「마지…」 「수평 …」 「EPS…」 「KVD…」(칸이 좁아 줄임표 + 툴팁), 키 줄 여섯 묶음 + 「모든 키 F1」 전부, 창 1280 |
| `home-1280-long.png` | 긴 이름 복사본 | 카드 이름 「Synthet···ongName」(가운데 줄임표), 표 이름 전체, 창 1280 |
| `home-1920.png` | 1920 | 칸 이름 · 값 전체, 줄임표 없음 |
| `home-tab0-1920.png` · `home-tab0-1280.png` | 모델 연 뒤 `--tab 0 --shot`(AC14 ③, 재현 `home-dbg.png` 와 같은 명령) | 홈 위에 옛 위젯 덩어리 **없음**, 문서 탭 「홈」 활성 · 구석 「Synthetic · 저장됨」 |
- 관찰(검토자에게): 1280 실제 화면에서 제목이 「A–A′ …」(5단계)까지 간다 — 축척 칸 84 · 아이콘 단추 셋 · 메뉴 · 세로 아이콘 · 고정 간격(8 · 6 · 4 · 6)을 더하면 단면 칸 328 px 를 넘는다. 스펙 순서(Q1 A)대로이고 툴팁에 전체 글. 1280 홈의 카드 칸 이름 「마지막 열림」 · 「수평 좌표계」가 「마지…」 「수평 …」로 줄어드는 것도 같은 규칙(칸 ≈ 62 px).
- offscreen 감사 캡처(`logs\<시각>\ui-audit-*\narrow.png · work.png`)는 □ 글꼴 — 판정은 규칙(겹침 · 잘림)이라 그대로 쓰고 모양은 위 실제 화면으로 본다.

## 완료 기준별
| AC | 결과 | 근거 |
| --- | --- | --- |
| AC1 단면 머리 | 통과 | `sec-header-overlap=0 kept=1 toggles-reachable=1`(모든 상태, RED 40 → 0) · dup-icons · tooltip · scale-controls 유지 · 캡처 work/draw-1280 |
| AC2 정보 줄 | 통과 | `strip-clipped=0 state-kept=1 elide-tip-missing=0`(RED 50 → 0) · 캡처 |
| AC3 눈금 글 | 통과 | `plan-tick-overlap=0 step=5`(offscreen 1280) · 실제 화면 1280 「2 m 4 m …」, 1920 1 m 마다(RED 38 → 0) |
| AC4 리본 1280 = 20 | 통과 | `narrow … look=20 … actual-look=20`, 상수 24 로 잠금 FAIL 확인, DESIGN_SPEC §2.1 · §3.3 · §12 |
| AC5 세로 과장 뒤 맞춤 | 통과 | `vex-fit=1 mode=keep ppm=30.2589/30.2589`(RED 0 → 1) |
| AC6 칩 픽셀 비교 | 통과 | `icon-dpr-bad=0 chip-dpr-bad=0`, 1.25 를 빼면 `chip-dpr-bad=21 FAIL` |
| AC7 1280 실행 probe | 통과 | `narrow … probe=1600x1000 resized=1`(1280 실행) · `probe=1920x1040 resized=1`(그 밖), 맞춤 길 끄면 `section-fits=0 FAIL` |
| AC8 홈 한 줄 글 | 통과 | `home-clipped=0 elide-tip-missing=0 min-width=1280`(합성 · 긴 이름 복사본 — RED 2621 → 1280) · 캡처 home-1280(-long) |
| AC9 ⓑ ⓒ ⓖ | 통과 | env `>=`(`overflow=0` 전부) · `proj.ortho(` · `view.rotate(` 가 planview.cpp 에 한 번(makeMatrices), resize 맞춤 끄면 `plan-fits=0 FAIL` · clearScene `fitted_ = false` |
| AC10 고정 완료 기준 | 통과 | V8 RESULT PASS(8 ok) · V2 RESULT PASS(2 ok) · 새 경고 0 · 51325 통과 · 기준값 일치(「전체 검증」) |
| AC11 캡처 | 캡처 제출(루브릭 점수는 kerf-ui-reviewer) | 「캡처」 표 — 1920 은 B5 와 같은 모양 |
| AC12 회귀 | 통과 | 아래 「회귀」 1–8, core/tests/CMake/CI diff 없음 |
| AC13 문서 | 통과 | DESIGN_SPEC §2.1 · §3.3 · §4(머리 · 정보 줄 순서) · §5(눈금 규칙) · §7(홈 줄임표 · 키 줄 · 다시 그리기) · §12(20). `+` 줄에 `C:[\\/]` · `HKEY_` 없음 |
| AC14 홈 다시 그리기 | 통과 | `home-stale-children=0`(RED 25 → 0) · `--tab 0 --shot` 1920 · 1280 캡처에 옛 위젯 없음 · clearLayout 재귀 + showStart 한 번 그리기 |

## 회귀(§4 「돌려야 하는 기존 시험」)
1. `asec_tests "~CoalescingWorker*"` — V8 tests(quick) 51325 통과(verify.md).
2. `asec-section` 기준값 `polylines=1 vertices=54` — V8 PASS.
3. `--icon-sheet` → `icon-sheet n=62 missing=0 … RESULT ok`(EXIT=0, 위 Task 7·8·9 절).
4. `--sheet-check`(offscreen 1920) → `sheet-check RESULT ok` · `level-pt minor=0.564 expect=0.564 major=1.411 expect=1.411 ok=1`(EXIT=0).
5. `--ctx-shot` → `ctx-shot ok: C:\dev\tmp\n1280\draw-ctx.png guide=1`(EXIT=0).
6. 기존 감사 항목 전부 — V8 · V2 의 `RESULT ok` 에 포함.
7. 긴 이름 감사 — Task 6 절(`home … min-width=1280 · RESULT` 는 vex-fit 고치기 전이라 fail, 홈 줄은 ok). 복사본은 정상으로 열림.
8. 캡처 — 위 「캡처」 표.
- `git diff --stat 308daac -- core tests tools CMakeLists.txt app/CMakeLists.txt .github` → 비어 있음(core · tests · CMake · CI 변경 없음, AC12).

## 남은 문제
- 1280 실제 화면에서 제목 「A–A′ …」(5단계) · 홈 카드 칸 이름 줄임표 — 스펙 순서대로지만 모양은 kerf-ui-reviewer 가 본다(AC11).
- AC11 의 3440 · 3840 실제 화면은 이 PC 화면(2293×912 논리)에 잘려 판정 불가 → offscreen 감사로 갈음(B5 와 같음).
- 125 · 175 % 진짜 모니터 눈 확인은 못 함(이 PC 150 % 하나) — V2(QT_SCALE_FACTOR 1.25 · 1.75 offscreen)로 갈음.
- `QLayout::activate()` 뒤 최소 폭 시점(context7 확답 없음)은 되풀이 맞춤으로 흡수 — 감사가 겹침 · 잘림을 직접 재므로 틀리면 FAIL 로 드러난다(모든 감사 ok).

## 수정 회차 기록
- 1회:
- 2회:
