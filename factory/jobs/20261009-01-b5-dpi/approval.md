# 승인 기록 — 20261009-01-b5-dpi 디자인 v5 B5 해상도 · DPI 안정성(4K · 와이드 · 배율, Windows만)

> **⚠ 사람 검토 필수 영역 해당:** ① CI(`.github/workflows/ci.yml` — Linux 작업 삭제 · 감사 판정 방식 변경 · 4K 200 % 추가) ② 화면 · 감사(`app/main.cpp` · `app/mainwindow_ui.cpp` — `--ui-audit` 기대값 추가). 자동으로 approved 로 가지 않습니다. 승인하려면 `/factory approve 20261009-01-b5-dpi` 라고 말씀해 주세요(병합은 따로 「병합」).

## 승인 보고서 (AI 검토 통과 뒤 스킬이 작성, 사용자에게 보여 준 것 — 2026-10-09 23:50)
- **변경 요약:** 디자인 v5 B5. ① `--ui-audit` 가 해상도 · 배율을 판정(env: 요청 크기 그대로 · 창 최소 폭 ≤ 1280 · 보이는 단추 창 밖 없음 · dpr == `QT_SCALE_FACTOR` · 아이콘 1.25/1.75 장 픽셀 동일; guide-step2 `dpr-rerender`; narrow `plan-fits` · `section-fits` · `zoom-kept`) ② `--size 1280x800` 이 1591 로 열리던 원인(홈 한 줄 라벨 넷이 최소 폭을 밀어냄) 고침 ③ 아이콘 배율 1 · 1.25 · 1.5 · 1.75 · 2 · 3(`kerf::iconDprs()` 한 곳) ④ 안내 칩이 `QEvent::DevicePixelRatioChange` 를 받으면 아이콘 다시 굽기(+ `place()` 안전판) ⑤ 창 크기가 바뀌면 「맞춤 상태」의 평면 · 단면만 다시 맞추고 사용자가 확대 · 이동한 보기는 축척 유지(`fitted_`) ⑥ CI: Linux 작업 삭제(Windows 만), 감사를 끝까지 기다려 판정, 1920×1040 ×1 + 3840×2160 ×2 ⑦ DESIGN_SPEC §2.1 여섯 장 · B5 규칙 세 줄.
- **변경 파일(`git diff --stat 0f7fbfe..66737c5`, 13개 · +189 −44):** `.github/workflows/ci.yml`(46) · `app/mainwindow_ui.cpp`(83) · `app/planview.cpp`(40) · `app/guideband.cpp`(16) · `app/sectionview.cpp`(15) · `app/icons.cpp`(10) · `app/guideband.hpp`(6) · `app/planview.hpp`(4) · `app/icons.hpp`(3) · `app/sectionview.hpp`(3) · `docs/design/DESIGN_SPEC.md`(3) · `app/main.cpp`(2) · `app/ribbon.cpp`(2). core · tests · CMake 변경 없음.
- **커밋(기준 `0f7fbfe` → 7개):** `b5640c0` 시험(RED) → `30a8413` --size 크기 → `c9b081e` 아이콘 dpr → `504e6c1` 안내 칩 → `73493e2` 자동 맞춤 → `066455a` CI → `66737c5` env 실제 화면 판정 범위.
- **완료 기준별 검증 결과**(verify.md 23:11 · 23:14 블록, test-runner 독립 실행 + 팩토리 AC10 재현):

