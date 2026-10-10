# 명세 — 20261010-02-msvc-c2026 MSVC 빌드 고침: theme.hpp QSS 원시 문자열 16 KB 아래로(C2026)

작성: spec-writer · 기준 커밋 373c0e7dee7c05a9d71689ef8272f73305fefc82 (worktree `C:\dev\kerf-wt\20261010-02-msvc-c2026` HEAD 와 같음, `app/theme.hpp` 는 본 저장소와 내용 같음 — `diff` 로 확인)

## 1. 목표 (1–3줄)
GitHub Actions Windows 작업(Qt 6.8.3 + MSVC)이 `app\theme.hpp(319,1): error C2026: string too big` 으로 빌드부터 실패한다(`docs/REVIEW_2026-10-10.md` E13 · §9 R0). `theme::styleSheet()` 의 긴 QSS 원시 문자열을 MSVC 한도 아래 조각들로 나눠 **MSVC 빌드를 통과**시킨다. QSS 내용(화면 모양)은 한 글자도 바뀌지 않는다.

## 2. 변경 범위
- **바꾸는 파일: `app/theme.hpp` 하나**, 그중 `inline QString styleSheet(bool highContrast = false)`(71–330행 근처) 안의 첫 `QStringLiteral(R"( … )")`(72–319행, 본문 약 20 240 바이트) 만.
  - **나누는 규칙(필수, 바이트 동일을 언어 규칙으로 보장하는 방법):** QSS 안에 이미 있는 **빈 줄 하나를 `)" R"(` 만 있는 줄로 바꾼다.** 그러면 앞 조각은 `…\n` 으로 끝나고 뒤 조각은 `\n…` 으로 시작해, 이어 붙인 결과가 원래의 빈 줄을 그대로 다시 만든다. `QStringLiteral(str)` 은 `u"" str` 로 펼쳐지므로(Qt 헤더 `C:/Qt/6.10.3/msvc2022_64/include/QtCore/qstring.h:1777` `#define QT_UNICODE_LITERAL(str) u"" str`, `:1789` `#define QStringLiteral(str) …QT_UNICODE_LITERAL(str)…`) 한 매크로 안에서 이웃 원시 문자열을 나열하면 컴파일러가 번역 6단계에서 하나로 잇는다 — 실행 때 `+` · 새 도우미 함수가 필요 없다.
  - **빈 줄이 아닌 곳에서 나누지 않는다**(줄 사이에 `)" R"(` 를 끼우면 `\n` 이 하나 더 생겨 「한 글자도 바뀌지 않음」이 깨진다). 빈 줄에서 나누면 `%CHECK%`(197행 근처) · `%ARROW%`(204행 근처) 자리표시가 잘릴 일도 없다.
  - **조각 크기 상한: 조각마다 8 000 바이트 이하**(이유는 §4 「허용오차」). 권장은 6 000–7 000 바이트 이하로 여유를 둔다.
  - 나눌 수 있는 빈 줄과 누적 바이트(기준 커밋, CR 뺀 값, 73행부터 셈 — 72행 `R"(` 바로 뒤 줄바꿈 1바이트는 별도): 89행 1 188 · 108행 3 024 · 121행 4 096 · 132행 4 865 · 138행 5 372 · 152행 6 595 · 161행 7 211 · 172행 8 404 · 176행 8 835 · 182행 9 256 · 190행 9 805 · 201행 10 879 · 208행 11 450 · 212행 11 738 · 215행 11 827 · 226행 12 596 · 265행 16 087 · 270행 16 340 · 273행 16 512 · 280행 16 961 · 312행 19 742 · 끝 20 240. (명령: `sed -n 73,318p app/theme.hpp | tr -d '\r' | awk '{n+=length($0)+1; if($0=="") print NR+72, n}'`)
  - **예시 분할(권장):** 121 · 201 · 265행 → 4조각 약 4.1 k · 6.8 k · 5.2 k · 4.2 k 바이트. builder 가 다른 빈 줄을 골라도 되지만 모든 조각이 상한 아래여야 하고, 고른 행과 조각 크기를 `build.md` 에 적는다.
  - 원시 문자열 바로 위에 **주석 2–3줄**을 단다: 「MSVC C2026 — 원시 문자열 한 조각은 8 000 바이트 이하. 늘어나면 빈 줄 하나를 `)" R"(` 줄로 바꿔 나눈다(빈 줄이 아닌 곳에서 나누면 줄바꿈이 하나 더 생김). 출처: Microsoft C++ 문서 C2026」. 주석 문구는 builder 재량, 위 세 내용(상한 · 나누는 법 · 이유)은 필수.
