# 구현 요약 — 20261009-01-b5-dpi 디자인 v5 B5 해상도 · DPI 안정성(4K · 와이드 · 배율, Windows만)

작성: builder · worktree `C:\dev\kerf-wt\20261009-01-b5-dpi` · 브랜치 `factory/20261009-01-b5-dpi`(기준 `0f7fbfe`) · 빌드 `C:\dev\kerf-wt\20261009-01-b5-dpi-build`
절차: `superpowers:test-driven-development`(Task 1 RED → Task 2–6 GREEN, Task 마다 커밋) · Task 2 원인은 `superpowers:systematic-debugging`(임시 탐침 → 원인 → 최소 수정, 탐침은 지움).

## 바꾼 것
| 파일 | 무엇을 | 왜(완료 기준 번호) |
| --- | --- | --- |
| `app/main.cpp` | `--size W×H` 를 `auditSize` 동적 속성으로(감사가 요청 크기를 앎) | AC1 |
| `app/mainwindow_ui.cpp` | `uiAudit` 0) `env` 줄(크기 · 최소 폭 · 넘침 · dpr · icon-dpr-bad) · 5b `dpr-rerender` · 2b `narrow` 에 actual-width/look/labels · plan-fits · section-fits · zoom-kept. `rebuildStartPage` 의 키 줄 · 한 줄 라벨 셋에 `QSizePolicy::Ignored`(가로). 실제 화면이면 ①은 화면에 드는 변만 · ④는 offscreen 만(판정 2) | AC1–AC4 · AC10 |
| `app/icons.hpp` · `app/icons.cpp` | `kerf::iconDprs()` = {1, 1.25, 1.5, 1.75, 2, 3} 한 곳, `icon` · `chipIcon` 이 이것만. 머리 주석 | AC2 · AC11 |
| `app/ribbon.cpp` | forceOn 칩의 손으로 적은 배율 목록 → `kerf::iconDprs()` | AC2 |
| `app/guideband.hpp` · `app/guideband.cpp` | `event()` 가 `QEvent::DevicePixelRatioChange` 를 받으면 아이콘을 무조건 다시 굽기(`bakeIcon`, `iconDpr_` 기억) + `place()` 안전판 + 감사용 `iconCacheKey()` | AC3 |
| `app/sectionview.hpp` · `.cpp` | `fitted_` + `fit()` 켬 · `wheelZoom` · `applyZoomAt` · `zoomBy` · 끌기 끔 · `resizeEvent` 맞춤이면 `fit()` · 감사용 `contentFits()` | AC4 |
| `app/planview.hpp` · `.cpp` | `fitted_` + `fitAll()` 켬 · `wheelZoom` · `applyZoomAt` · `zoomBy` · `setCamera` · `setViewPitch` · 끌기 · 돌리기 끔 · `resizeEvent` 재정의(기본 구현 먼저, 맞춤이면 `fitAll()` 아니면 `updateMatrices()`) · 감사용 `contentFits()`(지금 크기 · 지역 행렬) | AC4 · AC10 |
| `.github/workflows/ci.yml` | `linux-core` 작업 · 머리 주석 삭제, UI 감사 두 번(1920x1040 ×1 · 3840x2160 ×2)을 `Start-Process` + `WaitForExit(300000)` 로 기다려 종료 코드 · `ui-audit RESULT ok` 판정, 실행마다 폴더 따로 | AC8 |
| `docs/design/DESIGN_SPEC.md` §2.1 | 「dpr 1 · 1.25 · 1.5 · 1.75 · 2 · 3 여섯 장」 + B5 규칙 ①②③ | AC11 |

바꾸지 않음(AC7): `core/` `tests/` `tools/` `CMakeLists.txt` `app/CMakeLists.txt` — `git diff --stat 0f7fbfe -- core tests tools CMakeLists.txt app/CMakeLists.txt` 비어 있음. 본 저장소 쪽 문서(CLAUDE.md · DEV_PROGRESS · DEV_RULES R2.7 · profile · SKILL)는 손대지 않음.

