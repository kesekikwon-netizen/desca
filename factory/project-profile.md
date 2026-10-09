# Kerf 프로젝트 프로필 (팩토리용 사실 묶음)

> 팩토리 에이전트가 추측하지 않도록 **확인된 사실**만 적는다. 「(미검증)」은 아직 실행해 보지 않은 것. 바뀌면 이 파일을 고친다.
> 작성 2026-10-09. 저장소 규칙 전체는 `CLAUDE.md`, 진행 기록은 `docs/DEV_PROGRESS.md`.

## 1. 한 줄 소개
3MX 실사 메시를 열어 평면에서 단면선을 긋고 10 cm 레벨선 단면도를 보고 DXF · GeoTIFF · LAS · SVG/PDF 도면으로 내보내는 Windows 데스크톱 앱(Kerf, 실행 파일 이름 `SectionViewer`). C++17 · Qt 6 Widgets + OpenGL.

## 2. 기술 스택 · 의존성 (코드에서 확인)
- 두 층: `core/`(정적 라이브러리 `archsection`, namespace `asec`, **Qt 없음**) · `app/`(화면만). 계산은 모두 core, 바꾸면 `tests/`에 먼저 실패하는 시험.
- `Q_OBJECT` 금지(moc 없음). 연결은 람다, `qobject_cast` 대신 `static_cast`. 색 · 크기는 `app/theme.hpp` 토큰만.
- 번들: OpenCTM(C) · stb · nlohmann/json · Catch2 v3 — 전부 `third_party/`, **수정 금지**. 외부: zlib(vcpkg). GDAL · PROJ 없음(자체 TM · WKT 파서).
- CMake 소스 목록은 **손으로** 적는다: core 소스 · 시험 파일은 최상위 `CMakeLists.txt`, 앱 소스는 `app/CMakeLists.txt`.
- 포맷 · 린트 설정(.clang-format · .clang-tidy) 없음 → 바꾼 줄만 고치고 파일 전체 재포맷 금지.

## 3. 지원 OS · 빌드 경로
| 경로 | 상태 |
| --- | --- |
| **Windows, clang-cl + Ninja(이 PC, 개발 기준)** | `.claude/cpp-project.json` 계약. 본 저장소 빌드 폴더 `C:\dev\kerf-v4c`, 소스는 정션 `C:\dev\desca`(한글 경로에서 rc 멈춤). Qt 6.10.3 `C:\Qt\6.10.3\msvc2022_64`, vcpkg `C:\dev\vcpkg` |
| Windows, MSVC(GitHub CI) | `.github/workflows/ci.yml`: Qt 6.8.3 + VS 2022 + 시험(`~CoalescingWorker*`) + offscreen `--ui-audit` |
| Linux(core · 도구 · 시험만, CI) | `-DASEC_BUILD_APP=OFF`. 이 PC에서는 못 돈다 |
| MinGW 교차(`build-win.sh`, `ASEC_QT_WIN_DIR`) | 이 PC에 도구 없음. `sheetexport.cpp`의 `QSvgGenerator` 때문에 지금 빌드되지 않을 가능성(미검증) |

