# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) and any other AI coding agent working in this repository.

## 먼저 할 일 (모든 AI · 모든 계정)
- 지금 진행 중인 개발은 **Kerf 디자인 v4**다. 작업을 시작하기 전에 `docs/DEV_PROGRESS.md`를 끝까지 읽고 「다음 할 일」부터 이어서 한다.
- **단계 하나를 마칠 때마다 `docs/DEV_PROGRESS.md`를 고친다**(상태 표 · 기록 · 다음 할 일). 기록 없이 다음 단계로 가지 않는다.
- 지시서는 `docs/design-v4/FINAL_PLAN.md`, 단추 하나하나의 계약은 `docs/design-v4/boards/Buttons1.dc.html` · `Buttons2.dc.html`(HTML을 글로 읽는다).

@docs/DEV_PROGRESS.md

## 이 저장소의 규칙 (요약 — 자세한 것은 FINAL_PLAN §4)
- C++17 · Qt 6 Widgets. `Q_OBJECT` 금지(moc 없이 빌드 — MinGW 교차 빌드 전제). 연결은 람다, `qobject_cast` 대신 `static_cast`.
- 계산은 Qt 없이 `core/`에, 바꾸면 `tests/`에 먼저 실패하는 시험. `third_party/` 수정 금지.
- 색 · 크기는 `app/theme.hpp` 토큰만. 단면선 `#FF0000`, 흙색 `#B5573A`는 한 화면에 주 단추 하나.
- 높이는 파일 값 그대로(지오이드 재적용 없음). 평면도 그림 칸에 단면선 없음. 조판은 탭(별도 창 아님).
- **커밋 · 푸시 · 배포는 사용자가 말할 때만.** 인덱스에 줄바꿈만 바뀐 파일이 대기 중이면 커밋하지 말고 먼저 알린다. `stash@{0}`는 건드리지 않는다.
- 이 PC 빌드: 정션 `C:\dev\desca`로(한글 경로에서 rc가 멈춤). 명령은 `docs/DEV_PROGRESS.md` §2.
- 사용자는 비개발자다. 한국어로, 실행해서 본 것만 보고한다.
