## correctness-reviewer

검토: Base `0f7fbfe` → Head `66737c5`(worktree `C:\dev\kerf-wt\20261009-01-b5-dpi`, HEAD · 인덱스 · 브랜치는 건드리지 않음). 읽은 것: spec.md · build.md · verify.md · profile §7 · §8, `git diff 0f7fbfe..66737c5` 전체(app 12개 · ci.yml · DESIGN_SPEC), `sectionview.cpp` 의 fit · resizeEvent · zoomBy · setVerticalExaggeration · setResult · paintEvent 의 onViewChanged, `planview.cpp` 의 fitAll · updateMatrices · screenToLocalXY · viewRectLocal · setCamera · topView · homeView 와 카메라 값을 바꾸는 모든 곳(grep), `mainwindow_ui.cpp` uiAudit 0) · 5b · 2b · `C:\dev\tmp\b5-ac10\ui\ui-audit.txt`.

**판정: pass(blocking 0)** · should 2 · note 7

### 목록 점검
- 좌표(SRSOrigin double · 3MX x=동 y=북 · float): **해당 없음**(core · tests · tools · CMake 변경 없음: `git diff --stat 0f7fbfe..66737c5 -- core tests tools CMakeLists.txt app/CMakeLists.txt` 출력 없음). 평면 `contentFits` 는 장면 중심 기준 float 행렬을 쓰고 `center_`(double)를 나중에 더한다 — `screenToLocalXY` 와 같은 방식이고 큰 좌표를 float 로 계산하지 않는다.
- SectionFrame s · d · z: 단면 `contentFits` 는 s 를 `xf_.s0 … xf_.s0 + plot.width()/ppm` 로 잰다. `screenToSZ` · `zoomBy` 의 s 식과 같다. 방향 반전은 L 을 바꾸지 않아 영향 없다.
- 단위: 화면 축척(`screenDenom = logicalDpiX/(0.0254·ppm)`). 맞춤 상태에서 창 크기가 바뀌면 ppm 이 바뀌는데, `paintEvent` 의 `lastPpm` 감시(sectionview.cpp:378)가 `onViewChanged` → `updateHeader` 를 불러 축척 칸이 낡은 값으로 남지 않는다(확인함). 「1:50」 같은 축척을 고르면 `setScreenDenom → zoomBy` 가 `fitted_=false` 로 만들어, 창 크기가 바뀌어도 고른 축척이 그대로다. 측량 앱 기대와 맞다.
- 수직 기준 · 단면 자르기 두 패스 · 입출력 호환(DXF · GeoTIFF · LAS · SVG/PDF · 옛 설정): **해당 없음**(코드 변경 없음). asec-section 기준값 `polylines=1 vertices=54` 는 바뀌지 않았다(verify.md 모든 블록).
- 허용오차: `1e-6 m`(profile §7 「단면 끝점 s 좌표」를 빌림) · zoom-kept 완전 일치 · 아이콘 픽셀 완전 일치 · `qFuzzyCompare`. 새 허용오차가 없고 기대값도 바꾸지 않았다. RED → GREEN 기록이 있다(1920 에서 `plan-fits=0 section-fits=0`, `icon-dpr-bad=2`, `dpr-rerender=0`).

### 손계산 ①: 맞춤 직후에는 반드시 참, 잘린 상태에서는 반드시 거짓인가
- **단면 `fit()`** sectionview.cpp:43: `ppm = max(1e-3, min(pw/(1.03L), ph/(zr·vex·1.06)))` 이므로 `pw/ppm ≥ 1.03L` 이다.
  - 따라서 `s0 = L/2 − pw/(2ppm) ≤ −0.015L`, `sb = L/2 + pw/(2ppm) ≥ 1.015L`.
  - 12 m 선이면 양쪽 여유가 0.18 m 로 1e-6 m 보다 훨씬 크다. L=0 이어도 s0<0<sb 다. `plotRect` 폭은 최소 320−114=206 px 다. ppm 바닥값(1e-3)은 `pw/ppm` 을 키우는 쪽으로만 작용한다.
  - → **맞춤 직후에는 항상 참**이다.
  - 잘린 경우: 폭이 제한하는 맞춤에서 창 폭이 1.5 % 넘게 줄었는데 다시 맞추지 않으면 거짓이 된다. 1920→1280 에서 RED 로 실제 잡혔다.
  - 높이가 제한하는 맞춤(키 큰 단면 · 세로 과장)에서는 가로 여유가 커서 다시 맞추지 않아도 참일 수 있다(아래 N2).
