# 구현 요약 — 20261010-03-audit-tighten #3c 후속: 감사 판정 조이기 · 작은 정리(R1b)

작성: builder · worktree `C:\dev\kerf-wt\20261010-03-audit-tighten` · 브랜치 `factory/20261010-03-audit-tighten`

## 1차(T1–T4) — builder 1, 2026-10-10
**상태: T1 · T2 · T3 커밋. T3 에서 문턱 거짓 실패(배율 1.25 · 1.75 의 caret-band)를 만나 지시대로 멈춤 → T4 는 시작하지 않음.**

출처: context7 `/websites/doc_qt_io_qt-6_8` 「QPainter::CompositionMode」 — `CompositionMode_Clear`(2): 「The pixels in the destination are cleared (set to fully transparent) independent of the source.」, `CompositionMode_SourceOver`(0) 기본값.
감사 명령(모두 offscreen): `C:\dev\tmp\r1b\audit.ps1 -Size WxH -Scale s -Tag t` = `SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size WxH --settings C:\dev\tmp\r1b\set-t --ui-audit C:\dev\tmp\r1b\t --quit`(Start-Process + WaitForExit(240000), 배율은 `QT_SCALE_FACTOR`). 결과 txt 는 `C:\dev\tmp\r1b\<태그>\ui-audit.txt`(이 PC).
기준(고치기 전 d95706f, 1920×1): `refit=1/0 … carets=1 caret-ink=16/12` · `strip-clipped=0 state-kept=1 gap-kept=1` · RESULT ok. 처음 구성 · 빌드: `factory.cmd verify 20261010-03-audit-tighten -UiAudit off` → RESULT PASS(configure · build 새 경고 0 · tests 51325 통과 · asec-section polylines=1 vertices=54).

### 바꾼 것
| 파일 | 무엇을 | 왜 |
| --- | --- | --- |
| `app/mainwindow_ui.cpp` | 감사 `refit` → `refitA == 1 && refitB == 0` + `title-kept`(fullText == secTitleFull_ · 보이는 글이 전체이거나 「…」 앞부분이 일치 · secTitleFull_ == updateHeader 가 만드는 글) | AC1 |
| `app/mainwindow_ui.cpp` · `mainwindow.hpp` | `secHeaderKey_` QString(구분자 이음) → `QStringList` 칸별 비교 | AC5 |
| `app/mainwindow_ui.cpp` | 「그릴 것」 헛 `setIcon(kerf::icon("layers"…))` · 동적 속성 `caretBaked` 두 곳 삭제, 감사 주석의 그 이름 고침(`setIconSize` · `iconName` 속성은 남김) | AC10 |
| `app/mainwindow_ui.cpp` | 감사 `gap-kept`: `stripCut_` 가 숨겨져도 실패 | AC2 |
| `app/mainwindow_ui.cpp` · `mainwindow.hpp` | `fitInfoStrip` 첫머리 열쇠 `stripFitKey_`(QStringList: 폭 · 상태 서식 글 · 잘린 선 전체 · 짧은 글 · lvLine_ · lvLabel_ · stripBand_), 횟수 `stripPasses_`, `applyHighContrast` 에서 `stripFitKey_.clear()`, 새 감사 줄 `strip-refit=A/B`(1280d 뒤 `updateHeader()` 로 되돌린 뒤 1280p 글로 setStripTexts 두 번, 끝에 다시 `updateHeader()`) | AC3 · AC5 |
| `app/icons.cpp` · `icons.hpp` | `iconWithCaret(name, px, ink)` — `halo` 인자 삭제. 테는 `CompositionMode_Clear` 로 지우고(펜 2 px 둥근 끝, Clear 는 색을 쓰지 않아 `ink` 를 넘김) `SourceOver` 로 돌려 펜 없이 먹색 채움. 부르는 곳 둘(그릴 것 · 세로 과장) 고침 | AC7 |
| `app/mainwindow_ui.cpp` | 감사 carets ①–④: ① 빈 아이콘 · 장 폭 ≠ lround(16×dpr) 실패 ② need = lround(12 × 단추 dpr²) ③ 삼각형 바깥 (0.5, 1.0] 논리 px 띠의 alpha ≤ 64 비율 ≥ 90 % ④ `kerf::glyph` 와 견준 더해진 먹색 bbox ≥ (px/2 − 1)×dpr, 수 ≤ 24×dpr². 줄 꼴 `carets=1 caret-ink=N/M caret-band=P% caret-box=1 caret-area=K/L`. `<algorithm>` · `<climits>` include 명시(INT_MAX · std::clamp — MSVC CI 대비) | AC6 |

