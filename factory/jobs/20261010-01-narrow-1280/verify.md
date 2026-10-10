# 검증 기록 — 20261010-01-narrow-1280 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건

`factory verify 20261010-01-narrow-1280`(실행기 `factory.cmd`, 어느 폴더에서든) 가 실행마다 아래에 블록을 덧붙인다(명령 · 종료 코드 · 요약 · 로그 파일 이름).
큰 로그는 `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\<시각>\` 에만 있다(커밋하지 않음). 승인 근거는 이 파일의 요약 줄이므로, 중요한 출력(통과 수 · 기준값 줄 · RESULT 줄)은 요약에 들어 있어야 한다.

## 완료 기준별 결과 (test-runner 2026-10-10 05:36–07:25, HEAD `a2e2228`, worktree 깨끗)
**전체 결론: FAIL — 유일한 원인은 AC13(DESIGN_SPEC §12 새 키 3종 누락, 문서). 자동 · 코드 기준은 전부 통과(V8 · V2 RESULT PASS, 회귀 exit 0). 수동 AC11 은 kerf-ui-reviewer 대기. 미실행: 판별력 임시 변경 증명 4건(AC4 · AC6 · AC7 · AC9, 제품 코드 수정 금지라 build.md 기록 인용) · 전체 시험(CoalescingWorker 포함, core/tests 변경 없음).**

기호: **V8** = verify 8가지(1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × ×1 · ×2) 블록 05:36 · 로그 `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\`(exit 0, RESULT PASS). **V2** = 1280x800 × 1.25 · 1.75 블록 05:39 · 로그 `…\logs\20261010-053705\`(exit 0, RESULT PASS). 감사 줄은 `ui-audit-<크기>-x<배율>\ui-audit.txt`. 수동 캡처 · 회귀 출력 `C:\dev\tmp\n1280-verify\`.

| # | 완료 기준 | 결과 | 근거 |
| --- | --- | --- | --- |
| AC1 단면 머리 | 통과 | V8 8 + V2 2 모두 `sec-header-overlap=0 kept=1 toggles-reachable=1`(1280 ×1: `at=run:5:0,1280w:5:0,1280L:5:0,1280p:5:0,1280d:5:0,probe:3:0,1280v:5:0`, 1920: `run:1:0`, 3440 · 3840: `run:0:0`). `dup-icons=0 icons-missing=0 tooltip-missing=0 scale-controls=1` 유지. 실제 화면 캡처 `work-1280x800.png`(머리 5단계: 「A–A′ …」 · 그릴 것 메뉴 아이콘 · 세로 아이콘 · 「1:218」 · 맞춤 · 확대 · 축소, 겹침 없음), `work-1920x1040.png`(단계 0, 지금 모양). RED→GREEN(40 → 0)은 build.md 줄 인용(내가 RED 를 다시 돌리지는 않음) |
| AC2 정보 줄 | 통과 | 10가지 모두 `strip-clipped=0 state-kept=1 elide-tip-missing=0`(1280 ×1: `run:3 1280w:3 1280L:3 1280p:4 1280d:4 probe:2 1280v:3`). 캡처 1280: 「✓ 최종(잎) … 잘린 선 1줄 · 54점 · 빈 구간 없음」 잘림 없음, 1920: 전체 글(입면 뒤 · 레벨선 포함) |
| AC3 눈금 글 | 통과 | 10가지 모두 `plan-tick-overlap=0`(offscreen 1280w step=5 px-per-m=17.4, 1920 run step=2, 3440 · 3840 run step=1). 실제 화면: 1280 「2 m 4 m 6 m 8 m 10 m」 겹침 없음, 1920 「1 m … 11 m」 1 m 마다 |
| AC4 리본 1280 = 20 | 통과(잠금 확인은 builder 기록) | 10가지 모두 `narrow … look=20 labels=0 … actual-look=20 actual-labels=0`, `ribbon … look=20 … need20=1280 need32=1682`. DESIGN_SPEC §2.1 · §3.3 · §12 diff 읽음. 상수 24 로 FAIL 시키는 잠금 확인은 제품 코드를 못 고치는 역할이라 **재실행 안 함**(build.md Task 2 인용) |
| AC5 세로 과장 뒤 맞춤 | 통과 | 10가지 모두 `vex-fit=1 mode=keep`(1280: ppm=30.2589/30.2589, 1920: 43.2039/43.2039, 3440: 104.693, 3840: 120.874). RED(0)는 build.md 인용 |
| AC6 칩 픽셀 비교 | 통과(판별력은 builder 기록) | 10가지 모두 env `icon-dpr-bad=0 chip-dpr-bad=0`(×1.25 · ×1.75 포함). 1.25 장을 빼면 FAIL 하는 증명은 임시 변경이라 **재실행 안 함**(build.md 인용 `chip-dpr-bad=21`) |
| AC7 1280 실행 probe | 통과(판별력은 builder 기록) | 1280 실행 `probe=1600x1000 resized=1`, 1920 `probe=1920x1040 resized=1`, 3440 `probe=3440x1440`, 3840 `probe=3840x2160`(모두 `plan-fits=1 section-fits=1 zoom-kept=1`). 맞춤 길을 끄면 `section-fits=0` 증명은 **재실행 안 함**(build.md 인용) |
| AC8 홈 한 줄 글 | 통과 | ① 10가지 모두 `home-clipped=0 elide-tip-missing=0 min-width=1280`. ② 긴 이름 복사본(`C:\dev\tmp\n1280-verify\long\Synthetic_Excavation_Area3_TrenchA_LongName.3mx`, 1280x800 offscreen `--ui-audit`) → `EXIT=0`, `home-clipped=0 elide-tip-missing=0 min-width=1280`, `RESULT ok`(builder 의 RED 는 min-width=2621). 실제 화면 `home-1280-long-real.png`: 카드 이름 「Synthet···ongName」(가운데 줄임표), 표 이름 전체, 창 1280. 관찰: 카드 칸 이름 · 값이 「마지…」 「수평 …」 「EPS…」 「KVD…」 로 줄어 값(좌표계 · 높이)이 툴팁으로만 읽힘 → kerf-ui-reviewer 판단 |
| AC9 ⓑ ⓒ ⓖ | 통과(판별력은 builder 기록) | ⓑ `mainwindow_ui.cpp:2032` `r.right() >= width() \|\| r.bottom() >= height() \|\| r.left() < 0 \|\| r.top() < 0` 읽음, 10가지 모두 `overflow=0`. ⓒ `planview.cpp` 에 `ortho(` 1회(338) · `view.rotate(` 1회(340–341, makeMatrices) — 1033행 `rot.rotate` 는 다른 행렬. ⓖ `clearScene` 207행 `fitted_ = false`. `resizeEvent` 맞춤 끄기 증명은 **재실행 안 함**(build.md 인용 `plan-fits=0`) |
| AC10 고정 완료 기준 | 통과 | V8 `RESULT PASS` exit 0: build exit 0 · tests(quick) exit 0 `assertions: 51325 \| 51325 passed`(136 cases, 135 passed, 1 skipped) · asec-section `polylines=1 vertices=54` · ui-audit 8줄 모두 exit 0 `RESULT ok`. V2 `RESULT PASS` exit 0: 1280x800 ×1.25 · ×1.75 `RESULT ok`. `ui-audit.txt` 10개 모두 `FAIL` 0건. **새 경고:** verify 의 build 줄은 증분이라 0 은 자명 → 별도로 `cmake --build …-build --target SectionViewer --clean-first`(exit 0, 42 단계) 로그 `C:\dev\tmp\n1280-verify\clean-app-build.log` 의 `warning:` 15건 전부 기존(sscanf · strcpy · fopen · `_wfopen` · `mirrored`), 바뀐 파일에서 새 경고 없음 · 오류 0 |
| AC11 캡처 루브릭 | 수동(검토) | kerf-ui-reviewer 몫. 입력: `C:\dev\tmp\n1280-verify\work-1280x800.png` · `draw-1280x800.png` · `work-1920x1040.png` · `draw-1920x1040.png` · `home-1280-real.png` · `home-1280-long-real.png`(실제 화면 150 %, 픽셀 = 논리 × 1.5). 3440 · 3840 은 이 PC 화면에 잘려 offscreen 감사로 갈음 |
| AC12 회귀 | 통과 | `--icon-sheet` exit 0 `icon-sheet n=62 missing=0 … RESULT ok` · `--sheet-check`(1920×1040) exit 0 `sheet-check RESULT ok`, `level-pt minor=0.564 expect=0.564 major=1.411 expect=1.411 ok=1` · `--ctx-shot` exit 0 `ctx-shot ok: … guide=1`. `git -C <worktree> diff --stat 308daac -- core tests tools CMakeLists.txt app/CMakeLists.txt .github` → 출력 없음(exit 0). 기존 감사 항목(dup-icons · scale-controls · guide-step2 · square-borders · icons-missing …)은 10가지 `RESULT ok` 에 포함 |
| AC13 문서 | **실패(문서, 이번 변경 탓)** | DESIGN_SPEC diff 읽음. 있음: §2.1 · §3.3 · §12 의 1280 → 20(사용자 말 · 날짜), §4 단면 머리 · 정보 줄 순서, §5 눈금 글 규칙, §7 홈 줄임표 · 키 줄 · 다시 그리기(§4 · §5 · §7 불릿에 `sec-header-overlap` · `strip-clipped` · `plan-tick-overlap` · `home-clipped` 가 적혀 있음). `+` 줄에 `C:[\\/]` · `HKEY_` 없음(grep exit 1). **없음(명세 AC13 이 §12 에 요구한 것):** 새 키 `chip-dpr-bad=0` · `vex-fit=1` · narrow `probe=… resized=1` 세 가지가 `docs/` 어디에도 없음(`git grep` 결과 없음) — §12 는 `look=20` 한 줄 고침뿐이고 새 키 줄이 없다. 참고(AC13 항목은 아님): 세로 과장을 바꾼 보기는 사용자 보기라는 Q3 규칙도 문서에 없음. build.md 는 AC13 을 「통과」로 적었으나 명세 문구와 어긋남. 필요한 고침: DESIGN_SPEC §12 에 한 줄(새 키 여섯 + `home-stale-children=0` + narrow `probe/resized`) — 코드 변경 없음, 코드 재검증 불필요 |
| AC14 홈 다시 그리기 | 통과 | 10가지 모두 `home-stale-children=0`. 재현 명령 `SectionViewer.exe Synthetic.3mx --line … --tab 0 --size 1920x1040 --settings <새 폴더> --shot home-tab0-1920x1040.png --quit`(offscreen) exit 0 → PNG 를 눈으로: 옛 위젯 덩어리 없음(고치기 전 `C:\dev\tmp\kerf-shots\home-dbg.png` 는 왼쪽 위에 겹침). 1280 `home-tab0-1280x800.png` 도 겹침 없음. 「목록에서 지우기」 클릭 동작은 직접 시험하지 못함(감사 항목이 잔류 위젯 0 으로 간접 확인) |

## 기존 실패 vs 이번 변경의 실패
- 기존(기준 커밋에서도 실패): `CoalescingWorker: 끄는 동안의 미리보기는…` — 이번 verify 는 `~CoalescingWorker*` 로 제외해 돌렸고 core · tests 변경이 없어 전체 실행은 하지 않음(미실행) · `[real]` 1건 건너뜀(정상, tests(quick) 135 passed · 1 skipped) · ezdxf 없음(DXF 변경 없음이라 무관) · `ci(base)` WARN exit 3(push 전, 기준 `308daac` 실행 없음 — 기존 상태)
- 이번 변경으로 생긴 실패: **AC13(문서) 1건** — DESIGN_SPEC §12 에 `chip-dpr-bad=0` · `vex-fit=1` · narrow `probe/resized` 줄이 없음(시험 · 코드 실패는 없음). 자동 · 코드 기준(AC1–AC10 · AC12 · AC14)은 모두 통과 — 문서만 고치는 rework 로 닫힘.

## 회귀 · 캡처 명령 기록(test-runner, exe `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe`, PATH 앞 `C:\Qt\6.10.3\msvc2022_64\bin`, offscreen 은 `QT_QPA_PLATFORM=offscreen`, `Start-Process -PassThru` + `WaitForExit(240000)`, 설정은 새 폴더 `C:\dev\tmp\n1280-verify\s-*`)
- `--icon-sheet C:\dev\tmp\n1280-verify\icons.png --quit` → EXIT=0 · `icon-sheet n=62 missing=0 … RESULT ok`
- `C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings …\s-sheet --sheet-check …\sheet --quit` → EXIT=0 · `sheet-check RESULT ok`
- 같은 모델 · 선 `--size 1920x1040 --ctx-shot …\draw-ctx.png --quit` → EXIT=0 · `ctx-shot ok: … guide=1`
- 긴 이름 복사본 `--size 1280x800 --ui-audit …\ui-long --quit` → EXIT=0 · `home-clipped=0 elide-tip-missing=0 min-width=1280` · `RESULT ok`
- `--tab 0 --size 1920x1040 --shot …` / `--size 1280x800` → EXIT=0 · `shot ok`
- 실제 화면 캡처: `.claude\scripts\shot.ps1 -Exe <위 exe> -Scene work|draw -Size 1280x800|1920x1040`(exit 0 네 장), 홈은 합성 · 긴 이름 모델을 연 뒤 같은 `--settings` 로 `--start-shot … --size 1280x800`(exit 0)
## 검증 2026-10-10 01:47 · **FAIL** · HEAD `308daac82e8e46189600488c209d43020e046cc2` (+미커밋 변경 있음)
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 4개: app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-014457`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-014457\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | FAIL | 9 | 65 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-014457\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-014457\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | FAIL | 9 | 65 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-014457\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-014457\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 04:47 · **FAIL** · HEAD `308daac82e8e46189600488c209d43020e046cc2` (+미커밋 변경 있음)
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 4개: app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-044530`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 4 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=2ms image=14ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-044530\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | FAIL | 9 | 66 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-044530\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-044530\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | FAIL | 9 | 65 | exit=9 · ui-audit RESULT fail (9 = 자리 규칙 실패, 다른 값 = 앱이 뜨지 않음) | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-044530\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-044530\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 05:23 · **PASS** · HEAD `a2e222842d0f16e6ea3ba1472b565d81b0a61657`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 3 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=13ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 69 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-051359\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 05:25 · **PASS** · HEAD `a2e222842d0f16e6ea3ba1472b565d81b0a61657`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800 · 배율 1.25,1.75)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052313`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052313\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1.25 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052313\settings-1280x800-x1.25 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052313\ui-audit-1280x800-x1.25 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×1.75 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052313\settings-1280x800-x1.75 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052313\ui-audit-1280x800-x1.75 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 05:36 · **PASS** · HEAD `a2e222842d0f16e6ea3ba1472b565d81b0a61657`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 68 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-052745\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 05:39 · **PASS** · HEAD `a2e222842d0f16e6ea3ba1472b565d81b0a61657`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800 · 배율 1.25,1.75)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-053705`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-053705\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1.25 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-053705\settings-1280x800-x1.25 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-053705\ui-audit-1280x800-x1.25 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×1.75 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-053705\settings-1280x800-x1.75 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-053705\ui-audit-1280x800-x1.75 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 07:38 · **PASS** · HEAD `c9214e2ddf7fee423c2c50c77aa09c9741911302`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: off (크기 1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-073834`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 9 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-073834\sec` · `asec-section.log` |
| ui-audit | SKIP |  |  | 건너뜀: -UiAudit off |  |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 09:00 · **PASS** · HEAD `00c9a43844c46e13ea445379e75576e8ceebc498`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 8 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=22ms cut=2ms image=33ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 68 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 69 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-085100\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 09:03 · **PASS** · HEAD `00c9a43844c46e13ea445379e75576e8ceebc498`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800 · 배율 1.25,1.75)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-090044`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-090044\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1.25 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-090044\settings-1280x800-x1.25 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-090044\ui-audit-1280x800-x1.25 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×1.75 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-090044\settings-1280x800-x1.75 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-090044\ui-audit-1280x800-x1.75 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 09:22 · **PASS** · HEAD `00c9a43844c46e13ea445379e75576e8ceebc498`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=11ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 68 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 70 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2026-10-10 09:24 · **PASS** · HEAD `00c9a43844c46e13ea445379e75576e8ceebc498`
- 기준 `308daac82e8e46189600488c209d43020e046cc2` · 바뀐 파일 11개: app/icons.cpp, app/icons.hpp, app/mainwindow.hpp, app/mainwindow_ui.cpp, app/planview.cpp, app/planview.hpp, app/ribbon.cpp, app/ribbon.hpp, app/sectionview.cpp, app/sheetexport.cpp, docs/design/DESIGN_SPEC.md
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800 · 배율 1.25,1.75)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-092226`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-01-narrow-1280-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=11ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-092226\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1.25 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-092226\settings-1280x800-x1.25 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-092226\ui-audit-1280x800-x1.25 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×1.75 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-01-narrow-1280-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-092226\settings-1280x800-x1.75 --ui-audit C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-092226\ui-audit-1280x800-x1.75 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 308daac none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 308daac82e8e46189600488c209d43020e046cc2` |

