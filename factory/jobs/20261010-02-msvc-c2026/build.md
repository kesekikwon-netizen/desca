# 구현 요약 — 20261010-02-msvc-c2026 MSVC 빌드 고침: theme.hpp QSS 원시 문자열 16 KB 아래로(C2026)

작성: builder · worktree `C:\dev\kerf-wt\20261010-02-msvc-c2026` · 브랜치 `factory/20261010-02-msvc-c2026`

## 바꾼 것
| 파일 | 무엇을 | 왜(완료 기준 번호) |
| --- | --- | --- |
| `app/theme.hpp` | `styleSheet()` 첫 `QStringLiteral(R"( … )")` 안의 빈 줄 3개(기준 커밋 121 · 201 · 265행)를 `)" R"(` 만 있는 줄로 바꿈 → 4조각 4 096 · 6 783 · 5 208 · 4 154 바이트. 원시 문자열 위에 규칙 주석 3줄(상한 8 000 바이트 · 빈 줄에서 나누는 법 · 출처 MS 문서 C2026) | AC1 · AC3 · AC4 |

diff: `git diff 373c0e7 HEAD --numstat` → `6 3 app/theme.hpp`. 바뀐 덩어리는 「주석 3줄 추가」 + 「빈 줄 → `)" R"(`」 3곳뿐. `git diff --ignore-cr-at-eol --stat` 도 같은 6+/3−(줄바꿈만 바뀐 파일 없음. worktree 작업 사본은 원래 LF 라 git 이 「LF will be replaced by CRLF」 안내만 냄).

## 커밋
| SHA | 메시지 |
| --- | --- |
| `044ab95` | R0 MSVC C2026 고침: theme.hpp QSS 원시 문자열을 빈 줄 3곳에서 4조각(≤8000 바이트)으로 나눔 · AC1–AC5 |

## RED → GREEN
- **RED(기준):** `cmake --build C:\dev\kerf-wt\20261010-02-msvc-c2026-msvc --config Release --parallel` → exit 1, `theme.hpp(319,1): error C2026` (번역 단위 7개에서 반복), 로그 `C:\dev\tmp\r0-msvc-build.txt`.
- **GREEN(고친 뒤, 같은 폴더 · 같은 명령, TEMP/TMP=C:\dev\tmp):** exit 0. 로그 `C:\dev\kerf-wt\20261010-02-msvc-c2026-build\logs\msvc-build.txt` — `error` 0줄 · `C2026` 0줄 · `warning` 0줄. 링크 결과: `archsection.lib` · `asec-info.exe` · `asec-section.exe` · `asec-make-synthetic.exe` · `asec_tests.exe` · `app\Release\SectionViewer.exe` (+ z.dll 복사).

## 완료 기준별 근거
- **AC1 (MSVC 빌드):** 위 GREEN. 주의(과장 금지): 이번 빌드는 증분이라 `theme.hpp` 를 포함한 번역 단위만 다시 컴파일됐다. 그 단위들에서 `warning C` 는 하나도 나오지 않았다. 기존 `tests\test_raster.cpp(48,11) warning C4551` 은 그 파일이 바뀌지 않아 다시 컴파일되지 않았으므로 이번 로그에 다시 나오지 않았을 뿐, 없어진 것이 아니다(기준 로그에 있음). 구성(configure)은 기존 폴더(이미 구성됨, 설정 불변)를 그대로 썼다.
- **AC2 (clang-cl · 시험 · 감사):** `factory.cmd verify 20261010-02-msvc-c2026 -UiAudit on -UiSizes 1280x800,1920x1040,3440x1440,3840x2160 -UiScales 1,2` → **RESULT PASS** (전체 출력 `C:\dev\tmp\r0-verify-after.txt`, 기록 `verify.md`).
  - build exit 0 · 새 경고 0개(기존 CRT deprecated · fopen · mirrored 만)
  - tests(quick) 51325 / 51325 passed
  - asec-section `polylines=1 vertices=54`
  - ui-audit 1280x800 · 1920x1040 · 3440x1440 · 3840x2160 × 배율 1 · 2 → 8개 모두 `RESULT ok`(각 66–69 s). square-borders 항목이 다시 이은 `styleSheet(false)` 를 실제로 파싱함.
  - ci(base) WARN exit 3 — 373c0e7 은 push 전이라 Actions 실행 없음(예상된 것).