### 커밋
| SHA | 메시지 |
| --- | --- |
| `0cc52ff` | 감사 머리 refit=1/0 · title-kept 조이기, 머리 열쇠 칸별 비교, 헛 setIcon · caretBaked 정리 (R1b T1 · AC1 AC5 AC10) |
| `76ef15c` | 감사 정보 줄 gap-kept 숨김도 실패 · fitInfoStrip 입력 열쇠 · 새 판정 strip-refit=1/0 (R1b T2 · AC2 AC3 AC5) |
| `59c7a72` | 머리 ▾ 테 투명(CompositionMode_Clear) · halo 인자 삭제, 감사 carets 크기 · 띠 · 자리 · 넓이 조이기 (R1b T3 · AC6 AC7) |

빌드: 매 단계 `cmake --build C:\dev\kerf-wt\20261010-03-audit-tighten-build` exit 0, 경고는 기존 `_wfopen`(mainwindow.cpp) · `mirrored`(sheetexport.cpp) 두 개뿐. 줄바꿈: 바꾼 파일은 모두 CRLF 유지, `git diff --ignore-cr-at-eol --stat` 와 `git diff --stat` 같음.

### RED → GREEN 기록(감사 줄 원문)
**T1 · AC1** (임시: `updateHeader` 의 qScopeGuard 람다를 `--headerBatch_;` 만, 커밋 안 함)
- 옛 판정 + 임시(1920×1, `t1-old-red`): `refit=0/0 at=run:0:0,…` → **RESULT ok (거짓 통과)**
- 새 판정 + 임시(`t1-new-red`): `refit=0/0 title-kept=0 … FAIL` → EXIT 9
- 되돌린 뒤(`t1-1920` · `t1-1280`): `refit=1/0 title-kept=1` · `dup-icons=0` · `icons-missing=0` → RESULT ok. 되돌림 증거: 커밋 `0cc52ff` diff 에 `fitHeaderOnce` 줄 변경 없음.
- AC5 · AC10 grep: `caretBaked` 0건 · `kerf::icon(QStringLiteral("layers")` 0건 · `"%1|%2|%3|%4|%5"` 0건. AC5 는 먼저 실패하는 시험을 만들 수 없음(지금 감사 입력에 `|` 가 없음) — refit=1/0 · strip-refit=1/0 이 그대로인 것으로 확인.

**T2 · AC2** — 명세의 임시 변경(`apply` 안 `stripCut_->setVisible(step < 4)`)은 **실제로 아무것도 숨기지 않았다**: ④ 단계는 `apply(3)` 을 부르므로 step 4 에서도 보인다. 첫 시도(옛 판정 `t2-old-red`: `gap-kept=1 … at=…1280p:4:0` RESULT ok, 새 판정 `t2-new-red`: 여전히 `gap-kept=1`)는 판정을 재지 못한 것이라 버리고, 임시 줄을 `fitInfoStrip` 끝(`stripStep_ = step;` 앞)으로 옮겨 다시 쟀다(판정, 아래).
- 옛 판정 + 임시(1280×1, `t2-old-red2`): `strip-clipped=4 state-kept=1 gap-kept=1 elide-tip-missing=0 at=run:1:4,1280w:3:0,1280L:3:0,1280p:4:0,1280d:4:0,probe:2:0,1280v:3:0  FAIL` — 1280p 상태에서 gap-kept 는 거짓 통과(전체 FAIL 은 run 상태의 strip-clipped=4 때문 — 임시 줄이 숨겼다 다시 보인 라벨의 레이아웃 부작용으로 봄, 확인하지 못함)
- 새 판정 + 임시(`t2-new-red2`): `gap-kept=0 … FAIL`
**T2 · AC3** — 새 줄을 열쇠 없이(위 두 실행): `strip-refit=1/1  FAIL` (EXIT 9).
- 열쇠 + 임시 되돌림(`t2-1920` · `t2-1280`): `strip-clipped=0 state-kept=1 gap-kept=1 elide-tip-missing=0` · `strip-refit=1/0` · RESULT ok.