## 4. 명령 (2026-10-09 실제로 돌려 확인)
본 저장소(참고용 — 팩토리 작업은 worktree 에서 `verify.ps1`를 쓴다):
```bash
cmake --build C:/dev/kerf-v4c                                   # exit 0
C:/dev/kerf-v4c/asec_tests.exe "~CoalescingWorker*"             # 133건: 132 통과 · 1 건너뜀(약 2초)
C:/dev/kerf-v4c/asec_tests.exe                                  # 137건: 136 통과 · 1 건너뜀(불안정 시험 포함)
C:/dev/kerf-v4c/asec-section.exe C:/dev/kerf-synth/Synthetic.3mx 200002 450006 200014 450006 --out C:/dev/tmp/sec
#   → tiles(leaf)=12 … polylines=1 vertices=54 z=[44.20,47.50]  (기준값)
```
팩토리 worktree(`C:\dev\kerf-wt\<ID>`, 빌드 `C:\dev\kerf-wt\<ID>-build`) — **실행기 `factory.cmd`는 어느 폴더에서 불러도 된다**(저장소 루트 `desca\factory.cmd`, 한 단계 위 `desc\factory.cmd`는 같은 것을 부름. PowerShell 에서는 `.\factory …`, 다른 폴더에서는 전체 경로 `C:\Users\권을\Documents\desc\factory.cmd …`):
```powershell
.\factory status [-All]
.\factory new -Slug <slug> -Title "<제목>" -Request "<사용자 말>"
.\factory verify <ID> [-Full] [-UiAudit auto|on|off] [-UiSizes 1280x800,1920x1040,3440x1440,3840x2160] [-UiScales 1,2]
.\factory ci <sha> [-Wait]      # GitHub Actions 결과(gh, 읽기만). exit 0 success · 1 실패 · 3 실행 없음 · 4 진행 중
```
- 화면(`app/`)을 바꾸는 작업은 **반드시** `-UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2`(와이드 · 4K · 200 % 배율)로 돈다 — 사용자 요구 「4K · 와이드 모니터 어디서나 안정」(backlog #1).
- GitHub Actions(`.github/workflows/ci.yml`)는 push · PR 때만 돈다. 팩토리는 push 하지 않으므로 `verify.ps1` 의 `ci(base)` 줄은 **기준 커밋**의 결과(참고), 병합 커밋의 CI 는 사용자가 push 한 뒤 `ci-status.ps1 -Wait` 로 확인한다(SKILL §5-6). 2026-10-09 확인: `de14dc2` success(Linux · Windows 둘), 로컬 HEAD `4781e1d`는 아직 push 되지 않아 실행 없음.
- `verify.ps1`는 configure(처음 한 번) → build → tests → asec-section 기준값 → ui-audit(app/ 바뀌면) 순서로 돌고 `factory/jobs/<ID>/verify.md`에 덧붙인다. Qt DLL 복사 없이 `PATH` · `QT_QPA_PLATFORM_PLUGIN_PATH`로 Qt 폴더를 쓴다. 첫 configure + 전체 빌드 시간은 아직 재지 않음(미검증).
- 캡처(사람이 볼 때): `.claude/scripts/shot.ps1 -Scene work|draw|start -Out <png> -Exe <worktree-build>\app\SectionViewer.exe` — worktree 빌드의 exe 를 `-Exe`로 넘긴다(Qt DLL 은 PATH 에 `C:\Qt\6.10.3\msvc2022_64\bin`을 넣어야 뜬다).
- 앱 자동 실행은 항상 `--settings DIR`로 설정 격리. GUI 실행 파일은 PowerShell `Start-Process -Wait -PassThru`.

## 5. 시험 데이터
- 합성 3MX: `asec-make-synthetic.exe C:\dev\kerf-synth` → `Synthetic.3mx`(EPSG:5186, X 200000–200016 · Y 450000–450012). 표준 단면선 `200002 450006 200014 450006`(12 m).
- 실제 모델: 저장소에 없음. `[real]` 시험은 환경 변수 `ASEC_REAL_3MX`가 없으면 건너뜀(정상). 실좌표가 보이는 캡처 · 실제 모델은 **공개 저장소에 올리지 않는다**.

## 6. 시험 지도 (영역 → 파일 · 고르는 법)
`asec_tests`는 실행 파일 하나. 태그가 있는 것만 태그로, 나머지는 **이름(와일드카드)** 으로 고른다. `--list-tests`로 전체 이름.
| 영역 | 파일 | 고르기 |
| --- | --- | --- |
| 단면 자르기 · 이어붙이기 · 빈 구간 | `test_section.cpp` `test_stitch.cpp` `test_joingaps.cpp` `test_cutplace.cpp` `test_backdepth.cpp` `test_pipeline.cpp` | `"[cut]"` `"[gaps]"` `"[cutplace]"` `"[backdepth]"` `"평면-삼각형*"` `"합성 3MX 전 과정*"` |
| 레벨선 | `test_section.cpp` | `"레벨선*"` `"라벨 간격*"` |
| DXF | `test_dxf.cpp` (+ `tests/dxf_audit.py`, ezdxf 필요) | `"DXF*"` |
| 좌표계(SRS · WKT · TM) | `test_srs.cpp` | `"SRS*"` `"WKT*"` `"TM*"` `"analyzeSrs*"` `"metadata.xml*"` |
| 수직 기준(지오이드 이름표) | `test_vdatum.cpp` | `"수직 기준*"` `"지오이드*"` `"높이 기준 지정*"` |
| 래스터 · 입면 영상 | `test_raster.cpp` `test_relief.cpp` | `"입면 영상*"` `"축척*"` `"[relief]"` |
| TIFF · GeoTIFF | `test_tiff.cpp` | `"평면 GeoTIFF*"` `"TIFF*"` `"단면 GeoTIFF*"` `"복합 EPSG*"` |
| 점군 · 레이어 · 피킹 | `test_points.cpp` `test_layers.cpp` `test_pick.cpp` | 이름으로(`--list-tests`) |
| 3MX 읽기 · 캐시 · 스트리밍 · 스레드 | `test_tmx.cpp` `test_cache_parallel.cpp` `test_stream.cpp` `test_schedule.cpp` | 이름으로. `CoalescingWorker*` 하나는 알려진 불안정 |
| 도면(조판) · 파일 이름 · 위에서 본 범위 | `test_sheet.cpp` | `"[sheet]"` |
| 빗금 | `test_hatch.cpp` | `"[hatch]"` |
| 실제 모델 | `test_real.cpp` | `"[real]"`(환경 변수 없으면 건너뜀) |

## 7. 수치 허용오차 (기존 기준 — 새 시험은 이 중 가장 가까운 것을 쓰고 출처를 적는다)
| 무엇 | 값 | 출처 |
| --- | --- | --- |
| 단면 끝점 s 좌표 · 실좌표 변환 | `1e-6 m` | `test_pipeline.cpp:38` · `test_cutplace.cpp:38` |
| 레벨 자릿값(zMin 이 0.1 m 배수) | `1e-9` | `test_pipeline.cpp:46` |
| 단면 점이 메시 위 | `< 0.01 mm`(격자 보간 대비) · `< 3 cm`(해석해 대비) | `test_section.cpp` 「단면 점은 메시 위…」 |
| 수혈 바닥 높이 | `margin 0.006 m` | `test_cutplace.cpp:116` |
| TM(Krüger) 변환 | `1 mm`(PROJ 기준값 대비) · 왕복 `0.1 mm` | `test_srs.cpp:162` |
| 위경도 복원 | `1e-8°` | `test_srs.cpp:168` |
| 두 패스 자르기 스냅 · 수집 여유 | 1 mm(2패스만) · 빈 구간 판정 3 cm | `core/src/engine.cpp`, DEV_PROGRESS 단계 0 |
공통 상수는 없다. 기존 기준에 없는 새 허용오차는 **이유를 적고 사용자 확인**을 받는다.

## 8. 변경 위험이 큰 곳 (→ 사람 검토 필수)
| 영역 | 파일 | 왜 |
| --- | --- | --- |
| 단면 자르기 두 패스 | `core/src/engine.cpp` `section.cpp` | 1패스에 스냅을 걸면 정밀도 시험이 깨짐(실측). 스냅은 빈 구간 메우기(2패스)에만 |
| 좌표 · 높이 | `core/src/srs.cpp` `vdatum.cpp` | 실좌표 = 로컬 + SRSOrigin, 3MX x=동 · y=북. **높이 값은 절대 바꾸지 않음**(이름표만) |
| 파일 형식 | `core/src/dxf.cpp` `tiff.cpp` `pointcloud.cpp` `export.cpp` `sheet.cpp`, `app/sheetexport.cpp` | 옛 파일 · 옛 설정 호환(예: 뒤 깊이 0.5→3 m 이관 `resolveBackDepth`). DXF 순서 레벨선 → IMAGE → 단면선 |
| 스레드 | `core/src/schedule.cpp`, `app/mainwindow.cpp` | `CoalescingWorker` · `ResultGate` 세대 번호 · `QMetaObject::invokeMethod(..., Qt::QueuedConnection)`. GUI 스레드 파일 I/O 금지 |
| 빌드 · 의존성 · CI | `CMakeLists.txt` `app/CMakeLists.txt` `.github/workflows/ci.yml` `build-*.sh` `packaging/` | 분기 셋(clang-cl · MSVC · MinGW) 모두 영향 |
| 디자인 토큰 · 화면 | `app/theme.hpp` `mainwindow_ui.cpp` `main.cpp`(`--ui-audit`) | 기준은 **디자인 v5 `docs/design/DESIGN_SPEC.md`**(2026-10-09 확정, C1–C7 추천대로). v5 구현 계획 `docs/superpowers/plans/2026-10-09-design-v5.md`가 본 저장소에서 진행 중이면 같은 파일을 두 곳에서 고치지 않는다. `--ui-audit` 기대값은 v5 단계가 고친다 |

## 9. 기존 실패 · 환경 부족 (이번 변경 탓이 아닌 것)
- `CoalescingWorker: 끄는 동안의 미리보기는…`(`tests/test_schedule.cpp`) — Windows 타이머 간격 탓에 **가끔** 실패. 고치지 않는다. 2026-10-09 전체 실행에서는 통과.
- `[real]` 1건 건너뜀(실제 모델 없음).
- 빌드 경고는 `fopen` · `mirrored` 둘만 기존. 그 밖의 새 경고는 실패로 본다.
- Python 3.13은 있으나 **ezdxf 없음** → `tests/dxf_audit.py`는 못 돈다. DXF 변경은 Catch2 시험만으로 검증하고 그 사실을 적는다(설치는 사용자 허락 뒤 — `pip --user` 금지, 데스크톱 앱이 가로채므로 터미널에서).
- 전역 Stop 훅(`~/.claude/hooks/verify_on_stop.py`)은 `.claude/cpp-project.json`의 **본 저장소** 빌드(`C:/dev/kerf-v4c`)를 돈다. worktree 변경은 검증하지 않으므로 팩토리는 `verify.ps1`로 따로 검증한다.
- **`C:\dev\kerf-v4c` 는 기준선이 아니다.** 본 저장소의 미커밋 변경(다른 세션 작업 중인 것 포함)이 그대로 빌드된다. 2026-10-09 17:43 빌드는 아이콘 교체 작업 중이라 `--ui-audit` 가 **모든 크기에서** `icons-missing=33 → RESULT fail`(1920x1040 포함) — 해상도 문제가 아니라 진행 중 변경 탓. 기준선은 언제나 **깨끗한 커밋의 worktree 빌드**(`factory new` + `factory verify`)로 잰다.
- **해상도 · 배율 기준선(2026-10-09, HEAD `bc0aa32` worktree):** `--ui-audit` 1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × `QT_SCALE_FACTOR` 1 · 2 → **8가지 모두 RESULT ok**(각 62–66 s, offscreen). 주의: 이 감사는 위젯 자리 규칙(겹침 · 아이콘 · 툴팁)만 보고 픽셀 배치 · 글자 잘림은 보지 않으므로, 화면 변경은 캡처(`shot.ps1`, 크기별)를 `kerf-ui-reviewer`가 본 뒤에야 「안정」이라고 말한다.
- git 자격 증명은 `kwonyoungin11`, `gh`는 `kesekikwon-netizen`. 푸시는 사용자가 말할 때만, 명령은 DEV_PROGRESS §4 「푸시 해결」.

## 10. 브랜치 · 작업 트리 (2026-10-09)
- 기본 브랜치(origin/HEAD) `main` = 옛 1.2.1. **개발 · 병합 대상은 `feature/design-v2`**(`origin/feature/design-v2`와 같음, CI 통과 중).
- 본 저장소 `C:\Users\권을\Documents\desc\desca`에 사용자 미커밋 변경(`CLAUDE.md` · `docs/DEV_PROGRESS.md` · `.claude/` · `factory/`)이 있을 수 있다. **정리 · 삭제 · 리셋 금지.** `stash@{0}` 건드리지 않음.
- 팩토리 브랜치 `factory/<ID>` 는 기준 커밋(job.md `base_sha`)에서 갈라지며 worktree `C:\dev\kerf-wt\<ID>`에만 체크아웃된다. 커밋은 이 브랜치에서만.

## 11. superpowers 스킬 · context7 (사용자 지시 2026-10-09: 「안정적이게 개발에 이용할 수 있게」)
단계마다 쓰는 스킬을 고정한다(Skill 도구로 부른다. 스킬이 안 뜨면 그 사실을 기록하고 절차를 손으로 따른다 — 건너뛰지 않는다).
| 팩토리 단계 | 스킬 | 누가 |
| --- | --- | --- |
| queued → spec(요청이 모호하거나 설계가 갈릴 때) | `superpowers:brainstorming` | 스킬(사용자와) |
| spec(3개 넘는 파일 · 모듈 추가 · 구조 변경) | `superpowers:writing-plans` → 계획을 `spec.md` §3–§4에 녹임 | spec-writer |
| build | `superpowers:test-driven-development`(실패하는 시험 먼저) · 막히면 `superpowers:systematic-debugging` | builder |
| verify · 「끝났다」 말하기 전 | `superpowers:verification-before-completion` | test-runner · 스킬 |
| review | `superpowers:requesting-code-review`(브리프) · rework 때 `superpowers:receiving-code-review`(검토 지적을 검증한 뒤 반영) | 스킬 · builder |
| worktree | `superpowers:using-git-worktrees` 대신 **`factory new`**(ASCII 경로 `C:\dev\kerf-wt` 가 필요해서). 원칙은 같다 | 스킬 |
| approve → merge | `superpowers:finishing-a-development-branch` 의 점검표만(자동 병합 · 삭제는 하지 않음, SKILL §5) | 스킬 |

**context7**(MCP, 도구 `mcp__face6271-b51c-4ff4-9963-650e1c04ccfd__resolve-library-id` → `…__query-docs`): Qt · Catch2 · CMake 같은 **외부 API 를 쓰기 전에** 문서를 확인한다(기억으로 쓰지 않는다). 2026-10-09 확인: Qt 6 문서 ID `/websites/doc_qt_io_qt-6_8`(이 PC 는 Qt 6.10이지만 6.8 문서가 가장 가깝다 — 6.9+ 전용 API 는 `C:\Qt\6.10.3\msvc2022_64\include` 헤더로 확인). 질의는 한 번에 한 주제, 3번 넘게 부르지 않는다. 결과는 `spec.md` · `build.md`에 「출처: context7 <ID> <질의>」로 적는다. 도구 이름 앞의 UUID 는 이 PC 의 커넥터 ID 라 다른 PC 에서는 다를 수 있다 — 안 보이면 ToolSearch 로 `context7` 또는 `query-docs`를 찾는다.

## 12. 기록 보존
- 승인 근거 = `factory/jobs/<ID>/*.md`(Git). 큰 로그 · 빌드 결과 · 캡처는 `C:\dev\kerf-wt\<ID>-build\logs\`(미커밋, 이 PC). 그래서 `verify.md` 요약 줄에 통과 수 · 기준값 줄 · RESULT · 종료 코드가 들어가야 한다.
- 디자인 v4/v5 단계와 관련된 작업은 `docs/DEV_PROGRESS.md` §3 · §4 · §5 도 함께 고친다(저장소 규칙).