- **평면 `fitAll()`** planview.cpp:349: `mpp = max(w/W, h/H)·1.04`, `W = width−40` 이다.
  - 보이는 가로 = `width·mpp ≥ 1.04·w·width/(width−40) > w` 이고 세로도 같다. → yaw 가 0 · 90 · 180 · 270 이면(pitch 무관, refZ 평면 위 발자국이 축에 나란한 직사각형) **항상 참**이다.
  - 예외: 평면 칸이 48 px 보다 좁으면 `W = max(50, …)` 탓에 거짓이 될 수 있다. 실사용 크기는 아니다.
  - yaw 가 그 밖의 각도이면 거짓 통과가 생긴다(S2).

### 강점
- `PlanView::contentFits` 가 저장된 `proj_` 대신 지금 `width/height/mpp_/target_/yaw_/pitch_` 로 지역 행렬을 세운다. offscreen 에서 `resizeGL` 이 불리지 않아 생기는 거짓 통과를 정확히 피했다. 식은 `updateMatrices` · `screenToLocalXY` 와 줄 단위로 같다. 같은 refZ(장면 상자 윗면) 평면 가정이다.
- `fitted_` 를 켜는 곳(fit · fitAll)과 끄는 곳의 목록이 카메라 값을 쓰는 모든 줄(grep)과 들어맞는다.
  - 평면: setCamera · setViewPitch · 끌기 · 돌리기 · applyZoomAt · wheelZoom · zoomBy.
  - 단면: 끌기 · applyZoomAt · wheelZoom · zoomBy(setScreenDenom 포함).
  - `wheelZoom` 이 타이머가 돌기 전에 바로 끄는 것도 맞다.
- zoom-kept 의 완전 일치는 타당하다. 맞춤 상태가 아니면 resize 길(sectionview.cpp:60–67 · planview.cpp:327 `updateMatrices`)이 ppm · mpp 에 산술을 하지 않는다.
- `lround(18×1.25)=lround(22.5)=23`. `QSize×qreal` 의 `qRound(22.5)=23` 과 같다(둘 다 반올림 0 에서 멀리). 22.5 · 31.5 · 62.5 · 87.5 는 2진수로 정확히 표현되는 값이라 경계 흔들림이 없다. 실측(Qt 6.10.3: 23×23 · 32×32, 그 배율 장)과도 맞다.
- `qFuzzyCompare(dpr, scale)`: 1 · 1.25 · 1.5 · 1.75 · 2 는 모두 2진수로 정확하고 0 이 아니라 안전하다. `QString::toDouble` 은 C 로캘이고 앞뒤 공백을 무시한다. 실패하면 0 → 1.0 으로 돌아간다(아래 N7).
- `GuideBand::event` 는 `bakeIcon()` 뒤 `return QFrame::event(e)` 로 기본 처리와 반환값을 그대로 넘긴다. 올바르다. `iconDpr_=0` 첫 값은 `qFuzzyCompare(0,x)` 가 거짓이라 `place()` 안전판이 한 번 굽는다. 의도대로다.

### 지적

**should**