## 커밋(`factory/20261009-01-b5-dpi`, 기준 `0f7fbfe`)
| SHA | 메시지(요지) | Task |
| --- | --- | --- |
| `b5640c0` | 시험: B5 감사 판정(RED) — env · dpr-rerender · narrow 줄 | 1 |
| `30a8413` | `--size` 요청 크기로 열리게: 홈 한 줄 글이 창 최소 폭을 밀어내지 않게 | 2 |
| `c9b081e` | 아이콘 배율 1.25 · 1.75(`kerf::iconDprs`) + 스펙 §2.1 | 3 |
| `504e6c1` | 안내 칩 아이콘 배율 바뀌면 다시 굽기 | 4 |
| `73493e2` | 창 크기 바뀌면 맞춤 상태만 다시 맞춤(`fitted_`) | 5 |
| `066455a` | CI: Linux 삭제 + 감사 기다려 판정 + 4K ×2 | 6 |
| `66737c5` | env 줄을 실제 화면에서도 판정(①은 화면에 드는 변만 · ④는 offscreen 만) | 7(AC10) |

(주: Task 2 커밋은 처음 메시지 앞에 PowerShell BOM 이 붙어 `d2cb823` 로 들어갔던 것을 push 전 내 브랜치에서 메시지만 고쳐 `30a8413` 으로 바꾸고 Task 3 을 그 위에 다시 올렸다 — 내용 diff 동일, `git diff --stat b5640c0 HEAD` 로 확인.)

## RED → GREEN(명령 · 결과 그대로)
- **Task 1 RED** `factory.cmd verify 20261009-01-b5-dpi -UiAudit on -UiSizes 1280x800,1920x1040 -UiScales 1` → `RESULT FAIL`, 두 감사 모두 exit 9(로그 `logs\20261009-215739`):
  - 1280: `ui-audit env size=1591x800 want=1280x800 scale=1 dpr=1.00 min-width=1280 overflow=0 icon-dpr-bad=2  FAIL`
  - 1920: `ui-audit env size=1920x1040 want=1920x1040 scale=1 dpr=1.00 min-width=1280 overflow=0 icon-dpr-bad=2  FAIL`
  - 둘 다: `ui-audit guide-step2=0 inside=1 apart=1 text=1 title=1 dpr-rerender=0 …  FAIL` · `ui-audit narrow rule-at-1280 look=20 labels=0 window-min-width=1280 actual-width=1280 actual-look=20 actual-labels=0 plan-fits=0 section-fits=0 zoom-kept=1  FAIL`
  - 기대와 다르게 GREEN 인 줄 없음(spec Task 1 기대값과 일치).
