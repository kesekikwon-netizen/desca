# 명세 — 20261010-03-audit-tighten #3c 후속: 감사 판정 조이기 · 작은 정리(R1b)

작성: spec-writer · 기준 커밋 d95706fb546576bc6fff9b7d5fbb14f3730dd162 (#3c 병합 3c9b0d4 · R0 병합 5331124 포함)
출처: `factory/jobs/20261010-01-narrow-1280/review.md` 의 code 2차 · 3차(should A–C, note 1–10 / should 1–3, note 1–7) · correctness 2차(should R2-1 · R2-2, note R2-1 · R2-5) · security 2차(note 1–3) · kerf-ui 2차 · 3차(N1 · N2 · N6), 「종합 2차」 548행 · 「종합 3차」 557행, `factory/backlog.md` R1b 줄(45행). 사용자 사전 허가 DEV_RULES R1.5(고장이 아니라 판정을 조이고 정리하는 일).
줄 번호는 모두 d95706f 기준이다(worktree `C:\dev\kerf-wt\20261010-03-audit-tighten`. 명세를 쓰는 동안 읽기만 했다).
superpowers:writing-plans 를 불러 단계 계획을 §3–§4 에 넣었다(따로 계획 파일은 만들지 않음).

## 1. 목표 (1–3줄)
#3c 검토가 남긴 감사(`--ui-audit`)의 빈틈을 막는다. 지금은 화면이 틀려도 통과하는 판정이 몇 개 있다(제목 다시 맞춤 · 빈 구간 글 · 구운 ▾ · 홈 칸 배치). 이것을 틀리면 실패하도록 조인다.
작은 정리도 함께 한다: 헛 호출 · 안 쓰는 속성 · 열쇠 구분자 · 공개 로그에 남는 글 · ▾ 흰 테 · 1280 리본 메뉴 칩 ▾ 가 그림을 덮는 것 · (조건부) 단면 그림 오른쪽 끝 「A′」.
새 기능은 없다. 1920 이상의 화면 모양도 바뀌지 않는다. 눈에 보이는 변화는 셋뿐이다: ▾ 둘레(흰 테가 투명해짐), 1280 리본 메뉴 칩, 조건부로 1280 단면 그림 오른쪽 끝 이름.

## 2. 변경 범위
- 바꾸는 파일:
  - `app/mainwindow_ui.cpp` — 감사 판정(2417–2615) · `fitSectionHeader`(711–753) · `fitInfoStrip`(758–800) · `fitHomeCells`(209) · 「그릴 것」 단추(587–588) · `vexBtn_`(599 · 725) · `applyHighContrast`(1696)
  - `app/mainwindow.hpp` — `secHeaderKey_`(255) 형 바꾸기, 정보 줄 열쇠 · 횟수 멤버 추가(262 근처)
  - `app/icons.cpp` · `app/icons.hpp` — `iconWithCaret`(141–158, `halo` 인자 없앰, 선언 hpp:30) · `renderChip` 의 ▾ 자리(98–104)
  - (필요하면, T4) `app/ribbon.hpp` · `app/ribbon.cpp` — 감사가 네 크기의 칩을 직접 굽는 데 필요한 것(색 묶음 `colors_`(ribbon.hpp:81, 비공개)과 메뉴 칩 아이콘 이름 목록)을 내주는 읽기 접근자 하나. 동작은 바꾸지 않는다
  - (조건부, T6) `app/sectionview.cpp` 302–311 — A / A′ 글자를 자르는 사각형(clip)
  - `docs/design/DESIGN_SPEC.md` — §3.1 60행(메뉴 칩 ▾ 자리) · §4 86행(▾ 테가 투명함) · §12 155–156행(감사 기대값)
- 바꾸지 않는 것(명시):
  - 파일: `core/` · `tests/` · `tools/` · 최상위 `CMakeLists.txt` · `app/CMakeLists.txt` · `.github/workflows/ci.yml` · `factory/scripts/` · `third_party/`.
  - 새 파일 · 새 아이콘 · 새 색 토큰 · 새 Qt 모듈은 없다.
  - `docs/DEV_PROGRESS.md` 는 builder 가 고치지 않는다. 병합 뒤 본 저장소에서 고친다(R4.1, DEV_PROGRESS §5 항목 3).
  - 그대로 두는 화면: 1920 이상 홈 칸 배치(폴더 2 : 나머지 1), 리본 타일 50 칩 모양, 단면 머리 줄이기 순서(§4 ①–⑤), `paintSectionDoc` 의 다른 그림.
  - 범위 밖: ▾ 크기를 하나로 맞추는 일(kerf-ui 3차 N3), vex 밑줄 「_▾」 모양(code 3차 note 2), 안내 칩 「Shift …」 툴팁, 눈금 숫자 10 m 상한(correctness C2).
- Catch2 시험을 더하지 않는 이유: 바뀌는 것이 모두 Qt 화면 쪽(`app/`)이라 Qt 없는 `asec_tests` 로는 잴 수 없다. 이 저장소에서는 `--ui-audit` 판정 줄이 화면 쪽 시험 노릇을 한다(CLAUDE.md 「`app/main.cpp` 가 시험 장치다」). 전체 `asec_tests` 는 `factory verify` 가 그대로 돌린다.
- CMake: 새 소스가 없어 목록을 고치지 않는다. 목록은 손으로 적으므로, 새 파일이 생기면 `app/CMakeLists.txt` 에 직접 더해야 한다. 이번 작업은 새 파일을 만들지 않는다.
- **사람 검토 필수 영역: 해당.**
  - 이유: profile §8 「디자인 토큰 · 화면 — `mainwindow_ui.cpp` · `main.cpp`(`--ui-audit`)」. 이번 작업은 **감사 판정 자체를 바꾼다**(조이기 5개 + 새 판정 줄 2개).
  - 판정을 잘못 조이면 결함이 없는데도 CI 가 실패한다. 잘못 풀면 결함을 놓친다.
  - 조건부 T6(`sectionview.cpp`)은 `paintSectionDoc` 을 거쳐 도면 SVG/PDF · 단면 PNG/TIFF 내보내기 그림에도 들어간다. profile §8 「파일 형식」에는 들지 않는다. 형식은 그대로이고 글자 하나를 자르는 사각형만 바뀐다.
- v6 · v5 계획 겹침: 없음.
  - `docs/design/DESIGN_SPEC_v6.md` 에서 「▾ · 메뉴 칩 · 더보기 · 그릴 것」을 찾았는데 일치가 없다(grep 확인). v5 구현 계획과 겹치는 단계도 없다.
  - 대기 목록 R3 · R4 가 나중에 「더보기」 · 「좌표 입력」을 지운다. 그래서 항목 10은 「더보기」 하나가 아니라 **메뉴가 열리는 칩 전부의 ▾ 자리 규칙**으로 고친다. 그래야 R4 뒤에도 최근 · 이동 · 목록 파일에 그대로 남는다.

## 3. 완료 기준 ↔ 검증 방법 (기준마다 하나 이상)

### 3-0. 두 종류로 나눈다
- **감사 조이기(먼저 RED): AC1 · AC2 · AC3 · AC6 · AC9.**
  - 지금 코드에서 판정이 **거짓 통과**(또는 AC9 는 거짓 실패)하는 것을 임시 변경으로 먼저 보인다. 임시 변경은 커밋하지 않는다.
  - 판정을 고친 뒤 같은 임시 변경으로 FAIL(EXIT 9)이 나는 것을 보인다. 그다음 임시 변경을 되돌려 RESULT ok 를 본다.
  - 이 세 결과(전 · 후 · 되돌림)와 명령, 감사 줄 원문을 `build.md` 에 적는다. 되돌렸다는 증거는 그 임시 줄이 들어 있지 않은 `git diff` 다.
- **코드 · 화면 정리: AC4 · AC5 · AC7 · AC8 · AC10 · AC11 · AC12 · AC13.** grep, 판정 줄이 그대로임, 캡처로 확인한다(AC7 · AC8 은 새 판정 줄로 RED 도 보인다).

### 3-1. 기준 표
| # | 완료 기준 | 검증 방법(시험 이름 · 명령 · 확인 절차) | 종류 |
| --- | --- | --- | --- |
| AC1 | (항목 1) 감사 `refit` 판정을 `refitA == 1 && refitB == 0` 으로 조인다(지금 `refitA <= 1`, 2593행). `updateHeader()` 뒤에 **보이는 제목 검사**를 더한다. 세 조건이 모두 맞아야 한다: ⓐ `secTitle_` 의 동적 속성 `fullText == secTitleFull_` ⓑ 보이는 글이 `secTitleFull_` 와 같거나, 「…」로 끝나고 「…」 앞부분이 `secTitleFull_` 의 앞부분과 같다(`fm.elidedText(…, width())` 와 비교하지 않는다 — 맞춤은 라벨 폭이 아니라 `tw` 로 줄인다) ⓒ `secTitleFull_` 가 `updateHeader` 가 만드는 글(`"%1 단면".arg(sectionName())`, 선이 없으면 「단면」)과 같다. 판정 줄 꼴: `sec-header-overlap=… refit=1/0 title-kept=1 …`. | **RED:** 임시로 `updateHeader` 의 `qScopeGuard` 람다(803–804행)를 `--headerBatch_;` 만 하도록 바꾼다(`if (!headerBatch_)` 막이는 그대로). 지금 판정 → `refit=0/0` 인데 통과(거짓 통과)하고, 머리에 「단면 감사」가 남는다. 고친 판정 → `refit=0/0 title-kept=0 FAIL`(EXIT 9). 되돌린 뒤 → `refit=1/0 title-kept=1 RESULT ok`. `factory verify` ui-audit 단계 | 자동 |
| AC2 | (항목 2) 정보 줄 `stripCutShort_` 에 「빈 구간」이 있는데 `stripCut_` 가 숨겨져 있으면 `gap-kept=0`. 지금 2464행은 `isVisibleTo` 가 거짓이면 이 검사를 아예 건너뛴다. 그리는 중(1280d)은 거짓 실패가 나지 않는다: `updateHeader` 의 그리는 중 길(829–833행)이 `setStripTexts(…, QString(), QString())` 로 `stripCutShort_` 를 비우므로 「빈 구간」이 없다(코드 확인). | **RED:** 임시로 `fitInfoStrip` 의 `apply` 안에 `stripCut_->setVisible(step < 4);` 한 줄을 넣는다. 지금 판정 → `gap-kept=1 state-kept=1 strip-clipped=0`(셋 다 거짓 통과 — `clippedLabels` 는 숨은 라벨을 건너뜀). 고친 판정 → 1280p · 1280d 상태에서 `gap-kept=0 FAIL`. 되돌린 뒤 → `gap-kept=1 RESULT ok` | 자동 |
| AC3 | (항목 3) `fitInfoStrip` 이 지난번과 같은 입력이면 단계를 다시 훑지 않는다. 열쇠는 `fitInfoStrip` **안에** 둔다(머리의 `secHeaderKey_` 와 같은 자리 — 그래야 Resize 길도 덕을 본다). 열쇠 내용: 정보 줄 폭 · `stripStateRich_` · `stripCutFull_` · `stripCutShort_` · 레벨선 글(`lvLine_` · `lvLabel_`, 확대하면 바뀜 — updateHeader 851–852행) · 입면 뒤 글(`stripBand_`). `applyHighContrast`(1696행)가 머리 열쇠와 함께 이 열쇠도 지운다. **새 감사 줄**(조이기가 아니라 새 판정): 횟수 멤버 `stripPasses_` 를 두고, 감사가 `setStripTexts` 를 같은 인자로 두 번 부른다. 첫 번째 1회 · 두 번째 0회면 `strip-refit=1/0`. **재는 순서를 고정한다:** `updateHeader()` 로 실제 글로 되돌린 **뒤**, 직전 상태와 **다른** 글(1280p 의 글)로 두 번 부른다. 직전과 같은 글로 첫 호출을 하면 열쇠가 맞아 `0/0` 이 나오는데, 이것은 결함이 아니라 순서 탓이다(RED 의 `1/1` 도 첫 호출이 직전과 달라야 보인다). | **RED:** 새 줄만 먼저 넣고 열쇠 없이 돌린다 → `strip-refit=1/1 FAIL`(EXIT 9 — 지금은 매번 처음부터 맞춤). 열쇠를 넣은 뒤 → `strip-refit=1/0 RESULT ok`. 같은 실행에서 1280p 의 `gap-kept=1` · `strip-clipped=0` 이 그대로인지 확인 | 자동 |
| AC4 | (항목 4) 감사 실패 줄에 **사용자 · 파일에서 온 글이 찍히지 않는다.** 바꿀 곳: `homeNames`(2487 · 2532행), `clippedLabels` 의 `names`(140행), 칸 검사의 `nm << full.toString().left(16)`(2558행 근처). 모두 라벨 글 대신 고정 식별자를 담는다: 라벨의 `objectName`(없으면 부모의 `objectName`) + 칸 번호 + `cap`/`val`(예: `ccCell2.val`). home 줄(2606–2608)의 `.arg()` 사슬은 한 번에 바꾸는 꼴로 고친다(여러 인자 `.arg(a, b, …)`, 모두 QString) — 또는 `+` 로 이어 붙인다. | **증명(임시 변경):** 1280w 홈에서 「<모델> 기준」 라벨 앞에 `addStretch()` 를 임시로 다시 넣는다(#3c 1차 blocking 1 재현). 그러면 `home-clipped≥1` 줄이 나온다. 고치기 전 → 줄에 `Synthetic 기준` 이 보인다. 고친 뒤 → 같은 줄에 `Synthetic` · `kerf-synth` · `C:` 가 없다(줄 원문을 build.md 에). 되돌린 뒤 RESULT ok. grep: 감사 진단에 쓰는 `left(16)` 0건 | 자동 + grep |
| AC5 | (항목 5) 머리 열쇠 `secHeaderKey_` 를 구분자 글자로 이은 문자열로 두지 않는다. `QStringList`(칸별 비교) 또는 칸을 가진 작은 구조체 · `std::tuple` 로 바꾸고 `==` 로 비교한다. AC3 의 정보 줄 열쇠도 같은 방식으로 만든다. 그래서 제목 · 상태 글에 `\|` 가 있어도 다른 입력이 같은 열쇠가 되지 않는다. | 실패하는 시험을 먼저 만들기 어렵다(지금 감사 입력에 `\|` 가 없음). 그 사실을 build.md 에 적는다. 확인: AC1 `refit=1/0` · AC3 `strip-refit=1/0` 이 그대로 · `grep -n '"%1\|%2\|%3\|%4\|%5"' app/mainwindow_ui.cpp` 0건 · code-reviewer 가 읽음 | grep + 검토 |
| AC6 | (항목 8) 감사 `carets` 를 조인다(2442–2458행). 보이는 아이콘만 단추(「그릴 것」 · ③ 단계 세로 과장)마다 넷을 본다. ① 아이콘이 비었거나(`icon().isNull()` 또는 받은 장 `isNull()`) 받은 장의 폭 ≠ `lround(iconSize × dpr)` 이면 실패. ② 필요한 넓이 `need = lround(12 × dpr²)` 를 받은 장의 폭이 아니라 **단추의 `devicePixelRatioF()`** 로 구한다. ③ ▾ 바깥 띠: 삼각형 바깥 (0.5, 1.0] 논리 px 거리(테 폭 안)에 중심이 있는 픽셀 중 alpha ≤ 64 인 것이 90 % 이상(먹색도 흰 테도 아님 — 획과 떨어져 있고 테가 투명함. AC7 과 이어짐). ④ ▾ 의 자리와 넓이: 같은 이름 · 크기 · 배율로 ▾ 없이 그린 장(`kerf::glyph`)과 견줘 **더해진 먹색 픽셀**의 bbox 가 오른쪽 아래 사분면 안(16 px 기준 x ≥ 8 − 1 · y ≥ 8 − 1, 1 px 는 안티에일리어싱 여유)에 있고, 그 수가 24 × dpr² 이하. ③ ④ 의 사분면 · 넓이 상수는 `kerf::caretTriangle` 을 부르지 않는 **글자 상수**로 둔다(그리기와 감사가 같이 틀리는 사각지대 — code 3차 should 3). 먹색 판정 `alpha > 200 · 채널 ±40` 에 이유 주석 한 줄. 판정 줄 꼴: `carets=1 caret-ink=N/M caret-band=P% caret-box=1 caret-area=K/L`. | **RED 셋**(각각 임시 변경, 지금 판정은 모두 통과 = 거짓 통과): (a) 587–588 대신 `secTogglesMenu_->setIcon(QIcon())` → 지금 `caret-ink=0/0 carets=1`, 고친 뒤 `carets=0 FAIL` (b) `iconWithCaret` 의 테 단계(①)를 뺀다 → 지금 통과, 고친 뒤 띠 판정 FAIL (c) `caretTriangle` 에서 `w = px * 0.9` → 지금 통과, 고친 뒤 bbox 또는 넓이 FAIL. 또 AC7 을 고치기 전 코드(흰 테)에서는 띠 판정이 FAIL 한다(흰 테 alpha 255) — 이것이 AC7 의 RED 다. 되돌린 뒤 1280 × 1 · 1.25 · 1.75 · 2 모두 `carets=1 RESULT ok` | 자동 |
| AC7 | (항목 9) 구운 ▾ 의 테(바깥 1 px)를 바탕색(`theme::Card` #FFFFFF)으로 칠하지 않는다. `QPainter::CompositionMode_Clear` 로 지워 **투명**하게 한다. 그다음 `CompositionMode_SourceOver` 로 돌려 펜 없이 먹색으로 채운다. 그래서 `iconWithCaret` 의 `halo` 인자가 필요 없다 → `icons.hpp:30` 선언과 부르는 곳 588 · 599 에서 뺀다. 출처: context7 `/websites/doc_qt_io_qt-6_8` 「QPainter::CompositionMode」 — `CompositionMode_Clear`: 「The pixels in the destination are cleared (set to fully transparent) independent of the source.」 장(QPixmap)은 `blank()` 가 `Qt::transparent` 로 채운 알파 장이다(icons.cpp 36–41). 이 PC 의 Qt 6.10 에서 그대로 되는지는 AC6 ③ 판정 줄로 확인한다(그 전까지는 「확인하지 못함」). | AC6 ③ 띠 판정: 고치기 전 FAIL → 고친 뒤 ok(RED → GREEN). 캡처(수동): 실제 화면 `SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --vex 2 --settings DIR --shot vex-on-1280.png --quit`. 세로 과장이 켜진 단추(바탕 #FFF4D6)의 오른쪽 아래 ▾ 둘레에 흰 1 px 테가 없는지, kerf-ui-reviewer 가 확대 크롭(scratchpad)으로 본다. 마우스 올림 · 메뉴 열림 상태는 캡처할 수 없다. AC6 ③(테 픽셀이 투명)으로 갈음하고 남은 위험에 적는다 | 자동 + 수동 |
| AC8 | (항목 10) 리본에서 **메뉴가 열리는 칩 전부**(지금: 최근 · 이동 · 목록 파일 · 더보기 — `ribbon.cpp:188` `c.action->menu()`)의 ▾ 가 어느 타일 크기(50 · 32 · 24 · 20)에서도 그림(글리프)을 덮지 않는다. 원인: 지금 ▾ 자리는 타일 크기와 상관없이 `tile − 11, tile − 10` 고정이다(icons.cpp:101). 그래서 타일 20(글리프 12 px, 자리 4–16)에서 ▾(9–15 × 10–14)가 글리프 한가운데에 앉는다. 고치는 방식은 열린 질문 Q1(추천 A). 어느 방식이든 지킬 것: ▾ 크기 6 × 4 그대로(§3.1) · 타일 50 모양 그대로(지금 ▾ 39–45 × 40–44 는 글리프 11–39 밖) · ▾ 가 둥근 타일 밖으로 나가지 않음 · `chip-dpr-bad=0`(기준 장도 같은 `chipPixmap` 을 거치므로 그대로). **새 감사 줄 `chip-caret-cover`:** 메뉴 칩 아이콘 이름마다 × `Ribbon::looks()` 네 크기 × 배율 1 · 2 로 `kerf::chipPixmap(…, caret=false)` 과 `(…, caret=true)` 를 직접 굽는다(창 크기와 상관없이 네 크기를 모두 잼). 굽는 상태는 `QIcon::Normal · QIcon::Off`(`Ribbon::chipReference` 와 같음). 판정: ① ▾ 있는 장에 글리프 먹색 픽셀이 95 % 이상 남음 ② ▾ 바깥 1 px 띠에 글리프 먹색이 없음. | **RED:** 새 줄을 먼저 넣고 지금 코드로 돌린다 → 타일 20(그리고 24 · 32 중 겹치는 크기)에서 `chip-caret-cover=… FAIL`. 고친 뒤 → 네 크기 모두 0 · RESULT ok. 같은 줄에 타일 50 장이 고치기 전과 같은지(① 남음 100 %)도 기록. 캡처(수동): 실제 화면 1280×800 작업 화면 리본 왼쪽(최근) · 오른쪽(더보기) 크롭 — 「·▾」 · 「◔」 가 아니라 글리프 + ▾ 로 읽힘. 1920 캡처의 리본 칩은 전과 같음. `--icon-sheet` missing=0(아이콘 시트는 `caret=false` 라 바뀌지 않음) | 자동 + 수동 |
| AC9 | (항목 11) 감사 `cells-mode` 의 기대값을 합성 모델에 묶지 않는다. 지금(2577행)은 「실행 폭 ≥ 1920 이면 ratio, 1280w 면 content」 고정이다. 바꾼 뒤에는 감사가 `fitHomeCells` 를 부르지 않고 **스스로** 기대를 계산한다: `ccCells` 폭을 2 : 1 : 1 : 1 : 1 : 1 로 누적 반올림해 나눈다. 그 칸 폭에 칸 이름 전부와 칸 1–5 값(동적 속성 `fullText`)이 여백 12 × 2 + 1 로 들어가고, 폴더 칸이 「C:…」를 담으면 `ratio`, 아니면 `content`. 판정: 실제 모드 == 기대 모드. 모델이 열려 있는데 `ccCells` 가 숨으면(mode none) 실패, 모델이 없으면 none 허용. 판정 줄 꼴: `cells-mode=run:ratio/ratio,1280w:content/content`(실제/기대). 합성 모델에서는 지금과 같은 값(1920 이상 ratio · 1280 content)이 나와야 한다 — 같은 실행에서 기록. | **RED(거짓 실패를 보임):** 임시로 홈 카드 칸 이름 「수평 좌표계」를 30자 글로 늘린다. 지금 판정 → 1920 에서 결함이 없는데도 `cells-mode … FAIL`(칸은 내용 폭으로 바뀌었고 잘림은 없음 — `cells-elided=0`). 고친 판정 → 기대도 content 라 ok. 되돌린 뒤 V8 전부 `run:ratio/ratio`(1920 이상) · `1280w:content/content` | 자동 |
| AC10 | (항목 6 · 12) 헛 `setIcon` 정리: 587행의 첫 `setIcon(kerf::icon("layers", 16, theme::Hand))` 을 지운다. 같은 줄의 `setIconSize(QSize(16, 16))` 과 `setProperty("iconName", "layers")`(감사 `dup-icons` 가 읽음)는 남긴다. 읽는 곳 없는 동적 속성 `caretBaked`(588 · 725행)를 지우고, 감사 주석(2440행)의 그 이름도 고친다. | `grep -n caretBaked app/` 0건 · `grep -c 'kerf::icon(QStringLiteral("layers")' app/mainwindow_ui.cpp` 0 · 감사 `dup-icons=0` · `icons-missing=0` · `caret-ink` 값이 전과 같음(16/12 · 24/19 · 46/37 · 72/48). AC7 로 테 픽셀이 바뀌어도 삼각형 안쪽 먹색 수는 같아야 한다. 다르면 build.md 에 이유를 적는다 | grep + 자동 |
| AC11 | (항목 7) 좁은 창 길의 칸 폭에도 정수 반올림 보험 +1 px. 209행 근처 `width[i] = max(textW(cap), textW(val)) + 2 * pad` → `+ 2 * pad + 1`. 폴더 최소 폭(`folderMin`)도 같은 규칙. 옛 비율 길(198행)에는 이미 있다. | 1280 의 폴더 칸이 5 px 남짓 좁아진다. 1280 × 1 · 1.25 · 1.75 · 2 에서 `home-clipped=0 cells-elided=0 cells-mode=…1280w:content/content` 그대로인지 본다. 실패하는 시험을 먼저 만들 수 없음(지금 경계에 걸리는 글이 없음)을 build.md 에 적는다 | 자동 |
| AC12 | (항목 13, **조건부**) 1280 작업 화면 단면 그림 오른쪽 위 끝 이름이 「A′」 로 보인다. **원인 가설**(코드 읽기, 실행 안 함): 맞춤(`SectionView::fit`, 43행)은 폭에 맞을 때 양쪽 여유가 1.5 % 뿐이다. 그래서 `X(f.L)` 가 `pr.right()` 근처에 온다. 글자 사각형(`xs ± 18·ui`, 308행)의 오른쪽 반이 자르는 사각형(303행 `pr.left() … pr.width()`) 밖이라 「′」 가 잘린다. 1920 은 높이에 맞아 여유가 커서 보인다. **builder 가 먼저 이 가설을 확인한다**(superpowers:systematic-debugging — 예: 1280 맞춤에서 `pr.right() − X(f.L)` 를 한 번 찍어 봄, 커밋하지 않음). **맞으면** 자르는 사각형을 가로로만 `18·ui` 넓힌다(위아래는 그대로 — 「그림 칸 위 여백에서만」 규칙 유지. 넓힌 곳은 그림 칸 바깥 여백 왼쪽 60 · 오른쪽 54 안). **틀리거나 고칠 곳이 이 몇 줄을 넘으면** 고치지 않는다. build.md 에 원인을 적고 별도 작업으로 뺀다(Q2). | 캡처(수동): 실제 화면 `--size 1280x800 --shot work-1280.png` — 단면 오른쪽 위 「A′」(고치기 전 같은 명령의 캡처와 나란히). 1920 캡처는 전과 같음. `paintSectionDoc` 을 거치는 내보내기 확인: `--sheet-check DIR` EXIT 0 · `--cut-check` 결과 줄이 전과 같음 · 도면 미리보기 캡처에서 A′ 가 보이고 다른 것과 겹치지 않음 | 수동 + 자동 |
| AC13 | 문서 `DESIGN_SPEC.md`: §4 86행 「바탕색 테 1 px 를 먼저 깔고」 → 「테 1 px 는 투명하게 지우고(CompositionMode_Clear)」. §3.1 60행에 메뉴 칩 ▾ 자리 규칙(Q1 답대로 — 「타일이 작아도 글리프를 덮지 않는다」). §12 155행 `refit=≤1/0` → `refit=1/0 title-kept=1`, `gap-kept` 는 라벨이 숨겨져도 0, 새 줄 `strip-refit=1/0` · `chip-caret-cover=0`. §12 156행에 `carets` 의 크기 · 띠 · 자리 · 넓이와 `cells-mode=실제/기대`(모델 무관). 감사가 실제로 찍는 줄 이름과 문서의 이름이 글자까지 같다. | `grep -n "strip-refit\|chip-caret-cover\|title-kept" docs/design/DESIGN_SPEC.md app/mainwindow_ui.cpp` — 양쪽에 같은 이름. test-runner 가 감사 txt 한 개와 대조 | grep |
| AC14 | 빌드 · 회귀: 새 경고 0(기존 `fopen` · `mirrored` 만) · 전체 `asec_tests` 통과(CoalescingWorker 하나는 알려진 불안정 — profile §9) · `asec-section` 기준값 `polylines=1 vertices=54` · `--icon-sheet` EXIT 0 missing=0 · `--sheet-check` EXIT 0 · `--ctx-shot` guide=1 · core/tests/CMake/CI diff 없음. | `factory verify` 단계(build · tests · asec-section · ui-audit) + `git -C C:\dev\kerf-wt\20261010-03-audit-tighten diff --stat d95706f -- core tests tools CMakeLists.txt app/CMakeLists.txt .github` 가 비어 있음 | 자동 |
| AC15 | **해상도 · 배율(고정 완료 기준):** `--ui-audit` 이 1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × 배율 1 · 2 에서 모두 RESULT ok. 1280x800 · 2560x1440 × 1.25 · 1.75, 2560x1440 × 1 도 모두 RESULT ok. | ① `factory.cmd verify 20261010-03-audit-tighten -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2` ② `factory.cmd verify 20261010-03-audit-tighten -UiAudit on -UiSizes 1280x800,2560x1440 -UiScales 1.25,1.75` ③ `factory.cmd verify 20261010-03-audit-tighten -UiAudit on -UiSizes 2560x1440 -UiScales 1` — 셋 모두 RESULT PASS, verify.md 에 조합별 RESULT 줄 | 자동 |
| AC16 | **캡처에서 글자 잘림 · 겹침 없음.** 루브릭(전역 8항목) 모든 항목 4 이상. | kerf-ui-reviewer 가 실제 화면 캡처를 본다(이 PC 150 %, `shot.ps1 -Exe C:\dev\kerf-wt\20261010-03-audit-tighten-build\app\SectionViewer.exe` 또는 위 명령): work-1280 · draw-1280 · vex-on-1280(AC7) · 리본 1280 크롭(AC8) · work-1920 · home-1920(칸 2 : 1 그대로) · home-1280 · (T6 을 했으면) 도면 미리보기. 크롭은 scratchpad, 캡처는 `C:\dev\tmp\r1b-shots\`(저장소 밖) | 수동 |

### 3-2. 단계 계획 (writing-plans — 작업 하나 = 커밋 하나, 각 작업 끝에 빌드 + 1920 ×1 감사)
공통 제약:
- `Q_OBJECT` 금지 · 연결은 람다 · `static_cast` · 색은 `theme::` 토큰만(새 색 없음).
- 바꾼 줄만 고친다. 파일 전체를 다시 포맷하지 않는다.
- 앱 자동 실행은 늘 `--settings DIR`. GUI 실행은 `Start-Process -PassThru` + `WaitForExit`(R7.2).
- 커밋은 `factory/20261010-03-audit-tighten` 에서만, 파일 이름으로 `git add`(R5.2).

- **T1 머리(AC1 · AC5 · AC10)** — 파일: `app/mainwindow_ui.cpp`(587–588 · 599 · 711–718 · 725 · 1696 · 2479–2485 · 2593–2595), `app/mainwindow.hpp:255`.
  1. AC1 판정 줄(`refitA == 1` + `title-kept`)을 먼저 넣는다. 지금 코드로 RESULT ok 인지 본다(정상 코드는 1/0).
  2. 임시 변경(scope guard 맞춤 끄기)으로 `title-kept=0 FAIL` 을 보고 되돌린다. 옛 판정(`<= 1`)에서는 같은 임시 변경이 통과했다는 줄도 함께 적는다(거짓 통과의 증거).
  3. `secHeaderKey_` 를 `QStringList`(또는 구조체)로 바꾼다. `applyHighContrast` 의 `clear()` 는 그대로.
  4. 587 헛 `setIcon` 과 `caretBaked` 두 곳을 지운다.
  5. 빌드 + 1920 ×1 · 1280 ×1 감사 → `refit=1/0 title-kept=1`. 커밋.
- **T2 정보 줄(AC2 · AC3)** — 파일: `app/mainwindow_ui.cpp`(758–805 · 1696 · 2464 · 2597–2599), `app/mainwindow.hpp:262` 근처(멤버 이름은 `stripFitKey_` · `stripPasses_` 로 고정).
  1. AC2 판정을 고친다. 임시 `setVisible(step < 4)` 로 RED · GREEN · 되돌림을 본다.
  2. `strip-refit` 줄만 먼저 넣어 `1/1 FAIL` 을 본다.
  3. `fitInfoStrip` 첫머리에 열쇠 비교를 넣는다(T1 과 같은 방식, 돌아갈 때 `fittingStrip_` 를 풀 것). `applyHighContrast` 에 `stripFitKey_.clear()`.
  4. 1280 ×1 감사 → `strip-refit=1/0 gap-kept=1 strip-clipped=0`. 커밋.
- **T3 머리 ▾(AC6 · AC7)** — 파일: `app/icons.cpp:141–158`, `app/icons.hpp:30`, `app/mainwindow_ui.cpp:588 · 599 · 2438–2458`.
  1. AC6 판정 ①–④ 를 넣는다. 지금 코드에서 ③ 띠 판정이 흰 테 때문에 FAIL 하는지 본다(이것이 AC7 의 RED). ① ④ 는 임시 변경 (a) (c) 로 거짓 통과 → FAIL 을 보이고 되돌린다.
  2. `QIcon iconWithCaret(const QString& name, int px, const QColor& ink)` 로 바꾼다. 테는 `CompositionMode_Clear` 로 지우고, `SourceOver` 로 돌린 뒤 채운다. 부르는 곳 두 군데를 고친다.
  3. 임시 변경 (b)(테 단계 빼기)로 띠 판정 FAIL 을 보고 되돌린다.
  4. 1280 × 1 · 1.25 · 1.75 · 2 감사 → `carets=1`. vex-on-1280 캡처. 커밋.
- **T4 리본 메뉴 칩 ▾(AC8)** — 파일: `app/icons.cpp:84–106`(`renderChip`), `app/mainwindow_ui.cpp`(리본 칩 감사 근처에 `chip-caret-cover` 줄).
  1. `chip-caret-cover` 줄을 먼저 넣어 지금 코드로 타일 20 FAIL 을 본다.
  2. Q1 답대로 ▾ 자리 규칙을 고친다(추천 A — §6 Q1). 타일 50 장은 픽셀이 같아야 한다.
  3. 1280 · 1920 감사 → `chip-caret-cover=0 chip-dpr-bad=0`. 리본 1280 크롭 캡처. 커밋.
- **T5 홈(AC4 · AC9 · AC11)** — 파일: `app/mainwindow_ui.cpp`(131–149 `clippedLabels` 의 names · 209 근처 · 2487 · 2549–2580 · 2605–2608).
  1. AC9: 감사 안에 기대 모드 계산을 넣는다(`fitHomeCells` 를 부르지 않음). 임시 30자 칸 이름으로 「옛 판정은 거짓 실패, 새 판정은 ok」를 보이고 되돌린다.
  2. AC4: names 를 고정 식별자로 바꾸고 home 줄 `.arg` 를 한 번에 바꾸는 꼴로 고친다. 임시 `addStretch()` 로 실패 줄을 일부러 내 모델 글이 없음을 보이고 되돌린다.
  3. AC11: 좁은 길 +1 px.
  4. 1280 · 1920 × 1 · 2 감사 → `home-clipped=0 cells-elided=0 cells-mode=…`. 커밋.
- **T6 단면 끝 이름(AC12, 조건부)** — 파일: `app/sectionview.cpp:302–311`. 가설을 확인한다 → 맞으면 한 줄 고치고 `--sheet-check` · `--cut-check` · 캡처 → 커밋. 틀리면 고치지 않고 build.md 에 원인과 「별도 작업 제안」을 적는다.
- **T7 문서(AC13)** — 파일: `docs/design/DESIGN_SPEC.md` 60 · 86 · 155–156행. 마지막 감사 txt 와 줄 이름을 대조한다. 커밋.
- **마무리:** AC15 의 verify 세 번 → AC16 캡처 → build.md(임시 변경 RED 기록 전부 · 판정 · 남은 위험).

### 3-3. 검토 초점 (어느 작업의 판정도 직접 재지 않는 것 — 검토자가 볼 곳)
1. **거짓 실패:** 조인 판정(AC6 띠 90 % · 넓이 24 · AC8 95 %)이 배율 1.25 · 1.75 의 안티에일리어싱이나 offscreen(글꼴 □)에서 결함 없이 실패하지 않는가. AC15 ② 가 잰다. 실패하면 문턱을 몰래 풀지 않는다. build.md 에 값과 이유를 적고 사람에게 확인받는다(Q3).
2. **열쇠의 빈칸:** 정보 줄 열쇠에 빠진 입력이 있으면(예: Resize 없이 글꼴 · 배율이 바뀜), 다시 맞춰야 할 때 건너뛸 수 있다. code 2차 note 8 과 같은 질문이다. 확인하지 못하면 「확인하지 못함」으로 적는다.
3. **Clear 모드와 장 형식:** `QPixmap`(Windows 래스터)에서 `CompositionMode_Clear` 가 알파를 0 으로 만드는가. AC6 ③ 판정 줄이 유일한 근거다.
4. **로그 글:** AC4 를 고친 뒤에도 감사와 같은 실행의 stdout 다른 줄에 모델 이름이 남을 수 있다(예: 창 제목 `window-title=…`, `main.cpp:470`). 이번 범위는 home 줄뿐이다. security-reviewer 가 다른 줄을 따로 판단한다(범위를 넓히지 말고 note 로).
5. **T6 으로 자르는 사각형을 넓힌 뒤:** 도면 미리보기 · SVG 에서 A′ 글자가 그림 칸 오른쪽 여백(54·ui) 안에서 다른 것(축 숫자 · 축척 막대)과 겹치지 않는가.

## 4. 회귀 시험 계획
- 새로 쓸 시험: Catch2 시험은 없다(§2 의 이유). 새로 생기거나 바뀌는 `--ui-audit` 판정 줄(모두 `app/mainwindow_ui.cpp` 의 `MainWindow::uiAudit`):

  | 줄 | 먼저 실패해야 하는 이유(RED) |
  | --- | --- |
  | `sec-header … refit=1/0 title-kept=1` | 끝 맞춤을 끈 임시 변경에서 옛 `<= 1` 은 통과 → 새 판정은 FAIL |
  | `strip-clipped … gap-kept` | `stripCut_` 를 숨긴 임시 변경에서 지금은 통과 → 새 판정은 FAIL |
  | `strip-refit=1/0`(새 줄) | 열쇠가 없는 지금 코드에서 `1/1` FAIL |
  | `sec-header … carets caret-ink caret-band caret-box caret-area` | 빈 아이콘 · 테 없음 · 큰 삼각형 임시 변경에서 지금은 통과 → FAIL. 흰 테인 지금 코드에서 띠 판정 FAIL |
  | `chip-caret-cover=0`(새 줄) | 지금 코드의 타일 20 메뉴 칩에서 FAIL |
  | `home … cells-mode=실제/기대` | 긴 칸 이름 임시 변경에서 지금은 거짓 실패 → 새 판정은 ok |

- 돌려야 하는 기존 시험:
  - 전체 `asec_tests`(`factory verify` 가 돌림 — 태그 · 이름 고르기 없음. core 를 바꾸지 않으므로 기대 결과는 기준 커밋과 같음).
  - `asec-section` 기준값 · `--icon-sheet` · `--sheet-check` · `--ctx-shot` · (T6 을 했으면) `--cut-check`.
  - `--ui-audit` 의 다른 줄은 그대로 ok 여야 한다: `env … overflow=0 icon-dpr-bad=0 chip-dpr-bad=0` · `dup-icons=0` · `narrow … look=20` · `vex-fit=1` · `home-stale-children=0` · `plan-tick-overlap=0`.
- 수치 허용오차: profile §7 의 표는 core 계산(m · mm · 도)용이라 픽셀 판정에 쓸 값이 없다.
  - 기존 감사 값 중 그대로 쓰는 것: ▾ 안쪽 먹색 ≥ 12 × dpr²(`mainwindow_ui.cpp:2440 · 2454`, DESIGN_SPEC §12 156행) · 칸 2 : 1 판정 ±2 / ±1 px(2572–2574행) · 먹색 판정 `alpha > 200 · 채널 ±40`(2449행).
  - 새 값은 모두 **사용자 확인 필요**(Q3):
    - ▾ 더해진 먹색 넓이 상한 **24 × dpr²** — 8 × 5 삼각형 넓이 20 논리 px² 에 안티에일리어싱 여유 20 %.
    - ▾ 사분면 여유 **1 논리 px** — 테 폭이 1 px 라서.
    - ▾ 바깥 띠 **(0.5, 1.0] 논리 px 에서 alpha ≤ 64 인 픽셀 90 % 이상** — 테 폭(1 px) 안만 본다(그 바깥에는 원래 획이 있을 수 있음). 채움 가장자리의 반투명 픽셀을 10 % 까지 허용한다.
    - 메뉴 칩 글리프 먹색 **95 % 이상 남음** — 테 · ▾ 가장자리의 안티에일리어싱이 글리프 끝을 몇 픽셀 건드리는 것만 허용한다.
    - 칸 폭 보험 **+1 px** — 옛 비율 길(198행)과 같은 값.

## 5. 위험 · 되돌리기
- 위험:
  1. 판정을 조이면 결함 없이 실패할 수 있다(특히 1.25 · 1.75 배율의 안티에일리어싱, offscreen 글꼴 □). → AC15 ② 로 잰다. 실패하면 문턱을 몰래 풀지 않는다(전역 규칙 D) — Q3 로 사람에게 묻는다.
  2. CI(GitHub Actions)는 Qt 6.8.3 + MSVC 다. `CompositionMode_Clear` 는 Qt 6.8 문서에 있는 오래된 API 라 같게 동작한다고 본다. 그래도 CI 의 감사 결과는 push 뒤에만 볼 수 있다(팩토리는 push 하지 않음).
  3. 정보 줄 열쇠가 입력 하나를 빠뜨리면, 그 입력이 바뀌어도 다시 맞추지 않아 글이 잘린 채 남을 수 있다(눈에 보이는 회귀) — 검토 초점 2.
  4. T4 의 ▾ 자리 · 글리프 이동은 1280 리본 모양을 바꾼다(작은 타일에서 글리프가 1 px 남짓 위로). kerf-ui-reviewer 가 판단한다.
  5. T6 은 화면과 내보내기(SVG/PDF/PNG/TIFF)가 함께 바뀐다(`paintSectionDoc` 공유). 이미 저장한 내보내기 파일은 바뀌지 않는다(다시 저장할 때만 바뀜).
  6. 메뉴가 열린(눌림) 상태의 ▾ 는 캡처가 없다 — AC6 ③ 픽셀 판정으로만 갈음한다.
- 되돌리기: 브랜치 `factory/20261010-03-audit-tighten` 를 병합하지 않으면 끝. 병합 뒤라면 `git revert <병합 커밋>`. 작업마다 커밋하므로 T4 · T6 만 따로 되돌릴 수도 있다(`git revert <그 커밋>`).

## 6. 열린 질문 (사용자에게 물을 것 — 답이 없으면 사용자 사전 허가 R1.5 · 추천안 자동 진행 허가에 따라 추천안으로 진행)
- **Q1. 1280 리본의 메뉴 칩(최근 · 이동 · 목록 파일 · 더보기)에서 ▾ 가 그림을 덮는 것을 어떻게 고칠까?**
  - **A (추천)** ▾ (크기 6 × 4 그대로)를 타일 오른쪽 아래 구석으로 옮긴다. 그래도 겹치는 작은 타일에서만 그림을 1–2 px 위 · 왼쪽으로 비켜 놓는다. 타일 50(1920 이상)은 지금과 똑같다. → 그림과 ▾ 가 둘 다 온전히 보인다. 손계산: 타일 24 · 32 는 구석 자리만으로 안 겹치고, 타일 20 은 그림을 1 px 올리면 된다. 최종 수치는 builder 가 `chip-caret-cover=0` 으로 확인한다.
  - B ▾ 둘레에 타일색 테를 깔아 그림과 떼어 놓기만 한다(머리 ▾ 와 같은 방식, 자리는 그대로). → 「더보기」 점 셋 중 둘이 여전히 가려 「•▾」 로 보인다. 사용자 말 「겹치지 않게」를 채우지 못한다. 이 안을 고르면 AC8 ① 95 % 기준을 빼야 한다.
  - C ▾ 를 타일 크기에 비례해 줄인다(타일 20 에서 약 2.4 × 1.6 px). → 겹침은 줄지만 ▾ 가 점처럼 작아진다. #3c 2차에서 고친 「점처럼 보임」 문제가 다시 생긴다.
  - (참고) 칩 아이콘은 타일 바탕까지 한 장에 굽는다. 머리 ▾ 처럼 `CompositionMode_Clear` 로 뚫으면 타일에 구멍이 난다. 그래서 B 를 고르면 테는 그 상태의 타일색으로 칠한다.
- **Q2. 1280 단면 그림 오른쪽 끝 이름이 「A′」 가 아니라 「A」 로 보이는 것(전부터 있던 것)을 이번에 같이 고칠까?**
  - **추천:** 원인이 「글자를 그림 칸 폭에서 잘라 버리는 것」으로 확인되면 같이 고친다(몇 줄이고, 도면 · 내보내기 그림도 함께 나아짐). 원인이 다르거나 고칠 곳이 크면 이번에는 손대지 않고 별도 작업으로 뺀다.
- **Q3. 새 감사 문턱 값을 이대로 쓸까?** (§4: ▾ 넓이 상한 24 · 사분면 여유 1 px · 테 띠 90 % 투명 · 칩 그림 95 % 남음 · 칸 폭 +1 px)
  - **추천:** 그대로 쓴다. 근거는 각 값 옆에 적었다. 배율 1.25 · 1.75 에서 결함 없이 실패하면 몰래 풀지 않고, 값과 이유를 승인 보고서에 적어 다시 묻는다.
- **Q4. 감사 실패 줄에 잘린 라벨을 무엇으로 적을까?**
  - **추천:** 라벨의 위젯 이름 + 칸 번호(예: `ccCell2.val`). 모델 이름 · 폴더 경로가 공개 CI 로그에 남지 않는다. 대신 실패했을 때 어떤 글이 잘렸는지는 캡처를 봐야 안다.

## 7. 확인하지 못한 것
- 이 PC 의 Qt 6.10.3 에서 `QPixmap` 에 `CompositionMode_Clear` 가 실제로 알파를 0 으로 만드는지(문서는 6.8 기준, 실행 안 함) — AC6 ③ 이 확인한다.
- AC12 의 원인 가설(자르는 사각형) — 코드 읽기와 맞춤 식(`L × 1.03`)으로만 추론했다. 실행하지 않았다.
- AC8 추천안 A 의 정확한 ▾ 자리 수치 — 손계산뿐이다. builder 가 감사로 확정한다.
- 정보 줄 열쇠가 글꼴 · 배율 변화를 놓치지 않는지(Resize 없이 글 폭만 바뀌는 경우가 있는지) — Qt 내부 동작이라 확인하지 못했다.

## 열린 질문 답 (2026-10-10, 팩토리 — 사용자 사전 허가 DEV_RULES R1.5 · 추천안)
- Q1 → **A**(▾ 를 구석으로, 작은 타일에서만 그림을 1–2 px 비켜 놓음, 타일 50 그대로).
- Q2 → 원인이 「자르는 사각형」으로 확인되면 함께 고치고, 아니면 별도 작업으로 뺌(build.md 에 근거).
- Q3 → 문턱 값 그대로(▾ 넓이 상한 24 · 사분면 여유 1 px · 테 띠 90 % 투명 · 칩 그림 95 % 남음 · 칸 +1 px). 거짓 실패가 나오면 몰래 풀지 않고 build.md 에 적고 멈춤.
- Q4 → 실패 줄 라벨 식별자 = 위젯 이름 + 칸 번호.
