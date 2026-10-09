# 독립 검토 — 20261009-01-b5-dpi 디자인 v5 B5 해상도 · DPI 안정성(4K · 와이드 · 배율, Windows만)

검토 대상: `factory/20261009-01-b5-dpi` HEAD(verify.md 의 마지막 블록과 같은 SHA 여야 함). 검토자는 제품 코드를 고치지 않는다.
판정: **blocking**(고쳐야 함) · **should**(권장) · **note**(참고). blocking 이 하나라도 있으면 rework.

## code-reviewer (결함 · 유지보수성 · 동시성)
- 판정: **pass** (blocking 0 · should 3 · note 4). 범위 `0f7fbfe..66737c5`(worktree HEAD = `66737c5`, 미커밋 변경 없음 확인), 바뀐 13개 파일 diff 전부 + 호출처(`setResult` · `setVerticalExaggeration` · `setScreenDenom` · `scaleCombo_` 연결 · `setCamera` 호출 4곳 · `clearScene`)를 읽음.
- 확인한 것(문제 없음):
  - 동시성: 새 상태 `fitted_` · `iconDpr_` 는 GUI 스레드에서만 읽고 쓴다(`resizeEvent` · 마우스 · 휠 · `setResult`(QueuedConnection 람다 뒤) · `fit`/`fitAll`). `CoalescingWorker` · `ResultGate` · `LodStreamer` 쪽 코드는 바뀌지 않았고 작업 스레드가 새 멤버를 만지지 않는다. 람다가 새로 잡는 참조 없음.
  - 저장소 규칙: `Q_OBJECT` 없음, 새 색 · 크기 리터럴 없음(`theme::Hand` 토큰, 16 px 아이콘은 기존 값), `static_cast`/`qobject_cast` 해당 없음, core/tests/CMake 변경 없음(AC7), 명세 §2 표 밖 파일 없음, 재포맷 없음(바꾼 줄만).
  - `scaleCombo_` 는 `activated`(사용자 조작)만 연결(`mainwindow_ui.cpp:687`) → 맞춤 다시 하기로 축척 글이 바뀌어도 `setScreenDenom` → `fitted_=false` 로 되돌아가는 고리 없음.
  - `wheelZoom` 이 굴린 순간 `fitted_=false`(spec Review Focus 3), `SectionView::fit()` 은 `has_` 없으면 `fitted_` 를 켜지 않음, `PlanView::fitAll()` 은 상자가 무효면 켜지 않음. `qFuzzyCompare(iconDpr_=0, …)` 는 `iconName_` 가 빈 동안만 0 이라 문제 없음.
- 지적:
  - **should** `app/sectionview.cpp:467-483`(`setVerticalExaggeration`) × `:59`(`resizeEvent`) — 세로 과장을 바꿔도 `fitted_` 가 켜진 채인데, 이 함수는 `ppm` 을 그대로 두고 세로만 바꾸므로 지금 화면은 `fit()` 결과가 아니다. 그 뒤 **어떤 크기 변화든**(창 끌기뿐 아니라 판 접기 · 분할선 · 문서 탭 줄 표시로 가운데 칸 폭이 바뀌는 것) `fit()` 이 `pr.height()/(zr·vex·1.06)` 쪽으로 가로 축척을 다시 잡아 화면이 갑자기 작아진다. 재현: 합성 모델 · 표준 선 → 맞춤 상태에서 「세로 ×5」 → 단면 선이 가로를 채운 채 세로만 늘어남 → 창 폭을 1 px 바꿈 → `fit()` 의 높이 제약(`zr·vex`)이 이겨 가로 축척이 줄고 단면선이 가운데 좁은 띠로 바뀜(코드 읽기로 추론, 실행해 보지 않음 — 줄어드는 정도는 모델의 높이 범위 · 판 높이에 따름). spec 이 「맞춤 상태 유지」로 정했으나(§3-B Task 5) 결과가 사용자에게 「창을 건드렸더니 확대가 풀림」으로 보인다. 제안: `setVerticalExaggeration` 에서 `fitted_` 이면 바로 `fit()` 해 상태와 화면을 일치시키거나, `fitted_=false` 로 둔다(어느 쪽이든 판정을 build.md 에).
  - **should** `app/mainwindow_ui.cpp:1857` — 넘침 판정 `r.right() > width() || r.bottom() > height()`. `QRect::right()` = `left + width − 1` 이라 위젯이 창 밖으로 **1 px** 나간 경우(`right() == width()`)를 놓친다. 재현: 리본 오른쪽 끝 찾기 칸이 창 오른쪽 경계를 1 px 넘게 배치되면 `overflow=0` 으로 통과. 제안: `r.right() >= width() || r.bottom() >= height()`(또는 `!rect().contains(r)`).
  - **should** `app/planview.cpp:356-379`(`contentFits`) — `updateMatrices()`(:332)의 ortho · rotate · translate 식과 `screenToLocalXY`(:625)의 역투영 · 기준 높이 교차를 그대로 베꼈다(약 15줄). 나중에 카메라 식(예: 원근 · near/far)을 한쪽만 고치면 감사가 실제 화면과 다른 것을 재며 조용히 통과 · 실패한다. 제안: `static void makeMatrices(int w, int h, double mpp, …, QMatrix4x4& proj, QMatrix4x4& view)` 같은 도움 함수 하나를 `updateMatrices` 와 `contentFits` 가 같이 쓰게.
  - **note** `app/mainwindow_ui.cpp:2046` — `dpr-rerender` 는 배율이 그대로인 채 가짜 사건을 보내 cacheKey 가 바뀌는지만 본다: 「사건을 받으면 다시 굽는다」는 잠그지만, Qt 가 실제로 자식 위젯에 사건을 보내는지 · `place()` 안전판(`guideband.cpp:76`)이 도는지는 시험이 덮지 않는다(spec §5 「확인하지 못함」과 같은 구멍, AC3 수동 미실행).
  - **note** `app/mainwindow_ui.cpp:2137` · `:1137` — 저장된 카메라가 있으면(이미 연 적 있는 모델) 50 ms 뒤 `setCamera` 가 `fitted_=false` 로 만든다 → AC4(a) 자동 맞춤은 사실상 「처음 여는 모델」에만 적용된다(spec 이 정한 대로지만 사용자 설명에 적을 것). 같은 이유로 `--settings` 폴더를 다시 쓰면 `plan-fits` 가 맞춤 상태가 아닌 보기를 재서 거짓 FAIL 이 날 수 있다 — `plan-fits` 를 맞춤 상태일 때만 판정하면 더 단단하다(지금 CI · verify 는 새 폴더라 영향 없음).
  - **note** `app/planview.cpp:203-206`(`clearScene`) — `fitted_` · `bounds_` 를 지우지 않아 모델을 닫은 뒤 창 크기가 바뀌면 옛 상자로 `fitAll()` 이 돌고 `updateMatrices()` 는 건너뛴다. 그릴 장면이 없어 화면 결과는 없음(기록용).
  - **note** `.github/workflows/ci.yml:66-82` — 3840x2160 × `QT_SCALE_FACTOR=2` offscreen 은 백버퍼 7680×4320, 감사의 `grab()` 장마다 약 130 MB. 러너(7 GB)에서는 되지만 감사 1회 시간이 로컬 63–66 s 보다 길 수 있다 — 300 s 제한 안인지 push 뒤 첫 실행 시간을 볼 것.