- **Task 2 GREEN**(`logs\20261009-221038`): 1280 `env size=1280x800 want=1280x800 … icon-dpr-bad=2  FAIL`(크기 판정만 GREEN). 원인(탐침 `PROBE before-show/after-show/after-applyScene`, `minimumSizeHint().width()` 과 자식 위젯): 홈이 보이는 채로 뜨면 `QStackedWidget` 최소 폭 = 홈 최소 폭 1591 — `#homeKeys` 1495, 고친 뒤 1498 = hero 612(끌어 놓기 안내 라벨) + 48 + continueCard 742(보조 줄 420 + 280) + 여백 96 → 셋 다 `Ignored` 뒤 홈 1146 · 창 1280×800 로 뜸.
- **Task 3 GREEN**(`logs\20261009-221327`): 1920 `env … icon-dpr-bad=0`(FAIL 없음). Qt 6.10.3 `QIcon::pixmap(QSize(18,18), 1.25)` → 23×23 dpr 1.25, `(…, 1.75)` → 32×32 dpr 1.75 를 그 배율 장 그대로 돌려줘 `kerf::glyph` 와 픽셀 동일 — spec Task 3 의 `blank()` 조정은 필요 없었음(`lround(px×dpr)` = `QSize×qreal` 의 `qRound` 와 같음: 22.5→23, 31.5→32, 62.5→63, 87.5→88).
- **Task 4 GREEN**(`logs\20261009-223234`): 1920 `guide-step2=1 … dpr-rerender=1`.
- **Task 5 GREEN**(`logs\20261009-223412`): 1280 · 1920 둘 다 `narrow … plan-fits=1 section-fits=1 zoom-kept=1` · `RESULT PASS`.
- **Task 6**: 로컬 모의 실행 `C:\dev\tmp\b5-ci-sim.ps1`(ci.yml 단계를 exe · 모델 경로만 바꿈): 양성 `ui-audit 1920x1040-x1 exit=0 ui-audit RESULT ok` · `ui-audit 3840x2160-x2 exit=0 ui-audit RESULT ok`(env `scale=2 dpr=2.00`) · `CI-SIM OK` exit 0. 음성(모델 `DoesNotExist.3mx`) → `ui-audit 1920x1040-x1 exit=2 (ui-audit.txt missing)` → throw, exit 1(단계가 이제 실제로 판정함).
- **V8(Task 6 HEAD `066455a`, `logs\20261009-223945`)**: `RESULT PASS` — 1280x800 ×1 · ×2, 1920x1040 ×1 · ×2, 3440x1440 ×1 · ×2, 3840x2160 ×1 · ×2 모두 `ui-audit RESULT ok`(63–66 s), 빌드 새 경고 0, tests(quick) 51325 통과, asec-section `polylines=1 vertices=54`.
- **V8(최종 HEAD `66737c5`)**: 아래 「최종 검증」.

## 명세와 다르게 한 것 (판정)
1. **Task 2 수정 위치** — spec 후보(`resize(1600,950)` 순서 · 첫 표시 최소 크기 · `body_` 최소 폭) 중 실제 원인은 「홈 페이지의 한 줄 글(키 줄 · 끌어 놓기 안내 · 카드 보조 줄 · 표 도움말)이 `QStackedWidget` 최소 폭을 밀어낸 것」. `main.cpp` 순서는 바꾸지 않고 `rebuildStartPage` 의 네 위젯에 가로 `Ignored`(이 함수가 홈 칸 값 `vl` · `pth` 에 이미 쓰는 방식). 잘못이면: 아주 좁은 창(≈1100 미만)에서 홈 키 줄 · 라벨 오른쪽이 잘릴 수 있음(창 최소 폭 1280 은 리본 `minimumSizeHint` 가 지키므로 실제로는 안 생김 — 1280 홈 캡처 `C:\dev\tmp\b5-shots\start-1280.png` 에서 키 줄 전부 보임).
2. **env 줄 ① · ④ 를 실제 화면에서 제한**(`66737c5`) — spec AC1 은 「가로 · 세로 모두 같음」 · 「`qFuzzyCompare(dpr, QT_SCALE_FACTOR)`」. 이 PC 실제 화면(150 %, 사용 가능 2293×912 논리)에서 `--size 1920x1040` 은 940 으로 잘리고 dpr 은 OS 배율 × 변수 = 1.50 이라 spec 그대로면 AC10 이 어떤 배율 모니터에서도 통과 불가(실측 `size=1920x940 want=1920x1040 … dpr=1.50 … FAIL`). 그래서 **offscreen 이 아니면** ①은 요청 변이 `screen()->availableGeometry()` 안에 들 때만 같음을 요구, ④는 건너뛰고, 줄 끝에 ` screen-avail=WxH` 를 덧붙임. offscreen(V8 · CI)은 spec 그대로 판정. 잘못이면: 실제 화면에서 QT_SCALE_FACTOR 가 안 먹는 회귀를 감사가 못 잡음(offscreen 8가지 · CI ×2 가 잡음).
3. **env 줄 형식에 선택 꼬리 하나 추가**(` screen-avail=…`, 실제 화면일 때만) — spec §3-A 정본 이름은 그대로, offscreen 출력은 변함없음. 잘못이면: 실제 화면 로그를 파싱하는 쪽이 꼬리를 모를 수 있음(없음).
4. **Task 2 커밋 메시지 교정으로 내 브랜치 두 커밋을 다시 올림**(위 커밋 표 주) — push 안 된 작업 브랜치 안에서만. 잘못이면: 없음(diff 동일 확인).