## 검증 2차(rework 2, HEAD `00c9a43`) — test-runner 독립 검증 2026-10-10 09:13–09:31, worktree 깨끗(`git status --short` 비어 있음, HEAD `00c9a43844c46e13ea445379e75576e8ceebc498`)
**전체 결론: 자동 기준 전부 통과(V8 RESULT PASS · V2 RESULT PASS · 회귀 셋 EXIT 0 · 긴 이름 감사 ok · core/tests/CMake/CI diff 없음). 실패 0 · 이번 변경의 실패 없음. 수동 1(AC11 루브릭 = kerf-ui-reviewer 몫) · 미실행 5(아래 「미실행」). rework 2 의 새 감사 키(`carets=1` · `refit=1/0` · `gap-kept=1` · `cells-elided=0` · `home-clipped=0` · `sec-header-overlap=0` · `home-stale-children=0`)는 10조합 + 긴 이름 감사 모두 실제로 나왔다.**

기호: **V8** = verify 블록 09:22(1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × ×1 · ×2, `RESULT PASS` exit 0, 545 s), 로그 `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\20261010-091304\ui-audit-<크기>-x<배율>\ui-audit.txt`. **V2** = verify 블록 09:24(1280x800 × 1.25 · 1.75, `RESULT PASS` exit 0, 140 s, V8 이 끝난 뒤에 돌림), 로그 `…\logs\20261010-092226\ui-audit-1280x800-x1.25\` · `-x1.75\`. 회귀 · 긴 이름 출력 `C:\dev\tmp\n1280-v2nd\`. 각 ui-audit.txt 는 31줄 · `FAIL` 0건 · 끝줄 `ui-audit RESULT ok`(10개 모두 grep 으로 확인). 명령 출력 원문: `C:\dev\tmp\n1280-v2nd-v8.out.txt` · `n1280-v2nd-v2.out.txt`.

### 단계 판정(이번 실행)
| 실행 | 단계별 종료 코드 · 요약 |
| --- | --- |
| V8 | configure SKIP · build exit 0(새 경고 0) · tests(quick) exit 0 `assertions: 51325 ¦ 51325 passed` · asec-section exit 0 `polylines=1 vertices=54` · ui-audit 8줄 모두 exit 0 `RESULT ok`(66–70 s) · ci(base) WARN exit 3(push 전 — 기존 사정) → `RESULT PASS` |
| V2 | build exit 0 · tests(quick) exit 0 51325 · asec-section exit 0 `vertices=54` · ui-audit ×1.25 · ×1.75 exit 0 `RESULT ok`(67 s) · ci(base) WARN exit 3 → `RESULT PASS` |
| 회귀 `--icon-sheet` | EXIT=0 · `icon-sheet n=62 missing=0 … RESULT ok` |
| 회귀 `--sheet-check`(1920x1040, 합성 모델 · 선 `200002 450006 200014 450006`) | EXIT=0 · `level-pt minor=0.564 expect=0.564 major=1.411 expect=1.411 ok=1` · `sheet-check RESULT ok` |
| 회귀 `--ctx-shot` | EXIT=0 · `ctx-shot ok: C:\dev\tmp\n1280-v2nd\draw-ctx.png guide=1` |
| 긴 이름 감사(AC8 ②, `C:\dev\tmp\n1280-verify\long\Synthetic_Excavation_Area3_TrenchA_LongName.3mx`, 1280x800 offscreen) | EXIT=0 · `home-clipped=0 cells-elided=0 elide-tip-missing=0 min-width=1280` · `RESULT ok` |
| core/tests/CMake/CI diff | `git diff --stat 308daac -- core tests tools CMakeLists.txt app/CMakeLists.txt .github` → 출력 0줄(비어 있음) |

회귀 · 긴 이름 실행은 `QT_QPA_PLATFORM=offscreen`, PATH 앞 `C:\Qt\6.10.3\msvc2022_64\bin`, 설정 새 폴더, `Start-Process -PassThru` + `WaitForExit(240000)`(넷 모두 시간 안에 끝남, Kill 없음).

### 완료 기준 표(AC1–AC14)
| # | 결과 | 근거 |
| --- | --- | --- |
| AC1 단면 머리 | 통과 | 10조합 모두 `sec-header-overlap=0 kept=1 toggles-reachable=1 carets=1 refit=1/0`. 1280(×1 · ×2 · ×1.25 · ×1.75): `at=run:5:0,1280w:5:0,1280L:5:0,1280p:5:0,1280d:5:0,probe:3:0,1280v:5:0` · 1920: `run:1:0,1280w:5:0,…` · 3440 · 3840: `run:0:0,1280w:5:0,…`. env `overflow=0`. 캡처(builder 의 실제 화면 150 % `C:\dev\tmp\n1280-rework2\work-1280.png`, 내가 눈으로 봄): 「A–A′ …」 · 그릴 것 · 세로 · 「1:218」 · 맞춤 · 확대 · 축소 겹침 없음. |
| AC2 정보 줄 | 통과 | 10조합 모두 `strip-clipped=0 state-kept=1 gap-kept=1 elide-tip-missing=0`. 1280: `at=run:3:0,1280w:3:0,1280L:3:0,1280p:4:0,1280d:4:0,probe:2:0,1280v:3:0` · 1920 `run:1` · 3440 · 3840 `run:0`. 캡처 work-1280: 「✓ 최종(잎) … 잘린 선 1줄 · 54점 · 빈 구간 없음」 전체 보임. |
| AC3 평면 눈금 글 | 통과(자동) · 수동 일부 | 10조합 모두 `plan-tick-overlap=0`: 1280w `step=5 px-per-m=17.4`(offscreen 글 폭 약 2배), `run` 단계 1280=5 · 1920=2 · 3440 · 3840=1. 캡처 work-1280: 「2 m 4 m 6 m 8 m 10 m」 겹침 없음. 실제 1920 의 「1 m 마다」는 이번에 새로 캡처하지 않음(builder 1차 캡처 인용) → AC11 수동. |
| AC4 리본 1280 = 20 | 통과(잠금 판별은 build.md 인용) | 10조합 모두 `narrow rule-at-1280 look=20 labels=0 … actual-look=20 actual-labels=0`, 1280×1 `ribbon height=70 groups=7 look=20 labels=0 … width=1280 need50=1916 need32=1682 need20=1280`. DESIGN_SPEC diff 읽음: §2.1 · §3.3 · §12 에 「20」 · 사용자 말 · 날짜. 「상수 24 → FAIL」 임시 변경은 제품 코드 수정 금지라 **재실행 안 함**(build.md 인용). |
| AC5 세로 과장 뒤 맞춤 | 통과 | 10조합 모두 `vex-fit=1 mode=keep`: 1280 `ppm=30.2589/30.2589` · 1920 `43.2039/43.2039` · 3440 `104.693/104.693` · 3840 `120.874/120.874`. |
| AC6 칩 픽셀 비교 | 통과(판별력은 build.md 인용) | 10조합 모두 env `icon-dpr-bad=0 chip-dpr-bad=0`(`dpr=1.00 · 1.25 · 1.75 · 2.00` 포함). 「1.25 를 빼면 FAIL」 증명은 임시 변경이라 **재실행 안 함**. |
| AC7 1280 실행 probe | 통과(판별력은 build.md 인용) | 1280 실행 4조합 `probe=1600x1000 resized=1` · 1920 `probe=1920x1040 resized=1` · 3440 `probe=3440x1440 resized=1` · 3840 `probe=3840x2160 resized=1`, 모두 `plan-fits=1 section-fits=1 zoom-kept=1`. 「맞춤 길 끄면 FAIL」 증명은 **재실행 안 함**. |
| AC8 홈 한 줄 글 | 통과 | ① 10조합 모두 `home-clipped=0 cells-elided=0 elide-tip-missing=0 min-width=1280 at=run:0:0,1280w:0:0`(`home-clipped` 는 rework 2 의 강화판 — 빈 글 · 「…」만 남은 라벨 · 폭 < 「…」 한 자도 셈). ② 긴 이름 복사본 1280 감사 위 표대로 ok. 캡처(builder `n1280-rework2\home-1280.png`, 내가 눈으로 봄): 칸 이름 「폴더 · 마지막 열림 · 수평 좌표계 · 높이 · 단면 · 도면」 전부 보임 · 값 「C: › …synth · 1분 전 · EPSG:5186 · KVD1964 · 1개 · —」 · 폴더 칸만 가운데 줄임표 · 「작업 순서」 오른쪽 「Synthetic 기준」 보임. |
| AC9 ⓑ ⓒ ⓖ | 통과(ⓑ 자동 · 판별력은 build.md 인용) | 10조합 모두 env `overflow=0`(넘침 식 `>=`). ⓒ · ⓖ 의 diff 읽기와 「resize 맞춤 끄면 `plan-fits=0 FAIL`」 증명은 builder · test-runner 1차 기록 인용(이번에 `app/planview.cpp` 를 다시 읽지 않음, 재실행 안 함). |
| AC10 고정 완료 기준 | 통과 | V8 `RESULT PASS` exit 0(8조합 `RESULT ok`) + V2 `RESULT PASS` exit 0(2조합 `RESULT ok`). build 새 경고 0 · tests(quick) 51325 통과 · asec-section `polylines=1 vertices=54` — 위 「단계 판정」 표. |
| AC11 캡처 루브릭 | **수동(미실행 — kerf-ui-reviewer 몫)** | 점수 없음. 참고로 builder 캡처 3장(work-1280 · home-1280 · home-1920)과 home-tab0-1280 을 눈으로만 봄: 겹침 · 반 토막 글 없음, 단 ▾(`kerf::iconWithCaret`)는 확대해 보면 「그릴 것」 · 세로 아이콘 오른쪽 아래에 **아주 작게**(약 6×3.6 px) 보인다 — 알아볼 수 있는지는 검토자 판단. 이번에 새로 캡처한 것은 없음(실제 화면 캡처 = builder 09:05). |
| AC12 회귀 | 통과 | 위 「단계 판정」: `--icon-sheet` · `--sheet-check` · `--ctx-shot` EXIT 0 · `git diff --stat 308daac -- core tests tools CMakeLists.txt app/CMakeLists.txt .github` 비어 있음 · 기존 감사 항목 전부 V8 · V2 `RESULT ok` 에 포함. |
| AC13 문서 | 통과 | `git diff 308daac -- docs/design/DESIGN_SPEC.md`(15줄 바뀜) 읽음: §2.1 · §3.3 · §12 「20」 · 사용자 말 · 날짜, §4 머리 · 정보 줄 순서 · 세로 과장 규칙, §5 눈금 규칙, §7 홈 줄임표 · 키 줄, §12 153행(1차 키: `sec-header-overlap=0` · `strip-clipped=0` · `plan-tick-overlap=0` · `home-clipped=0` · `home-stale-children=0` · `chip-dpr-bad=0` · `vex-fit=1` …) + 155행(rework 2 키: `carets=1` · `refit=` · `gap-kept=1` · `cells-elided=0`). `+` 줄에 `C:[\\/]` · `HKEY_` 없음(grep exit 1). |
| AC14 홈 다시 그리기 | 통과 | 10조합 + 긴 이름 감사 모두 `home-stale-children=0`. 캡처 `n1280-rework2\home-tab0-1280.png`(모델 연 뒤 홈, 내가 눈으로 봄): 옛 위젯 덩어리 없음. 「목록에서 지우기」 클릭 뒤 화면은 직접 시도하지 않음(감사 키로 간접). |

### review.md 「종합(08:20)」 rework 2 목록 확인(blocking 2 + should 5)
| 지적 | 결과 | 근거 |
| --- | --- | --- |
| blocking 1 [code B1] 홈 「모델 기준」 글이 모든 폭에서 사라짐 | 통과 | 감사 `home-clipped=0`(강화판이 사라진 라벨을 셈 — RED 때는 `home-clipped=2 [run:Synthetic 기준,…]`, build.md 인용) + 캡처 home-1280 · home-1920 · home-tab0-1280 모두 「작업 순서」 오른쪽에 「Synthetic 기준」 보임(내가 눈으로 봄). |
| blocking 2 [UI B1] 1280 카드 칸 이름 · 값 줄임 | 통과 | `cells-elided=0`(10조합 + 긴 이름) + 캡처 home-1280 칸 이름 · 값 전부 읽힘. 폴더 칸만 줄임표(스펙 허용). RED 때 `cells-elided=10`(build.md 인용). |
| should [code S1 · correctness C1] 감사가 사라진 라벨 못 셈 | 통과 | `home-clipped` 강화판이 위 모든 실행에서 0, RED 에서 2 였음(build.md 인용). |
| should [correctness C1] 정보 줄 4단계가 빈 구간부터 자름 | 통과 | `gap-kept=1`(10조합 + 긴 이름), 1280p · 1280d 는 4단계(`1280p:4:0`)인데도 `strip-clipped=0 state-kept=1`. RED 때 `gap-kept=0`(build.md 인용). |
| should [correctness C4] 「EPSG:5186」 끝 줄임표 | 통과(캡처) | home-1280 캡처 「EPSG:5186」 · 「KVD1964」 전체 보임. 감사에는 이 값만의 키가 없고 `cells-elided=0` 이 포괄. |
| should [code S2 · correctness C3] 머리 맞춤 한 번 | 통과 | 10조합 모두 `refit=1/0`(맞춤 횟수 ≤ 1 / 0). RED 때 `refit=2/2`(build.md 인용). |
| should [UI S1] 좁은 머리 메뉴 단추에 ▾ | 통과(자동은 약함) · 수동 대기 | 10조합 모두 `carets=1`. 감사는 구운 ▾ 를 동적 속성 `caretBaked` 로 믿는 것이지 픽셀 검사가 아니다(builder 도 적음). 실제 근거는 캡처 확대(내가 work-1280 에서 잘라 5배 확대 `C:\dev\tmp\n1280-v2nd\header-crop.png`): 두 아이콘 오른쪽 아래에 ▾ 가 있으나 매우 작음 → 크기 적정성은 kerf-ui-reviewer. |

### 기존 실패 vs 이번 변경의 실패
- 이번 변경으로 생긴 실패: **없음**(RESULT FAIL 0 · 종료 코드 0 아닌 단계 없음. 단 `ci(base)` WARN exit 3 은 기준 커밋 308daac 에 대한 Actions 실행이 없다는 뜻 — push 안 됨, 기존 사정).
- 기존(profile §9, 이번 변경 탓 아님): `CoalescingWorker` 불안정 시험은 이번에도 `~CoalescingWorker*` 로 제외해 돌렸고 이번 실패는 없어 재실행하지 않음 · `[real]` 1건 건너뜀 · 기존 경고 `fopen` · `mirrored` 는 build 줄이 「새 경고 0」으로 보고.

### 미실행(통과로 쓰지 않음)
1. **AC11 루브릭 점수**(kerf-ui-reviewer 8항목 · 1280 작업 · 그리는 중 · 홈 · 긴 이름 홈, 1920 모양 그대로) — 사람/검토자 몫. 이번에 새 캡처 없음.
2. **판별력 임시 변경 증명 4건**(AC4 상수 24 · AC6 1.25 빼기 · AC7 맞춤 길 끄기 · AC9 resize 맞춤 끄기) — 제품 코드를 고치지 않는 역할이라 재실행하지 않고 build.md 기록만 인용. rework 2 의 RED(`50abbd3`)도 build.md 인용.
3. **전체 시험(`CoalescingWorker` 포함)** — core/tests 변경 없음(diff 비어 있음)이라 빠른 시험만.
4. **실제 125 · 175 % 모니터 · 3440 · 3840 실제 화면** — 이 PC 는 150 % 하나 · 화면에 잘림. offscreen V8 · V2 로 갈음(B5 와 같은 방식).
5. **「목록에서 지우기」 클릭 · 「그릴 것」 메뉴를 연 모양(체크 · 키 글) · 긴 단면 이름(`1280L`) 실제 화면** — 감사 키(`home-stale-children=0` · `toggles-reachable=1` · `1280L:5:0`)로만 간접 확인.