## correctness-reviewer (좌표 · 단위 · 수직 기준 · 계산 · 입출력 정확성)
- 판정: **pass** (blocking 0 · should 2 · note 6). 범위 `git diff 0f7fbfe 66737c5`(13개 파일 전부 읽음). 근거: spec AC1–AC11 · build.md · verify.md 의 RED/GREEN 줄, 식은 손으로 따라 계산.
- 확인한 것(문제 없음):
  - **좌표**: 해당 변경은 화면 축척뿐. `PlanView::contentFits`(planview.cpp:356–377)는 float 행렬을 **로컬**(target_ · refZ = 장면 중심 기준)에서만 쓰고 실좌표는 `+ center_.x/y`(double) 로 더한 뒤 `bounds_`(실좌표)와 비교 — 큰 좌표를 float 로 계산하지 않음. 식은 `updateMatrices`(같은 ortho · 같은 회전 순서) · `viewRectLocal`(1070행, 같은 네 모서리 광선)과 동일. SectionFrame s · d · z, 수직 기준(srs/vdatum), 단면 자르기, DXF/GeoTIFF/LAS 입출력, `asec-section` 기준값 — **해당 없음**(core · tests · tools 변경 0, verify.md `polylines=1 vertices=54` 그대로).
  - **단면 맞춤 식**: `fit()` 은 `ppm = min(W/(1.03 L), H/(1.06 zr·vex))` → 가로로 보이는 길이 `W/ppm ≥ 1.03 L`, `s0 = L/2 − W/2/ppm ≤ −0.015 L` → `contentFits`(sectionview.cpp:51–56) 의 `s0 ≤ 1e-6` · `s0 + W/ppm ≥ L − 1e-6` 를 맞춤 직후 항상 만족(여유 1.5 % L ≫ 1e-6). 비맞춤 resize 는 `ppm` 을 건드리지 않음(가운데만 옮김) → `zoom-kept` 의 「같음(허용오차 없음)」 기대가 식으로 맞다.
  - **평면 맞춤 식**: `mpp = max(w/(Wpx−40), h/(Hpx−40))·1.04` → 보이는 폭 `Wpx·mpp ≥ w·Wpx/(Wpx−40)·1.04 > w`(pitch 90 · yaw 0 일 때). 비맞춤 resize 는 `updateMatrices()` 만(mpp_ 불변).
  - **dpr 반올림**: `lround(px·dpr)` = Qt `QSize×qreal` 의 `qRound`(양수에서 반올림 같음): 18×1.25=22.5→23, 18×1.75=31.5→32, 타일 50→63 · 88, 34→43 · 60, 32→40 · 56, 24→30 · 42, 20→25 · 35. build.md:41 실측(23×23 · 32×32, 픽셀 동일)과 일치.
  - **QString::arg 사슬**: env 줄(mainwindow_ui.cpp:1882–1886) 자리표 %1–%12 와 `.arg` 12개, guide2Why 의 %1–%9 와 `.arg` 9개 순서 일치(Qt 는 %1–%99 지원, 가장 낮은 번호부터 채움).
  - **0 · NaN · 빈 값 경계**: `QT_SCALE_FACTOR` 없음 → 기본 "1", 빈 값 · 숫자 아님 · 0 이하 → `toDouble()≤0` → 1.0(1871행). `qFuzzyCompare` 는 두 값 모두 > 0 일 때만 불림(dpr > 0, d ∈ {1.25,1.75}). `GuideBand::iconDpr_` 첫 값 0 → `qFuzzyCompare(0, dpr)`=false → 다시 굽기(의도대로). `contentFits` 둘 다 `ppm ≤ 0` · 무효 칸 · `width()/height() ≤ 0` 을 먼저 거름, 광선 `|dz|<1e-9` 가드. `fit()` 은 `ppm ≥ 1e-3`, `fitAll()` 은 `max(50, …)` 로 0 나눔 없음.
  - **판정 식 ↔ 명세**: offscreen 에서 env ①(가로 · 세로 모두 같음) · ②(≤1280) · ③(overflow 0) · ④(qFuzzyCompare(dpr, scale)) 는 spec §3-A 그대로. 실제 화면 완화(①은 화면에 드는 변만, ④ 생략)는 build.md 판정 2 에 근거 · 「잃는 것」이 적혀 있고 offscreen 기대값은 바뀌지 않음 — 「통과시키려고」 바꾼 기대값 아님. RED 값(1591x800 · icon-dpr-bad=2 · dpr-rerender=0 · plan/section-fits=0)이 spec 예상과 일치(build.md:35–39).
  - **허용오차**: `contentFits` 1e-6 m = profile §7 기존 값 · 같은 단위(m). 새 허용오차 없음.