- **AC3 (QSS 바이트 동일):** 일회성 스크립트 `C:\dev\tmp\r0\ac3.py`(커밋 안 함, `python.exe -I`)가 `git show 373c0e7:app/theme.hpp` 와 새 파일의 `styleSheet` 안 `QStringLiteral(` 호출마다 원시 문자열 조각을 이어 붙여(CR 제거) 비교. 출력 그대로(`C:\dev\tmp\r0\ac3-out.txt`):
  ```
  normal equal=1 len_old=20241 len_new=20241 sha_old=d6f4b7afacf536f1 sha_new=d6f4b7afacf536f1
    normal pieces(new) bytes=[4096, 6783, 5208, 4154] max=6783 le8000=1
  hc equal=1 len_old=343 len_new=343 sha_old=2e77a2c589bb4a2c sha_new=2e77a2c589bb4a2c
    hc pieces(new) bytes=[343] max=343 le8000=1
  calls old=2 new=2
  ```
  (sha 는 SHA-256 앞 16자리.) diff 모양 확인(①)은 위 「바꾼 것」.
- **AC4 (조각 상한 · 규칙 주석):** 조각 최대 6 783 바이트 ≤ 8 000(위 출력). 주석 3줄이 원시 문자열 바로 위에 있음(상한 · 빈 줄을 `)" R"(` 로 바꾸는 법과 빈 줄이 아닌 곳에서 나누면 줄바꿈이 늘어난다는 이유 · 출처 「Compiler Error C2026」).
- **AC5 (화면 같음):** 고치기 전(기준 커밋 clang-cl 빌드, `verify -UiAudit off` 로 만든 `…-build\app\SectionViewer.exe`) · 고친 뒤(AC2 verify 가 다시 빌드한 같은 exe) 각각 `shot.ps1 -Scene work|start|draw -Size 1920x1040` → `C:\dev\tmp\r0-shots\before-*.png` · `after-*.png`. SHA-256(앞 16자리):
  | 장면 | 전 | 뒤 | 같음 |
  | --- | --- | --- | --- |
  | work | 33A6B62FF7214327 | 33A6B62FF7214327 | 1 |
  | start | 4889DED4F306F69E | 4889DED4F306F69E | 1 |
  | draw | 5749B92C48E6BDA4 | 5749B92C48E6BDA4 | 1 |
  세 장 모두 바이트 동일 → kerf-ui-reviewer 판정 불필요(명세 AC5 「같으면 끝」).

## 출처
- 새 Qt · Catch2 · CMake API 를 쓰지 않았다. 이웃 문자열 잇기는 C++ 언어 규칙(번역 6단계)이고, `QStringLiteral(str)` → `u"" str` 펼침은 명세가 Qt 헤더 `C:/Qt/6.10.3/msvc2022_64/include/QtCore/qstring.h:1777 · :1789` 로 확인해 두었다. 조각 한도 근거는 명세 §4(context7 `/microsoftdocs/cpp-docs` 「Compiler Error C2026」). 그래서 이번 구현에서 context7 을 새로 조회하지 않았다.

## 명세와 다르게 한 것 (판정)
- 캡처를 명세의 `…-build\logs\shots\` 대신 `C:\dev\tmp\r0-shots\` 에 둠 — 조정자 지시 — 잘못이면: 검토자가 다른 폴더에서 찾아야 함(내용 같음).
- MSVC 구성(configure)을 다시 돌리지 않음 — 폴더가 이미 같은 설정으로 구성돼 있었음 — 잘못이면: 없음(빌드 명령은 명세 그대로).
- AC1 경고 목록은 증분 빌드 범위만 보여 줌(위 AC1 주의) — 깨끗한 전체 MSVC 재빌드는 하지 않음 — 잘못이면: theme.hpp 와 무관한 MSVC 경고 목록이 이번 로그에 없음(기준 로그 `r0-msvc-build.txt` 에 C4551 하나).

## 검증 방법 변경
- 없음(시험 기대값 변경 없음, 새 시험 없음 — 명세 §4).

## 수정 회차 기록
- 1회: 최초 구현 `044ab95`. 다른 MSVC 오류는 나오지 않음(위험 1 해당 없음).
- 2회:
