# 검증 기록 — 20261009-01-b5-dpi 디자인 v5 B5 해상도 · DPI 안정성(4K · 와이드 · 배율, Windows만)

`factory verify 20261009-01-b5-dpi`(실행기 `factory.cmd`, 어느 폴더에서든) 가 실행마다 아래에 블록을 덧붙인다(명령 · 종료 코드 · 요약 · 로그 파일 이름).
큰 로그는 `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\<시각>\` 에만 있다(커밋하지 않음). 승인 근거는 이 파일의 요약 줄이므로, 중요한 출력(통과 수 · 기준값 줄 · RESULT 줄)은 요약에 들어 있어야 한다.

## 완료 기준별 결과 (test-runner 가 마지막 검증 뒤 채운다)
| # | 완료 기준 | 검증 방법 | 결과 | 근거(아래 블록 · 로그) |
| --- | --- | --- | --- | --- |
| AC1 | env 줄 8가지(요청 크기 · 최소 폭 · 넘침 · dpr) | V8 | **통과** | V8(23:02, HEAD `66737c5`, `logs\20261009-230247`) `RESULT PASS` exit 0. 8가지 모두 `ui-audit env size=WxH want=WxH scale=s dpr=s.00 min-width=1280(1280x800) / 1280 overflow=0 icon-dpr-bad=0`(FAIL 없음, `screen-avail` 꼬리 없음 = offscreen 은 spec 그대로 판정). 1280x800 ×1 `size=1280x800 want=1280x800 scale=1 dpr=1.00 min-width=1280`, 3840x2160 ×2 `scale=2 dpr=2.00`. RED 는 builder 가 `logs\20261009-215739` 에서 `size=1591x800 … FAIL` 로 확인(verify.md 22:00 블록, FAIL exit 9) |
| AC2 | 아이콘 dpr 1.25 · 1.75 | V8 env 줄 `icon-dpr-bad` + AC6 | **통과** | 위 8가지 + AC6 두 가지 모두 `icon-dpr-bad=0`. RED(`icon-dpr-bad=2 FAIL`)는 22:00 블록 |
| AC3 | DevicePixelRatioChange 다시 굽기 | V8 guide-step2 줄 | **통과(자동) · 수동 미실행** | 8가지 모두 `ui-audit guide-step2=1 inside=1 apart=1 text=1 title=1 dpr-rerender=1`. **수동(모니터 둘, 배율 다름, 창 옮겨 선명도 눈 확인): 미실행(환경 부족 — 이 PC 한 화면 150 %; 사용자 확인 예정)**. Qt 가 자식 위젯에 사건을 보내는지는 여전히 확인하지 못함(`place()` 안전판이 덮음) |
| AC4 | 창 줄일 때 자동 맞춤 · 사용자 보기 유지 | V8 narrow 줄 | **통과(offscreen)** | 8가지 모두 `narrow rule-at-1280 look=20 labels=0 window-min-width=1280 actual-width=1280 actual-look=20 actual-labels=0 plan-fits=1 section-fits=1 zoom-kept=1`(FAIL 없음). RED(`plan-fits=0 section-fits=0 … FAIL`)는 22:00 블록. GL 이 뜨는 길은 AC10 |
| AC5 | 고정 완료 기준(V8) | `factory.cmd verify … -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2` | **통과** | exit 0 `RESULT PASS`(23:11 블록): build exit 0 새 경고 0 · tests(quick) 51325 통과(CoalescingWorker 제외, 이 실행에서 실패 없음) · asec-section `polylines=1 vertices=54` · ui-audit 8가지 모두 exit 0 `ui-audit RESULT ok`(63–66 s). 로그 `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\` |
| AC6 | 125 · 175 % 전 과정 | `factory.cmd verify … -UiAudit on -UiSizes 1920x1040 -UiScales 1.25,1.75` | **통과** | exit 0 `RESULT PASS`(23:14 블록, `logs\20261009-231157`). `env size=1920x1040 want=1920x1040 scale=1.25 dpr=1.25 … icon-dpr-bad=0` · `scale=1.75 dpr=1.75 … icon-dpr-bad=0` · narrow `plan-fits=1 section-fits=1 zoom-kept=1` · 둘 다 `ui-audit RESULT ok` |
| AC7 | 회귀 없음 | spec §4 명령 3–5 + diff --stat | **통과** | `git -C C:\dev\kerf-wt\20261009-01-b5-dpi diff --stat 0f7fbfe -- core tests tools CMakeLists.txt app/CMakeLists.txt` → 출력 없음. worktree exe(offscreen, `--settings` 새 폴더, `Start-Process`+`WaitForExit(240000)`, `C:\dev\tmp\b5v\231426`): `--icon-sheet` EXIT=0 `icon-sheet n=62 missing=0 … RESULT ok` · `--sheet-check` EXIT=0 `level-pt minor=0.564 expect=0.564 major=1.411 expect=1.411 ok=1` `sheet-check RESULT ok` · `--ctx-shot` EXIT=0 `ctx-shot ok: …draw.png guide=1`. 기존 감사 항목 전부는 V8 `RESULT ok` 에 포함 |
| AC8 | CI | diff 읽기 + 로컬 모의 실행 | **부분 통과 · ①② 사람 검토 필요 · ③ 미실행(push 전)** | diff 확인: `linux-core` 작업 · 머리 주석 「- linux」 삭제, 남은 작업 `windows-app` 하나; 감사 단계가 `Start-Process -PassThru`+`WaitForExit(300000)`(넘으면 Kill+throw)·종료 코드 ≠ 0 또는 마지막 RESULT 줄 ≠ `ui-audit RESULT ok` 면 throw · 케이스마다 `--settings`/`--ui-audit`/log 폴더 따로 · 1920x1040 ×1 · 3840x2160 ×2. 모의 실행 `C:\dev\tmp\b5-ci-sim.ps1`(ci.yml 단계 본문과 같음, 경로만 다름): 양성 `ui-audit 1920x1040-x1 … RESULT ok` · `ui-audit 3840x2160-x2 exit=0 ui-audit RESULT ok` · `CI-SIM OK` exit 0; 음성(`-Negative`, 없는 모델) `ui-audit 1920x1040-x1 failed (exit=2, (ui-audit.txt missing))` throw, exit 1. 러너 환경(Qt 6.8.3 · MSVC · 글꼴)은 로컬과 달라 **GitHub Actions 결과는 미실행(push 전)** — 기준 커밋 `0f7fbfe` 의 Actions 는 이미 Windows=failure(원인 미조사) |
| AC9 | 캡처 루브릭(kerf-ui-reviewer) | shot.ps1 크기별 캡처 | **미실행(kerf-ui-reviewer 몫)** | 점수는 내가 매기지 않음. builder 캡처 `C:\dev\tmp\b5-shots\` 8장 존재. 내가 눈으로 본 것 둘: `work-1280.png` — 단면 머리 「A–A′ 북쪽을」 이 토글 단추와 겹치고 정보 줄 「입면 뒤 0–3.(」 · 「레벨선 10 cn」 이 잘림(**builder 가 build.md 「남은 문제」에 적은 것, 감사가 못 잡음 — 이 작업 전에도 있던 모양인지는 기준 커밋 캡처로 확인 못 함**); `start-1280.png`(빈 홈) 키 줄 · 라벨 잘림 없음. 그 PC 화면이 150 % 라 3440 · 3840 요청 캡처는 화면에 잘림(환경 부족) |
| AC10 | 실제 화면(GL) 감사 | CLAUDE.md 명령에서 offscreen 제거 | **미실행(내가 돌리지 않음; builder 기록만 있음)** | 이번 지시 범위 밖이라 직접 재현하지 않았다. builder 기록(build.md): EXIT=0 · `RESULT ok` · `plan-fits=1 section-fits=1 zoom-kept=1` · `first-draw-ms=117`. 사람 절차(창 모서리 끌기 · 휠 뒤 넓히기)도 미실행 → 사용자 확인 예정 |
| AC11 | 문서 · 포터블 제약 | diff 읽기 + Grep | **통과** | `git diff 0f7fbfe -- app` 과 `-- .github` 의 `+` 줄에서 `C:[\\/]` · `HKEY_` · `NativeFormat` → 0건(grep exit 1). DESIGN_SPEC.md · icons.cpp 는 diffstat 에 포함(여섯 장 문구 내용 자체는 내가 대조하지 않음 — 사람 검토) |

## 기존 실패 vs 이번 변경의 실패
- 기존(기준 커밋에서도 실패): `CoalescingWorker: 끄는 동안의 미리보기는…` (Windows 타이머 간격, 가끔) — 이번 검증은 `~CoalescingWorker*` 로 뺀 빠른 시험이라 해당 시험을 돌리지 않음 · `[real]` 은 `ASEC_REAL_3MX` 없으면 건너뜀(정상). 빌드 경고는 기존 `fopen` · `mirrored` 외 새 경고 0. 기준 커밋 `0f7fbfe` 의 GitHub Actions Windows 작업 failure(원인 미확인, 이 브랜치와 무관 여부는 push 뒤 확인).
- 이번 변경으로 생긴 실패: **없음**(이번 실행 V8 · AC6 · 회귀 3 · CI 모의 모두 통과). 이번 변경이 못 막은 기존 문제 1건: 1280 창 단면 머리 · 정보 줄 겹침 · 잘림(AC9 행, build.md 남은 문제).

## 판정 의견(test-runner, build.md 「명세와 다르게 한 것」 4개)
1. **Task 2 수정 위치(홈 한 줄 글에 가로 Ignored)** — 시험을 느슨하게 한 것이 아님. env 줄 판정 ①②는 spec 그대로이고, 원인(홈 최소 폭 1591)을 고쳐서 `size=1280x800 min-width=1280` 이 나옴. 단 `Ignored` 는 홈의 한 줄 글이 좁은 창에서 잘릴 수 있게 하는 대가가 있다 — 1280 빈 홈 캡처는 정상이지만 최근 모델이 있는 홈(카드 보조 줄)은 내가 보지 못함.
2. **env 줄 ① · ④ 를 실제 화면에서만 제한** — 통과시키려고 느슨하게 한 것으로 보지 않음: 근거는 build.md 실측(화면 가용 높이 940 < 요청 1040, dpr = OS 배율 × 변수). offscreen 에서는 spec 그대로이고 이 실행의 8가지 로그에서 `screen-avail` 꼬리가 없으며 `size=want`, `dpr=scale`(1.00/2.00) 로 판정됨을 확인. 약점: 실제 화면 감사는 크기 · QT_SCALE_FACTOR 먹힘을 못 잡으므로 그 둘은 offscreen(V8 · CI)에만 의존 — build.md 에 이미 적혀 있음.
3. **env 줄 선택 꼬리 ` screen-avail=…`** — offscreen 출력은 변함 없음(이 실행 확인). 문제 없음.
4. **Task 2 커밋 메시지 교정으로 브랜치 역사 재적재** — 내용에 영향 없음(HEAD `66737c5` 에서 worktree `git status --porcelain` 빈 상태, 변경 파일 목록은 spec 범위 안). 이 분기는 push 전 자기 브랜치라 허용 범위로 보나, 역사 재작성이므로 사람 검토가 알고 있어야 함.

## 검증 2026-10-09 22:00 · **FAIL** · HEAD `0f7fbfed7455c0c6db308af386ae935b27d6f77e` (+미커밋 변경 있음)
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 8개: app/guideband.cpp, app/guideband.hpp, app/main.cpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/sectionview.cpp, app/sectionview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-215739`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | PASS | 0 | 5 | CMake 구성 완료 | `cmake -S C:\dev\kerf-wt\20261009-01-b5-dpi -B C:\dev\kerf-wt\20261009-01-b5-dpi-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl "-DCMAKE_RC_FLAGS=/C 65001" -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/msvc2022_64` · `configure.log` |
| build | PASS | 0 | 15 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-215739\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | FAIL | 9 | 64 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-215739\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-215739\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | FAIL | 9 | 64 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-215739\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-215739\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 0f7fbfe none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 22:12 · **FAIL** · HEAD `b5640c017e20b23c648d452974239e095294d42e` (+미커밋 변경 있음)
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 8개: app/guideband.cpp, app/guideband.hpp, app/main.cpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/sectionview.cpp, app/sectionview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221038`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 3 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=17ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221038\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | FAIL | 9 | 63 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221038\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221038\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | FAIL | 9 | 64 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221038\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221038\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 0f7fbfe none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 22:14 · **FAIL** · HEAD `d2cb823d7fd57481433354da0cba0ee78c7b05d0` (+미커밋 변경 있음)
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 12개: app/guideband.cpp, app/guideband.hpp, app/icons.cpp, app/icons.hpp, app/main.cpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/sectionview.cpp, app/sectionview.hpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221327`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 6 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221327\sec` · `asec-section.log` |
| ui-audit 1920x1040 ×1 | FAIL | 9 | 64 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221327\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-221327\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 0f7fbfe none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 22:33 · **FAIL** · HEAD `c9b081e50751d8df116bd6334317b203f2f7a8ae` (+미커밋 변경 있음)
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 12개: app/guideband.cpp, app/guideband.hpp, app/icons.cpp, app/icons.hpp, app/main.cpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/sectionview.cpp, app/sectionview.hpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223234`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 6 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223234\sec` · `asec-section.log` |
| ui-audit 1920x1040 ×1 | FAIL | 9 | 64 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223234\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223234\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 0f7fbfe none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 22:36 · **PASS** · HEAD `504e6c103d1c63df490eaf980c1f2d42741bf498` (+미커밋 변경 있음)
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 12개: app/guideband.cpp, app/guideband.hpp, app/icons.cpp, app/icons.hpp, app/main.cpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/sectionview.cpp, app/sectionview.hpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223412`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 5 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223412\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 63 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223412\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223412\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223412\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223412\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 4 |  | CI 0f7fbfe in_progress (진행 중) https://github.com/kesekikwon-netizen/desca/actions/runs/37937880747 | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 22:48 · **PASS** · HEAD `066455a25bc8a2172a7732a9f75940d63600614e`
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 13개: .github/workflows/ci.yml,  …app/guideband.cpp,  …app/guideband.hpp,  …app/icons.cpp,  …app/icons.hpp,  …app/main.cpp,  …app/mainwindow_ui.cpp,  …app/planview.cpp,  …app/planview.hpp,  …app/ribbon.cpp,  …app/sectionview.cpp,  …app/sectionview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 63 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 65 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-223945\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 1 |  | CI 0f7fbfe completed failure jobs: Windows · 앱 전체=failure, Linux · 코어와 시험=success https://github.com/kesekikwon-netizen/desca/actions/runs/37937880747 | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 22:58 · **PASS** · HEAD `66737c515c4deac0b4ba08fbc4ff75af9f0252eb`
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 13개: .github/workflows/ci.yml,  …app/guideband.cpp,  …app/guideband.hpp,  …app/icons.cpp,  …app/icons.hpp,  …app/main.cpp,  …app/mainwindow_ui.cpp,  …app/planview.cpp,  …app/planview.hpp,  …app/ribbon.cpp,  …app/sectionview.cpp,  …app/sectionview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 63 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 65 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-224927\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 1 |  | CI 0f7fbfe completed failure jobs: Windows · 앱 전체=failure, Linux · 코어와 시험=success https://github.com/kesekikwon-netizen/desca/actions/runs/37937880747 | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 23:00 · **PASS** · HEAD `66737c515c4deac0b4ba08fbc4ff75af9f0252eb`
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 13개: .github/workflows/ci.yml,  …app/guideband.cpp,  …app/guideband.hpp,  …app/icons.cpp,  …app/icons.hpp,  …app/main.cpp,  …app/mainwindow_ui.cpp,  …app/planview.cpp,  …app/planview.hpp,  …app/ribbon.cpp,  …app/sectionview.cpp,  …app/sectionview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1920x1040 · 배율 1.25,1.75)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-225840`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=13ms cut=1ms image=16ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-225840\sec` · `asec-section.log` |
| ui-audit 1920x1040 ×1.25 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-225840\settings-1920x1040-x1.25 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-225840\ui-audit-1920x1040-x1.25 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1.75 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-225840\settings-1920x1040-x1.75 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-225840\ui-audit-1920x1040-x1.75 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 1 |  | CI 0f7fbfe completed failure jobs: Windows · 앱 전체=failure, Linux · 코어와 시험=success https://github.com/kesekikwon-netizen/desca/actions/runs/37937880747 | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 23:11 · **PASS** · HEAD `66737c515c4deac0b4ba08fbc4ff75af9f0252eb`
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 13개: .github/workflows/ci.yml,  …app/guideband.cpp,  …app/guideband.hpp,  …app/icons.cpp,  …app/icons.hpp,  …app/main.cpp,  …app/mainwindow_ui.cpp,  …app/planview.cpp,  …app/planview.hpp,  …app/ribbon.cpp,  …app/sectionview.cpp,  …app/sectionview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 63 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 65 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-230247\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 1 |  | CI 0f7fbfe completed failure jobs: Windows · 앱 전체=failure, Linux · 코어와 시험=success https://github.com/kesekikwon-netizen/desca/actions/runs/37937880747 | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