- 지적:
  - **should** `app/mainwindow_ui.cpp:1879` — AC2 의 **리본 칩 절반은 판별력이 없다.** 칩은 `pixmap(QSize(tile,tile), d).devicePixelRatio()==d` 만 보는데, Qt 6.8+ `QIcon::pixmap` 은 그 배율 장이 없어도 다른 장을 늘려 **요청한 dpr 을 붙여** 돌려준다(context7 「requested devicePixelRatio might not match the returned one」 — 늘린 장도 d 가 붙음). 증거: RED(1.25 · 1.75 장이 칩에 없던 때)에서 `icon-dpr-bad=2` = 선 아이콘 비교 2회(d 둘 × 1)뿐 — 보이는 칩 수십 개 × 2 가 하나도 안 걸렸다(build.md:36–37). 즉 칩 아이콘에서 1.25/1.75 장이 다시 빠지는 회귀를 감사가 못 잡는다. 지금 구현은 읽어서 맞음(`icons.cpp` `chipIcon` · `ribbon.cpp:197` 이 `iconDprs()` 사용). 고치는 법: 칩도 선 아이콘처럼 **픽셀 비교**(예: `kerf::chipIcon(…)` 로 직접 구운 `d` 장과 `c->icon().pixmap(…, d).toImage()` 비교) 하거나, 돌려받은 장의 실제 픽셀 크기가 `qRound(tile·d)` 인지에 더해 이미지 동일을 본다. 고친 뒤 칩 쪽에서 1.25 를 빼 RED 를 한 번 보일 것.
  - **should** `app/mainwindow_ui.cpp:2137–2146` — **1280×800 실행 두 가지(×1 · ×2)에서는 자동 맞춤 · zoom-kept 가 실제로 시험되지 않는다.** 창이 이미 1280×800(`keepSize`)이라 `resize(1280,800)` 과 `resize(keepSize)` 가 모두 크기 변화 없음 → resizeEvent 없음 → `plan-fits=1 section-fits=1` 은 첫 맞춤만 확인, `zoom-kept=1` 은 무조건 참. 판정이 거짓 FAIL 을 내지는 않으나, verify.md 「8가지 모두 zoom-kept=1」은 실효 6가지다. 고치는 법: `keepSize` 가 1280 이하이면 잠깐 다른 크기(예: 1600×1000)로 바꾼 뒤 판정하거나, 줄에 `zoom-kept=-1`(판정 안 함)로 찍어 거짓 확신을 없앤다.
  - **note** `app/mainwindow_ui.cpp:1857` — 넘침 판정 `r.right() > width()` 는 `QRect::right() = left + w − 1` 이라 **1 px 넘침을 통과**시킨다(정확한 조건은 `r.right() >= width()`, `bottom` 도 같음). 1 px 라 화면 영향은 작음.
  - **note** `app/planview.cpp:370–377` · `:348` — `contentFits` 는 네 모서리 광선의 **축 정렬 상자**로 재므로 yaw ≠ 0 이거나 pitch < 90 이면 실제 보이는 사다리꼴 · 기운 사각형보다 넓게 잡아 「들어감」을 과대 판정할 수 있고, `fitAll()` 자체도 yaw · pitch 를 무시한다(`setScene` 이 돌린 보기에서 fitAll 을 부르면 fitted_=true 인데 실제로는 모서리가 잘릴 수 있음). 감사는 `homeView`(yaw 0 · pitch 90) 상태라 영향 없음. 또 `Wpx < 90` 처럼 아주 좁으면 `max(50, …)` 탓에 맞춤이어도 안 들어감(창 최소 폭 1280 이라 실제로는 안 생김).
  - **note** `app/planview.cpp:327` — `fitted_` 가 참인데 `bounds_` 가 나중에 무효가 되면 `fitAll()` 이 일찍 돌아가 `updateMatrices()` 가 불리지 않는다(GL 길은 `resizeGL` 이 덮음, offscreen 만 해당). 장면을 비우는 길에서 `fitted_=false` 로 두면 깔끔.
  - **note** `app/mainwindow_ui.cpp:1869` — 실제 화면 ① 판정은 `availableGeometry`(창 테두리 포함 영역)와 **클라이언트** 크기 `width()/height()` 를 비교한다. 요청 변이 사용 가능 영역 바로 아래(테두리 · 제목 줄 두께 이내)면 판정 대상인데 OS 가 줄여 거짓 FAIL 이 날 수 있다(AC10 실측은 1040 > 912 라 비대상). 오프스크린 판정과는 무관.
  - **note** `app/mainwindow_ui.cpp:2137` — `section_->hasResult()` 가 거짓이면 `section-fits=-1` 로 통과(판정 안 함). 표준 선 감사에서 단면 실패는 종료 코드 3 으로 따로 잡히므로 지금은 문제 없음 — 로그 읽는 사람은 -1 을 「통과」로 오해하지 않게.
  - **note** `app/icons.cpp:19` 소수 배율 — 18 px 아이콘 1.25 장은 23 px 이라 논리 18.4 px 로 그려진다(0.4 px 큼, 흐림 아님 — 장을 늘리지 않고 1:1 로 찍힘). 의도된 반올림이며 Qt 와 같은 규칙.