## 검증 방법 변경
- 시험 기대값을 바꾸지 않음. 판정 2 는 「실제 화면에서만」 판정 범위를 줄인 것이고 offscreen 기대값은 spec 그대로.
- 허용오차: `contentFits` 경계 1e-6 m(profile §7) · `zoom-kept` 는 같음(허용오차 없음) · 아이콘 픽셀 완전 일치 · dpr `qFuzzyCompare` — 새 허용오차 없음.

## context7 출처(`/websites/doc_qt_io_qt-6_8`)
- `QIcon::pixmap(const QSize&, qreal devicePixelRatio, Mode, State)`(6.0+): 「might be smaller than requested, but never larger, unless dpr > 1 … The requested devicePixelRatio might not match the returned one … since Qt 6.8 it's the device independent size」. 어느 장을 고르는지는 문서에 없음 → Task 3 실측(위 GREEN 줄)으로 판단.
- `QWindow::devicePixelRatio`: 「receives an event of type QEvent::DevicePixelRatioChange when the device pixel ratio changes」. `QFrame::event(QEvent*)` protected virtual → `GuideBand::event` 재정의. 자식 위젯까지 전달되는지는 확인하지 못함(spec §5) → `place()` 안전판.
- `QOpenGLWidget::resizeEvent`: 「Avoid overriding … If that is not feasible, make sure that QOpenGLWidget's implementation is invoked too」 → `PlanView::resizeEvent` 가 기본 구현을 먼저 부름. `resizeGL` 은 「called whenever the widget has been resized」지만 GL 이 없는 offscreen 에서는 불리지 않음(실측 `first-draw-ms=-1`).

