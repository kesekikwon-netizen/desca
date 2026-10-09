# 독립 검토 — 20261010-01-narrow-1280 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건

검토 대상: `factory/20261010-01-narrow-1280` HEAD(verify.md 의 마지막 블록과 같은 SHA 여야 함). 검토자는 제품 코드를 고치지 않는다.
판정: **blocking**(고쳐야 함) · **should**(권장) · **note**(참고). blocking 이 하나라도 있으면 rework.

## code-reviewer (결함 · 유지보수성 · 동시성)
- 판정: **fail** (blocking 1 · should 2 · note 6)
- 범위: `git -C C:\dev\kerf-wt\20261010-01-narrow-1280 diff 308daac a2e2228`(코드 커밋 9, 11파일). 그 뒤 `c9214e2` 는 `docs/design/DESIGN_SPEC.md` +3줄뿐이다(`git diff --stat a2e2228 HEAD`). 그래서 아래 줄 번호는 `a2e2228`(= 지금 HEAD 코드) 기준이다. 근거는 코드 읽기와 이미 있는 캡처 · 감사 기록이다. 앱은 실행하지 않았다.
- 강점:
  - `app/mainwindow_ui.cpp:1314` `clearLayout`: 위젯은 `hide()` + `deleteLater()`, 하위 레이아웃은 재귀로 비운 뒤 항목을 `delete` 한다. AC14 의 원인(격자 안 위젯이 남던 것)을 바로 고친다. `showStart`(1548–1557)도 그리기를 두 번 하던 것을 한 번으로 줄였다.
  - 「차례대로 줄이기」 두 함수(`fitSectionHeader` 630 · `fitInfoStrip` 668)는 재진입 막기 플래그를 쓰고 반복 횟수에 상한(4 · 6)이 있다. 머리(32) · 정보 줄(26)은 높이가 고정이고 가로가 `Ignored`(491 · 547)라 자기 크기가 바뀌지 않는다. 그래서 크기 고리가 생기지 않는다.
  - 그리기와 감사가 같은 함수를 쓴다. `PlanView::tickLabels`(`planview.cpp:669`)를 `paintOverlay` 와 `tickLabelRects` 가 같이 쓴다. `chipPixmap`(`icons.cpp:135`)은 `chipIcon` 과 `Ribbon::chipReference` 가 같이 쓴다. 행렬 식은 `makeMatrices`(334) · `unprojectToRefZ`(346) 한 곳에 있다. 그래서 「판정 = 그림」이 코드 구조로 지켜진다. `unprojectToRefZ` 에 `w <= 0 || h <= 0` 막이도 생겼다(전에는 0 으로 나눌 수 있었음).
  - 그리기 · 그릴 것 메뉴는 같은 `QAction` 넷을 쓴다(`mainwindow_ui.cpp:515`). 체크 상태와 키 I · L · V 를 따로 맞추는 코드가 없어 어긋날 일이 없다.
  - 저장소 규칙: `Q_OBJECT` 없음 · 람다 연결 · `static_cast`(`inherits` 확인 뒤) · 새 색 없음(`theme::Hand` · 기존 서식 색만) · core/tests/CMake/CI 변경 없음.
- 지적:
  - **blocking 1 — 홈 「작업 순서」 머리의 「<모델> 기준」 글이 모든 창 폭(1920 포함)에서 사라짐 — 스펙 §1 「1920 이상 모양은 바뀌지 않는다」 · AC11 위반, 이번 변경 탓**
    - 어디: `app/mainwindow_ui.cpp:1496–1497` — `hd->addStretch(); hd->addWidget(elideLab(lab(「%1 기준」, "cap"), Qt::ElideMiddle));`.
    - 무엇 · 왜: `elideLab`(152)이 가로를 `QSizePolicy::Ignored` 로 바꾼다. `QBoxLayout` 은 `Ignored` 항목의 sizeHint 폭을 0 으로 본다. 이 항목에는 늘어나는 표시(ExpandFlag)도 없다. 같은 줄 앞의 `addStretch()` 는 늘어나는 빈 칸이다. 그래서 남는 폭은 전부 빈 칸으로 가고 라벨은 폭 0 으로 놓인다. 그러면 Resize 필터(386–389)가 글을 폭 0 에 맞게 줄여 빈 글이 된다. 기준 `308daac` 에서는 이 라벨이 기본 정책이었다(diff 의 `-` 줄).
    - 증거: builder 실제 화면 `C:\dev\tmp\n1280-shots\home-1920.png` — 「작업 순서」 오른쪽이 비어 있다. 같은 합성 모델의 옛 캡처 `C:\dev\tmp\kerf-shots\v5-d1-home.png` 에는 「Synthetic 기준」이 오른쪽 끝에 있다. `home-1280.png` 도 비어 있다.
    - 재현: 합성 모델을 연다 → 홈 탭, 창 1920×1040 → 「작업 순서」 줄 오른쪽 끝에 「Synthetic 기준」이 없다(툴팁만 남음).
    - 감사가 못 잡은 이유(10조합 모두 초록인데 1920 이 바뀐 까닭): `clippedLabels`(131–141)는 `width() < sizeHint().width()` 만 센다. 빈 글로 줄어든 라벨은 sizeHint 도 0 근처라 통과한다. `elide-tip-missing` 도 툴팁이 전체 글이라 통과한다. 그래서 새 잠금 `home-clipped=0` 은 「통째로 사라진 라벨」을 볼 수 없다.
    - 다른 `elideLab` 자리도 훑어봤다(1366 · 1381 · 1382 · 1398 · 1399 · 1421 · 1428 · 1461 · 1462 · 1489). 모두 세로 레이아웃 안이거나, 폭이 정해진 열(1428 썸네일 열 = 단추 76) 안이다. 늘어나는 빈 칸 뒤에 홀로 선 자리는 1497 하나다. 캡처에서도 나머지 글(카드 이름 · 보조 줄 · 칸 · 썸네일 「A–A′」 · 표 · 도움말)은 1920 에서 보인다. 그러니 고칠 곳은 한 군데다.
    - 고치는 법(글로): 이 짧은 캡션은 `elideLab` 에서 빼고 전처럼 sizeHint 폭으로 둔다. 창 최소 폭은 홈 오른쪽 열 `setColumnMinimumWidth(2, 280)`(1343)이 정하므로 이 글이 최소 폭을 밀 일은 거의 없다. 긴 모델 이름이 걱정되면 다른 방법이 둘 있다. `Ignored` 대신 `Preferred` 와 `setMaximumWidth` 를 함께 쓰고 그 폭에서 줄임표를 넣는 방법, 또는 빈 칸을 없애고 라벨에 stretch 1 과 오른쪽 정렬을 주는 방법이다. 어느 쪽이든 should 1 의 감사 보강으로 잠근다.
  - **should 1 — 감사 `clippedLabels` 가 「글이 통째로 사라진 라벨」을 세지 않음(blocking 1 이 초록으로 통과한 원인)**
    - 어디: `app/mainwindow_ui.cpp:131–141`(`home-clipped` · `strip-clipped` 가 같이 씀).
    - 왜: 줄임표 라벨은 보이는 글이 짧아질수록 sizeHint 도 같이 줄어 「폭 < sizeHint」에 걸리지 않는다. 폭 0 이 되어 글이 빈 경우도 마찬가지다.
    - 재현: 지금 코드 → 1920 · 1280 감사 `home-clipped=0` 인데, 캡처에는 「Synthetic 기준」이 없다.
    - 고치는 법(글로): 보이는 라벨 중 동적 속성 `fullText` 가 비어 있지 않은데 보이는 글이 비었거나, 폭이 「…」 글 폭(`fontMetrics().horizontalAdvance(QChar(0x2026))`)보다 좁으면 잘림으로 센다. 지금 코드로 RED(`home-clipped≥1`)를 먼저 확인한다. 그다음 blocking 1 을 고쳐 0 이 되는 것을 본다. kerf-ui-reviewer B1 이 제안한 「칸 라벨 줄임 수」 줄과 함께 넣어도 된다.
  - **should 2 — 머리 · 정보 줄 다시 맞추기가 매번 「모두 보임」부터 다시 시작하고, updateHeader 한 번에 두 번 돈다**
    - 어디: `fitSectionHeader` 630–666(642 제목을 전체 글로 되돌림 → 644 `apply(0)…apply(k)` 마다 `activate()`), `updateVexUi` 끝 624, `updateHeader` 728(`updateVexUi`) · 731(`setSecTitleText`) · 772(`setStripTexts`). 부르는 곳 `onViewChanged = [this]{ updateHeader(); }`(1966)는 `sectionview.cpp:378` 에서 ppm 이 바뀌는 그림마다(부드러운 확대 애니메이션 프레임마다) 불린다.
    - 무엇 · 왜: 1280 처럼 단계 5 인 창에서는 부를 때마다 「북쪽을 봄」 · 「크게」 · 토글 상자를 실제로 보였다 숨긴다. 「그릴 것」 단추는 숨겼다 보인다. 제목도 전체 글 → 줄인 글로 바꾼다. 그 사이에 레이아웃을 최대 6 번 다시 잰다. 이 일이 확대 한 프레임에 머리 2회 + 정보 줄 1회 일어난다. 무한 고리는 없다(위 강점). 같은 함수 안에서 끝나 그림이 그려질 틈이 없으니 눈에 보이는 깜박임도 없다고 판단한다. 다만 매 프레임 Show/Hide 사건과 다시 그리기 요청이 생긴다. 메뉴 단추가 열려 있는 동안 계산 결과가 와서 `updateHeader` 가 돌면, 열린 메뉴의 단추가 한 번 숨겨졌다 다시 보인다(그 영향은 확인하지 못함 — 보류 ①).
    - 재현: 1280 창 → 단면 칸에서 휠로 확대 → 프레임마다 `fitSectionHeader` 가 5단계를 다시 훑는다(코드 경로, 계측은 하지 않음).
    - 고치는 법(글로): 마지막 입력(머리 폭 · 전체 제목 · 세로 단추 글 · 상태 글들)을 기억해 두고, 같으면 바로 돌아간다. 또는 지금 단계 `secHeaderStep_` 에서 먼저 재 보고 필요할 때만 위아래로 옮긴다. 그리고 `updateHeader` 안에서는 `fitSectionHeader` 를 맨 끝에 한 번만 부르게 한다(지금은 `updateVexUi` 에서 옛 제목으로 한 번, `setSecTitleText` 에서 한 번 — 앞의 것은 헛일).
  - note 1 — `app/mainwindow_ui.cpp:512–517` 「그릴 것」 단추에는 `setAccessibleName` 이 없다(토글 단추 넷은 506 에서 단다). 툴팁이 있어 R2.8 위반은 아니다. 화면 읽기 프로그램을 위해 이름을 달면 좋다.
  - note 2 — `fitInfoStrip` 673 의 서식 지우기 정규식 `<[^>]*>` 는 `&amp;` · `&lt;` 같은 문자 참조를 풀지 않는다. 지금 상태 글에는 그런 것이 없다. 다만 나중에 상태 글에 `&` 가 들어가면 줄인 글(PlainText)에 `&amp;` 가 그대로 보인다. `QTextDocumentFragment::fromHtml(...).toPlainText()` 같은 변환이 더 안전하다.
  - note 3 — `clearLayout` 이 `MainWindow` 의 public static(`mainwindow.hpp:140`)이다. 순수한 레이아웃 도움 함수라 자리가 어색하다. `sheetexport.cpp` 가 이미 `mainwindow.hpp` 를 포함하므로(3행) 빌드 문제는 없고 자리만의 문제다.
  - note 4 — 감사 끝에서 세로 과장을 늘 ×1 로 되돌린다(`mainwindow_ui.cpp:2422`). `--vex N` 과 `--ui-audit` 를 함께 주면 처음 값으로 돌아가지 않는다. 감사 길에서만 생기고, 지금 스크립트 · CI 는 둘을 같이 쓰지 않는다.
  - note 5 — `pickTickLabelStep`(`planview.cpp:649`)은 10 m 로도 모자랄 때(아주 많이 축소함) 10 을 돌려준다. 그러면 숫자 글이 여전히 겹친다. 스펙 「없으면 10」 그대로이고 전(1 m 마다)보다는 낫다. 후속에서 「그때는 숫자를 숨김」을 생각해 볼 만하다.
  - note 6 — 이름 없는 수: 제목 바닥 24 px(652) · 정보 줄 바닥 40 px(694) · 되풀이 4 · 6 회 · probe 1600×1000(2353). probe 는 스펙 값이라 괜찮다. 나머지는 주석에 「왜 그 값인지」 한 마디가 있으면 좋다.
  - (참고, 남의 지적과 겹침) AC13(DESIGN_SPEC §12 새 키 누락)은 test-runner 가 잡았다. `c9214e2` 가 고쳤는지는 내 범위(`a2e2228` 까지의 코드)가 아니다.