- 확인하지 못함: Qt 6.8.3(CI)에서 `QIcon::pixmap(size, 1.25)` 가 6.10.3 과 같은 장(23×23)을 고르는지 — push 전이라 CI 결과 없음. `QT_SCALE_FACTOR` 가 0 · 음수 · 숫자 아님일 때 Qt 자체가 1 로 두는지(감사는 1.0 으로 가정) — 소스로 확인 안 함.

## security-reviewer (외부 입력 · 경로 · 파일 쓰기 · 비밀정보)
- 판정: **pass** (blocking 0 · should 0 · note 4). 범위 `0f7fbfe..66737c5`(worktree HEAD = `66737c5` 확인), diff 13개 파일 전부 읽음.
- 강점:
  - `.github/workflows/ci.yml:10-11` `permissions: contents: read` 그대로. 비밀값(`secrets.*`) 사용 없음. `run:` 안에 `${{ … }}` 보간 없음(`github.sha` 는 `with: name:` 에만, :86) → PR 제목 · 브랜치 이름 같은 외부 문자열로 명령 주입 불가.
  - 감사 단계(:62-82): 경로는 러너 `$env:TEMP` + 고정 문자열, `$argLine` 안 경로마다 큰따옴표, `Invoke-Expression` 없음. 실행마다 `--settings` · `--ui-audit` · 로그 폴더를 크기 · 배율 태그로 분리, 300 s 넘으면 `Kill` 뒤 throw(멈춤이 CI 를 묶지 않음). 감사 결과 폴더는 `$env:TEMP` 라 올리기(`dist/Kerf-ci`)에 섞이지 않음.
  - `app/main.cpp:294` 새 줄은 이미 `W > 0 && H > 0` 를 통과한 값만 `setProperty` — 0 · 음수는 그대로 건너뜀.
  - `app/mainwindow_ui.cpp:1871` `QT_SCALE_FACTOR` 가 숫자가 아니거나 0 이하면 `toDouble()`=0 → 1.0 으로 대체, 판정에만 씀(값을 계산 · 할당 크기에 쓰지 않음).
  - 포터블(R7.5): diff 의 `+` 줄 전체에서 `C:[\\/]` · `HKEY_` · `NativeFormat` · `Users` · `권을` · token/password/secret · `QProcess` · `system(` → 0건(직접 Grep). 새 파일 쓰기 · 지우기 코드 없음(원본 3MX/3SM 무관).
  - `docs/design/DESIGN_SPEC.md` 변경 2줄은 배율 목록과 규칙 문장뿐 — 좌표 · 모델 이름 · 개인 경로 없음. CI 의 `200002 450006 …` 은 합성 모델(`asec-make-synthetic`) 좌표.
  - 스크립트 · 훅 · 설정: `.claude/` · `factory/` · `*.ps1` · `*.cmd` · `*.sh` · `.gitignore` 변경 0건. 모의 실행 스크립트 `b5-ci-sim.ps1` 은 `C:\dev\tmp`(build.md:44)에 있고 `git ls-tree -r 66737c5` 에 없음. 새 의존성 없음.
