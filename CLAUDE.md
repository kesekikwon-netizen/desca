# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

발굴 단면뷰어(SectionViewer): 3MX 실사 메시(iTwin Capture/ContextCapture)를 열어 평면에서 단면선을 긋고, 10 cm 레벨선이 있는
단면도를 DXF·PNG/TIFF/GeoTIFF·XYZ/LAS·PDF 로 내보내는 Windows 데스크톱 프로그램. C++17, Qt 6.8 Widgets + OpenGL, 개발은 Linux,
배포는 MinGW-w64 교차 빌드. 문서·주석·커밋·UI 문구는 한국어.

## 빌드·시험

```bash
# 코어 + 도구 + 시험만 (Qt 불필요 — 2026-10-11 이 환경에서 확인: 106 시험 중 105 통과, [real] 1개 건너뜀)
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DASEC_BUILD_APP=OFF && ninja -C build-linux
./build-linux/asec_tests                       # 전체 (Catch2)
./build-linux/asec_tests "[backdepth]"         # 태그: [backdepth] [cutplace] [sheet] [real]
./build-linux/asec_tests "DXF 2D*"             # 이름(와일드카드)으로 한 개
./build-linux/asec_tests --list-tests

./build-linux.sh        # 앱 포함 전체 빌드(Qt6 dev 필요) + 시험 + 합성 3MX(/tmp/asec-synthetic)
./build-win.sh [--no-package]   # Windows 교차 빌드 → dist/ (QT_WIN=Qt 6.8.3 mingw_64 경로, mingw-w64 posix, nsis, zip)
python3 tests/dxf_audit.py out.dxf             # ezdxf 감사(ezdxf 설치 필요)
```

- 새 시험 파일은 `CMakeLists.txt` 의 `asec_tests` 소스 목록에 직접 추가해야 한다(GLOB 아님). 코어 소스도 같음.
- 시험 데이터는 `tools/synth.cpp`(`asec_synth`)가 합성 3MX 를 만들어 쓴다. 실제 3MX 시험은 선택:
  `ASEC_REAL_3MX=/경로/Production_2.3mx [ASEC_REAL_EXPECT_LABEL="수평 EPSG:5186 / 높이 타원체고(GRS80)"] ./build-linux/asec_tests "[real]"`.
  실제 데이터는 저장소에 넣지 않는다.
- `.github/workflows/cmake-single-platform.yml` 은 GitHub 기본 템플릿 그대로(`cmake -B build` 기본값 = 앱 ON → Qt6 필요).
  ubuntu-latest 에서 Qt 설치 단계가 없어 통과하는지는 확인하지 않았다.
- 앱은 GUI 없이 명령줄로 구동·캡처·자동 점검이 가능하다(`--line … --export-dxf … --quit`, `--cut-check`, `--undo-test`,
  `--settings DIR` 로 사용자 설정 격리 등). 옵션 전체는 `README.md` 와 `app/main.cpp` 머리 주석. GUI 없는 성능 점검은 `asec-info`.

## 구조

- `core/` → 정적 라이브러리 `archsection`. **Qt 에 의존하면 안 된다**(MicroStation/Descartes MDL 플러그인 재사용 대상).
  의존은 OpenCTM·zlib·nlohmann/json·stb 뿐(GDAL·PROJ 없음). 헤더 `core/include/asec/*.hpp`.
- `app/` → Qt 앱. **moc 를 쓰지 않는다**(`Q_OBJECT`·시그널 선언 금지, `QObject::connect` + 람다만). 그래서 Windows 교차 빌드가
  Qt 호스트 도구 없이 Qt 라이브러리를 직접 링크한다(`app/CMakeLists.txt` 의 `ASEC_QT_WIN_DIR` 분기). 새 Qt 모듈을 쓰면 그 분기의
  include·lib 목록과 `build-win.sh` 의 DLL 복사도 함께 고쳐야 한다(QtPrintSupport 는 일부러 안 씀 — 인쇄는 PDF 로).
- `tools/` → `asec-section`(명령줄 단면), `asec-info`(3MX 점검·성능), `asec-make-synthetic`. `tools/diag/` 는 빌드 대상 아님.

핵심 흐름(`asec/engine.hpp` `computeSection`, 앱·CLI 공용):
`MeshSource`(`TmxSource` = 3MX LOD 트리 + `TileCache`, `StaticSource` = OBJ) → 단면 띠의 메시 수집 → 수직 평면 절단
(`section`) → 이어붙이기·정리 → 입면 영상 래스터(`raster` `renderElevation`) → 레벨선(`planLevels`) → 내보내기(`dxf`, `tiff`, `pointcloud`, `export`, `sheet`).
- `leafMeshes` = 최고 해상도 잎, `bandMeshes` = 해상도별 미리보기 LOD, `cutMeshes` = 잘린 면용 잎 지오메트리만(텍스처 없음).
  **잘린 면(단면선)은 미리보기에서도 항상 잎으로 자른다**(1.2.1 회귀 시험 `test_backdepth`, `test_cutplace`). 거친 LOD 는 입면 배경에만.
- 앱 스레딩: UI 스레드에서 파일 읽기·디코드·단면 계산을 하지 않는다. 단면은 최신 요청만 남기는 `CoalescingWorker`(`schedule`),
  평면은 `LodStreamer`(`stream`, GPU 예산). `TileCache` 는 타일 단위 잠금(전역 잠금 없음 — `test_cache_parallel`).
- 큰 앱 파일: `mainwindow.cpp`(작업·내보내기), `mainwindow_ui.cpp`(화면 구성·시작 화면·단면 목록·되돌리기 QUndoStack),
  `planview.cpp`(QOpenGLWidget), `sectionview.cpp`(QPainter), `sheetexport.cpp`(도면 창·PDF), `theme.hpp`(Strata QSS).

## 지켜야 할 도메인 규칙

- 실좌표 = 로컬 + `SRSOrigin`(3MX x=동, y=북). 메시는 float32 로컬 좌표, 피킹·좌표 계산은 double(`asec/pick.hpp`).
- **높이(Z)는 모델 SRS 값 그대로** — 지오이드 변환·정표고 보정을 하지 않는다. 「높이 기준 지정」은 이름표만 바꾼다(Z 불변).
  지오이드 변환 틀(`vdatum`)은 있으나 UI·자동 변환·격자는 없다. SRS 해석은 `srs.hpp`, 높이 기준 식별은 `vdatum.hpp`.
- 레벨선 10 cm·라벨 50 cm(소수 1자리)가 기본이며 cm 정수 산술. 10 cm 보다 촘촘하게 그리지 않는다. DXF 는 축척과 무관하게 모든 레이어 기록.
- 그리기 순서는 화면·모든 내보내기에서 **레벨선 → 입면 영상 → 단면선(맨 위, 빨강)**. DXF 엔티티 순서도 같다.
- 단면 화면 세로 과장(`--vex`)은 화면 전용 — 도면·내보내기는 항상 1:1.
- 설정 키 이관 주의: 옛 저장값 호환(예: 뒤 깊이 0.5 → 3 m, `backUserSet`)은 시험으로 고정돼 있다. 모델별 상태는
  QSettings `model/<경로 해시>/`, 단면 목록 파일은 `.sections.json`.
- 버전 번호는 `CMakeLists.txt`(`project VERSION`)와 `build-win.sh`(`VER=`) 두 곳에 있다. 변경 내역은 `CHANGELOG.md`.

## 참고 문서

`plan/ROADMAP.md`(우선 원칙·현황), `plan/PERF-M0.md`(성능 구조), `research/ui-ref/UI-SPEC.md`(디자인 토큰·화면 규격),
`research/roadmap-tech-research.md`. 문서 안 `/workspace/...` 경로는 옛 작업 환경 기준이라 이 저장소에는 없다.