- **바꾸지 않는 것(명시):**
  - QSS 의 선택자 · 속성 · 색 · 크기 · 공백 · 줄 순서 — 한 바이트도.
  - 고대비 조각(321–326행, 342 바이트)과 `s.replace(%CHECK%/%ARROW%)` 줄(327–328행), `checkImagePath` · `arrowImagePath`, `styleSheet` 의 시그니처 · 반환.
  - `theme.hpp` 의 다른 부분(토큰 · `Ico` · `iconName`), 다른 파일 전부. **`CMakeLists.txt` · `app/CMakeLists.txt` · `.github/workflows/ci.yml` 은 건드리지 않는다**(CMake 단계의 크기 검사도 넣지 않음 — §6 판단 참고).
  - 새 명령줄 옵션 · 새 시험 장치 · 제품 코드의 자체 확인 코드 없음.
  - `tests\test_raster.cpp(48,11): warning C4551`(MSVC 에서만 나는 **기존** 경고, 기준 빌드 로그 `C:\dev\tmp\r0-msvc-build.txt`) — 고치지 않고 「새 경고」로 세지 않는다.
- **사람 검토 필수 영역 해당 여부: 없음** — 이유: SKILL §7 목록(좌표계 · 수치 알고리즘 / 파일 형식 · QSettings / 원본 덮어쓰기 / 공통 빌드 설정 · CI(`CMakeLists.txt` `app/CMakeLists.txt` `.github/` `build-*.sh` `packaging/`) / 비밀정보) 중 어디에도 닿지 않는다. MSVC 를 고치는 일이지만 **빌드 설정 파일은 바꾸지 않고** 소스 한 곳의 문자열 표기만 바꾼다. profile §8 의 「디자인 토큰 · 화면 `app/theme.hpp`」 항목은 토큰 · 값 · 선택자를 바꾸지 않으므로(AC3 이 증명) 걸리지 않고, v5 계획 `docs/superpowers/plans/2026-10-09-design-v5.md` · v6 계획의 어느 Task 와도 겹치지 않는다(QSS 내용을 바꾸는 Task 가 아님).

## 3. 완료 기준 ↔ 검증 방법 (기준마다 하나 이상)
기준(RED) 실행은 이미 있다: 같은 구성으로 `cmake --build … --config Release --parallel` → exit 1, 고유 오류 줄은 `theme.hpp(319,1): error C2026` 하나(7개 번역 단위에서 반복), 로그 `C:\dev\tmp\r0-msvc-build.txt`. 고친 뒤 같은 명령이 GREEN 이어야 한다.