| AC | 결과 | 근거 |
| --- | --- | --- |
| AC1 요청 크기 · 최소 폭 · 넘침 · 배율 | 통과 | 8조합 `env size=want … min-width≤1280 overflow=0` FAIL 없음(RED: 1280 `size=1591x800`) |
| AC2 아이콘 1.25 · 1.75 | 통과 | 8조합 + 1.25/1.75 `icon-dpr-bad=0`(RED 2) |
| AC3 화면 옮김 다시 굽기 | 자동 통과 · 수동 미실행 | `dpr-rerender=1`(RED 0); 모니터 둘 눈 확인은 이 PC 한 화면이라 못 함 |
| AC4 창 줄일 때 자동 맞춤 · 사용자 보기 유지 | 통과 | `plan-fits=1 section-fits=1 zoom-kept=1`(RED 0/0) + 실제 화면 AC10 재현 동일 |
| AC5 고정 기준(8조합 ok · 새 경고 0 · 빠른 시험 · 기준값) | 통과 | V8 `RESULT PASS`: 빌드 exit 0 · 경고 새 것 0 · 51325 assertions · `polylines=1 vertices=54` · ui-audit 8× `RESULT ok` |
| AC6 1.25 · 1.75 배율 전 과정 | 통과 | `-UiScales 1.25,1.75` PASS(`dpr=1.25` · `1.75`) |
| AC7 회귀 없음 | 통과 | `--icon-sheet` n=62 missing=0 · `--sheet-check` ok · `--ctx-shot` guide=1 · core/tests/CMake diff 비어 있음 |
| AC8 CI | ①② 통과(로컬 모의 양성 2 · 음성 1) · ③ 미실행 | GitHub Actions 는 push 전. **ci.yml diff 는 사람 검토** |
| AC9 캡처 루브릭 4점 이상 | **1280 미충족(전부터 있던 것 → 후속 #3c)** · 1920 이상 충족 | kerf-ui-reviewer 2차 표: 1920 · 1920×1.25 · 3440 · 3840 · 홈 1280 모두 4 이상, work/draw 1280 은 2–3점(머리 겹침 · 정보 줄 잘림 · 눈금 겹침) |
| AC10 실제 화면 감사 | 자동 통과 · 사람 절차 수동 | 팩토리 재현 `EXIT=0 RESULT ok`(`plan-fits=1 section-fits=1 zoom-kept=1`, GL 뜸); 창 끌어 줄이기 · 휠 뒤 넓히기는 사용자 확인 예정 |
| AC11 문서 · 포터블 제약 | 통과 | DESIGN_SPEC · icons.cpp 여섯 장; `+` 줄에 `C:\` · `HKEY_` · `NativeFormat` 0건 |

- **실패 · 미실행 항목과 이유:** 이번 변경 탓 실패 **없음**. 미실행: AC3 수동(모니터 둘 없음) · AC8 ③(push 전) · AC10 사람 절차(사용자 몫). 미충족: AC9 1280(전부터 있던 좁은 창 문제 — 사용자 결정으로 후속 작업 #3c).
- **독립 검토 결과:** code-reviewer **pass**(should 3 · note 4) · correctness-reviewer **pass**(should 2 · note 6) · security-reviewer **pass**(note 4) · kerf-ui-reviewer 1차 **fail**(blocking 2) / 2차 **pass**(should 4 · note 5) → 충돌을 사용자가 해소(1280 문제는 후속, 리본 1280 은 스펙을 20 으로). **종합 ai_review: pass**. 받아들인 should/note 와 남은 위험은 `review.md` 「종합」 — 모두 후속 작업 #3c 로(병합을 막지 않음): `setVerticalExaggeration` 뒤 맞춤 상태, 넘침 판정 1 px, `contentFits` 식 중복, 칩 dpr 판정 판별력, 1280 에서 `zoom-kept` 실효성, 1280 · 3440 의 ×1.25/×1.75 감사, 홈 라벨 줄임표.
- **그 밖에 알아둘 것:** builder 가 Task 2 커밋 메시지(BOM) 교정으로 자기 브랜치 역사를 재적재(push 전 · `factory/…` 브랜치만 · diff 동일). 이번 작업이 창을 1280 으로 **열 수 있게** 했으므로, 1280 에서 머리 글 겹침이 사용자 눈에 띄기 쉬워짐 — #3c 를 D2 보다 먼저 돌릴 것을 권함.
- **승인 대상 커밋 SHA:** `66737c5` (기준 `0f7fbfed7455c0c6db308af386ae935b27d6f77e` → 변경 `66737c515c4deac0b4ba08fbc4ff75af9f0252eb`), worktree `git status --porcelain` 비어 있음.
- **GitHub Actions(기준 커밋):** `ci-status.ps1 -Sha 0f7fbfe` → `completed failure` — **Windows · 앱 전체 = failure**, Linux = success(https://github.com/kesekikwon-netizen/desca/actions/runs/37937880747). 이 브랜치와 무관한 기존 실패(원인 미조사 — push 뒤 Actions 로그로 따로). 통과로 쓰지 않음.
- **되돌리기 방법:** 병합 전 = 브랜치 `factory/20261009-01-b5-dpi` 를 병합하지 않음(worktree 삭제) · 병합 뒤 = `git revert -m 1 <병합 커밋>`.
- **직접 확인하는 방법:** `C:\dev\kerf-wt\20261009-01-b5-dpi-build\app\SectionViewer.exe` 로 `C:\dev\kerf-synth\Synthetic.3mx` 를 열어(PATH 앞에 `C:\Qt\6.10.3\msvc2022_64\bin`) 창 오른쪽 아래 모서리를 끌어 1280 근처까지 줄임 → 평면 · 단면 그림이 잘리지 않음; 휠로 확대한 뒤 창을 넓혀도 확대가 풀리지 않음. 캡처 `C:\dev\tmp\b5-shots\`(work-1280 · work-1920 · work-1920-x1.25 · work-1920-x2 …).

## 사용자 승인 (사용자가 /factory approve 20261009-01-b5-dpi 라고 말한 뒤에만)
| 때 | 사용자 말(그대로) | 승인 SHA | 결과 |
| --- | --- | --- | --- |
| 2026-10-09 23:58 | 「승인 (추천)」 · 「병합까지 진행 (추천)」(AskUserQuestion 선택 — 질문: 「B5 작업 20261009-01-b5-dpi 를 승인할까요? (승인 대상 커밋 66737c5)」 · 「이어서 feature/design-v2 에 병합까지 할까요?」) | `66737c5` | approved. 병합은 SKILL §5 순서(최신 브랜치 merge → 전체 검증 → 재승인 → `--no-ff`)로 이어감, push 없음 |

## 승인 뒤 변경
- 승인 뒤 코드가 바뀌면 여기 적고 다시 검증 · 다시 승인(위 표에 새 줄).

## 병합 (사용자가 따로 「병합」을 말한 뒤에만)
- 최신 대상 브랜치와 통합한 커밋 · 다시 돈 검증(verify.md 블록 시각):
- 병합 커밋 SHA · 때:
- GitHub Actions(push 뒤, `factory ci <병합 커밋> -Wait` 결과 줄 그대로):
