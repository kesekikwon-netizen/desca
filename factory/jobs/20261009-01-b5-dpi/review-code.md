## code-reviewer

검토 범위: worktree `C:\dev\kerf-wt\20261009-01-b5-dpi` · `0f7fbfe..66737c5`(커밋 7, 13파일 +189/−44). 미커밋 변경 없음(`git status --porcelain` 빈 출력). diff 전체 + 바뀐 함수의 호출처(`setCamera` 복원 `mainwindow_ui.cpp:1134–1138`, 축척 칸 `:687–691`·`:595`, `setResult` `sectionview.cpp:24–31`, `GuideBand` 생성처 `mainwindow_ui.cpp:415`·`sheetexport.cpp:982`, 홈 격자 `mainwindow_ui.cpp:1170–1364`)를 읽었다. 실행은 하지 않았다(읽기 전용) — 아래 재현 시나리오는 코드 읽기에서 나온 것이고 「확인하지 못함」 표시가 붙은 것은 돌려 보지 않았다.

### 판정: **fail** (blocking 1)

### 강점
- `app/sectionview.cpp:58–59` · `app/planview.cpp:325–328`: 「맞춤 상태」 플래그 하나로 다시 맞춤과 사용자 보기 유지를 가른다. 사용자 보기 경로(가운데 · 축척 유지)는 옛 코드 그대로 남아 회귀 범위가 작다.
- `app/planview.cpp:1041` · `app/sectionview.cpp:408`: 휠을 굴린 순간(`wheelZoom`, 부드러운 확대 타이머가 돌기 전)에 `fitted_` 를 끈다(Review Focus 3). `hasScene_`/`has_` 조기 반환 뒤라 빈 화면에서는 상태가 안 바뀐다. 축척 칸은 `activated`(사용자 동작)에만 묶여 있고 `updateHeader` 의 `setEditText` 는 `QSignalBlocker` 아래라(`mainwindow_ui.cpp:595`), 다시 맞춘 뒤 머리 갱신이 `fitted_` 를 몰래 끄는 되먹임은 없다.
- `app/planview.cpp:356–378` `contentFits()`: 저장된 `proj_` 대신 지금 `width()/height()/mpp_/target_/yaw_/pitch_` 로 다시 잰다. offscreen 에서 `resizeGL` 이 불리지 않아 생기는 거짓 통과를 막는다(spec Task 1 주의 그대로). RED 기록 `plan-fits=0` 이 그 증거다.
- `app/icons.cpp:19,119` · `app/ribbon.cpp:197`: 배율 목록이 한 곳(`kerf::iconDprs()`)으로 모였고, 손으로 적은 목록이 남지 않았다.
- `app/guideband.cpp:41–45,77,112–115`: 굽기 함수가 하나(`bakeIcon`)이고 사건 경로 · `place()` 안전판 · `setTool` 이 모두 그것을 부른다. 빈 이름이면 아무것도 안 한다. 호출처 셋 모두 이름이 비어 있지 않다.
- `.github/workflows/ci.yml:69–81`: `Start-Process -PassThru` 뒤 `$null = $p.Handle`(ExitCode 를 잃지 않으려고), 시간 초과면 Kill + throw, 종료 코드와 마지막 `RESULT` 줄을 **둘 다** 본다. `ExitCode` 를 못 읽으면 `$null -ne 0` 이 참이라 거짓 통과가 아니라 거짓 실패 쪽으로 넘어진다. 실행마다 settings · audit · log 폴더가 따로다.
- 감사 판정을 느슨하게 하지 않았다: offscreen 판정(V8 · CI)은 spec §3-A 그대로이고, 판정 2 는 실제 화면에서만 범위를 줄이면서 ` screen-avail=` 꼬리로 범위를 드러낸다.
- 저장소 규칙: `Q_OBJECT` 없음, 새 연결 없음, 새 캐스트 없음, 새 색 없음(`theme::Hand` 만 씀), 재포맷 없음, `core/ tests/ tools/ CMakeLists.txt` 변경 0.

### 지적

