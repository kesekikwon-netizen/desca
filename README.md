# 발굴 단면뷰어 (Excavation Section Viewer)

3MX 실사 메시(iTwin Capture / ContextCapture)를 열어 평면에서 단면선을 긋고, 10 cm 레벨선이 있는 단면도를
바로 보고 DXF · PNG/TIFF/GeoTIFF · 점군(XYZ/LAS)으로 내보내는 Windows 데스크톱 프로그램.

- 언어/도구: C++17, Qt 6.8 Widgets + OpenGL, OpenCTM(C). Linux 에서 MinGW-w64 로 Windows x64 교차 빌드.
- 앱은 moc 없이(Q_OBJECT 미사용) 작성 → 교차 빌드에 Qt 호스트 도구 불필요.
- `core/` (`archsection` 정적 라이브러리)는 Qt 무관 순수 C++ — 3MX 로더, 단면 절단·이어붙이기·정리, 레벨선,
  입면 영상 래스터라이저, DXF/TIFF/GeoTIFF/LAS 작성. MicroStation/Descartes MDL 플러그인에서 재사용 가능.

## 구조
```
core/include/asec, core/src   Qt 무관 코어 (tmx, section, lod, stream, schedule, pick, srs, raster, dxf, export, tiff, pointcloud, obj, engine)
app/                          Qt 앱 (mainwindow, planview[OpenGL], sectionview[QPainter], theme)
tools/                        asec-section(명령줄 단면), asec-info(3MX 점검·성능), asec-make-synthetic(합성 3MX), synth
tests/                        Catch2 단위 시험 + dxf_audit.py(ezdxf)
packaging/                    아이콘·NSIS·라이선스·리소스
third_party/                  OpenCTM, stb, nlohmann/json, Catch2
```

## 빌드
- Linux 개발: `./build-linux.sh` (Qt6 dev, cmake, ninja, zlib)
- Windows: `./build-win.sh` → `dist/SectionViewer-<버전>-Setup.exe`, `dist/SectionViewer-<버전>-portable-win64.zip`, `dist/SHA256SUMS.txt`(이전 버전 줄은 유지)
  (필요: g++-mingw-w64-x86-64-posix, libz-mingw-w64-dev, nsis, zip, Qt 6.8.3 win64_mingw — `QT_WIN` 환경 변수로 경로 지정)

## 명령줄 자동화(시험·캡처용)
`SectionViewer <파일.3mx> --line AX AY BX BY [--local] [--front m] [--back m] [--scale N] [--dpi N]
 [--export-png|tiff|geotiff|dxf|plan|xyz|las|csv 파일] [--area whole|band|view] [--spacing m] [--shot 파일.png] [--log 파일]
 [--pick X Y]... [--hover] [--perf-log 파일.csv] [--quit]`
- `--pick X Y`: 실좌표(또는 `--local`)에서 잎 메시 연직 정밀 피킹 → 로그에 Z·출처·시간
- `--hover`: 평면 보기 가운데 마우스 이동 흉내 → 좌표줄 Z 가 대략(화면 LOD) → 잎 표면으로 바뀌는지 로그
- `--perf-log`: 카메라 경로(맞춤→확대→이동→축소, 240프레임) + 단면선 끌기(60프레임 ~16 ms) 재생, 프레임마다
  시간·그린 노드·업로드·대기·GPU MB·최대 깊이를 CSV 로, 요약(미리보기 수, 놓은 뒤 최종까지 ms)은 로그로

`asec-info <파일.3mx> [--tree] [--decode] [--pick X Y]... [--section AX AY BX BY] [--local] [--csv 파일]`
— 열기 시간, 좌표계 판별(수평 EPSG/높이 기준/경고), 원점·위경도·float32 정밀도, 트리(타일·노드·잎·깊이), 잎 전체
디코드(삼각형 수, 실제 Z 범위), 피킹 시간, 단면 미리보기/최종 차가움·따뜻 시간. GUI 없이 성능 비교용.

## 시험
`./build-linux/asec_tests` (Catch2). 실제 3MX 시험은 선택:
`ASEC_REAL_3MX=/경로/Production_2.3mx [ASEC_REAL_EXPECT_LABEL="수평 EPSG:5186 / 높이 타원체고(GRS80)"] ./build-linux/asec_tests "[real]"`
(변수가 없으면 건너뜀. 실제 데이터는 저장소에 넣지 않는다.)

## 좌표·높이
실좌표 = 로컬 + SRSOrigin (3MX 는 x=동 E, y=북 N). 높이는 모델 SRS 값 그대로 — 지오이드 변환 없음.
- SRS 해석(`asec/srs.hpp`): `EPSG:n`, `EPSG:h+v`, `ENU:lat,lon`, WKT1(PROJCS/COMPD_CS), WKT2(PROJCRS/COMPOUNDCRS/
  BOUNDCRS/VERTCRS). 수평 EPSG 는 ID/AUTHORITY → REMARK("Promoted to 3D from EPSG:5186") → 이름 표 → TM 매개변수 순.
  높이 기준은 VERTCRS 또는 'ellipsoidal height' 축으로 판별. 화면에는 항상 `수평 EPSG:5186 / 높이 타원체고(GRS80)` 형태로
  표시하고, SRS 없음·모름·metadata.xml 불일치·원점 없음·축 순서 의심·float32 정밀도(로컬 > 8192 m)는 노란 경고.
- iTwin 은 GCP 를 정표고로 넣어도 '타원체고'로 표기하는 경우가 있음 → 기준점과 비교해 확인(자동 변환 안 함).
- 복합 EPSG 는 수평/수직을 나눠 GeoTIFF GeoKey 3072/4096, LAS GeoKeyDirectory VLR 에 기록.
- 커서/측정 Z(`asec/pick.hpp`): 최고 해상도 잎 메시에서 CPU double 광선 교차(로컬 + 원점). 화면 LOD 값은 '대략'으로
  먼저 보이고 잎 결과가 오면 바뀜. 좌표줄 옆에 Z 출처 표시.

## 성능 구조(M0)
- 타일 캐시: 타일 단위 잠금, 잎 병렬 디코드(최대 8 스레드).
- 평면 보기: 시점·확대 기반 LOD 스트리밍(LodStreamer, 비동기 로드 + GPU 예산 `view/gpuBudgetMB` 기본 768 MB).
- 단면: 끄는 동안 최신 요청만 남기는 CoalescingWorker — 미리보기는 화면 해상도에 맞는 거친 LOD, 놓으면 잎 메시로 최종.
- 병합 3MX: 같은 SRS·원점의 meshPyramid 레이어 전부 사용(다르면 빼고 경고).
