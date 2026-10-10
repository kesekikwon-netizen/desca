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

### 2차(rework 2, 00c9a43)
- 판정: **pass** (blocking 0 · should 3 · note 10)
- 범위: `git -C C:\dev\kerf-wt\20261010-01-narrow-1280 diff c9214e2..00c9a43`(커밋 5개: 50abbd3 · a973c2a · 4d5f979 · 70b46b4 · 00c9a43, 5파일 +207/−54). `git rev-parse HEAD` = `00c9a43…`, `git status --porcelain` 비어 있음. 아래 줄 번호는 `00c9a43` 기준이다. 근거: 코드 읽기, build.md 「수정 회차 2」, offscreen 감사 `…-build\logs\20261010-085100\ui-audit-*\ui-audit.txt` 8개(모두 `RESULT ok`, `carets=1 refit=1/0`, `gap-kept=1`(1280p:4 · 1280d:4 포함), `home-clipped=0 cells-elided=0 at=run:0:0,1280w:0:0`), 실제 화면 캡처 `C:\dev\tmp\n1280-rework2\` 와 그 일부를 크게 잘라 본 것(scratchpad, 저장소 밖). 앱은 실행하지 않았다.

#### 1차 지적별 해결 여부
| 1차 지적 | 결과 | 근거 |
| --- | --- | --- |
| blocking 1 「<모델> 기준」이 모든 폭에서 사라짐 | **해결** | `mainwindow_ui.cpp:1579–1584` — 늘림 칸을 빼고 라벨에 stretch 1 + 오른쪽 정렬. 그래서 남는 폭이 이 라벨로 간다. 캡처 `home-1280.png` · `home-1920.png` 오른쪽 끝에 「Synthetic 기준」이 있고, `home-1280-long.png` 는 「Synthetic_Excava…nchA_LongName 기준」이다. builder 가 권장안(elideLab 빼기) 대신 대안 ②를 고른 판정에 **동의한다.** 빼면 긴 이름이 오른쪽 열(최소 280)을 밀어 창 최소 폭 1280 을 넘길 수 있다. ②는 Ignored(최소 폭 0)를 지키면서 폭 0 문제만 없앤다. 남는 위험: 나중에 Ignored 라벨 앞에 다시 늘림 칸을 두면 또 사라진다. 다만 이제 아래 S1 판정이 그것을 잡는다(RED `home-clipped=2 [run:Synthetic 기준,…]` → 0). |
| should 1 감사가 사라진 라벨을 못 셈 | **해결** | `:134–149` `gone` = fullText 가 있는데 보이는 글이 비었음 · 「…」만 남음 · 폭 < 「…」 한 자. fullText 가 빈 라벨(그리는 중 `stripCut_`, 줄이지 않은 `stripState_` 의 무효 속성)과 숨은 라벨(`isVisibleTo`)은 건너뛴다. 그래서 거짓 양성은 찾지 못했다. RED 가 1280 · 1920 둘 다에서 잡았다(build.md). 진단용 이름 목록 `[…]` 도 좋다. |
| should 2 머리 · 정보 줄을 매번 처음부터 다시 맞춤, updateHeader 한 번에 두 번 | **머리는 해결 · 정보 줄은 그대로**(아래 should B) | 머리: `:698–702` 입력 열쇠(폭 · 제목 · 방위 · 세로 글 · 상태)가 같으면 바로 돌아간다. `:800–801` `headerBatch_` + `qScopeGuard` 로 updateHeader 끝에서 한 번만 맞춘다. `:687` · `:737` 은 그 안에서 맞추지 않는다. `refit=1/0`(RED 2/2). 열린 메뉴 단추가 결과가 올 때마다 숨겨졌다 보이던 위험(1차 보류 ①)도, 열쇠가 같으면 손대지 않으므로 거의 사라졌다. 정보 줄: `fitInfoStrip`(`:742–784`)에는 열쇠가 없다. 그래서 확대 애니메이션의 프레임마다 `apply(0…k)` 를 다시 돈다(보였다 숨김 · 서식 글 ↔ 서식 없는 글 · `activate`). |
| (correctness C3) 방위 글보다 먼저 맞춤 | **해결** | `secFacing_` 글은 `updateHeader` 의 `:821` · `:823` 에서만 바뀐다(grep). 맞춤은 그 뒤 scope 끝에서 한다. `drawMode` 의 이른 `return`(`:816`) 길에서도 qScopeGuard 가 돈다. |
| note 1 「그릴 것」 accessibleName | 해결(범위 밖 한 줄 — 아래 note 2) | `:573` |
| note 2 정규식이 `&amp;` 를 풀지 않음 | 그대로(조정자가 미룸) | `:747` |
| note 3 `clearLayout` 자리 | 그대로(미룸) | — |
| note 4 감사가 세로 과장을 늘 ×1 로 되돌림 | 그대로(미룸) | — |
| note 5 눈금 숫자 간격 10 m 상한 | 그대로(미룸, = correctness C2) | — |
| note 6 이름 없는 수 | 일부 | 제목 바닥 24 → 「첫 글자 + …」 폭(`:721`, 이유 주석 있음). 정보 줄 바닥 40 · 되풀이 4 · 8 은 그대로다. |

#### 이번 차분의 새 지적
- **should A — 구운 ▾ 가 실제로는 3 px 남짓한 점으로 보인다(UI S1 고침이 눈으로는 약함)**
  - 어디: `app/icons.cpp:143–146` `iconWithCaret`.
  - 무엇 · 왜: 삼각형(16 px 에서 6 × 3.6)을 `p.setPen(QPen(halo, 1.5)); p.setBrush(ink); drawPolygon` 으로 그린다. 펜은 테두리 선의 가운데에 그려지므로 안쪽 0.75 px 가 먹색 채움을 흰 테로 덮는다. 손계산: 위 변이 0.75 내려가고, 꼭짓점이 0.75 / sin(39.8°) ≈ 1.17 올라간다. 그래서 보이는 먹색 ▾ 는 약 2.8 × 1.7 논리 px 다. 150 % 실제 캡처 `work-1280.png` 의 (1167,182) · (1216,182) 를 7 배로 키워 보니 두 단추 모두 ▾ 가 가로 3 · 세로 2 장치 px 정도의 점이다. 있기는 하지만 메뉴라는 표시로 읽히기 어렵다. 주석 「16 px → ▾ 6 × 3.6 … 리본 메뉴 칩의 ▾ 와 같은 자리 규칙」과도 다르다. 리본 칩(`:100–106`)은 펜 없이 6 × 4 를 채운다.
  - 감사가 못 잡는 이유: `carets`(`mainwindow_ui.cpp:2421–2428`)는 속성 `caretBaked` 를 믿는다. 그림은 보지 않는다(builder 도 남은 위험으로 적었다).
  - 재현: 1280 창 → 단면 머리 「그릴 것」 · 세로 과장 단추 → 오른쪽 아래 ▾ 가 거의 안 보인다.
  - 고치는 법(글로): 흰 테를 먼저 그리고(펜 1.5, 채움 없음) 그다음 펜 없이 먹색으로 채운다. 그러면 테는 바깥 0.75 만 남고 ▾ 는 6 × 3.6 그대로 보인다. 또는 테를 한 둘레 큰 삼각형으로 따로 칠한다. 눈으로 볼지 판단은 kerf-ui-reviewer 몫이다. 코드 쪽 원인만 적는다.
- **should B — (1차 should 2 남은 부분) 정보 줄 맞추기는 지금도 매번 처음 단계부터 다시 돈다**
  - 어디: `mainwindow_ui.cpp:742–784`, `:786–789`(`setStripTexts` 가 늘 `fitInfoStrip`).
  - 왜: 머리와 달리 입력 열쇠가 없다. 1280 은 3단계(`run:3`)라 프레임마다 레벨선 · 입면 뒤 라벨을 보였다 숨긴다. 4단계에서는 `setTextFormat` 을 RichText ↔ PlainText 로 오가며 글을 다시 넣는다. 무한 고리는 없고 같은 함수 안에서 끝나 깜박임도 없다고 판단한다. 비용만의 문제다. 열쇠에는 폭 · `stripStateRich_` · `stripCutFull_` · `stripCutShort_` · 레벨선 글(`lvLine_` · `lvLabel_`, 확대하면 바뀔 수 있음) · 입면 뒤 글이 들어가야 한다.
  - 고치는 법: 머리처럼 열쇠를 두고 같으면 바로 돌아간다. 이번 범위에서 고치지 않아도 된다(후속).
- **should C — 1920 이상에서도 이어서 작업 칸 줄의 모양이 바뀌었다. DESIGN_SPEC 은 그 규칙을 「좁은 창」 항목 안에 적었다**
  - 어디: `mainwindow_ui.cpp:174–215` `fitHomeCells` · `:1470` · `:1485`(옛 `ch->addWidget(c, i == 0 ? 2 : 1)`), `docs/design/DESIGN_SPEC.md` §7 110행.
  - 무엇: 전에는 칸이 폴더 2 : 나머지 1 비율이었다(옛 `n1280-shots\home-1920.png` · `kerf-shots\v5-d1-home.png`). 지금은 1920 에서도 칸 1–5 가 내용 폭이고, 폴더 칸이 남는 폭 전부(캡처에서 카드 몸통의 약 2/3)를 받는다(`n1280-rework2\home-1920.png`). spec.md 8행 「1920 이상 화면 모양은 바뀌지 않는다」 · AC11 과 글자로는 어긋난다.
  - 왜 blocking 이 아닌가: kerf-ui-reviewer B1 「고칠 방향 2」와 종합(08:20)의 `:1402` 지적이 이 배치를 요구했다. 결함이 아니라 의도한 변경이다.
  - 그래도 남는 것: DESIGN_SPEC §7 은 이 규칙을 「좁은 창(1280, …)」 항목 안에 적었다. 그래서 문서는 1280 전용으로 읽히는데 코드는 모든 폭에 건다. 종합에서 둘 중 하나를 고르면 된다. (a) 1920 변화를 받아들이고 문구를 「모든 폭」 규칙으로 옮긴다. (b) 옛 비율 폭에 칸 1–5 가 다 들어가면 옛 비율을 쓰고, 모자랄 때만 내용 폭으로 바꾼다. kerf-ui-reviewer 가 1920 캡처를 다시 볼 때 같이 보도록 여기 적어 둔다.
- note 1 — `:571–572` 첫 `setIcon(kerf::icon("layers"…))` 은 바로 다음 줄 `iconWithCaret` 이 덮어쓴다. 시작할 때 여섯 장을 헛되이 굽는 죽은 호출이다(`setIconSize` · `iconName` 속성은 그 줄에 있어야 함). 571 을 고치는 대신 줄을 더해서 생긴 것이다.
- note 2 — 범위 밖 한 줄 `secTogglesMenu_->setAccessibleName("그릴 것")`(`:573`): **받아들인다 · 되돌리지 않아도 된다.** 「범위 밖 변경 = blocking」은 요청하지 않은 기능을 막으려는 규칙이다. 이 줄은 기능이 아니다. 화면이 바뀌지 않고, 이 작업이 만든 단추에 글자 없는 아이콘 단추의 이름을 달 뿐이다. 1차 note 1 을 닫고, build.md 「범위 메모」에 기록도 남겼다.
- note 3 — 주석 자리: `stripGapTail`(`:152–156`)이 `setElidedText` 의 설명 주석(`:150–151`)과 그 함수 사이에 끼었다. 그래서 그 주석이 엉뚱한 함수 위에 있다.
- note 4 — `fitHomeCells`(`:177–183`)는 `cells->children()` 의 만든 순서로 `cs[0]` 을 폴더 칸으로 본다. 감사는 `objectName == "ccCellFirst"` 로 고른다(`:2482`). 지금은 순서가 같지만 이름으로 고르는 편이 단단하다.
- note 5 — 칸 폭 = `horizontalAdvance`(정수, 반올림) + 여백이다(`:185` · `:193`). 그런데 `elidedText` 는 실수 글 폭과 비교하고, QLabel sizeHint 는 올림 폭을 쓴다(Qt 소스 기억 — **확인하지 못함**). 글 폭에 소수가 있는 글꼴 · 배율에서는 칸 값이 정확히 맞는 경계에서 「EPSG:5186」 → 「…5186」으로 줄거나 칸 이름이 1 px 짓눌릴 수 있다. 이 PC 150 % 캡처(`home-1280.png`)에서는 모두 온전하다. 칸 폭에 +1 px, 또는 `QFontMetricsF` 올림이 싼 보험이다.
- note 6 — `fitHomeCells` 의 `!full` 길에서 좌표계 · 높이 값을 줄여도 모자라면 `width[0] = max(0, …)`(`:213`)가 폴더 칸을 0 px 로 만들고, 칸들이 카드 밖으로 넘친다. 손어림으로 1280 에서는 닿지 않는다. 실제 화면은 여백 12 로 들어가고, offscreen 은 여백 4 · 폴더 57 이다. 「2026-10-09」 같은 긴 날짜는 좌표계 값을 「…5186」으로 줄여 흡수한다. 그러니 극단적인 경우(긴 도면 글 등)에만 생긴다. 생겨도 `gone` 판정이 폴더 값을 잡는다.
- note 7 — `refit` 판정이 `refitA <= 1`(`:2549`)이다. 그래서 끝에서 한 번 맞추기를 누가 지워 0 이 되어도 통과한다(C3 회귀를 못 잡음). 제목이 실제로 바뀌므로 정상값은 늘 1 이다. `== 1` 이 더 정확하다. 시험 제목 「단면 감사」는 updateHeader 가 덮어쓰므로 모양일 뿐이다. 재는 것은 「진짜 제목으로 돌아올 때 1번 · 같은 입력이면 0번」이고, 판별력은 RED 2/2 → 1/0 으로 증명됐다.
- note 8 — 열쇠(`:699`)에는 글꼴 · 배율 변화가 없다. `applyHighContrast` 가 열쇠를 지우고(`:1680`), 화면을 옮기는 일은 보통 Resize 를 함께 부른다(폭이 열쇠에 있음). 그러니 지금은 빈틈이 없다고 판단한다. 다만 「Resize 없이 글 폭만 바뀌는 경우」가 없다는 것은 **확인하지 못함**.
- note 9 — 앱 필터(`:444`)는 `headerBatch_` 와 상관없이 Resize 때 맞춘다. updateHeader 도중에 머리가 동기 Resize 를 받으면, 옛 방위 글로 한 번 더 맞추고 끝에서 다시 맞춘다(열쇠가 다름). 결과는 맞고 고리도 없다.
- note 10 — 흰 테(`theme::Card`)는 머리 바탕(#FFFFFF)과 같다. 하지만 마우스를 올리면 바탕이 `QToolButton:hover` #F5F4ED(`theme.hpp:123`)라 ▾ 둘레에 옅은 흰 테가 보인다. 아주 작다. should A 를 고칠 때 같이 볼 일이다.
- 확인한 것(문제없음):
  - 저장소 규칙: `Q_OBJECT` 없음. 형 변환은 `static_cast`(`isWidgetType` · `inherits` 뒤). 색은 `theme::Hand` · `theme::Card` 토큰만 쓴다. `<QScopeGuard>` 는 Qt 5.12+ 이고, `const auto … = qScopeGuard(…)` 는 C++17 복사 생략으로 맞다. core · tests · CMake · CI 는 그대로다(build.md 의 `git diff --stat` 이 비어 있음).
  - 수명: `iconWithCaret` 는 만들 때 두 번만 불리고 멤버(`vexIcon_` · `vexCaretIcon_`)에 담긴다. 그래서 프레임마다 굽지 않는다. 배율 여섯 장은 B5 와 같다. scope guard 람다가 잡은 `this` 는 MainWindow 가 늘 더 오래 산다. `fitHomeCells` 는 잡는 것이 없는 자유 함수다. `setFixedWidth` 는 부모(`ccCells`, 가로 Ignored · 세로 Fixed)의 크기를 바꾸지 않으므로 Resize 고리가 없다. 필터가 layout 의 doResize 보다 먼저 받는 순서도 머리와 같다.
  - 재귀 갱신: `headerBatch_` 는 증감이 짝을 이룬다(++ 는 `!secTitle_` 이른 return 뒤). 안에서 updateHeader 가 다시 불려도 가장 바깥에서만 맞춘다.
  - correctness C1: 꼬리를 줄이는 것은 마지막 수단(`:778`)일 때뿐이고, 그때는 `gap-kept`(`:2434`)가 잡는다. 3) 단계에서 계산 상태가 「단…」까지 줄 수 있다. 하지만 손어림(정보 줄 ≈ 328, 상태 바닥 40 + 꼬리 ≈ 85 + 여백)으로는 1280 이상에서 그 단계에 닿지 않는다.
  - correctness C4: 좌표계 · 높이 값만 `ElideLeft` + `elideCode`(`:1481–1482`)다. 줄이는 것은 맨 마지막 단계에서만이다. 1280 실제 화면에서는 줄지 않는다(크게 잘라 본 캡처에서 「EPSG:5186」 · 「KVD1964」가 온전함).
- 판단을 보류한 것: ▾ 를 받아들일지(should A)와 1920 칸 배치(should C)는 화면 판단이라 kerf-ui-reviewer 와 종합에 맡긴다. note 5 · 8 은 Qt 내부 동작이라 실행 · 소스 없이 확인하지 못했다. 125 · 175 % 실제 모니터는 보지 못했다.
- 수: 판정 **pass** · blocking 0 · should 3 · note 10

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

### 2차(rework 2, 00c9a43)
- 판정: **pass** (blocking 0 · should 2 · note 6)
- 범위: 이번에 고친 차분 `git -C C:\dev\kerf-wt\20261010-01-narrow-1280 diff c9214e2..00c9a43`(5파일: `app/icons.{hpp,cpp}` · `app/mainwindow.hpp` · `app/mainwindow_ui.cpp` · `docs/design/DESIGN_SPEC.md`)을 전부 읽었고, 전체 범위 `308daac..00c9a43` 가운데 1차 뒤 바뀐 곳만 다시 봤다. HEAD = `00c9a43844c4…`이고 worktree 는 깨끗하다(`git status --porcelain` 비어 있음). 근거: 코드 읽기, 손계산, verify.md 09:00 · 09:03 블록(HEAD 같음), offscreen 감사 `…-build\logs\20261010-085100\ui-audit-*\ui-audit.txt` 8개와 `…\20261010-090044\` 의 ×1.25 · ×1.75, build.md 「수정 회차 2」의 RED/GREEN 줄, 실제 화면 캡처 `C:\dev\tmp\n1280-rework2\home-1280.png` · `work-1280.png`. 아래 줄 번호는 `00c9a43` 기준이다. 앱은 돌리지 않았다.
- **1차 지적별 해결 여부**
  | 1차 지적 | 결과 | 근거 |
  | --- | --- | --- |
  | C1 정보 줄 ④단계가 「빈 구간 있음/없음」부터 자름 | **해결** | `fitInfoStrip` ④(`mainwindow_ui.cpp:758–780`)는 짧은 글을 첫 「 · 」에서 앞부분(「잘린 선 n줄」)과 빈 구간 꼬리(`stripGapTail`, :153)로 나눈다. 줄이는 순서는 ① 앞부분(없앨 수 있음) → ② 상태 글 바닥 40 px → ③ 상태 글 「첫 글자 + …」 → ④ 꼬리(마지막 수단, :778)다. DESIGN_SPEC §4(rework 2 줄)와 Q2 A 「빈 구간 정보는 늘 보임」에 맞다. 감사 10개 모두 `1280p:4:0` 이다. 즉 가장 긴 글(「…빈 구간 12곳 10.25 m」)로 ④ 길을 실제로 탔고 `gap-kept=1` 이다. RED(`50abbd3`)에서는 `gap-kept=0 FAIL` 이었다. 손계산(실제 화면 1280, 정보 줄 ≈ 328): 미리보기 상태(≈ 230) + 「잘린 선 1줄 · 빈 구간 없음」(≈ 140)이 넘치는 만큼(≈ 74)은 먼저 앞부분 ≈ 60 + 「 · 」로 흡수된다. 나머지 ≈ 14 는 상태 글에서 줄인다 → 「단면선 정확 · 배경 미리보기 → 최종 계…」 + 「빈 구간 없음」. 숫자는 그대로다. 줄 수 · 점 수는 툴팁과 오른쪽 판 「잘린 선 1줄 · 54점」에 남는다 |
  | C2 눈금 숫자 10 m 로도 겹침 | 미룸(조정자 결정, 다시 다루지 않음) | 종합 08:20 「미룸」 |
  | C3 머리 맞춤이 방위 글을 바꾸기 전에 돎 | **해결** | `updateHeader` 가 `++headerBatch_` + `qScopeGuard`(:798–801)로 모든 return 길의 끝에서 한 번 맞춘다. `updateVexUi`(:687) · `setSecTitleText`(:740)는 묶음 안에서 맞추지 않는다. 그래서 방위 글(`secFacing_->setText`, :821)을 바꾼 뒤에 맞춤이 돈다. `fitSectionHeader` 의 입력 열쇠(폭 · 제목 · 방위 글 · 세로 글 · 세로 상태, :698–700)도 확인했다. 머리 폭을 바꾸는 다른 입력은 없다. 축척 칸은 `setFixedWidth(84)`(:925)이고, `active` QSS 는 아래 테두리만 바꾸며(theme.hpp:155), 시트 교체 때는 `applyHighContrast` 가 열쇠를 지운다. 감사 `refit=1/0`(10개 모두)이고 RED 는 `refit=2/2` 였다 |
  | C4 「EPSG:5186」 끝 줄임표가 원점 번호부터 지움 | **해결** | 좌표계 · 높이 값(i==2 · 3)은 `ElideLeft` + `elideCode`(:1481–1482)다. `fitHomeCells`(:174)가 칸 1–5 에 내용 폭을 주고 폴더 칸만 남는 폭을 받는다. 칸 여백은 12 → 8 → 4 로 줄이고, 그래도 모자라면 좌표계 · 높이 값을 「… + 끝 4자」까지 줄인다. 저장 값은 `EPSG:%1`(:1339, horizontalEpsg 가 있을 때)이라 끝 4자가 원점 번호다 → 「…5186」. 감사 `cells-elided=0`(10개)이다. 실제 화면 `home-1280.png` 에 「EPSG:5186」 · 「KVD1964」가 다 보이고, 폴더만 「C: › …synth」로 줄었다 |
- **값 · 단위**
  - 좌표 · 높이 값은 그대로다. 이번 차분은 라벨 글 · 폭 · 아이콘만 바꾼다. `meta srs/height`(:1339–1343), `gapTxt` · `profileGaps`(:842–853), 세로 과장 값은 손대지 않았다(`vexFull_` 는 글을 복사해 둘 뿐). `git diff --stat 308daac..00c9a43 -- core tests tools CMakeLists.txt app/CMakeLists.txt .github` 는 비어 있다. 이번 차분에는 `planview.cpp` · `sectionview.cpp` 도 없다. 합성 기준값은 09:00 · 09:03 모두 `polylines=1 vertices=54` 다.
  - 단위가 섞이지 않는다. `fitInfoStrip` · `fitHomeCells` · `clippedLabels` 는 모두 `fontMetrics().horizontalAdvance` · 위젯 폭 · 레이아웃 최소 폭을 쓴다(논리 px). `iconWithCaret`(`icons.cpp:135`)는 `renderGlyph` → `blank()`(:36–41)가 `setDevicePixelRatio(d)` 한 장에 그린다. 그래서 QPainter 좌표 `px·0.375` 등은 논리 px 이고, 여섯 배율 모두 ▾ 가 같은 자리에 온다. 감사 줄도 네 배율(×1 · ×1.25 · ×1.75 · ×2)에서 같은 값이다.
- **새 감사 판정이 그 성질을 재는가**
  - `home-clipped`(새 「사라짐」 조건, :134): 전체 글이 있는데 보이는 글이 비었거나 「…」만 남았거나 폭이 「…」 한 자보다 좁으면 센다. RED 에서 `run:Synthetic 기준` 이 잡혔다(code B1 재현). 판별력이 있다.
  - `cells-elided`: 폴더 칸을 뺀 칸 값 중 보이는 글 ≠ `fullText` 인 수를 센다. 칸 이름은 이제 `fullText` 가 없어 여기서는 안 세지만, 짓눌리면 `home-clipped` 의 「폭 < sizeHint」가 잡는다. 둘을 합치면 빈틈이 없다. RED 는 10 · 5 였다.
  - `gap-kept`: ④ 길에서 꼬리 전체 글이 들어 있는지를 문자열로 본다. 꼬리를 줄이면(「…10.2…」) 0 이 된다. 판별력이 있다. 빈틈은 should R2-2.
  - `refit`: 「같은 입력으로 또 부르면 0」(code S2)은 잰다. 「방위 글 뒤에 맞춤」(C3 순서)은 재지 않는다. 열쇠 비교만 있고 묶음이 없는 코드도 1/0 이 나오기 때문이다. 빈틈은 should R2-1.
  - `carets`: 속성(`caretBaked`)을 믿는다(note R2-1).
- 지적:
  - **should R2-1 — `refit` 이 0 을 통과로 받는다(`refitA <= 1`, `mainwindow_ui.cpp:2549`). 그래서 「updateHeader 끝 맞춤이 빠짐」을 못 잡고, 그때 머리에 이전 단면 제목이 남는다**
    - 감사는 묶음 밖에서 제목을 「단면 감사」로 맞춘 뒤 `updateHeader()` 로 진짜 제목에 되돌린다(:2451–2453). 제목이 바뀌었으니 맞춤은 정확히 1번이어야 한다(0 이 맞는 경우가 없다).
    - 앞으로 누가 끝 맞춤(:801 scope guard)을 지우고 `if (!headerBatch_)` 막이만 남기면 refitA = 0 이 되어 통과한다. 그러면 `secTitleFull_` 만 바뀌고 `secTitle_` 글은 이전 글로 남는다. 예: 1280L 의 「트렌치 3호 북벽…」 줄임 글이 다른 단면 머리에 남아 다음 크기 바뀜까지 이어진다. `sec-header-overlap` 은 제목 글을 보지 않아 이것을 못 잡는다. 보고서에 들어가는 값은 아니지만 「그럴듯하게 틀린 화면 글」이다.
    - 고치는 법: `refitA == 1` 로 조인다. 아울러 `updateHeader()` 뒤 「보이는 제목 == `secTitleFull_` 또는 그 줄임(`fullText` == `secTitleFull_`)」 한 줄을 둔다. 방위 순서까지 잠그려면 「마지막 맞춤 열쇠 == 지금 입력으로 만든 열쇠」를 확인한다. 지금 코드는 맞으므로(1/0) 후속 작업으로 해도 된다.
  - **should R2-2 — `gap-kept` 는 `stripCut_` 가 보일 때만 잰다(:2434). `stripCut_` 를 통째로 숨기면 `gap-kept` · `state-kept` · `strip-clipped` 가 모두 통과한다**
    - `clippedLabels` 는 숨은 라벨을 건너뛰고, `state-kept` 는 `stripState_` 만 본다. 지금 `fitInfoStrip` 에는 `stripCut_` 를 숨기는 길이 없어 해는 없다. 다만 Q2 B 같은 「통째로 숨김」 회귀가 들어오면 감사는 초록인데 빈 구간 정보가 사라진다.
    - 고치는 법: `stripCutShort_` 에 「빈 구간」이 있는데 `stripCut_` 가 안 보이면 `gapKept = false`. 후속으로 해도 된다.
  - note R2-1 — `carets` 는 `caretBaked` 속성과 「아이콘이 비지 않음」만 본다. `iconWithCaret` 는 SVG 를 못 찾으면 ▾ 없는 아이콘을 돌려주는데, 부르는 쪽은 속성을 그대로 켠다. 지금은 `lucide/layers.svg` · `kerf/vex.svg` 가 `icons.cmake` 에 있어 해가 없다(`--icon-sheet missing=0`). 실제 근거는 캡처다(builder도 적었다). ▾ 크기의 판단은 UI 검토 몫이다.
  - note R2-2 — `cells-elided=0` 이라는 기대 때문에, 칸 값을 「…5186」으로 줄이는 마지막 길(:198–209)은 감사에서 한 번도 돌지 않는다(그 길에 들어가면 FAIL). 손으로 확인했다. `minW = max(칸 이름, 「…」+끝 4자) + 2·여백` 이므로 ElideLeft 결과는 적어도 「…5186」이다. 폴더 칸이 0 이 되면 「사라짐」 조건이 잡는다.
  - note R2-3 — ④에서 「잘린 선 n줄」은 상태 글을 줄이기 전에 통째로 빠질 수 있다(위 손계산). DESIGN_SPEC §4 rework 2 문구(「잘린 선 n줄」 앞부분 → 계산 상태)와 같은 순서이고, 값은 툴팁 · 오른쪽 판에 있다. 앞부분을 「잘린 선 1…」처럼 숫자 중간에서 자를 수 있지만 「…」 + 툴팁이 있어 틀린 값으로 읽히지 않는다.
  - note R2-4 — 칸 1–5 는 이제 1920 에서도 내용 폭이다(전에는 stretch 2:1:1:1:1:1). 값 정확성과는 상관없다. 「1920 모양 그대로」에 드는지는 kerf-ui-reviewer 의 판단이다.
  - note R2-5 — `work-1280.png` 단면 그림 오른쪽 위 끝 이름이 여전히 「A」다(1920 은 「A′」). 이번 차분 밖(`paintSectionDoc` 안 바뀜)이고, 1차 UI N2 · 종합 「미룸」과 같다. 점검 문서에 남아 있는지만 확인할 일이다.
  - note R2-6 — 범위 밖 한 줄: `secTogglesMenu_->setAccessibleName`(:573)은 동작 · 값에 영향이 없다.
- 판단을 보류한 것:
  - 실제 화면 1280 의 **미리보기 상태** 정보 줄 모양은 보지 못했다. 캡처는 최종 상태뿐이라 ③ 단계이고, ④ 길은 offscreen(□ 글꼴) 감사로만 확인했다. 위 손계산은 맑은 고딕 글 폭을 어림한 것이다.
  - `qScopeGuard` 를 `const auto` 로 받는 것은 C++17 복사 생략(copy elision)과 Qt 문서(builder 인용)에 기댄 판단이다. Qt 소스는 확인하지 못했다.
  - ▾ 의 픽셀 모양 — 150 % 캡처에서 6 × 3.6 논리 px 라 눈으로 가려내기 어렵다. UI 검토 몫이다.
- 수: 판정 pass · blocking 0 · should 2 · note 6

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

### 2차(rework 2, 00c9a43)
- 판정: **pass** (blocking 0 · should 0 · note 3)
- 범위: `git -C C:\dev\kerf-wt\20261010-01-narrow-1280 diff c9214e2..00c9a43`(커밋 5: `50abbd3` · `a973c2a` · `4d5f979` · `70b46b4` · `00c9a43`, 5파일 +207/−54 — `app/icons.cpp` · `app/icons.hpp` · `app/mainwindow.hpp` · `app/mainwindow_ui.cpp` · `docs/design/DESIGN_SPEC.md`). 줄 번호는 HEAD `00c9a43` 기준이다. build.md 「수정 회차 2」와 기록 폴더도 읽었다. 앱은 실행하지 않았다.
- 확인한 것(강점):
  - 파일 쓰기 · 설정 · 프로세스: 이번 차분의 `+` 줄에서 `QSettings|QFile|QDir|.save(|remove(|QProcess|system(|ShellExecute|QDesktopServices|grab(` 를 찾았는데 일치가 없다(grep exit=1). 감사 출력 폴더에 새로 쓰는 파일은 없다. `narrow.png`(:2526)는 여전히 `measureHome("1280w")`(:2525) 다음에 찍는다. `measureHome` 은 `showStart(false)` 로 끝나므로 이 캡처에는 작업 화면만 담기고 홈(경로 · 모델 이름)은 들어가지 않는다. 실행 크기에서 새로 하는 홈 측정(:2492)도 그 사이에 캡처하지 않는다. `showStart`(:1635)는 홈을 다시 그리고 탭 · 상태 글만 바꾼다. 파일을 쓰지 않는다.
  - 멈춤 · 메모리(위협 ①):
    - `fitHomeCells`(:174)는 `ccCells` 의 Resize 때 불린다(:451). Resize 갈래는 `o->isWidgetType()` 일 때만 들어가므로(:438) 형 변환이 안전하다. 칸을 고르는 곳도 `isWidgetType` · 항목 2개 · `inherits("QLabel")` 을 확인한 뒤 형을 바꾼다. `ccCells` 는 가로 `Ignored`(:1470)다. 그래서 자식 칸의 고정 폭이 자기 크기로 되먹임되는 고리가 생기지 않는다. 폴더 칸 폭은 `max(0, avail − used)` 라 합이 avail 로 수렴한다.
    - 반복 상한: 여백 후보 3 × 칸 6, `fitInfoStrip` 8회, `fitSectionHeader` 5 + 4회, `iconWithCaret` 배율 6장.
    - 좌표계 값(`meta "srs"`)이 아주 긴 글(예: WKT)이어도 괜찮다. 앞쪽 줄임표가 「…」 + 끝 4자에서 멈추고, 그 밖에는 `horizontalAdvance` 한 번씩만 잰다.
    - 빈 글에 쓰는 `left(1)` · `right(4)` · `stripGapTail` 의 `mid` · `left` 는 범위를 넘지 않는다(앞부분 길이 = 「 · 」의 자리 ≥ 0). 대리 쌍 첫 글자가 반으로 갈리는 경우는 겉모양 문제일 뿐이다.
    - `iconWithCaret`(`icons.cpp:135`)는 이름이 없으면 `icon()` 으로 돌아간다. 크기는 코드에 박힌 16 이다. 외부 입력은 들어오지 않는다.
  - `updateHeader` 의 `headerBatch_` + `qScopeGuard`(:800–801)는 이른 반환 `if (!secTitle_) return;` 뒤에서 센다. 다시 들어와도 수가 쌓였다 풀리고, 맨 바깥에서 한 번만 맞춘다. `QScopeGuard` 는 Qt Core(5.12+) 헤더라 새 의존성이 아니다. 두 빌드 분기 모두 Qt Core 를 링크한다.
  - 감사 `setSecTitleText("단면 감사")`(:2452)는 바로 다음 `updateHeader()` 가 `sectionName()` 제목(:812)으로 되돌린다. `sections_` · 모델 메타 · 설정에는 들어가지 않는다.
  - 저장소 노출:
    - `git diff --diff-filter=A 308daac 00c9a43` → 더한 파일 0개. worktree 에 미추적 파일이 없다(`status --porcelain --untracked-files=all` 비어 있음).
    - 이번 차분 `+` 줄에서 `C:[\\/]|HKEY_|NativeFormat|권을|Users[\\/]|AppData|Program Files|token|password|secret|api[_-]?key|20000[0-9]|45000[0-9]` 를 찾았는데 일치가 없다.
    - DESIGN_SPEC 의 「…5186」은 공개된 EPSG 번호다. 실좌표가 아니다.
    - 기록 폴더에서 `token|password|secret|ghp_|github_pat|Authorization|@gmail|ASEC_REAL_3MX|Production_|strata-ref|HKEY_|AppData` 를 찾았다. 일치는 verify.md:25 하나인데, grep 패턴을 설명한 글이다.
    - build.md 의 캡처(`C:\dev\tmp\n1280-rework2\`)와 RED 로그(`C:\dev\tmp\n1280\`)는 저장소 밖에 있다. 「Synthetic_Excavation_Area3_TrenchA_LongName」은 합성 모델의 긴 이름 시험 복사본이다.
  - 스크립트 · 훅 · CI · 권한 · 의존성: **해당 없음**. 전체 범위 `git diff --stat 308daac 00c9a43 -- .github .claude factory CMakeLists.txt app/CMakeLists.txt core tests tools packaging build-win.sh` 가 비어 있다. 새 라이브러리도 없다.
  - 외부 입력 파서(JSON · OpenCTM · WKT · SQLite · CSV · 명령줄 인자) · 경로 처리: **해당 없음**(바뀌지 않음). 파일 쓰기(원본 3MX/3SM · 내보내기 덮어쓰기 · 삭제): **해당 없음**.
  - 1차 보류 「CI 가 감사 폴더를 산출물로 올리는가」를 이번에 확인했다. 올리지 않는다. 산출물은 `dist/Kerf-ci` 뿐이고(`ci.yml:83–88`), 감사 폴더는 러너 `$env:TEMP` 에 있다. 다만 `ci.yml:77` 이 stdout 의 `ui-audit ` 줄을 공개 Actions 로그에 그대로 찍는다(`main.cpp:519` 가 감사 줄마다 `log`). 아래 note 1 과 이어진다.
- 지적:
  - blocking: 없음
  - should: 없음
  - note 1 — **감사 진단 줄에 홈 라벨 글이 새로 남는다.**
    - 어디: `app/mainwindow_ui.cpp:2488` · `:2562–2564`. `clippedLabels(…, &nm)` 와 칸 줄 검사가 잘린 라벨의 전체 글 앞 16자(최대 6개)를 `homeNames` 에 담는다. 이것이 `ui-audit home-clipped=… [names]` 줄로 `ui-audit.txt` 와 stdout 에 나간다.
    - 담기는 글: 모델 기본 이름(「<모델> 기준」), 칸 값, 단면 이름(보조 줄), 그리고 폴더 · 경로 라벨이 잘린 경우 경로 조각(예: `C: › Users › …`).
    - 언제: `home-clipped>0` 이거나 칸이 줄었을 때(= FAIL 실행)만 나온다.
    - 지금 영향: 자동으로 부르는 곳(verify.ps1 · ci.yml)은 모두 합성 모델과 러너 TEMP 경로를 쓴다. 공개 로그에 나올 수 있는 글은 「Synthetic」과 러너 경로뿐이다. verify.md 에는 마지막 `RESULT` 줄만 옮긴다(verify.ps1:162).
    - 시나리오: 누군가 실제 모델로 `--ui-audit` 를 돌린다 → FAIL 줄을 `factory/jobs` 기록이나 이슈에 붙인다 → 실제 모델 이름 · 폴더 조각 · 단면 이름 16자가 공개 저장소에 올라간다.
    - 권장(후속): 진단 이름은 칸 이름 · 라벨 objectName 처럼 고정 글로 바꾼다. 또는 감사 기록을 옮길 때 이 줄을 확인한다는 규칙을 둔다.
  - note 2 — **같은 줄의 `.arg()` 사슬에 외부 유래 글이 끼어 있다**(로그 무결성).
    - 어디: `:2562–2564` `QStringLiteral("…%1%2 cells-elided=%3 … at=%6%7").arg(homeClipped).arg(<homeNames>).arg(homeCells)…`.
    - 왜: `QString::arg(QString)` 는 결과 글 전체에서 가장 작은 번호의 자리 표시를 바꾼다. 그래서 끼워 넣은 이름에 `%1`–`%9` 가 있으면 그 뒤 값들이 엉뚱한 자리로 밀린다. Windows 파일 이름에는 `%` 를 쓸 수 있고, JSON 단면 이름도 마찬가지다. 결과는 줄이 뒤섞이는 것과 Qt 의 「Argument missing」 경고다.
    - 판정 영향은 없다:
      - 판정은 정수와 불리언 `homeOk` 로 하고, 종료 코드 9 도 `ok` 에서 나온다.
      - CI 는 맨 앞이 `^ui-audit RESULT` 인 마지막 줄을 `'ui-audit RESULT ok'` 와 정확히 비교한다. verify.ps1 은 마지막 `RESULT` 줄을 쓴다. 진짜 RESULT 줄은 맨 끝에 있다.
      - 이름은 16자로 잘린다. 그래서 줄바꿈을 넣어 「ui-audit RESULT ok」(18자)를 꾸며 넣을 수 없다.
    - 고치는 법(글로): 이름 목록은 여러 인자 `.arg(a, b, …)`(한 번에 바꿈)로 넣는다. 또는 `+` 로 이어 붙인다.
  - note 3 — `fitSectionHeader` 의 입력 열쇠 `secHeaderKey_`(:698–701)는 `"%1|%2|%3|%4|%5"` 형식이다. 둘째 칸은 `secTitleFull_` 인데, JSON 으로 가져온 단면 이름일 수 있다. 이름에 `|` 가 들어 있으면 서로 다른 입력이 같은 열쇠가 될 수 있다. 그 결과는 다시 맞추기를 한 번 건너뛰는 것뿐이다(다음에 입력이 바뀌면 고쳐짐). 겉모양 문제이고 보안 문제는 아니다. 고칠지는 code · correctness 쪽에서 판단하면 된다. multi-arg `.arg` 라 `%` 가 들어 있어도 다시 바뀌지는 않는다.
- 1차 note 의 지금 상태:
  - note 1(라벨 · 툴팁 서식 글): 그대로다. 사용자 글이 새로 보이는 곳은 생기지 않았다. 정보 줄 툴팁 `stripCutFull_`(:771)은 앱이 만든 글이다. 제목 툴팁 경로는 바뀌지 않았다. `homeNames` 는 위젯이 아니라 로그로만 간다. `fitHomeCells` 가 서식 글을 평문 폭으로 재서 칸 폭이 어긋나는 일은 겉모양 문제일 뿐이다.
  - note 2(`sendPostedEvents(nullptr, DeferredDelete)`): 이제 `measureHome(…, stale=true)` 안에서만 돈다. 내용은 같다.
  - note 3(눈금 반복): 바뀌지 않았다.
  - note 4(`--settings` 없는 `--ui-audit`): 감사가 바꾸는 상태가 조금 늘었다(`showStart(true)` 두 번 더, 제목을 바꿨다가 되돌림). 그래도 모두 되돌리거나 QSettings 에만 남고, 부르는 곳은 모두 설정을 격리한다. 위험이 더 커지지 않았다.
  - note 5(「목록에서 지우기」 뒤 메타 잔류): 바뀌지 않았다.
- 판단을 보류한 것:
  - 앱 실행 · ASan/UBSan 은 하지 않았다(읽기 전용 역할).
  - `fitHomeCells` 의 Resize 수렴은 코드 읽기와 builder 의 감사 · 캡처 결과(`home-clipped=0 cells-elided=0 min-width=1280`)에 기댄 판단이다. 계측하지는 않았다.
  - 감사 `log()` 가 stdout 말고 다른 곳(파일)에도 쓰는지는 `main.cpp:519` 의 호출만 보고, 함수 몸통은 읽지 않았다.
- 수: 판정 pass · blocking 0 · should 0 · note 3

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

### 2차(rework 2, 00c9a43)
- 판정: **pass** (blocking 0 · should 2 · note 6). AC11(1280 의 작업 · 그리는 중 · 홈(합성 · 긴 이름) 캡처의 루브릭 모두 4 이상)을 충족한다. 1차 blocking B1 은 풀렸다. 1차 should S1 은 일부만 풀렸다(▾ 가 너무 작음 → S1′).
- 본 증거: 실제 화면(150 %, 그림 픽셀 = 논리 × 1.5) `C:\dev\tmp\n1280-rework2\` 의 여덟 장(work-1280 · draw-1280 · home-1280 · home-1920 · home-1280-long · home-1920-long · home-tab0-1280 · home-tab0-1920). 1차 캡처 `C:\dev\tmp\n1280-shots\`(home-1920 · work-1280)와 옛 `C:\dev\tmp\kerf-shots\v5-d1-home.png` 를 나란히 봤다. offscreen `…-build\logs\20261010-085100\ui-audit-1280x800-x1\`(ui-audit.txt · narrow.png), `ui-audit-1280x800-x2\work.png`, `ui-audit-1920x1040-x1\`(work.png · ui-audit.txt)도 봤다. 코드는 `app/icons.cpp:135–151`(`iconWithCaret`)을 읽었다. 화판 `docs/design/boards/kerf-design-v5.html:179`(`.ccard .cells`)와 worktree DESIGN_SPEC §4(86–87행) · §7(110행), spec.md 8행 · AC11 이 기준이다. offscreen 의 한글 □ · 빈 평면 칸은 지적하지 않았다.
- 한계: Read 도구에는 확대 기능이 없다. 그래서 ▾ 는 원본 크기로만 봤다. 판정 근거는 세 가지다: ① 같은 캡처 안 리본 ▾ 와 비교 ② offscreen ×2 그림 ③ 코드의 치수.

#### 캡처별 루브릭(전역 8항목: 1 위계 · 2 정렬·간격 · 3 글자 · 4 색·대비 · 5 상태 · 6 접근성 · 7 체계 일치 · 8 마감)
| 캡처 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 4 미만 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| work-1280 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음(▾ 는 S1′) |
| draw-1280 | 4 | 4 | 4 | 4 | 5 | 4 | 4 | 4 | 없음 |
| home-1280(합성) | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음(1차 3 → 4) |
| home-1280-long | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음(1차 3 → 4) |
| home-tab0-1280(모델 연 뒤) | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음(1차 3 → 4) |
| home-1920 | 4 | 4 | 5 | 4 | 4 | 4 | 4 | 4 | 없음(칸 배분은 S2) |
| home-1920-long | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음 |
| home-tab0-1920 | 4 | 4 | 5 | 4 | 4 | 4 | 4 | 4 | 없음 |

- 근거 요약
  - home-1280 · long · tab0-1280: 카드 칸 줄(그림 px x 775–1428 · y 405–465)에서 칸 이름 「폴더 · 마지막 열림 · 수평 좌표계 · 높이 · 단면 · 도면」과 값 「1분 전 · EPSG:5186 · KVD1964 · 1개 · —」가 모두 다 보인다. 줄인 것은 폴더 칸 하나뿐이다(「C: › …synth」 · 긴 이름은 「C: › …-long」 — 가운데 줄임표, §7 허용). 칸 다섯은 내용 폭이고 오른쪽 끝(px 1427)까지 빈 칸 없이 찬다. 「작업 순서」 줄 오른쪽에 「Synthetic 기준」(px ≈1741–1848, 538)이 있다. 긴 이름에서는 「Synthetic_Excava…nchA_LongName 기준」 이 가운데 줄임표다. 그 글과 「작업 순서」 사이는 ≈ 23 논리 px 다. 흙색은 「이어서 열기」 하나이고, 키 줄 여섯과 「모든 키 F1」 이 다 보인다.
  - home-1920 · tab0-1920 · 1920-long: 모든 칸 글과 「Synthetic 기준」(옛 v5-d1-home 과 같은 오른쪽 끝)이 다 보인다. 칸 배분은 바뀌었다 → S2.
  - work-1280: 머리 「A–A′ …」 · 그릴 것 · 세로 · 「1:218」 · 맞춤 · 확대 · 축소가 겹치지 않는다(`sec-header-overlap=0 … carets=1 refit=1/0`). 정보 줄 「✓ 최종(잎) … 잘린 선 1줄 · 54점 · 빈 구간 없음」이 전부 보인다. 평면 눈금은 「2 m 4 m 6 m 8 m 10 m」 이다. draw-1280: 그리는 중 상태가 다섯 곳 모두에 그대로 나온다.
- Kerf 스펙 항목(1–5): 쌓임 5 · 리본 5 · 문서 탭 5 · 안내 자리 4(N4 그대로) · 판 4 · 상태줄 5 · 아이콘 **4**(▾ 가 너무 작음, S1′) · 토큰/색 5(▾ 는 `theme::Hand` · 테 `theme::Card`, 새 색 없음) · 한글 조판 4(1차 3 → 4).

#### 1차 지적 해결 여부
| 1차 지적 | 결과 | 근거 |
| --- | --- | --- |
| UI blocking B1: 1280 카드 칸 이름 · 값 줄임표, 칸 다섯이 같은 폭 | **해결** | 위 home-1280 근거. 감사(새 엄격 판정) `home-clipped=0 cells-elided=0 … min-width=1280 at=run:0:0,1280w:0:0`(1280 ×1). 긴 이름도 같음 |
| code blocking B1: 「<모델> 기준」이 모든 폭에서 사라짐 | **해결** | home-1280 · home-1920 · tab0 두 장에 「Synthetic 기준」, 긴 이름은 가운데 줄임표(툴팁) |
| 내 1차 N6 「1920 모양 그대로 — 충족」 | **정정** | 1차 때 1920 의 「Synthetic 기준」 빠짐을 내가 놓쳤다. code 검토자가 맞다(종합 08:20 의 충돌 해소와 같음) |
| UI should S1: 좁은 머리 메뉴 단추 둘에 ▾ 없음 | **일부 해결** → S1′ | ▾ 는 구워졌다(`iconWithCaret` · `carets=1`). 하지만 보통 배율에서는 메뉴 표시로 알아볼 수 없다(아래) |
| N2 단면 그림 오른쪽 끝 이름 「A」(「A′」 아님) | 그대로(전부터) | work-1280 px ≈(1421,271) |
| N3 제목 「A–A′ …」 오른쪽 여유 | 그대로 | 제목 끝(px ≈1110) ~ 그릴 것(≈1150) 사이 ≈ 27 논리 px. 뜻은 지켜진다 |
| N4 안내 칩 「Shift …」 툴팁 없음 | 그대로(범위 밖) | draw-1280 px ≈(598–843, 252) |

#### 지적
- blocking: 없음.
- **should S1′ — 좁은 머리의 구운 ▾ 가 보통 배율에서 메뉴 표시로 보이지 않음(1차 S1 일부 해결)**
  - 어디: `work-1280.png` · `draw-1280.png` 단면 머리. 「그릴 것」(layers) 그림 px ≈(1167,182), 세로 과장 px ≈(1216,182)(논리 ≈(778,121) · (811,121)).
  - 보이는 것: 150 % 원본 크기에서 두 아이콘 오른쪽 아래에 ▾ 를 알아볼 수 없다. 1차 캡처 `n1280-shots\work-1280.png` 의 같은 자리와 눈으로 구별되지 않는다. 같은 그림의 리본 「기타 · 더보기」 칩 ▾ (px ≈1465,67, 같은 6 px 규격)는 또렷하다. offscreen ×2(`ui-audit-1280x800-x2\work.png`, 그림 px ≈(1620,240))에서야 세로 아이콘 옆에 점 하나가 보인다. 그래도 ▾ 모양은 아니다.
  - 원인(코드 치수): `app/icons.cpp:143–146` 은 6 × 3.6 삼각형을 채운 다음, 같은 다각형에 테 `QPen(halo, 1.5)` 를 친다. 테는 윤곽선 가운데에 그려진다. 그래서 안쪽 ≈ 0.75 px 를 바탕색이 덮는다. 남는 먹색은 ≈ 2.8 × 1.7 논리 px 다(내접원 반지름 1.4 에서 0.75 를 뺀 비율 ≈ 0.47). 150 % 에서 ≈ 4 × 2.5, 100 % 에서 ≈ 3 × 2 기기 px 라 점으로 보인다. 리본 칩 ▾(`icons.cpp:100–105`, 6 × 4 · 테 없음)와 크기 · 모양이 달라 체계 일치도 약하다.
  - 스펙: §4(86행) 「아이콘만 남은 메뉴 단추 … 아이콘 오른쪽 아래에 ▾ 를 굽는다(폭을 늘리지 않음)」. 자리와 「폭 그대로」는 지켰다. 머리 ▾ 크기 규정은 없다. 그래서 치수 위반은 아니고 should 다. 툴팁 · 키(I · L · V · X)로도 닿으므로 6 · 7 항목은 4 다.
  - 고칠 방향: 테를 같은 다각형의 윤곽선으로 치지 않는다. 대신 1 px 큰 삼각형을 테 색으로 먼저 깔고, 그 위에 6 × 3.6(또는 리본과 같은 6 × 4)을 테 없이 채운다. 또는 16 px 안에서 ≈ 8 × 5 로 키운다(단추 폭은 그대로). 고친 뒤 100 · 150 % 실제 화면 캡처에서 리본 ▾ 와 같은 크기로 보이는지 본다. 감사 `carets` 는 속성(`caretBaked`)만 믿는다. 그러니 원하면 구운 장의 오른쪽 아래 먹색 픽셀 수를 세는 줄을 더한다.
- **should S2 — 1920 홈 카드 칸 배분이 바뀜(spec.md 8행 「1920 이상 화면 모양은 바뀌지 않는다」와 어긋남, 이번 수정 탓)**
  - 어디: `home-1920.png` · `home-tab0-1920.png` · `home-1920-long.png` 카드 칸 줄, 그림 px x ≈ 778–2386 · y ≈ 464–547(논리 x ≈ 519–1591 · y ≈ 309–365).
  - 보이는 것: 폴더 칸이 남는 폭을 모두 받는다. 폴더 칸은 ≈ 734 논리 px 이고, 「C: › dev › kerf-synth」 오른쪽이 ≈ 600 논리 px 비어 있다. 나머지 다섯 칸은 오른쪽에 내용 폭(합 ≈ 338 논리)으로 몰려 있다. 1차 캡처 `n1280-shots\home-1920.png` 에서는 폴더 ≈ 305 · 나머지 각 ≈ 154 논리(2:1:1:1:1:1)로 고르게 나뉘었다. 글은 다 읽히고 겹침도 없다.
  - 기준: spec.md 8행 · AC11 「1920 이상은 모양 변화 없음」. DESIGN_SPEC §7(110행)의 「칸 1–5 내용 폭 · 폴더 칸만 남는 폭」은 「좁은 창(1280)」 글머리 안에 있지만 폭 조건이 없다. 화판 `.ccard .cells` 는 `grid-template-columns:repeat(6,1fr)`(6등분)이다. 그래서 1920 은 이 작업 전(2:1)에도 화판과 달랐다. 「전부터 어긋남, 이번에 다르게 어긋남」이다.
  - 원인: 내 1차 「고칠 방향 2」가 폭 조건 없이 「칸 1–5 는 내용 폭, 폴더 칸만 늘고 줄게」를 적었고, builder 는 그대로 따랐다(`fitHomeCells`). 치수 · 자리 · 색 규칙 위반도 잘림 · 겹침도 아니라서 blocking 으로 올리지 않는다.
  - 고칠 방향(후속 결정): 내용 폭으로 모으는 것은 칸이 고르게 나눈 폭(2:1 또는 화판 6등분)에 다 안 들어갈 때만 한다. 1920 은 옛 배분으로 둔다. 2:1 로 되돌릴지, 화판대로 6등분할지는 사용자 · 종합이 정한다. 그 뒤 DESIGN_SPEC §7 110행에 「좁을 때만」을 적는다.
- note
  - N1′: 1280 폴더 칸 「C: › …synth」 는 끝 「synth」만 남아 「kerf-」가 잘린다. 스펙(가운데 줄임표) 그대로이고 툴팁이 있다. 마지막 경로 마디를 통째로 남기는 줄임(「C: › … › kerf-synth」)이 더 잘 읽힐 수 있다.
  - N2′: 긴 이름에서는 1920 에서도 「작업 순서」 오른쪽이 가운데 줄임표다(오른쪽 열 ≈ 290 논리). builder 판정 1(창 최소 폭 보호)대로이고 툴팁이 있다. 받아들인다.
  - N3′: rework 2 뒤 1920 **작업 · 그리는 중 실제 화면 캡처는 없다**(`n1280-shots` 의 1920 두 장은 `70b46b4` · `00c9a43` 전 것). offscreen `ui-audit-1920x1040-x1\work.png` 로는 구조만 확인했다. 토글 넷 상자 · 「세로 …」 글 단추 · 「크게」 가 있고 겹침이 없다(`at=run:1:0`). 단계 1(「북쪽을 봄」 숨김)은 □ 글꼴 폭 탓이다(spec 위험 1). 글이 단계 0 그대로인지는 1차 1920 캡처로 갈음했다 — 실제 화면 확인은 못 함.
  - N4′: 정보 줄 4단계의 빈 구간 꼬리(correctness C1 고침)는 실제 캡처에 나오지 않는다(실제 1280 은 2 · 3단계). 그래서 감사 `gap-kept=1`(`1280p:4:0 · 1280d:4:0`)만 믿었다 — 눈으로는 못 봄.
  - N5′: 제목 바닥 「첫 글자 + …」(`1280L:5:0`)와 좌표계 · 높이 값 앞쪽 줄임(correctness C4 고침, `ElideLeft`)은 이번 캡처 어디에서도 발동하지 않았다. 코드와 감사 줄로만 확인했다.
  - N6′: 전부터 있던 N2(단면 그림 오른쪽 끝 「A」)와 범위 밖 N4(안내 칩 「Shift …」)는 그대로다. 점검 문서로 넘긴다.
- 남은 위험: S2 의 1920 칸 배분(2:1 복귀 또는 화판 6등분)은 후속 결정이다. S1′ 는 실제 화면 100 · 125 · 175 % 에서 눈으로 보지 못했다(이 PC 는 150 % 하나).
- 수: 판정 **pass** · blocking 0 · should 2(S1′ · S2) · note 6

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

## 종합 2차(2026-10-10 09:50, 팩토리 — rework 2/2 뒤 재검토)
- 검토 결과: code **pass**(blocking 0 · should 3 · note 10) · correctness **pass**(blocking 0 · should 2 · note 6) · security **pass**(note 3) · kerf-ui **pass**(blocking 0 · should 2 · note 6, 1280 캡처 루브릭 모두 4 이상 → AC11 충족).
- 1차 지적 해결: code B1(「<모델> 기준」 사라짐) **해결**(대안 ② 판정에 code 검토자 동의) · UI B1(칸 이름 줄임표) **해결** · code S1(감사 사라진 라벨) **해결**(RED `home-clipped=2` → 0) · correctness C1(빈 구간 꼬리) **해결**(`gap-kept=1`, RED 0) · C3 · code S2(머리 맞춤 한 번) **머리는 해결**(`refit=1/0`), 정보 줄은 아직 매번 처음부터(code should B) · C4(좌표계 값 앞쪽부터) **해결** · UI S1(메뉴 ▾) **구현됐으나 눈에 거의 안 보임 — 미해결(S1′)**: `app/icons.cpp:143–146` 이 채운 삼각형 위에 1.5 px 바탕색 테를 같은 윤곽에 쳐서 보이는 먹색이 약 2.8 × 1.7 논리 px(150 % 캡처에서 3 × 2 장치 px 점). 감사 `carets` 는 속성만 봐서 못 잡음(code A · UI S1′ 같은 결론).
- **스펙 어긋남(사람 결정 필요):** 1920 이상 홈 카드 칸 배치가 바뀜 — 전에는 폴더 2 : 나머지 1, 지금은 폴더 칸이 남는 폭을 전부 받고 나머지 다섯은 내용 폭으로 오른쪽에 몰림(1920 에서 약 600 px 빔, 2560 기준 화면에서는 더 큼). spec.md §1 「1920 이상 화면 모양은 바뀌지 않는다」 · AC11 과 글자로 어긋남(code C · UI S2 · correctness note 같은 결론). 원인 = 1차 UI 「고칠 방향 2」에 「좁을 때만」 조건이 빠졌고 builder 가 그대로 따름. DESIGN_SPEC §7 은 이 규칙을 「좁은 창」 항목 안에 적어 문서와 코드도 어긋남. 화판(`.ccard .cells` 6등분)과는 이 작업 전부터 다름.
- 검토 충돌: **없음**(넷 모두 pass, should 내용도 서로 같음).
- ai_review: **pass**(blocking 0). 단 위 두 건(▾ 안 보임 · 1920 배치)은 사용자 결정 몫 — 고치려면 수정 회차가 한도(2)를 넘으므로 사용자 허가가 필요하다(SKILL §2). 자동 승인하지 않고 사용자에게 묻는다.
- 후속으로 넘길 것(이번 수정과 무관): correctness S1 감사 `refit` 을 `== 1` 로 + 보이는 제목 확인 · correctness S2 `stripCut_` 가 숨겨져도 `gap-kept=false` · code should B `fitInfoStrip` 입력 비교 · code note(`:571` 헛 setIcon · 칸 폭 정수 반올림 경계) · security note 1–2(감사 실패 줄에 라벨 글 · `.arg` 연쇄) · 3(`secHeaderKey_` 의 `|`) · 1차 미룸(correctness C2 · 단면 오른쪽 끝 「A」 · 안내 칩 툴팁).
- 남은 위험(받아들인 것): `setAccessibleName("그릴 것")` 범위 밖 한 줄 — 화면 · 동작 불변(code 검토자 수용) · 125 · 175 % 진짜 모니터 미확인(offscreen V2 로 갈음) · 「…5186」 줄임 길은 감사에서 한 번도 안 돎(손 확인만).