- 물어 온 것에 대한 결론:
  - `clearLayout` 수명 — **문제없음.** 「목록에서 지우기」(1479–1485)의 흐름은 이렇다. `tbl` 의 `customContextMenuRequested` 람다 → `QMenu m(this)` · `m.exec()`(중첩 루프) → 동작 람다 → `rebuildStartPage()` → `tbl` 이 든 `recent` 위젯을 `deleteLater`. 지연 삭제는 그것을 올린 루프 단계를 벗어난 뒤(바깥 루프)에야 처리된다. 그래서 `m.exec()` 가 돌아온 뒤에도 `tbl` 은 살아 있고, 람다는 exec 뒤에 `tbl` 을 쓰지 않는다. 다른 람다가 잡은 위젯도 확인했다. `tbl`(1475 문맥 객체 · 1479 보내는 쪽) · `find` · `go` · `pic` · `all` 은 모두 보내는 쪽이거나 문맥 객체라 함께 끊긴다. `recentMenu_` 는 `rb->setMenu` 로 달릴 뿐 소유가 옮겨지지 않아 `rb` 가 지워져도 남는다. `rebuildStartPage` 는 홈 위젯을 멤버 포인터에 담지 않는다. 「이어서 열기」처럼 자기 클릭 처리 중에 홈이 다시 그려져도, `hide()` + 지연 삭제라 돌아오는 길이 안전하다.
  - 머리 줄이기의 resize 되풀이 — **무한 고리 없음 · 깜박임 없음**(위 강점). 비용만 should 2.
  - 공유 QAction 메뉴의 체크 상태 — **문제없음**(같은 객체). 메뉴는 단추가 부모(515)라 단추와 함께 사라진다.
  - 이벤트 필터 · 타이머 수명 — 새 타이머는 없다. 필터는 전부터 있던 `qApp->installEventFilter(this)`(1693)에 갈래만 더했다. 새 갈래는 멤버 포인터를 같은지만 비교하고(386–387) 속성 이름으로 고른다(388 · 392). 홈 라벨 · 키 줄은 새로 만들 때마다 자기 Resize 를 받아 다시 줄인다.
- builder 판정 10개 의견:
  1. probe 뒤 `fitAll` · `fit` 명시 — 동의. 판정을 약하게 한 것이 아니라 전제를 만든 것이다. 끄면 `section-fits=0` 이 되는 판별력 증명도 있다.
  2. Task 7 · 8 · 9 를 한 빌드로 GREEN — 받아들임. 서로 다른 판정 줄이고 ①② 임시 변경 증명이 있다.
  3. `home-stale-children` 을 좁은 창 블록 안에서 잼 — 받아들임. 판정식이 §8 과 같다.
  4. `sheetexport.cpp:951` 변경 — 동의. §8 이 그렇게 하라고 했고, 옛 줄(숨김 + 지연 삭제 + 항목 삭제)과 평평한 레이아웃에서 동작이 같다.
  5. 줄임표 대상을 넓힘 — 원칙은 동의한다. 하지만 1497 「… 기준」은 늘어나는 빈 칸 뒤라 폭 0 이 되어 사라진다 → **blocking 1**. 1398 칸 이름은 kerf-ui-reviewer B1 이 다룬다.
  6. 키 묶음을 QWidget 으로 감쌈 — 받아들임. 여백 0 · 간격 8 그대로다.
  7. 4단계에서 상태 글을 PlainText 로 — 받아들임. 「✓」 초록이 빠지는 것은 가장 좁을 때뿐이다(spec §5 위험 3).
  8. 「그릴 것」 단추에 QSS 를 더하지 않음 — 토큰 규칙상 받아들임. ▾ 가 없는 것은 kerf-ui-reviewer S1 몫이다.
  9. `px-per-m = 1/mpp` 로 기록 — 받아들임. 기록값일 뿐이고 판정(사각형 겹침)과는 상관없다.
  10. 눈금 글 사각형 폭 = 글 폭 + 2 — 받아들임. 스펙 지시 그대로이고, 가운데 정렬이라 보이는 자리는 같다.