**T3 · AC6 · AC7** (1280×1)
- 옛 판정 + 임시 (a)(b)(c) **한 실행에 함께**(`t3-old-abc`): `carets=1 caret-ink=0/12 … RESULT ok` — 셋을 넣어도 옛 판정 통과(거짓 통과). 명세는 셋을 따로 재라 했으나 한 번으로 갈음(판정).
- 새 판정 + 고치기 전(흰 테) (`t3-new-white`) = **AC7 RED**: `carets=0 caret-ink=16/12 caret-band=0% caret-box=1 caret-area=12/24 … FAIL`
- Clear 구현 뒤(`t3-clear`): `carets=1 caret-ink=16/12 caret-band=100% caret-box=1 caret-area=12/24` RESULT ok → 이 PC 의 Qt 6.10.3 QPixmap 에서 Clear 가 알파를 0 으로 만든다(배율 1 · 2 에서 띠 100 %).
- 새 판정 + 임시 (a) `setIcon(QIcon())`(`t3-a`): `carets=0 caret-ink=0/12 … FAIL`
- 새 판정 + 임시 (b) Clear 단계 빼기(`t3-b`): `carets=0 caret-ink=17/12 caret-band=66% … FAIL`
- 새 판정 + 임시 (c) `w = px * 0.9`(`t3-c`): `carets=0 caret-ink=58/12 caret-band=90% caret-box=0 caret-area=55/24 … FAIL`
- 되돌린 뒤(`grep -rn TEMP- app/` 0건): 1280×1 `t3-final1280` · 1920×1 `t3-final1920` → `carets=1 caret-ink=16/12 caret-band=100% caret-box=1 caret-area=12/24 refit=1/0 title-kept=1` · `strip-refit=1/0` · `dup-icons=0` · `icons-missing=0` · RESULT ok.
- AC10 의 caret-ink 값: ×1 16/12 · ×1.25 24/19 · ×1.75 46/37 · ×2 72/48 — 명세 기대값과 같음(Clear 로 바뀌어도 삼각형 안쪽 먹색 수는 같음).
- 캡처(실제 화면, 이 PC 150 %): `C:\dev\tmp\r1b-shots\vex-on-1280.png`(`--size 1280x800 --vex 2 --shot`, EXIT 0). 눈 확인은 하지 않음 — kerf-ui-reviewer 몫.

### 멈춘 이유 — 문턱 거짓 실패(Q3, 몰래 풀지 않음)
1280 × 배율 4개(`t3-s1` · `t3-s1.25` · `t3-s1.75` · `t3-s2`, 최종 코드와 같은 판정):
- ×1 `caret-band=100%` RESULT ok · ×2 `caret-ink=72/48 caret-band=100% caret-area=58/96` RESULT ok
- **×1.25 `carets=0 caret-ink=24/19 caret-band=73% caret-box=1 caret-area=20/38 … FAIL`** (EXIT 9)
- **×1.75 `carets=0 caret-ink=46/37 caret-band=78% caret-box=1 caret-area=33/74 … FAIL`** (EXIT 9)

