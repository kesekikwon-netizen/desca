# 검증 기록 — 20261010-01-narrow-1280 1280 좁은 창: 단면 머리 · 정보 줄 · 평면 눈금 · 리본 기대값 20 + B5 검토 should 7건

`factory verify 20261010-01-narrow-1280`(실행기 `factory.cmd`, 어느 폴더에서든) 가 실행마다 아래에 블록을 덧붙인다(명령 · 종료 코드 · 요약 · 로그 파일 이름).
큰 로그는 `C:\dev\kerf-wt\20261010-01-narrow-1280-build\logs\<시각>\` 에만 있다(커밋하지 않음). 승인 근거는 이 파일의 요약 줄이므로, 중요한 출력(통과 수 · 기준값 줄 · RESULT 줄)은 요약에 들어 있어야 한다.

## 완료 기준별 결과 (test-runner 가 마지막 검증 뒤 채운다)
| # | 완료 기준 | 검증 방법 | 결과 | 근거(아래 블록 · 로그) |
| --- | --- | --- | --- | --- |
| AC1 | | | 통과 / 실패 / 미실행(이유) | |

## 기존 실패 vs 이번 변경의 실패
- 기존(기준 커밋에서도 실패): `CoalescingWorker: 끄는 동안의 미리보기는…` (Windows 타이머 간격, 가끔) · `[real]` 은 `ASEC_REAL_3MX` 없으면 건너뜀(정상)
- 이번 변경으로 생긴 실패:

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