- 판단을 보류한 것:
  - ① 열린 InstantPopup 메뉴의 단추를 다시 맞추는 동안 숨겼다 보일 때 Qt 가 메뉴를 어떻게 다루는지는 확인하지 못했다(context7 · 실행 없음). 그래서 should 2 의 그 부분은 위험 표시로만 적는다.
  - ② `QFontMetrics::elidedText` 가 폭 0 에서 빈 글을 돌려준다는 것, 숨김 · 보임이 부모 레이아웃을 바로 무효로 만든다는 것, 같은 값의 `setVisible` · `setText` 는 아무 일도 하지 않는다는 것 — 이 셋은 Qt 소스 기억에 기댄 판단이다. blocking 1 의 결과(글이 없음)는 캡처로 확인했다.
  - ③ should 2 의 실제 비용(프레임당 µs)은 재지 않았다.
  - ④ 125 · 175 % 실제 모니터는 보지 못했다(offscreen V2 로 갈음).
  - ⑤ 1280 카드 칸 줄임(「마지…」 「EPS…」 「KVD…」)이 받아들일 만한지는 kerf-ui-reviewer 의 판단(B1)에 맡긴다. 내가 따로 매기지 않는다.
  - ⑥ kerf-ui-reviewer N6 은 「1920 모양 그대로 — 충족」이라고 적었다. 하지만 같은 `home-1920.png` 에서 「Synthetic 기준」이 빠져 있다(blocking 1). 이 차이는 종합에서 맞춰 볼 일이다.
- 수: 판정 fail · blocking 1 · should 2 · note 6

## correctness-reviewer (좌표 · 단위 · 수직 기준 · 계산 · 입출력 정확성)
- 판정: **pass** (blocking 0 · should 4 · note 7)
- 범위: 코드 `git -C C:\dev\kerf-wt\20261010-01-narrow-1280 diff 308daac a2e2228` 전체(11파일). 그 뒤 HEAD `c9214e2` 는 `docs/design/DESIGN_SPEC.md` +3줄(문서만, `git show --stat`)이고 verify.md 07:38 블록이 그 SHA 로 PASS — 판정에 영향 없음. 줄 번호는 `a2e2228` 기준. 근거는 코드 읽기 · 손계산 · verify 로그(`…-build\logs\20261010-052745` · `…053705` 의 `ui-audit.txt` 10개). 앱은 돌리지 않았다.
- 강점(물어 온 것 ①–⑤):
  - ② 행렬 식이 같다: `makeMatrices`(`app/planview.cpp:334`)는 옛 `updateMatrices` 와 같은 식(ortho ±w·mpp/2 · ±h·mpp/2 · ±4·diag, X 축 −(90−pitch), Z 축 −yaw, −target 이동)이다. 옛 `contentFits` 는 새로 만든 행렬(처음부터 단위 행렬)에 ortho 를 걸었으니 `setToIdentity` 와 같다. `unprojectToRefZ`(:346)는 옛 `screenToLocalXY` 와 같은 식(NDC z −1 · +1 두 점 → refZ 평면 교점)이고, 행렬 · A · B 는 장면 중심 기준 float, `center_` 는 double 로 더한다(실좌표 = 로컬 + center, float 는 로컬에서만 — 규칙 그대로). 달라진 것은 w · h ≤ 0 에서 false 를 돌려주는 것뿐이다(옛 코드는 0 으로 나눠 NaN 좌표를 true 로 돌려줌) — 개선.
  - ① 눈금 숫자 = 실제 거리: 글 자리 `f.planXY(s, 0)`, 글 `s`('f', 0), s = k·step(정수 덧셈이라 정확) — A 에서 잰 수평 거리 그대로다. A↔A′ 반전 때도 A 기준(전과 같음). 눈금 선은 0.5 m 반복 · 1 m 굵기(`meter`) 그대로이고 숫자만 `tickLabels()` 로 옮겼다. px/m = 화면 단면선 길이(`localToScreen`, `width()` · `height()` 기준 = 논리 px) ÷ L, 글 폭 = `monoFont(11)`(`setPixelSize` — 논리 px, DPI 와 무관). 둘 다 논리 px 라 dpr 과 섞이지 않는다 — 로그로 확인: 1280 ×1 · ×2 · ×1.25 · ×1.75 모두 `px-per-m=17.4 step=5`, 1920 ×1 · ×2 모두 `run:2`. ortho 라 화면에서도 숫자 간격이 일정(= step × px/m)하다. 판정 사각형 = 그림 사각형(같은 `tickLabels()`).
  - ① 손계산: offscreen 1280 의 17.4 px/m 에서 step 2 는 글 ≤ 26.8 px, step 5 는 ≤ 79 px 일 때 고른다. □ 글꼴(폭 약 2배)은 5, 실제 Consolas 11 px 「11 m」(≈ 4 × 6.05 = 24 px)이면 2 → 실제 화면 캡처 「2 m 4 m 6 m 8 m 10 m」와 맞는다.
  - ① 어느 방향 단면선이든 겹치지 않는가(손계산): 사각형 W = 글 폭 + 2, H = 글꼴 높이 + 2, 중심 간격 d ≥ 가장 넓은 글 폭 + 8 = W + 6. 겹치려면 |dx| < W 이고 |dy| < H 여야 하는데, |dx| < W 면 dy² > (W+6)² − W² = 12W + 36. 그래서 12W + 36 ≥ H² 면 겹치지 않는다. Consolas 11 px 「1 m」 W ≈ 20 · H ≈ 15 → 276 ≥ 225. 스펙 §4 손계산(높이 16)과 결론이 같다.
  - ⑤ Q3: `setVerticalExaggeration` 은 값이 바뀌고 결과 · 그림 칸이 있을 때만 `fitted_ = false`(`app/sectionview.cpp:482`). 「맞춤」 단추와 두 번 클릭(:428)은 `fit()`(:37)을 부르고, `fit()` 은 `max(1, vex)` 로 위아래까지 넣은 뒤 `fitted_ = true` — 세로 과장을 지킨 채 다시 맞춤 상태가 된다. `vex-fit` 의 「허용오차 없는 같음」은 타당하다: 맞춤이 아닌 `resizeEvent` 길(:58–68)은 `xf_.ppm` 을 건드리지 않고, probe ≠ 1280×800 이라 RED(`30.2589/15.2659`)가 결정적이다.
  - ④ `kNarrowTile = 20`(`app/mainwindow_ui.cpp:2348`) = DESIGN_SPEC §2.1(43행) · §3.3(79행) · §12(146행)의 20, `Ribbon::looks()` 표의 20(아이콘 12, 글자 숨김)과 같다. 24 로 바꾸면 FAIL(build.md).
  - ③ env 넘침 `>=`: `QRect::right() = x + w − 1` 이라 `right() >= width()` ⇔ x + w > width — 1 px 넘침부터 잡고 딱 맞닿음(x + w = width)은 통과한다. 정확.
  - ③ `home-stale-children`: `clearLayout` 이 위젯을 `hide()` 한 뒤 `deleteLater` 하므로 `isVisibleTo` 가 삭제 대기분을 이미 뺀다 — 지연 삭제가 언제 처리되든 같은 판정이고, RED 25 → 0 은 실제다. `chip-dpr-bad` 는 자기 자신과 비교가 아니다(N1) — 1.25 장을 빼면 21 FAIL(build.md)로 판별력이 있다.
  - ③ narrow probe: 1280 실행에서 probe 로 키운 뒤 `fitAll()` · `fit()` 을 명시한 것(builder 판정 1)은 판정을 약하게 하지 않는다 — probe 크기의 축척을 만들어 두어, 1280 으로 돌아올 때 다시 맞춤이 없으면 들어가지 않게 한다(끈 실험 `section-fits=0`). 1920 이상 실행은 이 줄을 건너뛰어 「열고 난 뒤 자연스러운 맞춤 상태」를 그대로 잰다.