진단(임시 진단 줄, 커밋 안 함 — `t3-dbg`, ×1.25): 띠 판정에서 떨어진 픽셀은 전부 아래와 같다(같은 단추가 상태마다 되풀이).
```
layers d=1.25 px=12,12 dist=1.00 a=92  base=184
layers d=1.25 px=13,12 dist=1.00 a=127 base=255
layers d=1.25 px=14,12 dist=1.00 a=127 base=255
layers d=1.25 px=15,12 dist=1.00 a=96  base=192
layers d=1.25 px=11,17 dist=0.94 a=101 base=252
vex    d=1.25 px=10,12 dist=1.00 a=105 base=212
```
해석: 픽셀 중심이 삼각형에서 **정확히 1.00 논리 px**(×1.25 의 행 12 중심 = 12.5/1.25 = 10.0, 삼각형 윗변 y = 11)에 있고, alpha 가 바탕 획(base)의 약 절반이다. 2 px Clear 펜(테 1 px)의 **바깥 안티에일리어싱 경계**다. 배율 1 · 2 에서는 픽셀 중심이 0.5 · 1.5 처럼 경계에 닿지 않는다. 즉 띠의 바깥 한계(1.0)가 테의 바깥 한계와 겹쳐, 소수 배율에서 50 % 덮임 선을 재는 측정 기하 문제다 — 그림의 결함은 아니라고 본다. 그래도 통과시키려면 문턱(띠 정의) 또는 제품(테 폭)을 바꿔야 하므로 builder 가 정하지 않는다.
사람에게 물을 선택지:
- (a) 띠 바깥 한계를 테 안쪽으로: `(0.5, 1.0 − 0.5/dpr]`(경계 픽셀을 뺌). 배율 1 · 2 결과는 그대로일 것(확인 안 함)
- (b) 경계 줄(거리 > 1.0 − 0.5/dpr)만 alpha ≤ 128 허용
- (c) Clear 펜을 2.5 px(테 1.25 px)로 — 제품 변경(스펙 「테 1 px」와 다름)
이 문제가 풀리기 전에는 AC15 ②(1280x800 · 2560x1440 × 1.25 · 1.75)가 caret-band 로 FAIL 한다.

### 명세와 다르게 한 것 (판정)
- T2 RED 임시 변경 자리를 `apply` 안 → `fitInfoStrip` 끝으로 — 명세 자리는 ④ 단계가 `apply(3)` 을 불러 다시 보이게 하므로 아무것도 숨기지 않음(실행으로 확인) — 잘못이면: 없음(임시 변경, 커밋 안 함).
- T3 옛 판정 RED 를 (a)(b)(c) 한 실행으로 — 셋을 함께 넣어도 통과했으므로 — 잘못이면: 각 임시 변경의 옛 판정 줄 값을 따로 못 봄.
- Clear 펜 색으로 `ink` 를 넘김(글자 색 상수 대신) — Clear 는 원본 색을 쓰지 않음, theme 토큰 규칙과 부딪치지 않게 — 잘못이면: 없음.
- 감사 ③ 띠는 `kerf::caretTriangle`(그린 삼각형) 둘레로 잼. ④ 사분면 · 넓이는 글자 상수(px/2 − 1, 24) — 명세대로.
- 감사의 단추 아이콘 이름: 「그릴 것」은 `iconName` 속성, 세로 과장은 `theme::iconName(Ico::Vex)`(vexBtn_ 에 `iconName` 속성을 더하면 dup-icons 가 읽을 수 있어 더하지 않음).

### 확인하지 못한 것
- 정보 줄 열쇠가 글꼴 · 배율 변화(Resize 없이 글 폭만 바뀜)를 놓치는지 — 확인하지 못함(검토 초점 2).
- 1.25 · 1.75 에서 Clear 테의 눈 모양 — 캡처 없음.
- T4(AC8) · T5–T7 — 하지 않음. T4 끝 `factory verify … -UiSizes 1280x800,1920x1040 -UiScales 1,2` 는 돌리지 않았다(1차가 멈춘 상태라 PASS 로 보이면 상태를 잘못 알림). 단일 감사로 ok 를 본 조합은 1280 × 1 · 2 와 1920 × 1 뿐(1920 × 2 는 돌리지 않음).

## 수정 회차 기록
- 1회: (검토 뒤)
- 2회:
