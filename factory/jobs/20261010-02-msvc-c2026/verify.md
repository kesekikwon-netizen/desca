# 검증 기록 — 20261010-02-msvc-c2026 MSVC 빌드 고침: theme.hpp QSS 원시 문자열 16 KB 아래로(C2026)

`factory verify 20261010-02-msvc-c2026`(실행기 `factory.cmd`, 어느 폴더에서든) 가 실행마다 아래에 블록을 덧붙인다(명령 · 종료 코드 · 요약 · 로그 파일 이름).
큰 로그는 `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\<시각>\` 에만 있다(커밋하지 않음). 승인 근거는 이 파일의 요약 줄이므로, 중요한 출력(통과 수 · 기준값 줄 · RESULT 줄)은 요약에 들어 있어야 한다.

## 완료 기준별 결과 (test-runner 가 마지막 검증 뒤 채운다)
| # | 완료 기준 | 검증 방법 | 결과 | 근거(아래 블록 · 로그) |
| --- | --- | --- | --- | --- |
| AC1 | | | 통과 / 실패 / 미실행(이유) | |

## 기존 실패 vs 이번 변경의 실패
- 기존(기준 커밋에서도 실패): `CoalescingWorker: 끄는 동안의 미리보기는…` (Windows 타이머 간격, 가끔) · `[real]` 은 `ASEC_REAL_3MX` 없으면 건너뜀(정상)
- 이번 변경으로 생긴 실패:

## 검증 2026-10-10 18:01 · **PASS** · HEAD `373c0e7dee7c05a9d71689ef8272f73305fefc82`
- 기준 `373c0e7dee7c05a9d71689ef8272f73305fefc82` · 바뀐 파일 0개: (없음)
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: off (크기 1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180043`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | PASS | 0 | 6 | CMake 구성 완료 | `cmake -S C:\dev\kerf-wt\20261010-02-msvc-c2026 -B C:\dev\kerf-wt\20261010-02-msvc-c2026-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl "-DCMAKE_RC_FLAGS=/C 65001" -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/msvc2022_64` · `configure.log` |
| build | PASS | 0 | 17 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-02-msvc-c2026-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=32ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180043\sec` · `asec-section.log` |
| ui-audit | SKIP |  |  | 건너뜀: -UiAudit off |  |
| ci(base) | WARN | 3 |  | CI 373c0e7 none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 373c0e7dee7c05a9d71689ef8272f73305fefc82` |

## 검증 2026-10-10 18:11 · **PASS** · HEAD `373c0e7dee7c05a9d71689ef8272f73305fefc82` (+미커밋 변경 있음)
- 기준 `373c0e7dee7c05a9d71689ef8272f73305fefc82` · 바뀐 파일 1개: app/theme.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 6 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-02-msvc-c2026-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=14ms | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 68 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 69 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-180239\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 373c0e7 none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 373c0e7dee7c05a9d71689ef8272f73305fefc82` |

## 검증 2026-10-10 18:19 · **PASS** · HEAD `044ab95e15e9995de134b3ee9c00b48a5b234559`
- 기준 `373c0e7dee7c05a9d71689ef8272f73305fefc82` · 바뀐 파일 1개: app/theme.hpp
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: on (크기 1280x800,1920x1040 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-02-msvc-c2026-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=2ms image=14ms | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-181447\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 373c0e7 none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 373c0e7dee7c05a9d71689ef8272f73305fefc82` |

## 독립 검증(test-runner) — HEAD 044ab95

| AC | 판정 | 근거 |
| --- | --- | --- |
| AC1 | 통과 | `cmake --build C:\dev\kerf-wt\20261010-02-msvc-c2026-msvc --config Release --parallel --clean-first` exit 0 (51 s, 로그 `C:\dev\tmp\r0-msvc-clean.txt`). ` error ` 줄 0 · C2026 줄 0. 경고는 기존 `tests\test_raster.cpp(48,11): warning C4551` 하나뿐(theme.hpp 경고 없음). asec_tests.exe · SectionViewer.exe 링크 완료. |
| AC2 | 통과(일부 조합만 실행) | `factory.cmd verify … -UiAudit on -UiSizes 1280x800,1920x1040 -UiScales 1,2` RESULT PASS: build exit 0 새 경고 0 · tests(quick) 51325 통과 · asec-section polylines=1 vertices=54 · ui-audit 1280x800/1920x1040 × 1/2 4조합 모두 RESULT ok. **3440x1440 · 3840x2160 은 미실행(이번 요청 범위 밖, 지시된 조합만)**. build 단계는 증분(1 s). ci(base) 는 push 전이라 none. |
| AC3 | 통과 | `git diff 373c0e7..044ab95 --ignore-cr-at-eol --stat` = `app/theme.hpp` 1개(+6 −3). 바뀐 줄은 주석 3줄 + 빈 줄 3곳(121·201·265행)→`)" R"(` 뿐. ac3.py(원시 문자열 조각을 호출별로 이어 붙여 CR 제거 후 비교): `normal equal=1 len=20241/20241 sha=d6f4b7afacf536f1` · `hc equal=1 len=343/343 sha=2e77a2c589bb4a2c` · calls 2/2. |
| AC4 | 통과 | 조각 바이트 [4096, 6783, 5208, 4154] max 6783 ≤ 8000(le8000=1), 고대비 343. 주석(상한 · 나누는 법 · 출처) 3줄 diff 에서 확인. |
| AC5 | 미실행 | 캡처 비교는 이번 독립 검증 범위 밖(builder 기록 참조). |

## 검증 2026-10-10 18:29 · **PASS** · HEAD `362e331c655031eeb5caadfeaeb015ac6a056fbd`
- 기준 `373c0e7dee7c05a9d71689ef8272f73305fefc82` · 바뀐 파일 7개: app/theme.hpp, factory/jobs/20261010-02-msvc-c2026/approval.md, factory/jobs/20261010-02-msvc-c2026/build.md, factory/jobs/20261010-02-msvc-c2026/job.md, factory/jobs/20261010-02-msvc-c2026/review.md, factory/jobs/20261010-02-msvc-c2026/spec.md, factory/jobs/20261010-02-msvc-c2026/verify.md
- 시험 범위: 전체(core/tests/tools/CMake 변경 또는 -Full) · ui-audit: on (크기 1280x800,1920x1040,3440x1440,3840x2160 · 배율 1,2)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | SKIP |  |  | 건너뜀: 이미 구성됨(CMakeCache.txt 있음) |  |
| build | PASS | 0 | 1 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-02-msvc-c2026-build` · `build.log` |
| tests(full) | PASS | 0 | 4 | assertions: 51339 ¦ 51339 passed /  | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\asec_tests.exe ` · `tests-full.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=13ms cut=2ms image=18ms | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\sec` · `asec-section.log` |
| ui-audit 1280x800 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\settings-1280x800-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\ui-audit-1280x800-x1 --quit` · `ui-audit.txt` |
| ui-audit 1280x800 ×2 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1280x800 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\settings-1280x800-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\ui-audit-1280x800-x2 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\settings-1920x1040-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\ui-audit-1920x1040-x1 --quit` · `ui-audit.txt` |
| ui-audit 1920x1040 ×2 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 1920x1040 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\settings-1920x1040-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\ui-audit-1920x1040-x2 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×1 | PASS | 0 | 66 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\settings-3440x1440-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\ui-audit-3440x1440-x1 --quit` · `ui-audit.txt` |
| ui-audit 3440x1440 ×2 | PASS | 0 | 68 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3440x1440 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\settings-3440x1440-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\ui-audit-3440x1440-x2 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×1 | PASS | 0 | 67 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\settings-3840x2160-x1 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\ui-audit-3840x2160-x1 --quit` · `ui-audit.txt` |
| ui-audit 3840x2160 ×2 | PASS | 0 | 69 | ui-audit RESULT ok | `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe C:\dev\kerf-synth\Synthetic.3mx --line 200002 450006 200014 450006 --size 3840x2160 --settings C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\settings-3840x2160-x2 --ui-audit C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\20261010-182002\ui-audit-3840x2160-x2 --quit` · `ui-audit.txt` |
| ci(base) | WARN | 3 |  | CI 373c0e7 none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha 373c0e7dee7c05a9d71689ef8272f73305fefc82` |