- 지적:
  - **note** `.github/workflows/ci.yml:22,24,84` — 액션이 커밋 SHA 가 아니라 주 판 태그(`actions/checkout@v4` · `jurplel/install-qt-action@v4` · `actions/upload-artifact@v4`)로 고정. 서드파티 태그가 옮겨지면 다른 코드가 돈다. 이번 diff 가 바꾼 줄은 아니고 권한이 `contents: read` · 비밀값 없음이라 피해는 작다. 고치는 법: 다음 CI 작업 때 서드파티(`jurplel/…`)부터 전체 SHA + 주석 판으로.
  - **note** `app/main.cpp:236,294` — `--size 100000x100000` 같은 거대 값은 그대로 `resize` → offscreen 백버퍼 · `grab()` 이 수십 GB 할당을 시도해 죽을 수 있음. 새 줄 이전부터 있던 길이고 사용자가 직접 주는 시험용 인자라 영향은 그 실행 하나. 고치는 법(원하면): 해석 때 `W,H` 를 예컨대 16384 이하로 자름.
  - **note** `app/mainwindow_ui.cpp:1852,1882` — 환경 변수 문자열 `scale` 을 `QString::arg` 사슬 중간에 넣음. 값에 `%10` 같은 표지가 있으면 뒤 `.arg` 가 그 자리를 채워 env 로그 줄이 흐트러질 수 있음(판정 `envOk` · 마지막 `RESULT` 줄에는 영향 없음 — CI 는 마지막 RESULT 줄만 봄). 환경을 바꿀 수 있는 사람은 이미 실행을 통제하므로 위협 아님. 고치는 법(원하면): `scale` 대신 `QString::number(scale.toDouble())` 를 넣거나 맨 마지막 `.arg` 로.
  - **note** `app/sectionview.cpp:59` · `app/planview.cpp`(`resizeEvent`) — 맞춤 상태면 창 크기 바뀔 때마다 `fit()`/`fitAll()`. 창이 아주 작아져 그림 칸 폭이 음수여도 `fit()` 은 `ppm ≥ 1e-3`, `fitAll()` 은 `max(50, …)` 로 받쳐 0 나누기 · NaN 없음(읽어서 확인). 메모리 · 멈춤 위험 없음 — 기록용.
- 판단을 보류한 것:
  - Linux CI 작업 삭제(ci.yml)가 코어 이식성 검출을 줄이는 것 — 보안이 아니라 플랫폼 정책(DEV_RULES R7.4) 문제라 범위 밖.
  - CI 감사 단계가 GitHub 러너(Qt 6.8.3)에서 실제로 통과하는지 — push 전이라 실행 결과 없음(build.md:69), 이 검토는 코드 읽기만.

## kerf-ui-reviewer (UI 변경이 있을 때만)
- 판정: **fail** (blocking 2 · should 3 · note 4)
- 본 증거: verify.md AC 표 · 23:11/23:14 블록 · AC10 재현 줄; `logs\20261009-231157\ui-audit-1920x1040-x1.25|x1.75\ui-audit.txt`(둘 다 `RESULT ok`, `icon-dpr-bad=0`, `narrow … look=20 labels=0 … plan-fits=1 section-fits=1 zoom-kept=1`); 캡처 `logs\20261009-230247\ui-audit-1280x800-x1\work.png` · `ui-audit-1280x800-x2\narrow.png` · `ui-audit-3840x2160-x1\work.png` · `logs\20261009-231157\ui-audit-1920x1040-x1.75\work.png` · 실제 화면 `C:\dev\tmp\b5-shots\work-1280.png` · `work-1920.png` · `draw-1280.png`. 1280 ×1.25 · ×1.75 와 3440 캡처는 열어 보지 않음(감사 텍스트만). 실제 화면 3440 · 3840 캡처는 화면(150 %) 에 잘려 판정 불가(verify AC9) — 확인하지 못함. offscreen 의 한글 □ · 빈 평면 칸은 알려진 한계라 지적하지 않음.
- 점수(1–5):

| 항목 | 점수 | 근거 |
| --- | --- | --- |
| 쌓임 | 4 | 1280–3840 모두 리본 · 문서 탭 · 판 넷 · 상태줄 순서 그대로, 넘침 없음(`overflow=0`). 1280 머리 줄 문제는 아래 |
| 리본 | 3 | 1920 이상 `look=50 labels=1 height=118` 스펙 일치, 3440 · 3840 왼쪽 채움 · 오른쪽 끝 묶음만 붙음(스펙 §2.1). 1280 은 `look=20`(스펙 32) |
| 문서 탭 | 5 | 홈 · 단면 · 구석 「Synthetic · 저장됨」 모든 크기 동일 |
| 안내 자리 | 4 | 칩 · 되돌리기 칸 안 겹침(`guide-step2 … apart=1`), 1280 에서 문장 줄임표 「Shift …」 |
| 판 | 4 | 판 폭 고정(list 348 · side 320), 가운데만 늘어남 |
| 상태줄 | 5 | 배지 둘 24 px, 1280 에서도 다 보임 |
| 아이콘 | 5 | `icon-dpr-bad=0`(1 · 1.25 · 1.75 · 2), ×1.75 · ×2 캡처 리본 · 머리 아이콘 테두리 번짐 없음, `dpr-rerender=1` |
| 토큰/색 | 5 | 새 색 없음, 흙색 주 단추 「도면」 하나, 단면선 빨강 그대로 |
| 한글 조판 | 3 | 1280 단면 머리 · 정보 줄 잘림(아래) |