#### blocking
1. **`app/mainwindow_ui.cpp:1209` — 이어서 작업 카드의 모델 이름(`heroName`, 40 px 명조)이 아직 창 최소 폭을 밀어낸다. 그래서 B5 목표(1280 창)와 새 스펙 규칙 ①이 실제 모델 이름에서는 깨진다.**
   - 무엇이: Task 2 는 홈의 한 줄 글 넷(`:1194` 끌어 놓기 안내, `:1213` 카드 보조 줄, `:1318` 표 도움말, `:1355` 키 줄)에만 가로 `Ignored` 를 줬다. 같은 카드의 모델 이름 라벨 `lab(QFileInfo(f0).completeBaseName(), "heroName")` 은 그대로 둬서, 최소 폭이 글 전체 폭이다(줄바꿈 없는 QLabel).
   - 왜 문제: `QStackedWidget body_` 의 최소 폭은 숨은 페이지를 포함해 모든 페이지의 최소 폭 중 가장 큰 값이다(builder 가 찾은 1591 의 원인과 같은 길). 홈 격자는 `g->setColumnMinimumWidth(0, 420)` + 간격 48×2 + `setColumnMinimumWidth(2, 280)` 이고, 카드가 1–2열을 차지한다(`:1264`). 따라서 카드 최소 폭 = 이름 폭 + 40(여백) + 280(`ccSide` 고정)이 그대로 홈 최소 폭에 더해진다. builder 실측으로 「Synthetic」(9자)일 때 홈 1146 · 창 1280(리본 `need20` 이 정함)이니, 이름 쪽 여유는 약 130 px 뿐이다. 40 px 명조에서 한글 8–9자(예: 「○○유적3호주거지」), 영문 15자 정도를 넘으면 창 최소 폭이 1280 을 넘는다. 보통 발굴 모델 파일 이름은 이보다 길다. 감사는 합성 모델 이름이 「Synthetic」이라 이것을 잡지 못한다(AC1 ②가 거짓 통과하는 입력).
   - 재현(확인하지 못함 — 실행 안 함): `C:\dev\kerf-synth\Synthetic.3mx` 를 같은 폴더에 `Synthetic_Excavation_Area3_TrenchA.3mx` 로 복사한다(ASCII 경로, 타일은 상대 경로). `SectionViewer.exe <그 파일> --line 200002 450006 200014 450006 --size 1280x800 --settings <새 폴더> --ui-audit <폴더> --quit`(offscreen)을 돌리면 `ui-audit env size=(1280보다 큼)x800 want=1280x800 … min-width=(1280보다 큼) … FAIL` 로 예상된다. 사용자 쪽에서는 1366×768 노트북에서 최근 모델 이름이 긴 채로 앱을 켜면 창을 1280 으로 줄일 수 없고 화면 밖으로 나간다.
   - 고치는 법: `heroName` 라벨에도 가로 `QSizePolicy::Ignored` 를 주고, 잘림 대신 줄임표가 나오게 한다(예: `resizeEvent` 에서 `fontMetrics().elidedText(…, Qt::ElideMiddle, width())`, 툴팁에 전체 이름). 감사(또는 V8)에 긴 모델 이름 실행을 하나 더해 잠근다. 같은 김에 카드 안의 다른 고정 글(`cap` 「저장됨 · 이 PC」 등)도 살펴본다.

#### should
1. **`.github/workflows/ci.yml:46` (+ 지운 `linux-core`) — 이제 CI 어디에서도 `CoalescingWorker` 시험이 돌지 않는다.** 지운 Linux 작업이 `asec_tests` 전체를 돌리던 유일한 곳이었다. Windows 작업은 `"~CoalescingWorker*"` 로 네 개를 모두 뺀다. 그런데 CLAUDE.md 에 따르면 불안정한 것은 그중 하나(「끄는 동안의 미리보기는…」)뿐이다. 단면 계산 스레드의 핵심(마지막 요청만 남기기 · 최종이 실행 중 계산을 취소)을 시험하는 안정적인 3개가 회귀 감시에서 조용히 빠졌는데, spec 은 이 부수 효과를 적지 않았다. 고치는 법: Windows 에서는 그 하나만 이름으로 빼거나(Catch2 test spec), 후속 작업으로 그 시험에 태그(`[flaky]`)를 달고 `~[flaky]` 로 바꾼다. 최소한 승인 보고서 「후속 일」에 올린다.
2. **`app/mainwindow_ui.cpp:1134–1138` + `app/planview.cpp:235` — 저장된 카메라를 되살리면 맞춤 상태가 꺼진다. 그래서 자동 맞춤은 사실상 「그 모델을 처음 열 때」만 동작한다.** 카메라는 늘 저장된다(`:1079`). 따라서 최근 목록 · 「이어서 열기」로 다시 연 모델(보통의 사용 흐름)은 50 ms 뒤 `setCamera` 로 `fitted_=false` 가 된다. 사용자가 손대지 않았어도 창을 줄이면 평면이 잘린다. spec Task 5 가 `setCamera`(복원 포함)를 「끄는 곳」으로 정했으니 builder 가 spec 을 따른 것이다. 하지만 사람이 기대하는 것(지난번에 맞춤으로 닫았으면 이번에도 맞춤)과 다르다. 고치는 법(사람 결정): 저장할 때 `fitted` 를 함께 적고(`camera` 넷째 칸 또는 `planFitted` 키), 복원 때 맞춤이었으면 `setCamera` 대신 `fitAll()` 을 부른다. 단면은 복원 때 `fitNextResult_` 길로 이미 맞춤이 되므로 문제 없음을 확인했다.