- **S1 · `app/sectionview.cpp:467–483`(`setVerticalExaggeration`) × `:59`(`resizeEvent` 맞춤 길) — 세로 과장을 바꾼 뒤 창 크기를 조금만 바꿔도 가로 축척이 뛴다.**
  - 무엇이 일어나나: 맞춤 상태에서 「세로 ×N」을 고르면 ppm(가로)은 그대로 두고 세로만 늘린다. 이때 `fitted_` 는 참으로 남는다(spec Task 5 「바꾸지 않는 곳」). 그 뒤 창 · 판 분할선 · 오른쪽 판 접기로 resize 가 한 번이라도 일어나면 `fit()` 이 `ph/(zr·vex·1.06)` 으로 ppm 을 다시 계산한다.
  - 합성 모델로 계산(z 44.20–47.50 → zr 3.3 m, L 12 m). 그림 칸이 1100×350 px 라고 가정하면:
    - ×1 맞춤: ppm = min(1100/12.36, 350/3.50) = min(89.0, 100) = 89(폭이 제한).
    - ×2 선택 직후: ppm 89 그대로이고, 세로 3.3×2×89 = 587 px 로 칸 350 px 를 넘쳐 위아래가 잘린다. 그런데도 `fitted_`=참이다.
    - 1 px resize 뒤: ppm = 350/(3.3×2×1.06) = 50. 화면 축척이 약 1:285 → 1:508 로 바뀐다.
  - 왜 문제인가: 사용자는 가로를 건드리지 않았는데, 판을 조금 움직이는 순간 가로 축척이 거의 절반이 된다. 「맞춤 상태」 표시는 참인데 지금 화면은 맞춤 화면이 아니다. B5 전에는 resize 가 축척을 지켰으니 새로 생긴 동작이다.
  - 고치는 법(명세 · 사용자 결정): ⓐ `setVerticalExaggeration` 에서 `fitted_` 이면 곧바로 `fit()` 해 「보이는 것 = 맞춤」을 맞춘다(세로 과장 즉시 가로가 줄어듦) ⓑ 세로 과장 변경을 사용자 보기로 보고 `fitted_=false`(창을 줄이면 가로가 잘릴 수 있음) ⓒ 세로 과장 뒤 resize 맞춤은 가로 조건만 쓴다. 어느 쪽이든 감사에 「vex 뒤 resize 시 ppm」 줄을 하나 더하면 잠긴다.

- **S2 · `app/planview.cpp:356–377`(`PlanView::contentFits`) — yaw 가 90° 의 배수가 아니면 거짓 통과한다.**
  - 무엇이 일어나나: 화면 네 모서리를 refZ 평면에 내린 점들의 **축 나란 상자(AABB)**를 장면 상자와 비교한다. 화면이 돌아가 있으면 그 AABB 가 실제로 보이는 사다리꼴 · 마름모보다 넓다.
  - `fitAll()` 은 yaw 를 0 으로 되돌리지 않는다. 그래서 돌린 뒤 F · 가운데 클릭으로 `fitted_`=참이 되는 상태가 실제로 있다.
  - 계산: 장면 16×12 m, 평면 1000×600 px, yaw 45°.
    - fitAll: mpp = max(16/960, 12/560)·1.04 = 0.02229 → 보이는 범위 22.29×13.37 m.
    - 돌린 장면의 화면 세로 폭 (16+12)/√2 = 19.8 m > 13.37 m → **실제로 잘림**.
    - 그런데 contentFits 의 AABB 는 (22.29+13.37)/√2 = 25.2 m ≥ 16 · 12 → **참(거짓 통과)**.
  - 왜 문제인가: 머리 주석은 「장면 상자(XY)가 지금 창 크기에서 화면에 다 들어가는가」로 일반 판정을 약속한다. 지금 감사는 yaw 0 · pitch 90 에서만 불러 결과에 영향은 없지만, 나중에 다른 상태에서 부르면 조용히 통과한다.
  - 고치는 법: 장면 상자 꼭짓점(refZ 평면의 4개, 또는 8개)을 지역 `proj*view` 로 NDC 에 보내 `|x|,|y| ≤ 1(+ε)` 를 검사한다. 어떤 yaw · pitch 에서도 정확하다. 또는 `yaw_` 가 90° 배수가 아니면 판정 불가(-1)를 돌려준다. 아울러 `fitAll` 자체가 위에서 본 화면 전용 식이라는 점(기존 동작)을 주석에 적는다.