- 지적:
  - **blocking** `b5-shots\work-1280.png` 단면 머리(대략 x 1040–1500, y 165–200 / 1280 논리 기준 x 690–1000, y 110–135) — 「A–A′ 북쪽을」 글 뒤로 토글 단추 4개가 겹쳐 「| ( | :」 처럼 찌그러진 조각만 보이고, 축척 칸 「1:218 ▾」 오른쪽 끝이 확대 아이콘을 덮는다. offscreen `1280x800-x1\work.png`(x 695–940, y 110–132) · `1280x800-x2\narrow.png`(x 1090–1470, y 175–205) 에서 같은 모양 재현. 스펙 §2.1 은 창 최소 폭 1280 을 지원 크기로 정했고(①) 위젯끼리 겹침은 허용하지 않는다. 이번 작업이 창을 1280 까지 줄일 수 있게 만들면서 처음 드러난 것이고 `--ui-audit` 은 잡지 못함(verify.md AC9 에도 기록). 고칠 방향: 좁을 때 단면 머리의 줄이는 순서를 정한다(예: 「북쪽을 봄」 글 → 토글 글/간격 → 축척 칸 폭) 그리고 감사에 「머리 위젯 사각형 서로 안 겹침」 항목을 더해 RED 를 먼저 만든다.
  - **blocking** 리본 1280: `ui-audit.txt` `narrow rule-at-1280 look=20 labels=0`, `need32=1682`; 캡처 `work-1280.png` 리본(y 40–90) 타일이 아이콘 크기 ≈ 20 논리 px. 스펙 §2.1 「창 폭 1280(글자 숨김 타일 32)」 · §12 「창 폭 1280 에서 `look=32 labels=0`」과 다르다. 감사는 `chooseLook` 규칙만 보고 통과시킨다. 고칠 방향(둘 중 하나, 사람 결정): ① 1280 에서 리본 폭을 1280 아래로 줄여(입면 묶음 위젯 320 · 오른쪽 끝 270 이 가장 큼) 타일 32 를 맞추고 감사 기대값을 스펙 숫자로 잠근다 ② 스펙 §2.1 · §12 의 1280 기대값을 20 으로 고치는 디자인 결정을 사용자에게 받는다 — 이 검토자는 ②를 직접 고르지 않는다.
  - **should** `b5-shots\work-1280.png` 정보 줄(y 215–240) — 「잘린 선 1줄 ·」 · 「입면 뒤 0–3.(」 · 「레벨선 10 cn」 처럼 글자가 반쯤 잘림; `draw-1280.png` 「단면선을 긋는 중 — 단」 도 같음. 1920 에서는 「잘린 선 1줄 · 54점 · 빈 구간 없음」 전체가 보임. 고칠 방향: 가로 Ignored 로 둔 대신 칸 단위로 숨기거나 끝에 줄임표(elide)를 넣어 글자 반 토막이 남지 않게.
  - **should** `b5-shots\work-1280.png` 평면 단면선 눈금 글(x 625–920, y 703) — 「1 m2 m3 m4 …」 숫자와 단위가 서로 붙어 읽기 어려움(1920 캡처는 「1 m 2 m 3 m」 정상). 고칠 방향: 간격이 좁으면 눈금 글을 건너뛰기(2 · 4 m 마다) 또는 단위를 끝 하나만.
  - **should** 1280 · 3440 의 ×1.25 · ×1.75 감사를 돌리지 않음(이번 AC6 은 1920 만). 1280 ×1.75 는 위 겹침이 가장 심해질 조합 — 확인하지 못함. 다음 검증에 `-UiSizes 1280x800 -UiScales 1.25,1.75` 추가 권장.
  - **note** `draw-1280.png` 안내 칩(y 235–270) 문장이 「Shift …」 로 줄임표 — 스펙이 정한 줄이기 순서 안의 동작이라 위반 아님.
  - **note** `3840x2160-x1\work.png` — 배율 1 의 4K 는 글자 · 타일이 물리적으로 아주 작지만 이는 OS 배율 100 % 의 정상 모양(논리 픽셀 그대로). 레이아웃은 넘침 · 겹침 없음.
  - **note** `ui-audit … same-name-different-action=not-checked` 는 기존(D3) 미완 항목 그대로.
  - **note** 둥글기: 감사 `square-borders=0`, 캡처의 칩 · 축척 칸 · 배지 · 좌표 입력 단추 모서리 둥긂. 이번 diff 가 새 사각 테두리를 만든 흔적 없음.

### kerf-ui-reviewer 2차(독립 · 동시 검토분, 브리프는 `review-ui.md` 요청 — 파일 만들기 도구가 없어 이 절 안에 덧붙임. 위 1차는 고치지 않음)
- 판정: **pass**(blocking 0 · should 4 · note 5). 기준은 브리프의 분류다: blocking = 「4점 미만이고, 원인이 이번 변경」. **위 1차(fail, blocking 2)와 결론이 다르다 → 검토 충돌, needs-human.** **spec AC9(루브릭 모두 4 이상)는 1280 에서 충족하지 못함** — 받아들이고 후속 작업으로 넘길지, rework 할지는 사람이 정한다.
- **1차의 blocking 둘을 「전부터 있던 것」으로 본 근거**
  - ① 단면 머리 · 정보 줄: worktree `app/mainwindow_ui.cpp:425–494`(`buildSectionFrame`)가 기준 `0f7fbfe`(본 체크아웃, app 미커밋 변경 없음) `:424–493` 과 한 줄 밀린 것 말고는 같다. 판 폭 348 · 272 도 그대로다. 그리고 **RED 실행(b5640c0 — 감사 줄만 더한 상태)에서 이미 `min-width=1280`** 이었다(build.md:36). 즉 기준 커밋에서도 사용자가 창을 1280 까지 끌어 줄일 수 있었다. 이번 변경이 바꾼 것은 「`--size 1280x800` 이 1591 이 아니라 1280 으로 열린다」 하나다. 1차의 「이번 작업이 창을 1280 까지 줄일 수 있게 만들면서 처음 드러남」은 이 로그와 맞지 않는다.
  - ② 리본 1280 `look=20`: 리본 코드의 변경은 `ribbon.cpp:197` 배율 목록뿐이다. RED 줄도 `narrow rule-at-1280 look=20`. 다만 이번 작업이 DESIGN_SPEC §2.1 을 고치면서 43행 「1280(글자 숨김 타일 32)」을 그대로 둬, 스펙과 실제의 어긋남이 이 작업 안에 남았다 → should.
- 캡처 조건: 이 PC 화면 배율 150 % → 그림 픽셀 = 논리 × 1.5. `-x1.25` 는 실제 dpr 1.875(논리 1837×752). `-x2` 는 실제 dpr 3 이라 창(5760 px)이 화면(3440 px)보다 커서 잘렸다(오른쪽 · 아래 흰 칸). 그래서 배율 2 의 배치는 offscreen `logs\20261009-230247\ui-audit-1920x1040-x2\work.png` 로 판정했다.

| 캡처(전역 루브릭 8항목: 1 위계 · 2 정렬·간격 · 3 글자 · 4 색·대비 · 5 상태 · 6 접근성 · 7 체계 일치 · 8 마감) | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 4 미만 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| work-1280 | 4 | **3** | **2** | 4 | 4 | **3** | **3** | **3** | 2 · 3 · 6 · 7 · 8(모두 전부터 있던 것) |
| draw-1280 | 4 | **3** | **2** | 4 | 4 | **3** | **3** | **3** | 같음 |
| start-1280 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음 |
| work-1920(논리 1920×940) | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음 |
| work-1920-x1.25 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음 |
| work-1920-x2(offscreen work.png) | 4 | 4 | 보류(□) | 4 | 4 | 4 | 4 | 4 | 없음 |
| work-3440 · 3840(논리 2296×940 으로 잘림) | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 4 | 없음 |

- 브리프 질문 답
  - ② 홈 한 줄 라벨에 가로 Ignored 를 준 뒤: `start-1280.png` 에서 잘림 없음. 끌어 놓기 안내는 x≈652 px 에서 끝나고, 키 줄 「되돌리기 200단계」와 「모든 키 F1」 사이에 여유가 있다.
  - ③ 배율: 1.875 · 3(실제 화면) · 2(offscreen)에서 리본 타일 · 칩 · 머리 · 배지 아이콘 모두 또렷하다. 글자 · 타일 · 판 폭 비율도 ×1 과 같다.
  - ④ 창 줄이기: `ui-audit-3840x2160-x2\narrow.png`(1280)에서 단면 s 0–12 가 그림 칸 안에 다 들어간다. `work-1280.png` 는 평면 장면 전체 · 단면 0–12 m 가 보인다 → AC4 잘림 없음.
- **should**
  - S1(전부터 있던 것 · AC9 미충족의 주된 원인) `work-1280.png`: 머리 토글 넷이 「북쪽을」 뒤에서 짓눌린다(px x≈1160–1215, y≈165–200). 축척 칸(x≈1275–1395)이 확대 아이콘을 덮는다. 정보 줄 「입면 뒤 0–3.(」 「레벨선 10 cn」 이 잘린다(y≈215–240). draw 에서는 「단면선을 긋는 중 — 단」. 원인: 가로 Ignored 부모(`secTitleBar_` · `infoStrip`)가 자식 최소 폭 합보다 좁아진다. 고칠 방향: 머리 크기 바뀜 때 폭이 모자라면 ① `secFacing_` 숨김 ② `vexBtn_` 아이콘만 ③ 축척 칸 좁힘 ④ 「크게」 숨김. 정보 줄은 `stripState_` 를 elide 하고 범례 셋을 숨긴다. 감사에 `sec-header-overlap=0` · `strip-clipped=0` 을 더해 1280 에서 RED → GREEN. 후속 작업 1건으로.
  - S2(전부터 있던 코드, 이번 변경 「맞춤 상태면 창 바뀔 때 다시 맞춤」으로 더 자주 보임) `work-1280.png` px x≈625–920, y≈703: 평면 눈금 숫자 「1 m2 m3 m…」 이 겹친다. `app/planview.cpp:773–781` 은 px/m 와 상관없이 1 m 마다 36 px 칸에 숫자를 쓴다. 고칠 방향: 숫자 간격을 {1, 2, 5, 10} m 중 `간격 × px/m ≥ 글 폭 + 8` 을 만족하는 가장 작은 값으로(눈금 선은 그대로).
  - S3 리본 1280 `look=20`(아이콘 12) vs `DESIGN_SPEC.md` §2.1 43행 · §12 「1280 → 32」. need32 는 1682(offscreen) · 1614(실제). 사람이 정한다 — 스펙 문구를 규칙(1280 → 20)으로 바꾸거나, 입면 320 · 오른쪽 끝 270 을 줄여 need32 ≤ 1280 으로 만든다.
  - S4(이번 변경) Ignored 를 준 홈 라벨(`mainwindow_ui.cpp:1194 · 1213 · 1318 · 1355`)은 폭이 모자라면 줄임표 없이 글자 중간에서 잘린다. 빈 홈 1280 은 문제가 없다. 그러나 최근 모델이 있는 홈의 카드 보조 줄 `ccSub` 는 캡처가 없다. 고칠 방향: 크기 바뀔 때 `elidedText` + 툴팁에 전체 글. 「최근 모델 있는 start 1280」 캡처를 더한다.
- **note**
  - N1 `work-1920-x2.png` 는 환경 탓 캡처다(위 캡처 조건). 100 % 모니터에서 다시 찍거나 shot.ps1 안내에 적어 두기를 권한다.
  - N2 맞춤 뒤 나침반이 평면 장면 오른쪽 아래 모서리에 걸친다(`work-1920-x1.25.png` 표시 ≈(1000,700), `work-3840.png` ≈(1000,720)). `fitAll` 여유에 나침반 자리를 넣는 것을 고려할 만하다.
  - N3 빈 홈의 이어서 작업 카드 오른쪽 절반이 빈 칸이다(`start-1280.png` px x≈1428–1846, y≈197–475). 전부터 있던 모양이다.
  - N4 1280 안내 칩 「… Shift …」 줄임표는 스펙 §5 줄이기 순서 ③대로다. 칩 툴팁에 전체 문장을 넣기를 권한다.
  - N5 색 · 토큰: 흙색 주 단추는 화면마다 하나, 단면선 빨강, 배지 둘, 문서 탭 모두 스펙대로다. 새 색은 없다.
- 판단 보류: 진짜 125 · 175 % 모니터 눈 확인(이 PC 는 150 % 하나) · AC3 화면 옮김(모니터 하나) · 3440 · 3840 실제 크기(화면에 잘림) · 배율 2 한글 조판(offscreen □) · 최근 모델이 있는 홈(캡처 없음).

## 종합
- ai_review: **pass** (blocking 0 — UI 1차의 blocking 2 는 2차 검토 · 사용자 결정으로 「전부터 있던 것 → 후속 작업」으로 재분류)
- 검토 충돌(검토자끼리 다른 결론): **있음 → needs-human → 사용자 결정으로 해소(2026-10-09 23:45, AskUserQuestion):** ① 1280 좁은 창 문제(단면 머리 겹침 · 정보 줄 잘림 · 평면 눈금 겹침)는 「후속 작업으로 빼고 B5 는 승인 보고서로」 ② 리본 1280 타일은 「스펙을 실제(1280 → 20)로 고친다」. 근거: UI 2차가 보인 대로 RED 실행(b5640c0, 감사 줄만 더한 상태)에서 이미 `min-width=1280` 이었고 단면 머리 코드(`buildSectionFrame`)는 이번 diff 에 없다 — 이번 변경이 만든 결함이 아님. 다만 spec AC9(1280 루브릭 4점 이상)는 미충족 → 후속 작업 backlog #3c.
- 남은 위험(고치지 않고 받아들인 것과 이유) — 모두 후속 작업 **backlog #3c**(1280 좁은 창 · 감사 보강) 로 넘김, 병합을 막지 않음:
  - [UI blocking→후속] 1280 단면 머리 「북쪽을 봄」·토글·축척 칸 겹침, 정보 줄 글 잘림, 평면 눈금 「1 m2 m3 m」 겹침(전부터 있던 것, 이번 변경으로 1280 이 「열리는 크기」가 되어 드러남). 감사 항목 `sec-header-overlap` · `strip-clipped` 추가 필요.
  - [UI blocking→스펙 수정] 리본 1280 `look=20` vs 스펙 §2.1 · §12 「32」 — 사용자 결정: 스펙 숫자를 20 으로(#3c 에서 문구 + 감사 기대값).
  - [code should 1] `setVerticalExaggeration` 뒤 `fitted_` 가 켜진 채라 크기 바뀌면 `fit()` 이 가로 축척을 다시 잡아 「확대가 풀린」 듯 보일 수 있음(코드 추론, 미재현).
  - [code should 2 · correctness note] 넘침 판정 `r.right() > width()` 는 1 px 넘침을 놓침(`>=` 로).
  - [code should 3] `PlanView::contentFits` 가 `updateMatrices` · `screenToLocalXY` 식을 베낌 — 도움 함수로 합치기.
  - [correctness should 1] AC2 리본 칩 판정은 dpr 값만 보아 판별력 없음(픽셀 비교로).
  - [correctness should 2] 1280×800 실행에서는 `zoom-kept` · fits 가 실효 시험이 아님(크기 변화 없음) → 잠깐 다른 크기로 바꾸거나 `-1`.
  - [UI should] 1280 · 3440 의 ×1.25 · ×1.75 감사 미실행 · 홈 라벨 `Ignored` 줄임표 없음(최근 모델 있는 홈 1280 캡처 없음).
  - [security note] 액션 `@v4` 태그 고정 · 거대 `--size` 값 · 로그 `%` 표지 — 전부터 있던 것, 위협 낮음.
  - [미실행] AC3 수동(모니터 둘 배율 다름 — 이 PC 한 화면) · AC8 ③ GitHub Actions(push 전) · AC10 사람 절차(창 끌기 · 휠 뒤 넓히기).
  - [기록] builder 가 Task 2 커밋 메시지(BOM) 교정으로 자기 브랜치 역사를 재적재(push 전 · factory 브랜치만, diff 동일) — 사람 검토가 알고 있을 것.