## 회귀 · 캡처 · 실제 화면(Task 7)
- §4 회귀 3–5(실제 화면, worktree exe, `--settings` 새 폴더, `Start-Process` + `WaitForExit`): `--icon-sheet` → `icon-sheet n=62 missing=0 … RESULT ok` EXIT 0 · `--sheet-check` → `level-pt minor=0.564 expect=0.564 major=1.411 expect=1.411 ok=1` · `sheet-check RESULT ok` EXIT 0 · `--ctx-shot` → `ctx-shot ok: … guide=1` EXIT 0(`C:\dev\tmp\b5\r224034\`).
- AC11: `git diff 0f7fbfe -- app` 의 `+` 줄에 `C:[\\/]` · `HKEY_` · `NativeFormat` 없음(Grep 0건). DESIGN_SPEC §2.1 · icons.cpp 머리 주석 = 여섯 장.
- AC9 캡처(`shot.ps1 -Exe <worktree exe>`, `C:\dev\tmp\b5-shots\`): `work-1280.png` · `work-1920.png` · `work-3440.png` · `work-3840.png` · `draw-1280.png` · `start-1280.png` · `work-1920-x2.png`(QT_SCALE_FACTOR 2) · `work-1920-x1.25.png`(1.25). **이 PC 화면은 150 %(dpr 1.5) · 논리 약 2293×940** 이라 그림 픽셀 = 논리 × 1.5: `start-1280.png` 1920×1200 px = 논리 1280×800(spec 의 「픽셀 1280×800」은 100 % 화면 기준 — 여기서는 논리 크기로 확인). 3440 · 3840 요청은 화면에 잘려 3444×1410 px(논리 2296×940)로 찍힘 — **환경 부족(그 크기 모니터 없음)**, 그 크기는 offscreen 감사 8가지로 대신. 눈으로 본 것: 홈 1280 키 줄 · 라벨 잘림 없음, 1.25 배율 아이콘 선명, 1920 ×2 리본 · 칩 선명.
- AC10 실제 화면(`QT_QPA_PLATFORM` 없이 `--size 1920x1040 --ui-audit`, `C:\dev\tmp\b5\ac10-224903\`): `EXIT=0` · `ui-audit RESULT ok` · `env size=1920x940 want=1920x1040 scale=1 dpr=1.50 min-width=1275 overflow=0 icon-dpr-bad=0 screen-avail=2293x912` · `narrow … plan-fits=1 section-fits=1 zoom-kept=1` · `stream: first-draw-ms=117`(GL 이 실제로 뜸). 사람 절차(창 모서리 끌기 · 휠 뒤 넓히기)는 **미실행(사람 검토 몫)**.
- AC3 수동(모니터 둘 배율 다름): **미실행(환경 부족 — 이 PC 모니터 수 · 배율 차이 확인 못 함)**.
- AC8 ③ GitHub Actions: **미실행(push 전)** — `factory.cmd ci <병합 커밋> -Wait` 는 사용자 push 뒤.

## 최종 검증(HEAD `66737c5`, worktree 미커밋 변경 없음)
- **V8** `factory.cmd verify 20261009-01-b5-dpi -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2`(`logs\20261009-224927`) → **`RESULT PASS`** exit 0: build exit 0 새 경고 0 · tests(quick) 51325 통과 · asec-section `polylines=1 vertices=54` · ui-audit 1280x800 ×1 ok · ×2 ok · 1920x1040 ×1 ok · ×2 ok · 3440x1440 ×1 ok · ×2 ok · 3840x2160 ×1 ok · ×2 ok(각 63–66 s). ci(base) `0f7fbfe` = Windows failure · Linux success(기준 커밋 쪽, 참고).
- **AC6** `… -UiSizes 1920x1040 -UiScales 1.25,1.75`(`logs\20261009-225840`) → **`RESULT PASS`**: `env size=1920x1040 want=1920x1040 scale=1.25 dpr=1.25 … icon-dpr-bad=0` · `scale=1.75 dpr=1.75 … icon-dpr-bad=0`, 둘 다 `ui-audit RESULT ok`.

## 남은 문제
- **1280 창의 단면 머리 · 정보 줄**(`C:\dev\tmp\b5-shots\work-1280.png`): 가운데 칸이 약 330 px 라 단면 머리 「A–A′ 북쪽을 봄」 글이 토글 단추와 겹치고 정보 줄 「입면 뒤 0–3.0…」 「레벨선 10 cn」 이 잘림. 이 작업 전에도 있던 모양(머리 · 정보 줄은 `Ignored` 라 넘침) — B5 범위(크기 · 배율 · 맞춤)가 아니라 좁은 창의 머리 줄임 규칙(디자인 결정)이라 고치지 않음. 후속 작업 제안: 좁으면 부제 숨김 → 토글 묶음 아이콘만 → 축척 칸 숨김 순서.
- 기준 커밋 `0f7fbfe` 의 GitHub Actions(사용자가 push 한 듯): Windows 작업 `failure`, Linux `success`(`verify.md` ci(base) 줄) — 이 브랜치 변경과 무관, 원인은 Actions 로그로 따로 봐야 함.
- 홈 앱 아이콘 56 px(dpr 2 고정) · 「작업 순서」 체크 표시는 화면을 옮겨도 다시 굽지 않음(spec §2 범위 밖).
- DEV_RULES R2.7 「dpr 1 · 1.5 · 2 · 3」 문구는 본 저장소에서(spec 판정).

## 수정 회차 기록
- 1회: (없음 — 첫 구현)