**note**

- **N1 · `app/mainwindow_ui.cpp:1857`** — 넘침 판정 `r.right() > width()` 는 1 px 를 놓친다. `QRect::right()` 는 `x+w−1` 이므로 창 밖으로 정확히 1 px 나간 위젯(`right()==width()`)이 통과한다. `r.right() >= width()`, `r.bottom() >= height()` 로 바꾼다.
- **N2 · `app/sectionview.cpp:51–55`** — 단면 `contentFits` 의 판정 범위가 좁다.
  - 지금 크기(`plotRect(rect())`) 대신 저장된 `xf_.plot` 을 쓴다. 평면 쪽에서 spec 이 지적한 것과 같은 함정이다. `resizeEvent` 가 아예 빠지는 회귀면 낡은 plot 으로 참이 된다.
  - 가로(s 0–L)만 보므로 높이가 제한하는 맞춤에서는 다시 맞추지 않아도 참일 수 있다.
  - 지금 RED 는 실제로 잡혔으니 blocking 은 아니다. `plotRect(rect(),1.0).width()` 로 재고, z 범위(`zMin…zMax`가 `zTop − ph/ppmZ … zTop` 안)도 함께 보면 단단해진다.
- **N3 · `app/mainwindow_ui.cpp:2134`** — `--size 1280x800` 실행에서는 `resize(1280,800)` 과 `resize(keepSize)` 가 같은 크기라 resize 가 일어나지 않는다. 그래서 그 두 실행(×1 · ×2)의 `plan-fits` · `section-fits` · `zoom-kept` =1 은 아무것도 시험하지 않은 값이다. AC4 증거는 1920 · 3440 · 3840 실행에서만 나온다. 로그에 `resized=0/1` 을 붙이거나 1280 이면 다른 크기(예: 1600)를 거쳐 오면 분명해진다.
- **N4 · `app/mainwindow_ui.cpp:1137`(`setCamera` 복원) × `:1079`(카메라 늘 저장)** — 한 번 연 모델은 다음에 열 때 저장된 카메라가 `setCamera` 로 복원되어 `fitted_=false` 가 된다. 사용자가 확대한 적이 없어도 두 번째부터는 창 크기 변경 자동 맞춤이 꺼진다. spec 이 「저장된 카메라 복원 = 사용자 보기」로 정한 것이지만 「보통 사용자 기대」와는 어긋날 수 있다. 맞춤 여부를 함께 저장(`camera` 옆 `fitted`)하면 풀린다.
- **N5 · `app/sectionview.cpp:29`(기존 동작)** — `keepView` 결과에서 L 이 2 % 안에서 늘면 다시 맞추지 않는다. 맞춤 여유가 1.5 % 라 A′ 쪽이 최대 0.5 % L 잘린다(12 m 선이면 6 cm). B5 전부터 있던 동작이지만, 이제 `fitted_` 가 있으니 `fitted_` 이면 늘 `fit()` 하는 한 줄로 고칠 수 있다.
- **N6 · `app/guideband.cpp`** — `dpr-rerender` 감사는 같은 dpr 로 가짜 사건을 보내 cacheKey 가 바뀌는지만 본다. 새 배율로 구웠는지는 증명하지 못한다(`iconDpr_` 를 감사 줄에 함께 찍으면 보인다). `place()` 안전판은 호스트 resize 때만 돈다. 같은 논리 크기로 모니터를 옮기면 resize 가 없을 수 있으니, 사건이 자식에게 오지 않으면 다시 굽지 못한다. **확인하지 못함:** Qt 가 `DevicePixelRatioChange` 를 자식 위젯에 보내는지, 그 시점에 `devicePixelRatioF()` 가 이미 새 값인지(context7 도구가 이 세션에 없음). AC3 수동이 미실행이니 사람 검토에 남긴다.
- **N7 · `app/mainwindow_ui.cpp:1840 근처`(env ④ · 아이콘)** — 두 가지다.
  - `QT_SCALE_FACTOR` 를 해석하지 못하는 값(예: `1,25`)이면 Qt 도 무시해 dpr=1, 감사도 1.0 으로 돌아가 통과한다. `scale=` 원문이 로그에 찍히니 사람이 볼 수는 있다.
  - Qt 6.8.3(CI) 에서 `QIcon::pixmap(QSize(18,18),1.25)` 가 23×23 장을 그대로 돌려주는지는 **확인하지 못함**(실측은 6.10.3 만). 다르면 CI env 줄이 FAIL 로 드러난다. 거짓 통과 쪽이 아니라 안전하다.
  - 덧: `contentFits` 의 1e-6 m 는 float 행렬(상대 정밀도 약 6e-8, 16 m 장면에서 약 1e-6 m) 앞에서는 이름뿐인 값이다. spec 대로 맞춤 여유(1.5 % · 20 px+4 %)가 판정을 정하므로 문제는 아니다.

