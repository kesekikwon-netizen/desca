# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 프로젝트

발굴 단면뷰어(SectionViewer): 3MX 실사 메시(iTwin Capture / ContextCapture)에서 평면에 단면선을 긋고 10 cm 레벨선이 있는
단면도를 보여 주며 DXF · PNG/TIFF/GeoTIFF · XYZ/LAS · PDF 도면으로 내보내는 Windows 데스크톱 프로그램.
C++17, Qt 6.8 Widgets + OpenGL, CMake + Ninja. 배포 대상은 Windows x64이고 Linux에서 MinGW-w64로 교차 빌드한다.
문서·주석·커밋 메시지·UI 문구는 한국어로 쓴다. 사용자 기능 설명과 자동화 옵션 전체 목록은 `README.md`, 변경 이력은 `CHANGELOG.md`.

## 빌드·시험

```bash
# Qt 없이 코어·도구·시험만 (Qt6 dev 가 없는 환경에서 확인된 방법)
cmake -S . -B build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DASEC_BUILD_APP=OFF
ninja -C build-linux
./build-linux/asec_tests                      # Catch2 전체 (ctest 는 이것을 'core' 하나로 돌림)
./build-linux/asec_tests "[sheet]"            # 태그: [sheet] [backdepth] [cutplace] [real]
./build-linux/asec_tests "레벨선: cm 정수 산술, 등급, 표기"   # 시험 이름 하나 (--list-tests 로 이름 확인)

./build-linux.sh        # 앱 포함 전체 + 시험 + /tmp/asec-synthetic 합성 3MX (Qt6 dev, zlib 필요)
./build-win.sh [--no-package]   # Windows 교차 빌드 → dist/ (MinGW posix, Qt 6.8.3 mingw_64 = $QT_WIN, nsis, zip)
```

- 시험 데이터는 저장소에 없다. 시험은 `tools/synth.*`(`asec_synth`)로 해석적 지형의 합성 3MX를 임시 폴더(`tests/common.hpp` `tmpDir`)에 만들어 쓴다.
  `asec-make-synthetic <폴더>` 로 같은 데이터를 파일로 만들 수 있다.
- 실제 3MX 시험은 선택: `ASEC_REAL_3MX=/경로/x.3mx [ASEC_REAL_EXPECT_LABEL="수평 EPSG:5186 / 높이 타원체고(GRS80)"] ./build-linux/asec_tests "[real]"`
  (변수가 없으면 건너뜀 — 그래서 평소에 1 skipped). 실제 데이터는 커밋하지 않는다.
- DXF 검증: `python3 tests/dxf_audit.py a.dxf ...` (ezdxf 필요, 기본 환경에 없을 수 있음).
- GUI 없이 앱 동작을 확인하려면 `SectionViewer` 명령줄 자동화(`--line … --export-* … --log … --quit`, `--settings DIR` 로 사용자 설정 격리)나
  `asec-section`, `asec-info` 를 쓴다. 옵션은 README "명령줄 자동화" 절.
- 경고 옵션 `-Wall -Wextra`(코어). 별도 린터·포매터 설정은 없다.
- `.github/workflows/cmake-single-platform.yml` 은 기본 옵션(`ASEC_BUILD_APP=ON`)으로 ubuntu-latest 에서 빌드하는데 Qt6 설치 단계가 없다
  (CI 통과 여부는 미확인).

## 구조

- `core/` → 정적 라이브러리 `archsection`. **Qt 의존 금지** — MicroStation/Descartes MDL 플러그인에서 재사용할 대상이다.
  의존은 OpenCTM·zlib·nlohmann/json·stb 뿐(GDAL/PROJ 없음, TIFF/GeoTIFF/LAS/DXF 작성기는 직접 구현).
- `app/` → Qt 앱 `SectionViewer`. **moc 를 쓰지 않는다**(`Q_OBJECT`·signals/slots 선언 금지, `QObject::connect` + 람다만).
  Windows 교차 빌드가 Qt 호스트 도구 없이 Qt 정적 라이브러리를 직접 링크하기 때문(`app/CMakeLists.txt`). 새 Qt 모듈을 쓰면
  두 분기(Linux `find_package`, Windows 수동 링크)와 `build-win.sh` 의 DLL 복사를 모두 고쳐야 한다. 명령줄 자동화 처리는 `app/main.cpp`.
- `tools/` → `asec-section`, `asec-info`, `asec-make-synthetic`, 합성 데이터 `synth`. `tools/diag/` 는 빌드 대상이 아닌 진단 코드.

핵심 데이터 흐름(여러 파일에 걸침):
1. `tmx` 3MX 읽기(meshPyramid 레이어 — SRS·원점이 같은 레이어 전부) → `srs`/`vdatum` 로 수평 EPSG·높이 기준 판별.
2. `lod` `TileCache`(타일 단위 잠금, 잎 병렬 디코드) / `stream` `LodStreamer`(평면 보기 비동기 LOD, GPU 예산) / `schedule` `CoalescingWorker`(끄는 동안 최신 요청만).
3. `engine` `computeSection(MeshSource&, SectionRequest)` 가 앱·CLI 공용 파이프라인: 띠 안 메시 수집 → `section` 자르기·이어붙이기·정리
   → `raster` `renderElevation` 입면 영상. 잘린 면(단면선)은 미리보기에서도 항상 최고 해상도 잎(`cutMeshes`, ±1 mm 띠),
   입면 배경만 `meshRes>0` 일 때 거친 LOD.
4. 내보내기: `dxf`, `tiff`(GeoTIFF GeoKey), `pointcloud`(XYZ/LAS), `export`, `sheet`(도면 용지 배치 — 앱 `sheetexport.cpp` 가 PDF/PNG/TIFF/DXF 로 그림).

## 지켜야 할 도메인 규칙

- 실좌표 = 로컬 + SRSOrigin (3MX x=동, y=북). 로컬은 float32 이므로 정밀 계산(피킹 `pick`)은 double 로 한다.
- **높이(Z)는 모델 SRS 값 그대로.** 지오이드 변환·정표고 보정을 하지 않는다. 「높이 기준 지정」은 Z 는 두고 이름표만 바꾼다
  (GeoidModel/convertHeight 틀은 있으나 UI·자동 변환 없음).
- 레벨선은 cm 정수 산술(`planLevels`/`levelLines`): 선 10 cm · 숫자 50 cm 기본, 10 cm 보다 촘촘하게 그리지 않음.
- 그리기 순서는 화면·모든 내보내기에서 동일: 레벨선(맨 밑) → 입면 영상 → 단면선(맨 위, 순수 빨강).
- UI 스레드에서 파일 읽기·디코드·단면 계산을 하지 않는다.
- 설정은 QSettings(모델별 키 `model/<경로 해시>/`, `heightDatum/<경로 해시>`). 옛 저장값 이관 규칙(예: `backUserSet` 없는 0.5 m → 3 m)이 있으니
  기본값을 바꿀 때 이관과 `.sections.json` 호환을 함께 확인한다.
- 버전은 `CMakeLists.txt` `project(... VERSION)` 과 `build-win.sh` `VER` 두 곳에 있다.
- 설계 배경: `plan/ROADMAP.md`(원칙·마일스톤), `plan/PERF-M0.md`(성능), `research/ui-ref/UI-SPEC.md`(Strata 디자인 토큰).
