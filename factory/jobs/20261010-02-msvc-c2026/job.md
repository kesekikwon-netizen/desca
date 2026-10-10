---
id: 20261010-02-msvc-c2026
title: MSVC 빌드 고침: theme.hpp QSS 원시 문자열 16 KB 아래로(C2026)
status: verify            # queued | spec | build | verify | review | rework | needs-human | approved | merged | closed | rejected
rework_count: 0           # 최대 2. 넘으면 needs-human
ai_review: pending        # pending | pass | fail  (검토자 셋·UI 검토 결과 종합)
user_approval: pending    # pending | approved | rejected  (사용자가 /factory approve 로 말한 뒤에만 approved)
base_branch: feature/design-v2
base_sha: 373c0e7dee7c05a9d71689ef8272f73305fefc82
branch: factory/20261010-02-msvc-c2026
worktree: C:\dev\kerf-wt\20261010-02-msvc-c2026
build_dir: C:\dev\kerf-wt\20261010-02-msvc-c2026-build
head_sha: 044ab95
last_verify: PASS (2026-10-10 18:11)
human_review_required:    # 사람 검토 필수 영역에 해당하면 그 이유(좌표계·단위·수직 기준·수치 알고리즘 / 파일 형식·호환 / 원본 덮어쓰기·삭제 / 빌드·의존성·패키징·CI / 비밀정보·권한)
created: 2026-10-10 17:52
updated: 2026-10-10 18:11
---
# 20261010-02-msvc-c2026 — MSVC 빌드 고침: theme.hpp QSS 원시 문자열 16 KB 아래로(C2026)

## 요청 (사용자 말 그대로)
(점검 E13 · 사용자 결정 무관 고장, R1.5) GitHub Actions Windows 작업이 MSVC 컴파일 오류 theme.hpp(319,1) error C2026 string too big 로 실패 — R0

## 상태 기록
| 때 | 상태 | 누가 | 메모 |
| --- | --- | --- | --- |
| 2026-10-10 17:52 | queued | factory | 작업 생성. 기준 feature/design-v2 @ 373c0e7dee7c05a9d71689ef8272f73305fefc82 |

| 2026-10-10 12:40 | spec → build | spec-writer → factory | spec.md AC1–AC5(MSVC 빌드 exit 0 · clang-cl V8 PASS · QSS 바이트 동일 증명 · 조각 ≤ 8000 바이트 + 규칙 주석 · 캡처 같음). 빈 줄을 `)" R"(` 로 바꿔 나눔(121 · 201 · 265행 권장). 열린 질문 없음 · 사람 검토 필수 아님. RED 는 팩토리가 이미 실행(MSVC exit 1, C2026 하나 — C:/dev/tmp/r0-msvc-build.txt). builder 시작 |

| 2026-10-10 13:20 | build → verify · review | builder → factory | 커밋 `044ab95`(theme.hpp 6+/3−, 빈 줄 셋 → `)" R"(` · 규칙 주석). builder: MSVC 증분 빌드 exit 0 · C2026 0 · clang-cl V8 PASS · QSS 바이트 같음(len 20241 · 343) · 조각 최대 6783 · 캡처 work/start/draw 해시 같음. test-runner(MSVC 깨끗한 빌드 포함) + code-reviewer 시작. **판정:** 화면 불변(캡처 해시 같음) · 좌표 · 입력 · 파일 무관이라 correctness · security · kerf-ui 검토는 해당 없음 |

## 참조
- 명세 `spec.md` · 구현 요약 `build.md` · 검증 `verify.md` · 검토 `review.md` · 승인 `approval.md`
