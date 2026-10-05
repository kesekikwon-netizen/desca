# 발굴 단면뷰어 (Excavation Section Viewer)

3MX 실사 메시(iTwin Capture / ContextCapture)를 열어 평면에서 단면선을 긋고, 10 cm 레벨선이 있는 단면도를
바로 보고 DXF · PNG/TIFF/GeoTIFF · 점군(XYZ/LAS)으로 내보내는 Windows 데스크톱 프로그램.

- 언어/도구: C++17, Qt 6.8 Widgets + OpenGL, OpenCTM(C). Linux 에서 MinGW-w64 로 Windows x64 교차 빌드.
- 앱은 moc 없이(Q_OBJECT 미사용) 작성 → 교차 빌드에 Qt 호스트 도구 불필요.
- `core/` (`archsection` 정적 라이브러리)는 Qt 무관 순수 C++ — 3MX 로더, 단면 절단·이어붙이기·정리, 레벨선,
  입면 영상 래스터라이저, DXF/TIFF/GeoTIFF/LAS 작성. MicroStation/Descartes MDL 플러그인에서 재사용 가능.

## 구조
```
core/include/asec, core/src   Qt 무관 코어 (tmx, section, lod, raster, dxf, export, tiff, pointcloud, obj, engine)
app/                          Qt 앱 (mainwindow, planview[OpenGL], sectionview[QPainter], theme)
tools/                        asec-section(명령줄 단면), asec-make-synthetic(합성 3MX), synth
tests/                        Catch2 단위 시험 + dxf_audit.py(ezdxf)
packaging/                    아이콘·NSIS·라이선스·리소스
third_party/                  OpenCTM, stb, nlohmann/json, Catch2
```

## 빌드
- Linux 개발: `./build-linux.sh` (Qt6 dev, cmake, ninja, zlib)
- Windows: `./build-win.sh` → `dist/SectionViewer-1.0.0-Setup.exe`, `dist/SectionViewer-1.0.0-portable-win64.zip`, `dist/SHA256SUMS.txt`
  (필요: g++-mingw-w64-x86-64-posix, libz-mingw-w64-dev, nsis, zip, Qt 6.8.3 win64_mingw — `QT_WIN` 환경 변수로 경로 지정)

## 명령줄 자동화(시험·캡처용)
`SectionViewer <파일.3mx> --line AX AY BX BY [--local] [--front m] [--back m] [--scale N] [--dpi N]
 [--export-png|tiff|geotiff|dxf|plan|xyz|las|csv 파일] [--area whole|band|view] [--spacing m] [--shot 파일.png] [--log 파일] [--quit]`

## 좌표
실좌표 = 로컬 + SRSOrigin. 높이는 모델 SRS 값 그대로(변환 없음). 복합 EPSG(`EPSG:5186+5193`)는 수평/수직을 나눠
GeoTIFF GeoKey 3072/4096, LAS GeoKeyDirectory VLR 에 기록.
