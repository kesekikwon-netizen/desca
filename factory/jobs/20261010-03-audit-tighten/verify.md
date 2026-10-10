# 검증 기록 — 20261010-03-audit-tighten #3c 후속: 감사 판정 조이기 · 작은 정리(R1b)

`factory verify 20261010-03-audit-tighten`(실행기 `factory.cmd`, 어느 폴더에서든) 가 실행마다 아래에 블록을 덧붙인다(명령 · 종료 코드 · 요약 · 로그 파일 이름).
큰 로그는 `C:\dev\kerf-wt\20261010-03-audit-tighten-build\logs\<시각>\` 에만 있다(커밋하지 않음). 승인 근거는 이 파일의 요약 줄이므로, 중요한 출력(통과 수 · 기준값 줄 · RESULT 줄)은 요약에 들어 있어야 한다.

## 완료 기준별 결과 (test-runner 가 마지막 검증 뒤 채운다)
| # | 완료 기준 | 검증 방법 | 결과 | 근거(아래 블록 · 로그) |
| --- | --- | --- | --- | --- |
| AC1 | | | 통과 / 실패 / 미실행(이유) | |

## 기존 실패 vs 이번 변경의 실패
- 기존(기준 커밋에서도 실패): `CoalescingWorker: 끄는 동안의 미리보기는…` (Windows 타이머 간격, 가끔) · `[real]` 은 `ASEC_REAL_3MX` 없으면 건너뜀(정상)
- 이번 변경으로 생긴 실패:

## 검증 2026-10-10 18:49 · **PASS** · HEAD `d95706fb546576bc6fff9b7d5fbb14f3730dd162`
- 기준 `d95706fb546576bc6fff9b7d5fbb14f3730dd162` · 바뀐 파일 0개: (없음)
- 시험 범위: 빠른 시험(~CoalescingWorker*) · ui-audit: off (크기 1920x1040 · 배율 1)
- 로그 폴더(미커밋, 이 PC): `C:\dev\kerf-wt\20261010-03-audit-tighten-build\logs\20261010-184753`

| 단계 | 판정 | 종료 코드 | 초 | 요약 | 명령 · 로그 |
| --- | --- | --- | --- | --- | --- |
| configure | PASS | 0 | 55 | CMake 구성 완료 | `cmake -S C:\dev\kerf-wt\20261010-03-audit-tighten -B C:\dev\kerf-wt\20261010-03-audit-tighten-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl "-DCMAKE_RC_FLAGS=/C 65001" -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/msvc2022_64` · `configure.log` |
| build | PASS | 0 | 22 | 빌드 성공, 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만) | `cmake --build C:\dev\kerf-wt\20261010-03-audit-tighten-build` · `build.log` |
| tests(quick) | PASS | 0 | 3 | assertions: 51325 ¦ 51325 passed /  | `C:\dev\kerf-wt\20261010-03-audit-tighten-build\asec_tests.exe "~CoalescingWorker*"` · `tests-quick.log` |
| asec-section | PASS | 0 | 1 | tiles(leaf)=12 tris=186272 rawSeg=602 polylines=1 vertices=54 z=[44.20,47.50] image=3000x825 res=0.0040  collect=12ms cut=1ms image=15ms | `C:\dev\kerf-wt\20261010-03-audit-tighten-build\asec-section.exe C:\dev\kerf-synth\Synthetic.3mx 200002 450006 200014 450006 --out C:\dev\kerf-wt\20261010-03-audit-tighten-build\logs\20261010-184753\sec` · `asec-section.log` |
| ui-audit | SKIP |  |  | 건너뜀: -UiAudit off |  |
| ci(base) | WARN | 3 |  | CI d95706f none (이 커밋의 실행 없음 — push 안 됨 또는 아직 시작 전) | `ci-status.ps1 -Sha d95706fb546576bc6fff9b7d5fbb14f3730dd162` |
