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

- **ⓔ 판별력(Step 3):** `sectionview.cpp:59` 의 `if (fitted_ && has_) { fit(); return; }` 를 `if (false && …)` 로 임시로 끄고 1280x800 ×1 → 처음에는 `section-fits=1`(1280 실행은 처음부터 1280 에 맞춰져 있어 1280 → probe → 1280 을 오가도 다시 맞춤 없이 들어맞음) → 좁은 창 블록 1단계에서 probe 로 키운 뒤 `plan_->fitAll(); section_->fit();` 를 명시(「맞춤 상태(probe)」 전제, 판정 1) → `… plan-fits=1 section-fits=0 zoom-kept=1 probe=1600x1000 resized=1  FAIL`. 되돌린 뒤 `section-fits=1`(위 1280 줄). 임시 변경은 커밋하지 않음(`git diff -- app/sectionview.cpp` 0줄 확인).

## 명세와 다르게 한 것 (판정)
- (채움)

## 검증 방법 변경
- 시험 기대값을 바꿨다면 그 근거(계산 · 문서 · 사용자 확인):
  - 리본 1280 기대값 32 → 20: 사용자 결정 2026-10-09 「스텍을 실제(1280 → 20)로 고친다」(spec §1 · §7, B5 review.md 종합 ②). Task 2 에서 적용.

## context7 출처(`/websites/doc_qt_io_qt-6_8`, 2026-10-10)
- `QWidgetItem::isEmpty()` — 「위젯이 숨겨져 있으면 true」(QLayoutItem::isEmpty 재구현) → 숨긴 자식은 레이아웃 최소 폭에서 빠진다(spec §2 「확인하지 못함」 ①).
- `QAction` 상세 — 「메뉴 · 도구 모음에 더한 동작은 UI 를 자동으로 동기화한다(Bold 단추를 누르면 Bold 메뉴 항목도 체크)」 → 「그릴 것」 메뉴에 같은 QAction 넷을 넣으면 체크 상태 · 키가 함께 바뀐다(「확인하지 못함」 ②).
- `QFontMetrics::elidedText(text, mode, width, flags)` — 「width(픽셀)보다 넓으면 "..." 이 든 글, 아니면 원래 글」. `Qt::ElideMiddle` 은 URL 류, `Qt::ElideRight` 는 그 밖의 글.
- `QLayout::activate()` 뒤 바로 잰 최소 폭(「확인하지 못함」 ③): 문서 질의로 확답을 얻지 못함 → 구현은 `activate()` 뒤 `minimumSize()` 를 쓰되, 줄임표 폭은 되풀이(최대 4회)로 맞춰 1 px 오차에 기대지 않는다. 감사가 실제 겹침 · 잘림을 재므로 틀리면 FAIL 로 드러난다.

## 캡처
- (채움) `C:\dev\tmp\n1280-shots\`

## 남은 문제
- (채움)

## 수정 회차 기록
- 1회:
- 2회:
