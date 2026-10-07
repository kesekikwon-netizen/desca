# Claude Code 이어가기

2026-10-06 밤, Cursor에서 여기까지 하고 넘긴다. 한 번에 한 목표만 고친다. 고친 뒤에는 실행해서 본 것만 말한다. 커밋·푸시·배포는 사용자가 말할 때만 한다.

시안: `D:\des\build-msvc\design-proposal\index.html` (단면뷰어 디자인 개선안.zip). 제품 규칙은 `.cursor/rules/product-rules.mdc`가 이긴다. 고시에 없는 자북·GPS는 넣지 않는다. 나침반 글자는 진북만. 좌표는 메시 로컬 + SRSOrigin. 높이는 파일 값 그대로. 지오이드를 다시 씌우지 않는다.

## 빌드·실행

- Qt: `D:\Qt\6.8.3\msvc2022_64` (Svg 모듈을 쓴다. `app/CMakeLists.txt`가 `Qt6::Svg`를 링크한다)
- zlib: `D:\deps\zlib`
- 빌드: `D:\des\build-msvc` (MSVC 2022, `/utf-8`)
- 최신 실행 파일: `D:\des\build-msvc\app\SectionViewer.exe`
- 바탕화면 `C:\Users\kwonyoungin1\Desktop\발굴 단면뷰어\SectionViewer.exe` 는 사용자가 앱을 연 채라 마지막 복사가 실패했다. 앱을 끈 뒤에만 복사한다.
- 시험 모델: `E:\new project jeju1\Productions\Production_2\Scene\Production_2.3mx` (공백. 따옴표로 넘긴다)
- 좌표 입력 창(`coordEntryDialog`, `--coord-shot`)은 소스에만 있고, 그 변경 이후 빌드·캡처는 없다. 먼저 빌드한다.

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cmake --build D:\des\build-msvc --target SectionViewer
```

PowerShell에서 실행할 때 PATH에 `D:\Qt\6.8.3\msvc2022_64\bin` 과 `D:\deps\zlib\bin` 을 넣고, `QT_QPA_PLATFORM_PLUGIN_PATH` 는 `D:\Qt\6.8.3\msvc2022_64\plugins\platforms` 로 둔다. `$HOME` 은 PowerShell 예약 변수라 캡처 경로 변수 이름으로 쓰지 않는다.

## 이미 실행해서 본 것

조판 잠금. 같은 모델, 선 `148083.065 98121.10 148084.595 98118.52`, `--back 0.5`, `--sheet-check D:\des\build-msvc\out-sheet`. 마지막 결과 `sheet-check RESULT ok`.

- 평면 1배: 그림이 칸을 채움, 범위가 영상 안, 단면선 없음, 바깥 숫자 14개 모두 칸 밖, 글자 비율 유지
- 평면 2.07배: 간격만 넓어지고 글자 크기는 같음
- 단면 1배·2.07배: 영상과 빨간 선, 표고·거리는 칸 밖
- 12 mm 이동: 도곽 고정, 바깥 숫자 값은 바뀜
- 축척 단계 10

다른 캡처 (`D:\des\build-msvc\out-sheet\`):

- `work.png`, `steps.png`: 오른쪽 「선택한 단면」(3.00 m, 깊이 1.70 m), 범례, 뒤 0.50 m, 리본 세 걸음
- `sync.png`: 평면 1 m–2 m 사이 검은 고리, 상태줄 「단면 커서」
- `plane.png`, `fade.png`, `band.png`: 피치 42°. 단면 사진이 세로로 서고, 앞쪽은 옅고, 뒤에 빨간 반투명 벽
- `home-thumbs.png`: 이어서 카드, 「이 모델의 단면」 A–A′ 썸네일
- `splash.png`: 여는 화면
- `datum.png`: 「이 모델의 높이는 어느 기준입니까?」, EGM96 추천, 지정 단추
- `sheet-tab.png`, `sheet-svg.png`: 조판 탭. 형식에 SVG
- `layers.dxf`: `DRAW_SOIL` 1회, `DRAW_OUTLINE` 1회 (빈 층). `SECTION_PROFILE` 은 단면선
- `layers.svg`, `plan.svg`: `<g id="DRAW_SOIL"/>`, `<g id="DRAW_OUTLINE"/>`

핵심 코드: `core/src/sheet.cpp` 의 `fitDenomStep10`, `placePlan`, `sectionPaperWindow`. 조판 그리기는 `app/sheetexport.cpp`. 휠은 `QPainter::scale` 을 쓰지 않는다. 3D 면은 `PlanView::setSectionPlane`, 앞쪽 알파 0.22 는 `planview.cpp` 셰이더 `uPass`.

## 남은 일

1. 좌표 입력 창을 빌드하고 `--coord-shot` 으로 찍어, 물음 「A와 A′는 어디입니까?」와 X(동)·Y(북), 「단면선 적용」이 보이는지 확인한다.
2. PDF 층. Qt 6.8 `QPdfWriter` 에는 층 API가 없다. 문서를 확인한 뒤에만 넣고, 없으면 사용자에게 그 한계를 말한다. DXF·SVG 빈 층은 이미 있다.
3. 아이콘을 시안 7장의 선 그림과 한 획씩 맞춘다. 지금 `theme::icon` 은 획 2, 둥근 끝이다. 시안과 다른 획만 고친다.
4. 단면 커서가 입면 깊이(깊이 버퍼)까지 들어가 띠 안에 고리를 찍는 것. 시안은 「제안」이다. 지금은 잘린 선 위(d=0)만 된다.
5. 시안 전체를 픽셀 단위로 같다고 하지 않는다. 빠진 창만 하나씩 열고 캡처한다.

하지 말 것: 자북·GPS를 지어 넣기, 지오이드 재적용, 조판을 별도 OS 창으로 되돌리기, 작업 화면 왼쪽·오른쪽 배치를 바꾸기, 커밋.


## 2026-10-07 Claude Code에서 한 일 (실행해서 본 것만)

- 여는 화면: 사진 없는 선 그림 모션, 전용 창(맨 위·화면 가운데) 5초. `--splash-live DIR`로 1–5초 장면을 그 창에서 직접 찍어 확인.
- 아이콘: `packaging/assets/make_icons.py` 를 사진 바탕 없이 벡터로 새로 그림(app.ico·png·설치 그림).
- 조판 선명도: 평면 미리보기는 보이는 범위만 최소 1 mm/px로 다시 렌더(휠·조판편집 멈춘 0.25초 뒤), 단면 미리보기 1.5 mm/px, PDF dpi 제한 제거·기본 600, 단면 SVG 영상 300 dpi. `--sheet-wheel N` 으로 실제 휠 뒤 캡처 확인.
- 3D 「앞쪽 숨기기」(한쪽 흐리게) 없앰 — 사용자 요청.
- 평면도 맞춤 축척: 왼쪽 화면 범위를 모델로 자른 뒤 계산(1:390 → 1:260). 단면도도 맞춤으로 열고 「맞춤」 단추.
- 축척 칸 옆 ▲▼ 단추(앱 QSS가 숫자 칸 단추를 숨기므로 따로).
- 선택한 단면: 지표·바닥 EL·깊이를 빨간 잘린 선에서 잼. 오른쪽 단추는 보조(흙색은 리본 「도면」 하나).
- 평면 「뒤 n m」 이름표는 띠 바깥으로, 선이 80 px 미만이면 숨김. 축척 막대는 범례와 안 겹치게 48 mm 이내. 홈 표 머리글 왼쪽 정렬.
- 확인: `--sheet-check` RESULT ok, `asec_tests "[sheet],[orbit]"` 88개 통과.
- 바탕화면 실행 파일은 아직 옛것(앱이 켜져 있어 복사 안 함).