## 검증 2026-10-09 23:14 · **PASS** · HEAD `66737c515c4deac0b4ba08fbc4ff75af9f0252eb`
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 13개: .github/workflows/ci.yml,  …app/guideband.cpp,  …app/guideband.hpp,  …app/icons.cpp,  …app/icons.hpp,  …app/main.cpp,  …app/mainwindow_ui.cpp,  …app/planview.cpp,  …app/planview.hpp,  …app/ribbon.cpp,  …app/sectionview.cpp,  …app/sectionview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1920x1040 · 배율 1.25,1.75)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-231157`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-231157\sec` · `asec-section.log` |
| ui-audit 1920x1040 ×1.25 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-231157\settings-1920x1040-x1.25 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-231157\ui-audit-1920x1040-x1.25 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1.75 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-231157\settings-1920x1040-x1.75 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-231157\ui-audit-1920x1040-x1.75 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 1 |  | CI 0f7fbfe completed failure jobs: Windows · 앱 전체=failure, Linux · 코어와 시험=success https://github.com/kesekikwon-netizen/desca/actions/runs/37937880747 | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |

### AC10 재현(팩토리, 2026-10-09 23:23 · 실제 화면 · offscreen 아님)
- 명령: `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\tmp\b5-ac10\set --ui-audit C:\dev\tmp\b5-ac10\ui --quit`(PATH 앞 Qt bin, QT_QPA_PLATFORM 없음) → `EXIT=0`
- `ui-audit env size=1920x940 want=1920x1040 scale=1 dpr=1.50 min-width=1275 overflow=0 icon-dpr-bad=0 screen-avail=2293x912` · `ui-audit narrow rule-at-1280 look=20 labels=0 window-min-width=1275 actual-width=1280 actual-look=20 actual-labels=0 plan-fits=1 section-fits=1 zoom-kept=1` · `ui-audit RESULT ok`
- 사람 절차(창 모서리 끌어 줄이기 · 휠 확대 뒤 넓히기)는 사용자 확인 예정(수동).