#### note
1. **`app/guideband.cpp:77` — `place()` 안전판은 호스트 크기가 바뀌어야 돈다.** Windows 에서 Qt 6 는 배율이 다른 모니터로 옮길 때 논리 크기를 지키므로(WM_GETDPISCALEDSIZE 처리) 호스트 `Resize` 가 오지 않을 수 있다. 그 경우 Qt 가 `DevicePixelRatioChange` 를 자식 위젯까지 보내지 않으면(spec §5 「확인하지 못함」, 나도 Qt 소스가 이 PC 에 없어 확인하지 못함) 안전판도 돌지 않는다. 감사 `dpr-rerender` 는 사건을 직접 보내므로 이 길을 재지 않는다. 또 이 감사는 「다시 구웠다(cacheKey 바뀜)」만 보고 「새 배율로 구웠다」는 보지 않는다. 더 튼튼한 길: `icon_` 을 고정 pixmap 대신 `kerf::icon()`(여섯 장 QIcon)을 그리는 작은 위젯/`QToolButton` 으로 바꾸면 Qt 가 그릴 때 배율을 고르므로 다시 굽기가 필요 없다. AC3 수동(모니터 둘) 확인 전까지 위험으로 남긴다.
2. **`app/planview.cpp:325–328` — 맞춤 상태에서 크기가 바뀌면 GL 경로가 한 번에 두 번 그린다.** `QOpenGLWidget::resizeEvent` 기본 구현은 `resizeGL` 뒤 그 자리에서 그림 사건을 보낸다(Qt 5–6 구현 기억 — 이번에 소스로는 확인하지 못함). 그래서 옛 카메라로 한 장을 그린 뒤 `fitAll()` 의 `update()` 로 한 장을 더 그린다. 결과는 맞지만, 4K 에서 창 모서리를 끄는 동안 프레임 비용이 두 배이고 한 장은 옛 축척일 수 있다. 고치는 법: `fitted_` 이면 카메라(target_ · mpp_)를 먼저 계산하고 기본 구현을 그 뒤에 부른다(`fitAll` 의 계산 부분을 `update()` 없는 함수로 나눔).
3. **`app/mainwindow_ui.cpp:1355`(키 줄) · `:1213`(카드 보조 줄) — `Ignored` 라벨은 좁아지면 줄임표 없이 글자 중간에서 잘린다.** 실제 화면 1280(150 %) 캡처 `C:\dev\tmp\b5-shots\start-1280.png` 에서는 키 줄이 약 1227 논리 px 로 다 들어갔다. 하지만 builder 가 offscreen 글꼴로 잰 키 줄 최소 폭은 1495 였다. 글꼴이 다르거나(대체 글꼴) Windows 「텍스트 크기」를 키우면 오른쪽 「모든 키 F1」부터 잘린다. 후속으로 좁으면 뒤 키를 숨기거나 줄임표를 쓰는 규칙을 검토한다.
4. **`app/mainwindow_ui.cpp:1857` — 넘침 판정이 1 px 넘침을 놓친다.** `QRect::right()` 는 `x + w − 1` 이라 `r.right() > width()` 는 `x + w ≥ width() + 2` 일 때만 참이다. 창 밖으로 정확히 1 px 나간 위젯은 넘침이 아니다. `r.right() >= width()`(또는 `!rect().contains(r)`)가 의도에 맞다. bottom 도 같다.
5. **`app/icons.cpp:131,141` — 칩 QIcon 한 개가 이제 4 모드 × 2 상태 × 6 배율 = 48장을 미리 굽는다(전에는 32장).** 타일 50 기준으로 칩 하나에 약 1.3 MB 에서 1.7 MB 가 된다. 리본 모양(타일 50/32/24/20)이 바뀔 때마다 새로 굽는(`chipIconCached` 열쇠에 tile 이 있음) 시간도 1.5배다. 측정은 없다(감사 시간은 첫 그림 기다림 60 s 가 대부분이라 드러나지 않음). 이미 미룬 `QIconEngine`(필요할 때 굽기)로 풀리는 문제라 후속 기록만 권한다.
6. **`.github/workflows/ci.yml` — GCC/Linux 컴파일이 사라졌다.** core 를 MSVC 밖에서 컴파일하는 곳이 없어진다(R7.4 · 사용자 선택이라 결정은 맞음). 다만 core 의 Qt 없음은 `archsection` 이 Qt 를 링크하지 않는 한 Windows 빌드에서도 지켜지므로 큰 손실은 아니다. 참고만.
7. **기준 커밋 `0f7fbfe` 의 Windows CI 가 이미 failure 다(verify.md ci(base)).** 원인이 빌드 · 시험 단계라면 새 감사 단계는 실행되지도 않는다. 「CI 가 이제 판정한다」(AC8)는 push 뒤 그 실패를 먼저 풀어야 확인된다.
8. **감사 `zoom-kept`(`app/mainwindow_ui.cpp:2139–2145`)는 `zoomBy` 경로만 잠근다.** 휠 · 끌기 · 돌리기 · `setCamera` · `setViewPitch` 의 `fitted_=false` 는 코드로만 확인된다(나도 각 줄을 읽어 확인함: planview 235 · 851 · 985 · 993 · 1027 · 1041 · 1079, sectionview 399 · 408 · 434 · 457). 회귀를 잠그려면 `wheelZoom(…, 1)` 직후(타이머 전) `resize` 하는 감사 한 줄을 더하는 것이 싸다(Review Focus 3).