### 판단을 보류한 것
- CI 에서 Linux 작업을 삭제한 것(교차 컴파일러로 core 를 검사하던 유일한 길이 없어짐): 빌드 · CI 영역이라 code · security 검토와 사람 검토 몫이다.
- 홈 라벨 `Ignored` 의 화면 잘림 · 모양: 화면 영역이라 kerf-ui-reviewer 몫이다.
- `--ui-audit` 넘침 목록에 들어가지 않은 위젯(판 안 자식, 단면 머리 칸): 화면 규칙 영역이다.
- Qt 의 `DevicePixelRatioChange` 전달 경로: 도구가 없어 확인하지 못함(N6).

### builder 판정 4개 의견
1. **홈 한 줄 글 `Ignored`** — 수치 영향이 없고 원인 분석(홈 최소 폭 1591 = 612+48+742+96 …)이 실측에 근거한다. 동의한다. 잘림 확인은 UI 검토 몫이다.
2. **실제 화면에서 env ① · ④ 제한** — 수치로 타당하다.
   - 사용 가능 영역 2293×912 논리는 3440×1368 물리 ÷ 1.5 와 맞는다. 요청 높이 1040 > 912 라 그 크기로 열릴 수 없다. 실측 940 은 화면 높이(1440/1.5=960)에서 창틀을 뺀 값으로, OS 가 잘랐다는 해석과 맞는다.
   - dpr 1.50 = OS 150 % × `QT_SCALE_FACTOR` 1 이므로 ④ 를 그대로 두면 100 % 가 아닌 모든 모니터에서 반드시 실패한다.
   - offscreen(V8 · CI)은 명세 그대로 판정한다. 통과시키려고 느슨하게 한 것이 아니다.
   - 약점 하나: `judgeDim` 이 **창틀을 뺀 크기 want** 를 **창틀을 포함한 사용 가능 영역**과 비교한다. want ≤ avail < want + 창틀(약 30–40 논리 px)인 높이에서는 거짓 실패가 날 수 있다(거짓 통과가 아니라 드러나는 쪽). `want + (frameGeometry().size() − size())` 와 비교하면 정확해진다.
3. **` screen-avail=` 꼬리** — offscreen 출력에 영향이 없다. 동의한다.
4. **커밋 메시지 교정으로 두 커밋 다시 올림** — 수치 · 정합성 영역 밖이다. builder 가 diff 가 같음을 확인했다고 적었고, 나는 결과 HEAD `66737c5` 만 읽었다.