## 검증 2026-10-09 23:54 · **PASS** · HEAD `b378b3f38811375d08fc6fa19f7f75992c7e6184`
- 기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` · 바뀐 파일 27개: .claude/cpp-project.json,  ….claude/skills/verify/SKILL.md,  ….github/workflows/ci.yml,  …app/guideband.cpp,  …app/guideband.hpp,  …app/icons.cpp,  …app/icons.hpp,  …app/main.cpp,  …app/mainwindow_ui.cpp,  …app/planview.cpp,  …app/planview.hpp,  …app/ribbon.cpp
- 시험 범위: 전체(core/tests/tools/CMake 변경 또는 -Full) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261009-01-b5-dpi-build` · `build.log` |
| tests(full) | PASS | 0 | 3 | assertions: 51339 ¦ 51339 passed /  | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec_tests.exe ` · `tests-full.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=11ms cut=1ms image=17ms | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 63 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 63 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 65 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 64 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261009-01-b5-dpi-build\logs\20261009-234517\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 1 |  | CI 0f7fbfe completed failure jobs: Windows · 앱 전체=failure, Linux · 코어와 시험=success https://github.com/kesekikwon-netizen/desca/actions/runs/37937880747 | `ci-status.ps1 -Sha 0f7fbfed7455c0c6db308af386ae935b27d6f77e` |