- 지적:
  - **should C1 — 정보 줄 4단계가 「빈 구간 있음/없음」을 잘라 낸다. 스펙은 「늘 보인다」고 적었고, 감사는 이것을 못 잡는다**
    - 어디: `app/mainwindow_ui.cpp:684–700`(`fitInfoStrip` 4단계). 상태 글과 잘린 선 글을 넘친 만큼 비례로 줄여 **끝** 줄임표(`ElideRight`)로 자른다. 빈 구간 글은 잘린 선 글의 맨 끝(「잘린 선 n줄 · 빈 구간 k곳 x m」)이라 가장 먼저 잘린다.
    - 기준: DESIGN_SPEC §4(87행) 「계산 상태 · 빈 구간 있음/없음은 늘 보인다」, 사용자 답 Q2 A 의 결과 문장도 같다.
    - 손어림(실제 화면 1280, 정보 줄 ≈ 328, 맑은 고딕 12 px 어림): 미리보기 상태 「단면선 정확 · 배경 미리보기 → 최종 계산 중…」 ≈ 230 + 짧은 글 「잘린 선 1줄 · 빈 구간 없음」 ≈ 140 + 간격 16 + 여백 16 ≈ 400 > 328 → 4단계. 넘침 ≈ 74 를 (230−40):(140−40) 비례로 나누면 잘린 선 글 ≈ 115 px → 「잘린 선 1줄 · 빈 구…」. 끄는 동안 · 최종 계산 중에는 빈 구간 정보가 사라진다. 최종 「✓ 최종(잎)」이 오면 2단계라 다시 보인다(실제 캡처는 이 상태라 안 보였다). 「빈 구간 12곳 10.2…」처럼 숫자 중간에서 잘릴 수도 있지만 「…」 + 툴팁이 있어 틀린 값으로 읽히지는 않는다 — 그래서 blocking 은 아니다.
    - 감사가 못 잡는 이유: `strip-clipped` 는 `width() < sizeHint()` 라 줄임표로 바꾼 라벨은 구조상 통과하고, `state-kept` 는 `stripState_` 가 보이는지만 본다(test-runner 지적 · code should 1 과 같은 뿌리). 판정이 말하는 「반 토막 없음」은 맞지만 「무엇이 남았나」는 재지 않는다.
    - 고치는 법: 4단계에서 상태 글을 먼저 바닥(40)까지 줄인 뒤 잘린 선 글을 줄이거나, 잘린 선 글은 앞쪽(「잘린 선 n줄」)을 줄이고 빈 구간 꼬리는 남긴다(또는 빈 구간을 따로 라벨로). 잠금: `1280p` 에서 보이는 글에 「빈 구간」이 들어 있는지 감사 한 줄.
  - **should C2 — 눈금 숫자 간격의 마지막 값 10 m 로도 모자라면 글이 겹친다. 맞춤 상태의 보통 유적 크기에서 생긴다(AC3 「사각형끼리 겹치지 않는다」가 깨짐)**
    - 어디: `app/planview.cpp:649–652` `pickTickLabelStep` — {1, 2, 5, 10} 중 못 고르면 10.
    - 손계산: Consolas 11 px 「20 m」 ≈ 24 px → 10 m 간격이 들어가려면 10 × px/m ≥ 32 → px/m ≥ 3.2(mpp ≤ 0.31 m/px). 평면 칸이 ≈ 330 논리 px(창 1280) · ≈ 650(1920)이면, 맞춤 상태에서 모델 폭이 약 100 m(1280) · 200 m(1920)를 넘고 단면선이 20 m 보다 길 때 10 m 숫자 글이 서로 겹친다. 드론 3MX 발굴 모델은 100 m 넘는 것이 흔하다. code note 5 는 「아주 많이 축소할 때」로 적었지만, 손계산으로는 **처음 열었을 때(맞춤)** 에도 생긴다.
    - 판단: 전(1 m 마다)보다 훨씬 나아서 회귀는 아니다. 스펙도 「없으면 10」이라 규칙대로다. 감사는 16 m 합성 모델 · 가로 선만 재서 못 잡는다.
    - 고치는 법(후속 가능): 단계를 20 · 50 · 100 … 으로 늘리거나, 맞는 값이 없으면 숫자를 숨긴다(끝 하나만 남기는 것도 됨). DESIGN_SPEC §5(100행) 문구도 같이 고친다.
  - **should C3 — 단면 머리 맞추기가 「북쪽을 봄」 글을 바꾸기 전에 돈다(spec Task 3 「updateHeader() 끝」과 다름)**
    - 어디: `app/mainwindow_ui.cpp:731` `setSecTitleText`(→ `fitSectionHeader`)가 `:740` `secFacing_->setText` · `:742` `secFacing_->clear()` 보다 먼저 돈다. `updateHeader` 끝에서는 다시 맞추지 않는다.
    - 왜: 첫 단면이 생기거나(빈 글 → 「북동쪽을 봄」) 방위가 바뀌어 글 폭이 늘면, 그 `updateHeader` 에서는 옛 폭으로 단계를 고른 채로 남는다. 경계 폭에서는 머리 자식이 겹치거나 짓눌린다. 다음 `updateHeader`(결과 도착)나 크기가 바뀔 때 스스로 고쳐지는 잠깐의 일이다. 1280 은 1단계 이상이라 글이 숨어 있어 안 보인다. 감사는 바뀐 뒤의 정상 상태만 재서 못 잡는다.
    - 고치는 법: 방위 글을 `setSecTitleText` 위로 올리거나, `updateHeader` 끝에서 `fitSectionHeader()` 를 한 번 부른다(code should 2 의 「맨 끝에 한 번만」과 같은 고침).
  - **should C4 — 홈 카드 「수평 좌표계」 값이 끝 줄임표라 원점 번호가 가장 먼저 잘린다(kerf-ui-reviewer B1 을 정확성 쪽에서 뒷받침)**
    - 어디: `app/mainwindow_ui.cpp:1399` `i == 0 ? Qt::ElideMiddle : Qt::ElideRight`.
    - 왜: 「EPSG:5186」을 끝에서 줄이면 「EPSG:518…」이 된다. 5185 · 5186 · 5187 · 5188(서부 · 중부 · 동부 · 동해 원점)을 가르는 마지막 자리가 먼저 사라진다(실제 1280 캡처는 「EPS…」). 「…」 + 툴팁이 있어 틀린 값으로 읽히지는 않는다. 다만 모델별 원점 지정(backlog #8 5186 · 5187)과 바로 닿는 칸이다. 높이 칸(「KVD…」)도 같다.
    - 고치는 법: 좌표계 · 높이 값은 줄이지 않는다(B1 의 「내용 폭 칸」). 꼭 줄여야 하면 `ElideLeft` 로 번호를 남기거나 번호만 보인다.
  - note N1 — `chip-dpr-bad` 의 기준 장(`chipReference` → `chipPixmap`)과 칩 아이콘은 같은 `renderChip` 에서 나온다. 그래도 자기 비교는 아니다. 아이콘 쪽은 QIcon 이 저장한 장 중 하나를 골라(없으면 늘려서) 돌려주고, 기준은 그 배율로 바로 그린 장이다. 그래서 「그 배율의 장이 있고, 지금 리본 크기 · 종류 · ▾ 로 구웠는가」를 잰다. 못 재는 것은 셋이다: `renderChip` 이 분수 배율에서 잘못 그리는 경우(둘 다 같이 틀림), Off 자리만 비교하는 것(체크된 칩의 On · Active · Disabled 자리는 안 봄), 배율 1.25 · 1.75 만 보는 것.
  - N2 — `plan-tick-overlap` 은 가로 단면선에서는 거의 늘 참이다(d ≥ 글 폭 + 8 > W). 이 판정의 실제 힘은 px/m 와 글 폭의 단위가 어긋날 때(dpr 이 섞일 때) 잡는 것이다 — 네 배율에서 같은 px-per-m 로 확인했다. 비스듬하거나 세로인 선은 감사에 없다(위 손계산으로 갈음).
  - N3 — `kNarrowTile == 20` 은 한쪽만 잠근다. `Ribbon::chooseLook`(`app/ribbon.cpp:166–170`)은 어느 크기도 안 들어가면 마지막(20)을 돌려준다. 그래서 「1280 에 아예 안 들어감」은 이 줄이 아니라 `narrowW ≤ 1280` · env `min-width ≤ 1280` 이 잡는다(지금 둘 다 있음). `need20 = 1280` 이라 여유가 0 px 다. 리본에 칩이나 글이 1 px 만 늘어도 1280 감사가 FAIL 한다. 잡히는 것은 좋지만, 다음 작업(#11–#22)에서 미리 알고 있어야 한다.
  - N4 — `state-kept` 는 `fitInfoStrip` 에 `stripState_` 를 숨기는 길이 없어서 늘 1 이다. `toggles-reachable` 은 상자가 보이면 그 안의 단추 넷이 보이는지는 보지 않는다. 지금 코드로는 문제가 없다.
  - N5 — `vex-fit` · `plan-fits` · `section-fits` 의 −1(결과 없음)은 통과로 센다(기존 방식). 합성 모델 감사에서는 늘 결과가 있어 해가 없다.
  - N6 — 1280 실행은 probe 에서 맞춤을 명시하므로, 「열고 난 뒤 맞춤 상태가 유지되나」는 1920 이상 실행만 잰다. 둘을 합치면 빈틈은 없다.
  - N7 — `showStart` 에서 `refreshSteps()` 를 뺀 것은 맞다. `refreshSteps` 는 홈 다시 그리기만 하고, 같은 함수의 `setRibbonContext(0)` 는 `updateEnabled()`(`app/mainwindow.cpp:242`, `updateHeader` 를 부르지 않음)만 불러 홈을 두 번 그리지 않는다. 그리는 중에 홈으로 가면 `setDrawMode(false)` 를 거쳐 `updateHeader` 로 갈 수 있으나(그 콜백 길은 확인하지 못함), 고친 `clearLayout` 이 있어 두 번 그려도 해는 없다.
- 목록 항목 중 「해당 없음」: 수직 기준(높이 값 · 이름표 변경 없음) · 단면 자르기(`git diff --stat 308daac a2e2228 -- core tests tools CMakeLists.txt` 가 비어 있음) · 입출력 호환(DXF · GeoTIFF · LAS · SVG/PDF 층 · 설정 키 변경 없음. `sheetexport.cpp:951` 은 점검 상자 비우기 한 줄로 동작이 같다) · 새 허용오차 없음(`vex-fit` · `zoom-kept` 의 같음 비교는 근거가 있고, `contentFits` 1e-6 m 는 profile §7) · 합성 기준값 `polylines=1 vertices=54` 그대로(verify.md 05:36 · 07:38).
- 판단을 보류한 것:
  - C1 의 실제 화면 폭 — 맑은 고딕 글 폭은 어림이다. 실제 캡처는 최종 상태뿐이고, 미리보기 상태의 1280 실제 화면은 보지 못했다.
  - C2 의 경계(모델 폭 100 · 200 m) — Consolas 글 폭 · 평면 칸 폭을 어림한 값이다.
  - 비스듬하거나 세로인 단면선의 숫자 글 겹침 — 손계산만 했고 감사는 없다.
  - 1280L(긴 단면 이름)의 실제 화면 모양 — 감사 줄만 있다.
  - 「그릴 것」 메뉴의 QAction 체크 동기 — context7 문서가 근거이고 실행으로는 확인하지 않았다.
  - `QLayout::activate()` 바로 뒤 `minimumSize()` 의 시점과 여백 포함 여부 — Qt 내부라 확인하지 못했다. 되풀이 맞춤과 감사 결과로 갈음한다.
  - 판별력 임시 변경 실험 4건(AC4 · AC6 · AC7 · AC9) — 읽기 전용이라 다시 돌리지 않았고 build.md 기록을 믿었다.
  - `c9214e2` 문서 내용 — stat 과 더한 줄만 봤다(내 범위 밖).
- 수: 판정 pass · blocking 0 · should 4 · note 7

## security-reviewer (외부 입력 · 경로 · 파일 쓰기 · 비밀정보)
- 판정: **pass** (blocking 0 · should 0 · note 5)
- 범위: `git -C C:\dev\kerf-wt\20261010-01-narrow-1280 diff 308daac a2e2228`(지시대로, 11파일 +493/−103). 그 뒤 rework `c9214e2` 는 `docs/design/DESIGN_SPEC.md` +3줄만이고 `+` 줄 grep 이 깨끗함 — 코드는 바뀌지 않음(`git diff --stat a2e2228 c9214e2`).
- 강점:
  - 새 파일 쓰기 · 설정 쓰기 · 프로세스 실행 · 파일 삭제가 diff 에 하나도 없다(`+` 줄 grep `QSettings|QFile|QDir|save(|remove(|QDesktopServices|QProcess|system(|ShellExecute` → 경로 표시용 `QFileInfo` · `QDir::toNativeSeparators` 와 `QString::remove(tags)` 뿐).
  - ① 「목록에서 지우기(파일은 그대로)」(`app/mainwindow_ui.cpp:1484`)는 이번에 바뀌지 않았고 `QSettings "recent/files"` 만 고친다. 모델 파일에는 손대지 않는다. 경로가 보이는 범위도 그대로다. 전체 경로는 전부터 화면(`pth`)과 툴팁(`it0`, 1458)에 있었고, `elideLab` 은 글이 줄었을 때만 같은 전체 글을 툴팁으로 단다. 감사 캡처 `narrow.png`(2403) · `work.png`(2449)는 `showStart(false)`(2401) 뒤에 찍으므로 홈의 경로가 감사 폴더 · CI 산출물에 남지 않는다.
  - ② `clearLayout`(1314)의 수명 처리: 위젯은 `hide()` + `deleteLater()`(지연 삭제)로 지운다. 하위 레이아웃은 먼저 재귀로 비운 뒤 `delete it` 한다(QLayout 이 QLayoutItem 이고, `takeAt` 이 부모를 떼므로 두 번 지워지지 않음). 가장 위험한 길은 「목록에서 지우기」다. `tbl` 의 `customContextMenuRequested` 람다 → `QMenu::exec`(중첩 루프) → 동작 람다 → `rebuildStartPage` → `tbl->deleteLater()` 순서로 불린다. Qt 는 지연 삭제를 「올린 루프 단계가 끝난 뒤」에만 처리하므로 `tbl` 은 exec 와 그 슬롯이 돌아갈 때까지 살아 있다. 게다가 exec 뒤 람다는 `tbl` 을 다시 쓰지 않는다. 그래서 사용 후 해제가 없다. 람다가 잡은 위젯(`tbl` · `find` · `go` · `pic` · `all`)은 모두 보내는 쪽이거나 문맥 객체라 함께 끊긴다. `recentMenu_` 는 MainWindow 가 부모이고(234) `setMenu` 는 소유하지 않으므로 `rb` 가 지워져도 남는다. `rebuildStartPage` 는 멤버 포인터에 홈 위젯을 담지 않아 매달린 멤버도 없다. `sheetexport.cpp:951` 도 같은 비우기이고 그 레이아웃은 평평하다.
  - ③ 감사는 그리기 켜기·끄기 + `setLine`, 세로 ×10, `showStart` 두 번, 크기 바꾸기로 상태를 더 많이 바꾼다. 하지만 저장되는 곳은 모두 `QSettings` 이고 `--settings` 가 이를 INI 폴더로 돌린다(`app/main.cpp:186–188`). 감사를 부르는 곳(`factory/scripts/verify.ps1:160` · `.github/workflows/ci.yml:73` · CLAUDE.md 명령)은 모두 `--settings` 를 준다. 시험 제목 「트렌치 3호 …」 · 시험 정보 줄 글은 라벨에만 넣고 `updateHeader()` 로 되돌린다. `sections_` 이름이나 모델 메타에는 들어가지 않는다.
  - ④ `+` 줄 grep `C:[\\/]|HKEY_|NativeFormat|권을|Users[\\/]|AppData|Program Files|registry|token|password|secret|api[_-]?key|20000[0-9]|45000[0-9]|EPSG:51` → 일치 없음(exit=1). 절대 경로 · 레지스트리 · 설치 경로에 기대는 곳이 새로 생기지 않았다(R7.5).
  - ⑤ `git diff --stat 308daac a2e2228 -- .github .claude factory CMakeLists.txt app/CMakeLists.txt core tests tools packaging build-win.sh` → 비어 있음 → 스크립트 · 훅 · CI · 권한 · 의존성 **해당 없음**. 새 라이브러리 없음(Qt 헤더 `QRect` · `QVector` 만).
  - ⑥ DESIGN_SPEC `+` 줄에 실좌표 · 실제 모델 이름 · 비밀값이 없다. spec · verify.md 의 `200002 450006 200014 450006` 은 합성 모델(EPSG:5186 합성 상자 X 200000–200016 · Y 450000–450012) 좌표다. 기록 폴더를 `token|password|secret|ghp_|github_pat|Authorization|@gmail|ASEC_REAL_3MX|Production_|strata-ref` 로 grep 해도 일치가 없다. 개인 경로는 이미 허용된 `C:\Users\권을\…` 뿐이다.
  - 저장소: `git diff --diff-filter=A 308daac a2e2228` → 더한 파일 0개. `git ls-tree -r a2e2228` 에 `LongName` · `n1280` · `*.3mx` · `*.3sm` · `*.ctm` 이 없다. 긴 이름 시험 복사본(`C:\dev\tmp\n1280-long`)과 캡처(`C:\dev\tmp\n1280-shots`)는 저장소 밖이다. worktree 에 미추적 파일이 없다(`git status --porcelain --untracked-files=all` 비어 있음).
- 지적:
  - blocking: 없음
  - should: 없음
  - note 1 — 사용자 · 파일에서 온 글이 툴팁으로 새로 노출됨(`app/mainwindow_ui.cpp:144` `setElidedText`, 630 `fitSectionHeader` 5단계, 1428 썸네일 이름). 시나리오: 「목록 파일」로 가져온 JSON(`importSectionsJson`, 외부 입력)의 단면 이름에 `<img src=…>` · `<b>` 같은 서식 글이 들어 있으면 Qt 가 서식을 자동으로 알아봐 제목 · 썸네일 이름 라벨과 줄인 뒤의 툴팁에 서식 글로 그린다. 영향: 겉모양 위조나 로컬 그림 읽기 정도다. 스크립트 · 네트워크 접근은 없다. 라벨 글 자체는 전부터 그랬고 이번 diff 는 툴팁 한 곳을 더했을 뿐이다. Windows 파일 이름에는 `<>` 를 쓸 수 없어 모델 · 경로 글은 해당하지 않는다. 다음에 사용자 · 파일 유래 글을 담는 라벨에 `Qt::PlainText` 를 거는 것을 권한다(이번 범위 밖).
  - note 2 — 감사의 `QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete)`(2390)는 홈 위젯뿐 아니라 앱 전체의 지연 삭제 대기분을 한꺼번에 비운다. 감사 길에서만 돌고 지금 감사 스택에는 그 객체를 잡은 곳이 없어 문제는 없다. 다만 감사 앞 단계에 `deleteLater` 뒤에도 객체를 쓰는 코드가 생기면 감사에서만 터질 수 있다. `clearLayout` 이 이미 `hide()` 하므로 이 줄 없이도 판정(보이는 비관리 위젯 수)은 같다.
  - note 3 — `tickLabels()`(`app/planview.cpp:669`, 반복 681)는 `L/step` 번 돈다. 악성 입력으로 단면선 길이가 터무니없이 크면 멈출 수 있다. 하지만 같은 함수 안의 전부터 있던 0.5 m 눈금 반복(810)이 두 배 이상 많이 돌므로 이번 변경이 위험을 키우지 않는다(step ≥ 1).
  - note 4 — `app/main.cpp:516` 은 `--settings` 없이 `--ui-audit` 를 줘도 거부하지 않는다. 이번 감사가 상태를 더 많이 바꾸므로, 손으로 `--settings` 를 빠뜨리고 돌리면 실제 사용자 설정(레지스트리)의 `ui/geometry` · 모델 메타가 감사 상태로 덮일 수 있다. 전부터 있던 문제이고 문서 · 스크립트 · CI 는 모두 격리하므로 범위 밖으로 둔다(나중에 `--ui-audit` 이면 `--settings` 를 요구하거나 임시 폴더로 격리하는 안).
  - note 5 — 「목록에서 지우기」는 목록에서만 빼고, 그 모델의 메타(`modelKey(f)` 의 썸네일 PNG · 단면선 실좌표 · 카메라)는 사용자 설정에 남는다. 전부터 있던 동작이고 사용자 자신의 PC 설정이라 노출은 아니다. 사용자가 「지우기」를 흔적 삭제로 이해할 수 있다는 점만 적어 둔다.
- 판단을 보류한 것:
  - 앱을 실행하지 않았고 ASan · UBSan 같은 메모리 검사도 돌리지 않았다(읽기 전용 역할). 메모리 안전은 코드 읽기로만 봤다.
  - `clearLayout` 의 지연 삭제 판단(QMenu::exec 안 `deleteLater`)은 Qt 이벤트 루프 규칙(문서)에 기댄 추론이다. 실제로 계측하지는 않았다(builder 의 AC14 캡처 · `home-stale-children=0` 은 겉모양 근거일 뿐).
  - `sectionsFromJson`(core)이 단면 이름 길이 · 단면선 길이를 제한하는지는 읽지 않았다(이번 diff 밖). 그래서 note 1 · 3 의 심각도는 겉보기 판단이다.
  - CI 가 감사 폴더를 산출물로 올리는지는 다시 읽지 않았다(ci.yml 이 바뀌지 않았고, 홈은 캡처되지 않는 것으로 판단).
  - 감사 1280d(그리기 켜기·끄기 + `setLine`)가 격리된 설정의 모델 메타에 남는 단면을 만드는지는 정확성 검토 몫으로 넘긴다. `--settings` 격리 안이라 사용자에게는 영향이 없다.
- 수: 판정 pass · blocking 0 · should 0 · note 5

## kerf-ui-reviewer (UI 변경이 있을 때만)
- 판정: **fail** (blocking 1 · should 1 · note 7). AC11(1280 캡처의 루브릭이 모두 4 이상)은 홈 1280 의 「글자」 항목 하나 때문에 충족되지 않는다. 작업 · 그리는 중 1280 은 B5 의 blocking 둘과 should 셋이 모두 풀려 모든 항목이 4 이상이다.
- 본 증거: 실제 화면(150 % → 그림 픽셀 = 논리 × 1.5) `C:\dev\tmp\n1280-verify\` 의 work-1280x800 · draw-1280x800 · work-1920x1040 · draw-1920x1040 · home-1280-real · home-1280-long-real, offscreen home-tab0-1280x800 · home-tab0-1920x1040. builder 실제 화면 `C:\dev\tmp\n1280-shots\home-tab0-1280.png` · `home-tab0-1920.png` · `home-1920.png`. 비교 `C:\dev\tmp\b5-shots\work-1280.png` · `draw-1280.png` · `C:\dev\tmp\kerf-shots\home-dbg.png`. 감사 `…\logs\20261010-052745\ui-audit-1280x800-x1\ui-audit.txt` · `narrow.png` 와 `…\logs\20261010-053705\ui-audit-1280x800-x1.75\work.png`. 기준은 worktree `docs/design/DESIGN_SPEC.md` §2.1 · §3.3 · §4 · §5 · §7 · §12, spec §7 Q1–Q3, §8 AC14. offscreen 의 한글 □ · 빈 평면 칸은 지적하지 않음.

### 캡처별 루브릭(전역 8항목: 1 위계 · 2 정렬·간격 · 3 글자 · 4 색·대비 · 5 상태 · 6 접근성 · 7 체계 일치 · 8 마감)
| 캡처 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 4 미만 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| work-1280(실제) | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음 |
| draw-1280(실제) | 4 | 4 | 4 | 4 | 5 | 4 | 4 | 4 | 없음 |
| work-1920(실제, 논리 1920×940) | 4 | 4 | 5 | 4 | 4 | 4 | 5 | 4 | 없음 |
| draw-1920(실제) | 4 | 4 | 5 | 4 | 5 | 4 | 5 | 4 | 없음 |
| home-1280-real(최근 = 합성) | 4 | 4 | **3** | 4 | 4 | 4 | 4 | 4 | 3 글자(B1) |
| home-1280-long-real(긴 이름) | 4 | 4 | **3** | 4 | 4 | 4 | 4 | 4 | 3 글자(B1) |
| home-tab0-1280(offscreen 배치 + builder 실제 `n1280-shots\home-tab0-1280.png`) | 4 | 4 | **3**(실제) | 4 | 4 | 4 | 4 | 4 | 3 글자(B1) |
| home-tab0-1920(offscreen 배치 + builder 실제 `n1280-shots\home-tab0-1920.png`) | 4 | 4 | 5 | 4 | 4 | 4 | 5 | 4 | 없음 |

- 근거 요약
  - work-1280: 머리가 단계 5 다. 「A–A′ …」 · 그릴 것 · 세로 · 「1:218」 · 맞춤 · 확대 · 축소가 서로 겹치지 않는다. 정보 줄 「✓ 최종(잎) … 잘린 선 1줄 · 54점 · 빈 구간 없음」이 전부 보인다. 평면 눈금은 「2 m 4 m 6 m 8 m 10 m」 이다. 흙색 주 단추는 리본 「도면」 하나, 상태줄 문장 · X Y · 배지 둘이 다 보인다.
  - draw-1280: 그리는 중 상태가 다섯 곳 모두에 나온다. 칩, 정보 줄 「단면선을 긋는 중 — 단면은 A′를 찍은 뒤 계산됩니다」(전체), 단면 위 띠, 오른쪽 판 「새 단면 · 그리는 중」, 꺼진 내보내기 칩이다.
  - 1920 두 장: 머리 단계 0(「북쪽을 봄」 · 토글 넷 · 「세로 ×1 ▾」 · 「크게」)이다. 정보 줄은 레벨선 · 입면 뒤까지 전부 보이고, 평면 눈금은 1 m 마다다. B5 1920 과 모양이 같다.
  - 홈 1280: 카드 칸 이름 · 값이 줄어 읽히지 않는다(B1). 나머지(키 줄 여섯 + 「모든 키 F1」, 표, 작업 순서)는 정상이다.
- Kerf 스펙 항목(1–5): 쌓임 **5**(1280 리본 70 · 1920 118 → 문서 탭 → 판 넷 → 상태줄, `overflow=0`) · 리본 **5**(1280 `look=20 labels=0` = 개정 §2.1, 묶음 이름 11 이 보임, 흙색 칩 1) · 문서 탭 **5** · 안내 자리 **4**(칩과 되돌리기 칸이 겹치지 않음, 1280 문장 줄임표는 N4) · 판 **4**(판 폭 348 · 272 고정, 머리 줄이기는 §4 순서대로, ▾ 없음은 S1) · 상태줄 **5** · 아이콘 **4**(×1.75 offscreen 까지 또렷, `chip-dpr-bad=0`, 메뉴 표시 없음은 S1) · 토큰/색 **5**(새 색 없음 · 흙색 하나 · 단면선 빨강) · 한글 조판 **3**(B1).

### 지적
- **blocking B1 — 홈 카드 칸의 이름 · 값이 1280 에서 읽히지 않음(AC11 미충족 + 스펙 §7 글 위반, 이번 변경)**
  - 어디: `home-1280-real.png` 카드 아래 칸 줄, px x 775–1428 · y 405–465(논리 x 517–952 · y 270–310). 칸 이름 「마지…」(px ≈1003,417) · 「수평 …」(≈1098,417), 값 「EPS…」(≈1095,451) · 「KVD…」(≈1190,451) · 「C: › de…f-synth」. `home-1280-long-real.png`(「C: › de… › long」)와 builder 실제 `n1280-shots\home-tab0-1280.png`(모델을 연 뒤 홈)도 같다. 그런데 「단면 1개」 · 「도면 —」 칸(px x 1243–1428)은 절반 넘게 비어 있다. 1920(`n1280-shots\home-1920.png` · `home-tab0-1920.png`)은 「마지막 열림 · 수평 좌표계 EPSG:5186 · 높이 KVD1964」가 전부 읽힌다.
  - 스펙: §7 「칸 6(폴더 · 마지막 열림 · 수평 좌표계 · 높이 · 단면 n · 도면 n)」. §7 좁은 창 규칙이 줄임표를 허용한 목록은 「끌어 놓기 안내 · 카드 모델 이름 · 보조 줄 · **칸 값** · 최근 표 파일 이름 · 경로 · 표 도움말」이고 **칸 이름(캡션)은 들어 있지 않다.** 그런데 `app/mainwindow_ui.cpp:1398` 이 캡션 `ks[i]` 에도 `elideLab` 을 씌워 「마지…」 「수평 …」 이 된다. 값 줄임표는 스펙이 허용하지만, 이어서 작업 카드의 좌표계 · 높이 기준이 「EPS…」 「KVD…」 로만 보이면 마우스를 올리기 전에는 무엇인지 알 수 없다. 1280 이 지원 최소 폭이 된 지금, 사용자가 기대하는 「읽힘」에 못 미친다. 아래 표에 같은 값이 있다는 것으로는 낮추지 않는다(카드가 홈의 초점).
  - 원인(코드): `mainwindow_ui.cpp:1402` `ch->addWidget(c, i == 0 ? 2 : 1)` 때문에 칸 다섯이 내용과 상관없이 같은 폭이다. 1280 에서 카드 몸통은 ≈ 434 논리(카드 714 − 오른쪽 280)다. 그래서 폴더 ≈ 124, 나머지 칸 ≈ 62 이고, 여백 12 × 2 를 빼면 글 폭이 ≈ 38 논리 px 밖에 안 된다.
  - 감사가 못 잡은 이유: `home-clipped` 는 `width() < sizeHint().width()` 만 센다. 줄임표 + 툴팁으로 바꾼 라벨은 sizeHint 가 줄인 글 폭이라 통과한다(10조합 모두 `home-clipped=0`).
  - 고칠 방향:
    1. 칸 이름(2–6자)은 줄이지 않는다. 1398행의 `elideLab` 을 빼고 sizeHint 를 최소 폭으로 둔다.
    2. 칸 1–5(마지막 열림 · 수평 좌표계 · 높이 · 단면 · 도면)는 내용 폭으로 둔다(이름 · 값 sizeHint 중 큰 쪽 + 여백 24, stretch 0). **폴더 칸만** 늘고 줄게 한다(가운데 줄임표 — 스펙 허용). 1280 어림: 82 + 92 + 77 + 46 + 46 ≈ 343 → 폴더 ≈ 90 논리. 더 필요하면 좁을 때 칸 여백 12 → 8(폴더 ≈ 130)로 하거나, 홈 왼쪽 열 최소 폭 420(1343행)을 좁힌다. 값 줄임표는 그래도 모자랄 때(긴 좌표계 이름)만 쓴다.
    3. 잠금: 감사에 줄 하나를 더한다 — `#ccCells` 아래 라벨 중 보이는 글이 `fullText` 와 다른 수(폴더 칸 제외), 1280 합성 모델에서 기대값 0. 지금 코드로는 4(RED)가 나오는 것을 먼저 본다.
- **should S1 — 좁은 단면 머리의 메뉴 단추 둘에 ▾ 가 없음**
  - 어디: `work-1280x800.png` 단면 머리, px ≈(1167,182) 「그릴 것」(layers 아이콘)과 ≈(1216,182) 세로 과장 아이콘(논리 ≈(778,121) · (811,121)). `draw-1280x800.png` 도 같다.
  - 스펙: §4 ④ 「그릴 것 ▾」 메뉴 단추, ③ 「세로 ×1 ▾」 아이콘만. 1920 은 「세로 ×1 ▾」 글에 ▾ 가 보이는데, 1280 에서는 두 단추 모두 메뉴라는 표시가 없다. `app/theme.hpp:127` `QToolButton::menu-indicator { image: none; width: 0px; }` 가 모든 메뉴 표시를 지운다.
  - 고칠 방향: 두 단추에 ▾ 를 직접 붙인다. 리본 메뉴 칩처럼 아이콘에 ▾ 를 구워 넣는 방법(`kerf::chipPixmap(…, caret=true)` 와 같은 방식)이 있고, 단추 글을 「▾」 하나로 두고 `Qt::ToolButtonTextBesideIcon` 으로 하는 방법도 있다(약 10 논리 px 더 넓어지는데, 1280 머리 제목 옆에 ≈ 30 논리 px 여유가 있다). 세로 단추는 ③ 단계에서 `IconOnly` 대신 글 「▾」 로 한다. 툴팁 · 키는 그대로. 툴팁 · 키(I · L · V)로 닿기 때문에 6 · 7 항목은 4 로 두었다.
- **note**
  - N1: 정보 줄 ④ 단계(감사 `1280p` · `1280d` 만 해당)에서는 상태 글이 서식 없는 글이 되어 「✓」 초록이 빠진다(spec §5 위험 3). 실제 1280 은 「54점」까지 보이므로 ③ 전 단계라 초록이 남아 있다. 글 「최종」이 남아 뜻이 색에만 기대지 않으므로 둬도 된다. 원하면 줄인 글에서도 ✓ 만 색 span 으로 감싼다.
  - N2: 단면 그림 오른쪽 위 끝 이름이 1280 에서는 「A」(work · draw 1280 px ≈(1421,271))이고 1920 은 「A′」 다. B5 `b5-shots\work-1280.png` 도 같으므로 전부터 있던 것이다(`paintSectionDoc` 은 이번 diff 밖). 후속에서 확인할 것.
  - N3: 단면 제목이 「A–A′ …」(px 1040–1110)로 줄었는데 오른쪽에 ≈ 30 논리 px 여유가 보인다. `fitSectionHeader`(mainwindow_ui.cpp:645–654)는 레이아웃 최소 폭 넘침 전부를 글 폭에서 빼는데, 그 넘침에 늘림 · 간격 최소가 섞였을 수 있다. 「A–A′」 가 남아 뜻은 지켜진다.
  - N4: 1280 안내 칩 문장이 「… Shift …」 로 줄고 툴팁이 없다. 스펙 §5 순서 ③ 그대로이고 B5 N4 와 같다(guideband 는 이번 범위 밖).
  - N5: 긴 이름 카드 「Synthet···ongName」 은 가운데 줄임표(§7)라 앞뒤가 남아 구분된다. 명조 글꼴에서 「…」 가 띄운 점 셋으로 보이는 것은 글꼴 모양이다.
  - N6: AC11 후반(1920 모양 그대로)은 충족한다 — 위 1920 근거와 같다. 1280 ×1.75 offscreen `work.png` 도 머리 겹침이 없다(단계 5).
  - N7: 색 · 토큰에서 새 색은 없다. 흙색 주 단추는 화면마다 하나다(작업 = 리본 「도면」, 그리는 중 = 꺼짐, 홈 = 「이어서 열기」). AC14: `home-dbg.png` 왼쪽 위의 옛 위젯 덩어리가 `home-tab0-1280x800.png` · `home-tab0-1920x1040.png` · builder 실제 두 장에서 모두 사라졌다(`home-stale-children=0`).

### B5 지적 해결 여부(`factory/jobs/20261009-01-b5-dpi/review.md` kerf-ui-reviewer 1차 · 2차)
| B5 지적 | 결과 | 근거 |
| --- | --- | --- |
| 1차 blocking 1 / 2차 S1: 1280 단면 머리에서 토글이 짓눌리고 축척 칸이 확대 아이콘을 덮음 | 해결 | work-1280 머리 겹침 없음(위), 감사 `sec-header-overlap=0 kept=1 toggles-reachable=1`(10조합) |
| 1차 blocking 2 / 2차 S3: 리본 1280 이 20 인데 스펙은 32 | 해결(스펙을 실제로 고침 — 사용자 결정) | DESIGN_SPEC §2.1 43행 · §3.3 79행 · §12 147행이 「20」, `narrow … look=20 … actual-look=20`, `kNarrowTile` |
| 1차 should: 정보 줄 글 반 토막 | 해결 | work-1280 · draw-1280 정보 줄 전체 글(위) |
| 1차 should / 2차 S2: 평면 눈금 「1 m2 m3 m」 | 해결 | 실제 1280 「2 m 4 m 6 m 8 m 10 m」, 1920 은 1 m 마다, `plan-tick-overlap=0` |
| 1차 should: 1280 ×1.25 · ×1.75 감사 안 함 | 해결 | V2 RESULT PASS, ×1.75 `work.png` 겹침 없음 |
| 2차 S4: 홈 라벨에 줄임표 없음 · 최근 모델 있는 홈 캡처 없음 | 일부 | 줄임표 + 툴팁 · 캡처가 생겼다. 그러나 카드 칸 이름까지 줄어 읽히지 않음 → B1 |
| 1차 note / 2차 N4: 안내 칩 「Shift …」 툴팁 | 그대로(범위 밖) | N4 |
| 2차 N2: 맞춤 뒤 나침반이 걸침 | 그대로(범위 밖, spec §2) | 이번 캡처의 나침반은 장면 밖 오른쪽 아래(work-1280 · 1920) |
| 2차 N3: 빈 홈 카드 오른쪽 절반이 빔 | 해당 없음 | 최근 모델이 있는 홈은 주 단추 · 썸네일이 채움 |

### 판단 보류
- 긴 단면 이름(`1280L`)의 실제 화면 모양 — 감사 줄(`1280L:5:0`)만 있고 캡처가 없다.
- 실제 125 · 175 % 모니터에서 눈으로 확인 — 이 PC 는 150 % 하나다(offscreen ×1.25 · ×1.75 로 갈음).
- 3440 · 3840 실제 크기 — 화면에 잘려 offscreen 감사로 갈음했다.
- 「목록에서 지우기」 클릭 뒤 홈 — `home-stale-children=0` 으로 간접 확인뿐이다.
- 「그릴 것」 메뉴를 연 모양(체크 표시 · 키 글) — 캡처가 없다.
- 배율 2 · 홈 탭 offscreen 의 한글 조판 — □ 로 보여 판정할 수 없다.

## 종합(2026-10-10 08:20, 팩토리)
- ai_review: **fail → rework 2/2**(마지막). code **fail**(blocking 1) · correctness **pass**(should 4) · security **pass**(note 5) · kerf-ui **fail**(blocking 1 · should 1).
- **blocking(고칠 것):** ① [code B1] 홈 「작업 순서」 줄 오른쪽 「<모델> 기준」 글이 `elideLab`(가로 Ignored) + 앞의 `addStretch()` 때문에 폭 0 → **모든 창 폭(1920 포함)에서 사라짐**(`mainwindow_ui.cpp:1496–1497`, 증거: n1280-shots home-1920/1280 vs 옛 v5-d1-home) ② [UI B1] 1280 홈 카드 칸 **이름**까지 줄임표(「마지…」 「수평 …」), 칸 다섯이 같은 폭(`:1398` · `:1402`) — 스펙 §7 은 칸 값만 허용.
- **함께 고칠 should(같은 뿌리 · 작음):** [code S1 = correctness C1 의 원인] 감사 `clippedLabels` 가 빈 글 · 「…」만 남은 라벨을 못 잡음 → `fullText` 대비 보이는 글이 비었거나 줄임표 최소 폭보다 좁으면 잘림으로(RED 먼저) · [correctness C1] 정보 줄 4단계 줄임표가 「빈 구간 있음/없음」부터 잘라 「늘 보인다」(스펙 §4 · Q2 A) 위반 · [correctness C4] 홈 카드 좌표계 값 「EPSG:5186」 끝 줄임표가 마지막 자리부터 지움 · [code S2 = correctness C3] `fitSectionHeader` 를 `updateHeader` 끝에서 한 번만 · 입력 같으면 바로 돌아가기 · [UI S1] 좁은 머리의 메뉴 단추(「그릴 것」 · 세로 과장 아이콘)에 ▾.
- **검토 충돌 해소:** kerf-ui N6 「1920 모양 그대로 — 충족」 ↔ code B1 「1920 에서 『… 기준』 사라짐」 — 사실 문제이고 code 검토자가 같은 캡처 두 장으로 증거를 댐 → code 쪽을 따름(needs-human 아님).
- **미룸(note → 점검 문서):** correctness C2(맞춤 상태에서 모델 폭이 약 100 m(1280) · 200 m(1920) 넘으면 10 m 간격으로도 눈금 숫자 겹침 — 스펙대로 · 회귀 아님), security note 1 · 4 · 5, code note 6건, UI note 7건(그중 단면 그림 오른쪽 끝 이름이 「A′」 가 아니라 「A」 로 보임 — 전부터).
- 남은 위험: rework 뒤 다시 blocking 이 나오면 rework 횟수가 넘어 needs-human.
- ai_review: pass | fail
- 검토 충돌(검토자끼리 다른 결론): 없음 | 있음 → needs-human
- 남은 위험(고치지 않고 받아들인 것과 이유):