| # | 완료 기준 | 검증 방법(시험 이름 · 명령 · 확인 절차) | 종류 |
| --- | --- | --- | --- |
| AC1 | **MSVC 빌드 성공**: 오류 0 · exit 0(앱 `SectionViewer` · `asec_tests` · 도구 링크까지). C2026 줄 0개 | 같은 폴더에서 다시: `cmake -S C:/dev/kerf-wt/20261010-02-msvc-c2026 -B C:/dev/kerf-wt/20261010-02-msvc-c2026-msvc -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows -DCMAKE_PREFIX_PATH=C:/Qt/6.10.3/msvc2022_64` → `cmake --build C:/dev/kerf-wt/20261010-02-msvc-c2026-msvc --config Release --parallel` exit 0. 로그를 `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\msvc-build.txt` 에 남기고 `grep -c "error"` = 0, `grep "warning C"` 목록을 `build.md` · `verify.md` 에 적는다(기존 C4551 외의 경고가 `theme.hpp` 에서 나오면 실패) | 자동(명령) |
| AC2 | **clang-cl 빌드 · 시험 · 감사 통과**(이 PC 개발 기준이 깨지지 않음): 새 경고 0 · 빠른 시험 통과 · asec-section 기준값 · `--ui-audit` 이 1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × 배율 1 · 2 에서 모두 RESULT ok | `factory.cmd verify 20261010-02-msvc-c2026 -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2` → RESULT PASS. 그 안의 단계: build(새 경고 0, 기존 `fopen` · `mirrored` 만 — profile §9) · tests(`asec_tests "~CoalescingWorker*"`) · asec-section(`polylines=1 vertices=54`) · ui-audit 8조합 `RESULT ok`. `--ui-audit` 의 square-borders 항목(`app/mainwindow_ui.cpp:2388` 이 `theme::styleSheet(false)` 를 파싱)이 바뀐 문자열을 실제로 읽는 연기 시험이 된다 | 자동(`factory verify`) |
| AC3 | **QSS 가 바이트 단위로 같다**(보통 · 고대비 둘 다) | ① `git -C C:/dev/kerf-wt/20261010-02-msvc-c2026 diff 373c0e7 -- app/theme.hpp` 의 바뀐 덩어리가 **「빈 줄 → `)" R"(` 줄」과 추가된 주석 줄뿐**임을 builder · 검토자가 확인(그 밖의 `-`/`+` 줄이 있으면 실패). ② **일회성 확인 스크립트**(제품 코드 밖 — `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\` 또는 scratchpad 에 둠, 커밋 안 함, Python `%LOCALAPPDATA%\Programs\Python\Python313\python.exe`): `git show 373c0e7:app/theme.hpp` 와 새 `app/theme.hpp` 각각에서 `styleSheet` 함수 안 `QStringLiteral(` 호출들의 원시 문자열 본문(`R"(` 와 `)"` 사이, 구분자 있는 `R"x(…)x"` 도 처리)을 뽑아 호출마다 이어 붙인 뒤 CR 을 빼고 두 결과(첫 호출 = 보통 QSS, 둘째 = 고대비 조각)가 **같은지 비교**, 길이와 SHA-256 을 출력. 기대: `normal equal=1 len=20241 …`(72행 뒤 줄바꿈 포함 길이 — 실제 값을 적는다) · `hc equal=1`. 출력 전체를 `build.md` 에 붙인다. 스크립트가 조각마다의 바이트 수도 출력해 AC4 근거로 쓴다 | 자동(일회성 스크립트) + 검토자 diff 확인 |
| AC4 | **조각 크기 상한이 지켜지고, 앞으로 늘어나도 다시 깨지지 않게 규칙이 코드에 남는다**: 모든 원시 문자열 조각 ≤ 8 000 바이트, 원시 문자열 위에 §2 의 주석(상한 · 나누는 법 · 출처) | AC3 ② 스크립트의 조각별 바이트 수 출력(모두 ≤ 8000) + 검토자가 주석을 읽음. **장래의 실제 관문 = MSVC 빌드**(로컬 AC1 명령 · push 뒤 GitHub Actions Windows 작업, `factory.cmd ci <sha>`). `static_assert` 는 쓰지 않는다 — 이유 §4 | 자동(스크립트) + 수동(검토) |
| AC5 | **화면이 전과 같다**: 캡처에서 글자 잘림 · 겹침 없음, 바꾸기 전 캡처와 같음 | builder 가 **고치기 전**(기준 커밋 빌드) 과 **고친 뒤** 각각 `powershell -NoProfile -ExecutionPolicy Bypass -File .claude/scripts/shot.ps1 -Scene work|start|draw -Out <png> -Exe C:\dev\kerf-wt\20261010-02-msvc-c2026-build\app\SectionViewer.exe`(1920x1040, PATH 에 `C:\Qt\6.10.3\msvc2022_64\bin`) 로 3장씩 찍어 `…-build\logs\shots\before-*.png` · `after-*.png` 에 둔다. PNG 를 바이트 비교(`certutil -hashfile … SHA256` 또는 Python) → 같으면 끝. 다르면 kerf-ui-reviewer 가 두 장을 나란히 보고 「차이가 시간 · 커서 등 QSS 와 무관한 것뿐, 글자 잘림 · 겹침 없음」을 판정한다(AC3 이 1차 증명, 캡처는 확인용) | 수동(캡처) + 자동(해시) |

## 4. 회귀 시험 계획
- **새로 쓸 시험: 없음.** 이유: `tests/` 는 Qt 없는 core 시험(`asec_tests`)이라 Qt 헤더인 `theme.hpp` 를 넣을 수 없고, 고장 자체가 「MSVC 가 컴파일을 거부함」이라 RED/GREEN 은 **MSVC 빌드 명령 자체**다(RED = 기준 로그 `C:\dev\tmp\r0-msvc-build.txt` exit 1, GREEN = AC1 exit 0). 바이트 동일은 언어 규칙(이웃 문자열 잇기)으로 보장되므로 제품 코드에 시험 장치(새 명령줄 옵션 · 문자열 덤프)를 더하지 않는다 — **추천: 더하지 않음**(AC3 의 diff 모양 확인 + 일회성 스크립트로 충분, 이미 `--ui-audit` square-borders 가 `styleSheet(false)` 를 읽어 연기 시험 역할).
- **돌려야 하는 기존 시험:** `asec_tests "~CoalescingWorker*"`(빠른 시험, `factory verify` 안) · `--ui-audit` 8조합(AC2) · `asec-section` 기준값. core 를 바꾸지 않으므로 전체 시험(`-Full`)은 필수 아님(원하면 돌려도 됨).
- **수치 허용오차:** 해당 없음(수치 계산 없음). 대신 **조각 크기 상한 8 000 바이트**의 근거 — 출처: context7 `/microsoftdocs/cpp-docs` 「Compiler Error C2026」: 「이웃 문자열이 이어지기 전, 한 문자열은 16 380 단일 바이트 문자를 넘을 수 없다」 · 「그 절반쯤 길이의 유니코드 문자열도 이 오류를 낸다」. 조각 자체는 접두사 없는 원시 문자열이지만 `QStringLiteral` 이 `u""` 를 붙여 결과가 UTF-16 이므로, MSVC 가 어느 쪽으로 세든 걸리지 않게 절반(약 8 190) 아래인 8 000 을 상한으로 한다. 같은 문서의 「잇고 난 뒤 65 535 바이트 한도」는 VS 2022 17.0 이전 것이고, 지금 합계도 약 20 240 자 × 2 = 약 40 KB 라 그 아래다. 같은 문서의 「MSVC 14.51 부터 조각 한도 없음」은 이번에 쓰는 도구에 해당하지 않는다 — 이 PC 의 VS 2022(msbuild 17.14.51) 실행에서 실제로 C2026 이 났다. 이 PC MSVC 도구 집합의 정확한 번호(14.xx)와 GitHub windows-2022 러너의 번호는 **확인하지 못함**.
- **`static_assert` 를 쓰지 않는 이유(AC4):** 한 `QStringLiteral(...)` 안의 이웃 원시 문자열들은 번역 6단계에서 이미 한 문자열이 되므로 조각별 `sizeof` 를 잴 수 없다. 조각마다 재려면 조각을 `constexpr char` 배열 같은 이름 있는 상수로 빼고 실행 때 잇는 구조 변경이 필요해(=「표기만 바꿈」 범위를 넘음) 이번에는 하지 않는다. CMake 단계 크기 검사도 `CMakeLists.txt`(SKILL §7 사람 검토 영역)를 건드려야 해서 넣지 않는다. 대신 코드 주석(규칙) + MSVC 빌드(로컬 · CI)가 관문이다.

## 5. 위험 · 되돌리기
- **위험 1 — 다음 MSVC 전용 고장이 드러날 수 있음:** 기준 MSVC 실행은 컴파일에서 멈춰 링크 · `windeployqt` · CI 의 시험 · `--ui-audit` 단계가 MSVC 로 한 번도 끝까지 돈 적이 없다. C2026 을 고친 뒤 AC1 에서 **다른** 오류(컴파일 · 링크)가 나오면 builder 는 **이 작업에서 고치지 않고** 오류 줄 · 파일 · 로그 경로를 `build.md` 에 적고 멈춘다(범위 = `theme.hpp` 의 C2026 만). 조정자가 새 작업으로 나눌지 정한다. CI 의 `qoffscreen.dll` 자리 문제(점검 문서 B6 → R22)는 이 작업 범위가 아니다.
- **위험 2 — 잘못 나눠 줄바꿈이 하나 더 생김:** 빈 줄이 아닌 곳에서 나누면 QSS 에 `\n` 이 하나 늘어난다(화면에는 영향 없지만 「한 글자도」 위반). AC3 ①②가 잡는다.
- **위험 3 — clang-cl 에서는 여전히 통과하므로 회귀를 늦게 앎:** 앞으로 QSS 가 한 조각에 8 000 바이트를 넘게 커져도 이 PC 빌드는 통과한다. 주석(AC4)과 CI Windows 작업이 막는다 — CI 는 push 뒤에만 돈다(push 는 사용자 지시 때만).
- **위험 4 — push 전에는 GitHub 초록불을 못 봄:** 이 작업의 완료 판정은 로컬 MSVC 빌드(AC1)다. 병합 · push 뒤 `factory.cmd ci <병합 커밋 sha> -Wait` 로 Windows 작업 결과를 따로 확인한다(SKILL §5-6). 빌드 단계를 지난 뒤 CI 의 다른 단계(감사 · offscreen 플러그인)가 실패하면 그것은 R22 몫이다.
- 되돌리기: 브랜치 `factory/20261010-02-msvc-c2026` 를 병합하지 않으면 끝. 병합 뒤라면 `git revert <병합 커밋>`. MSVC 구성 폴더 `C:\dev\kerf-wt\20261010-02-msvc-c2026-msvc` 는 기록 확인 뒤 지워도 되는 빌드 결과물(사용자 확인 뒤).

## 6. 열린 질문 (사용자에게 물을 것)
- 없음. (판단해 둔 것: 제품 코드에 시험 장치를 더하지 않음 — 언어 규칙으로 같음이 보장되고 일회성 스크립트 + diff 모양으로 증명 / `static_assert` · CMake 검사 대신 주석 + MSVC 빌드를 관문으로 / C2026 뒤 다른 MSVC 오류가 나오면 이 작업에서 고치지 않고 보고.)