### 판단을 보류한 것
- 1280 작업 화면의 단면 머리 · 정보 줄 겹침/잘림(`work-1280.png`): 기준 커밋에도 있던 모양으로 보이고, 줄이는 순서는 디자인 결정이다. kerf-ui-reviewer · 후속 작업 몫이라 등급을 매기지 않았다.
- 홈 앱 아이콘 56 px(dpr 2 고정) · 「작업 순서」 체크 표시를 다시 굽지 않는 것: spec §2 가 범위 밖으로 명시했다.
- 첫 실행 기본 크기 1600×950 이 작은 화면보다 큰 것: spec Q1 이 닫혔고 backlog #2 로 넘어갔다.
- Linux CI 작업 삭제 자체: 사용자 선택 · DEV_RULES R7.4 라 결정으로 받아들였다. 부수 효과만 should 1 · note 6 으로 적었다.
- 기준 커밋 Windows CI 실패의 원인: 이 diff 와 관계가 확인되지 않았다(로그를 보지 않음).
- AC9 캡처 루브릭 점수: kerf-ui-reviewer 몫이다.
- `SectionView::setVerticalExaggeration` 이 맞춤 상태를 유지하는 것: spec 이 명시했고, 다음 크기 변화 때 세로 과장을 반영해 다시 맞추므로 사용자 기대와 어긋나지 않는다고 보았다.

### builder 판정 4개 의견
1. Task 2 수정 위치(홈 한 줄 글에 가로 `Ignored`): 원인 진단과 방식(이미 쓰던 `vl` · `pth` 와 같은 방식)은 맞다. 다만 같은 원인의 `heroName`(모델 이름)을 빠뜨려 **불완전하다** → blocking 1.
2. env ① · ④를 실제 화면에서만 제한: 타당하다. offscreen 판정은 spec 그대로이고, 실제 화면에서 창이 사용 가능 영역에 잘리고 dpr 이 OS 배율 × 변수인 것은 사실에 맞다. 「`want` ≤ `avail` 인데 테두리 때문에 잘리는」 경우는 거짓 실패 쪽이라 안전하다.
3. ` screen-avail=` 꼬리: 문제없다. 판정 범위를 드러내는 좋은 선택이고, offscreen 출력은 그대로다.
4. push 전 자기 브랜치에서 커밋 메시지를 고쳐 다시 올린 것: 허용 범위다(공유 브랜치 아님, 내용 diff 같음, 기록함). 승인자가 알면 된다.
