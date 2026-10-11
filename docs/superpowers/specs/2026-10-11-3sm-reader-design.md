# 3SM(ScalableMesh) 직접 열기 — 설계안

2026-10-11. 기준 커밋 `7ab7d8f`(로컬 main = origin/main eb4972d + CLAUDE.md). 브랜치 `feature/3sm-reader`, 작업 트리 `D:\desca-3sm`.
형식 조사: `research/3sm-format.md`.

## 1. 왜 지금 안 열리나(코드 근거)

- `app/mainwindow.cpp:134-145` — 확장자가 `.obj` 가 아니면 전부 `TmxSource::open`(3MX JSON) 으로 보낸다. `.3sm` 분기가 없다.
- `app/mainwindow.cpp:258` 열기 창 거르개 `3MX 실사 메시 (*.3mx);;OBJ 메시 (*.obj);;모든 파일` → 목록에 .3sm 이 안 보인다.
- `app/mainwindow.cpp:342,348` 끌어다 놓기도 `.3mx`/`.obj` 만 받는다. `app/main.cpp:2` 명령줄도 같다.
- 코어에 SQLite 가 없다(`CMakeLists.txt` 의존: OpenCTM·zlib·json·stb). 3SM 은 SQLite DB 라 판독기 자체가 없다.

## 2. 목표

H:\3sm 의 .3sm 을 3MX 처럼 연다: 평면 표시(LOD 스트리밍) → 단면선 → 단면·레벨선 → DXF 등 내보내기. 전체 메모리 적재 없음,
원본 불변(읽기 전용·부속 파일 없음), 기존 예산(TileCache 1.5 GB, GPU 768 MB) 유지. 쓰기·변환 캐시·지오이드·UI 디자인·버전·패키징은 제외.

## 3. 접근 비교

| 안 | 내용 | 평가 |
|---|---|---|
| A. 3MX 캐시 변환(design-v2/kerf 의 `sm3convert.cpp`) | 처음 열 때 모든 노드를 읽어 .3mxb 수천 개로 저장 | 수 GB 파일 전체를 메모리에 올리고 같은 크기 캐시를 만든다. 앱(Qt Sql) 의존. **기각** |
| B. 완전히 새 MeshSource + 새 스트리머 | 코어에 SmSource 와 평면 스트리밍 경로를 따로 둠 | `LodStreamer`·`collect*`·점군 내보내기가 모두 `.3mxb` 경로 기반이라 중복 구현이 큼 |
| **C. 가상 타일(채택)** | 3SM 노드 하나를 "가상 .3mxb 타일" 로 본다. 경로 `…\x.3sm\n<NodeId>` 를 `readTmxTile` 이 알아보고 SQLite 에서 그 노드만 읽어 `TmxTile`(노드 1개, 메시 디코드 완료, 텍스처는 JPEG 바이트) 을 돌려준다 | `TileCache`·`LodStreamer`·`collectLeafMeshes`·`selectDisplayMeshes`·점군 내보내기가 **그대로** 동작. 메모리 회계(`tileBytes`)·LRU 퇴거도 그대로 |

## 4. 구성(안 C)

### 코어(`archsection`, Qt 없음)
- `third_party/sqlite/sqlite3.c, sqlite3.h` — 공식 아말감 3.54.0(public domain). `CMakeLists.txt` 에 OpenCTM 처럼 정적 라이브러리 `sqlite3` 추가,
  정의 `SQLITE_THREADSAFE=2`(멀티스레드: 연결마다 한 스레드) `SQLITE_OMIT_LOAD_EXTENSION` `SQLITE_DEFAULT_MEMSTATUS=0` `SQLITE_DQS=0`. 경고 끔.
- `core/include/asec/sm3.hpp`, `core/src/sm3.cpp` (CMake 소스 목록에 직접 추가):
  - `struct SmNodeInfo { int64 id, parent, texId; Box3 extentWorld; double geomRes; std::vector<int64> children; bool hasMesh; }`
  - `class SmFile`: `open(path, err)` — `sqlite3_open_v2(uri, SQLITE_OPEN_READONLY|SQLITE_OPEN_URI|SQLITE_OPEN_NOMUTEX)`,
    URI `file:///<퍼센트 인코딩 UTF-8 경로>?immutable=1`(잠금·저널 검사 없음 → 읽기 전용 매체·부속 파일 없음). 열 때 읽는 것은
    `SMMasterHeader`(GCS, RootNodeId) 와 `SMNodeHeader` 전체(색인만, 4.3 GB 파일 55k 행 ≈ 수 MB). 기하·텍스처는 안 읽음.
    `origin` = 뿌리 `Extent` 중심을 xy 는 1 m 로 반올림, z = 0(높이 값 그대로). `bounds`(로컬) = 뿌리 Extent − origin.
  - 연결 풀: `SmFile` 이 `sqlite3*` 를 여러 개(요청 스레드 수만큼 지연 생성, 최대 8) 뮤텍스로 빌려 준다. 한 연결은 한 번에 한 스레드만(멀티스레드 모드 규칙).
  - `bool readNodeTile(int64 id, TmxTile& out, err)`: PK 조회 3번(`SMPoint`, `SMUVs`, `SMTexture`) → zlib 풀기(`uncompress`, 크기 = Size*)
    → 1 기반 → 0 기반, double 실좌표 − origin → float32, UV 그대로(v 뒤집지 않음 — 화면에서 확인). `UVIndexData != IndexData` 이면
    (점, UV) 짝마다 꼭짓점을 나눠 푼다. `TmxNode{ id, bb=Extent−origin, maxScreenDiameter = 대각선/geomRes (geomRes 0 또는 잎이면 0),
    children = 자식 가상 경로, meshes = [Mesh], decoded = true }`, 텍스처 `Texture{format "jpg", encoded = JPEG 바이트}` 를 메시에 붙인다.
    BLOB 은 `sqlite3_column_blob` 으로 한 노드분만 읽는다(최대 수백 KB — `sqlite3_blob_open` 증분 읽기는 불필요, 검토 결과 기록).
  - 가상 경로 규약: `<3sm 절대 경로>/n<NodeId>`. `.3sm` 은 파일이라 실제 하위 경로와 충돌하지 않는다. 등록부(경로 → `weak_ptr<SmFile>`) 를 `sm3.cpp` 가 가진다.
  - `readTmxTile`(`core/src/tmx.cpp`) 머리에 분기 추가: `p.parent_path().extension() == ".3sm"` 이면 `sm3ReadVirtualTile(p, out, err)` 로 위임.
    `decodeNode` 는 `decoded == true` 라 바로 돌아온다(변경 없음). `TileCache::decode` 는 `ready == false` 라 JPEG 디코드 콜백을 돈다(변경 없음).
- `class SmSource : public TmxSource`(`engine.hpp`): `openSm(path, err)` 가 `SmFile` 을 열고 `scene`(이름, `srs{srs = GCS WKT 원문, origin}`,
  `layers[0].rootFile = 가상 뿌리 경로`), `bounds` 를 채운다. `leafMeshes/bandMeshes/cutMeshes/areaMeshes` 는 `TmxSource` 것을 그대로 쓴다.
  `dynamic_cast<TmxSource*>` 를 쓰는 점군 내보내기(`mainwindow.cpp:553,598`) 도 그대로 동작.
- `core/src/srs.cpp`: Bentley GCS 이름 `Origin Latitude`·`Scale Reduction` 인식, `VERT_DATUM[...,2002]`/`"Ellipsoid"` → 타원체고.
  (kerf 브랜치의 같은 변경을 가져오되 시험은 이 파일들의 WKT(동경 129° → EPSG:5187) 로 다시 쓴다.) 높이 값은 안 바꾼다.

### 앱
- `loadScene`: `ext == ".3sm"` 또는 파일 첫 16 B 가 `"SQLite format 3\0"` 이면 `SmSource`. `kind = "3SM"`. 루트 가상 타일에서 거친 메시를 읽는 기존 코드 그대로.
- 열기 거르개 `실사 메시 (*.3mx *.3sm *.obj);;3MX (*.3mx);;3SM ScalableMesh (*.3sm);;OBJ (*.obj);;모든 파일`, 끌어다 놓기·명령줄 주석·시작 화면 안내에 `.3sm`.
- 파일 읽기는 전부 기존 작업 스레드(`LodStreamer`, `CoalescingWorker`, 열기 스레드) 에서 일어난다 — UI 스레드에서 SQLite 를 열지 않는다.
- D:\desca 의 미커밋 `tmx.cpp` 거절 로직과는 겹치지 않는다(그건 `readTmxScene`, 이건 `readTmxTile`·`loadScene` 분기). 병합 때 두 가지가 함께 있어도 된다.

## 5. 좌표·단위·높이

- 3SM 점은 절대 실좌표(double, m). 실좌표 = 로컬(float32) + `SRSOrigin` 규칙을 지키려고 origin 을 위처럼 잡는다. 현장 크기 ≤ 300 m 라 8192 m 경고 규칙에 걸리지 않는다.
- Z 는 파일 값 그대로. 좌표계 표기는 WKT 를 `srs.hpp` 가 해석: 매개변수로 EPSG:5187, 수직은 WKT 의 `Ellipsoid`(2002) 근거로 타원체고. 해석 실패 시 경고만.
- 3MX 와 달리 `metadata.xml` 이 없으므로 대조 없음.

## 6. 오류 처리

- SQLite 가 아니거나 `SMMasterHeader`/`SMNodeHeader` 가 없으면 한국어 오류("3SM(ScalableMesh) 형식이 아닙니다"). 뿌리 노드가 없거나 Extent 가 깨지면 오류.
- 노드 하나의 BLOB 이 깨지면 그 타일만 실패(기존 `fallbackNodes`/`tileFailures` 경로) — 전체 열기는 계속.
- `SmFile` 이 닫힌 뒤(모델 닫음) 남은 가상 경로 요청은 오류로 돌려준다(등록부 weak_ptr 만료).

## 7. 시험(TDD, `tests/test_sm3.cpp` 를 CMake 목록에 추가)

1. 합성 3SM 만들기(시험 코드 안에서 sqlite3 API 로 직접 생성, 확인된 스키마 그대로): LOD 2단계(뿌리 + 자식 2개, 손자 1개), 삼각형 몇 개, 텍스처 1장(stb 로 JPEG 쓰기), GCS WKT 원문.
2. `SmFile::open`: 노드 수·뿌리·자식 관계·`bounds`·origin, 기하 BLOB 을 읽지 않았음(연결 통계 또는 읽은 노드 수 0).
3. `readTmxTile(가상 경로)`: 꼭짓점 좌표(실좌표 − origin, 1e-6), 1 기반 보정, UV, 텍스처 JPEG 디코드(`decodeTexStb`) 크기, `maxScreenDiameter`, 자식 경로.
4. UVIndex ≠ Index 인 노드의 꼭짓점 분리.
5. `SmSource` + `computeSection` 결과가 같은 형상의 `StaticSource`(OBJ 경로) 결과와 일치(단면 점·레벨).
6. 좌표계: Bentley WKT → EPSG:5187·타원체고 표기.
7. 읽기 전용: 열고 읽은 뒤 파일 크기·내용 해시 동일, `-journal/-wal/-shm` 없음. 읽기 전용 속성 파일도 열림.
8. 오류: 빈 SQLite(표 없음) → 오류 문구, 깨진 BLOB 노드 → 그 타일만 실패.
9. `[real]`: `ASEC_REAL_3SM` 환경 변수가 있을 때 실제 파일 열기·노드 수·범위·단면.

## 8. 검증(실제 파일, 새 빌드 `D:\desca-3sm\build-msvc\app\SectionViewer.exe`)

125 MB·345 MB·4.3 GB 를 명령줄 `--line … --export-dxf … --quit` 로 열어 종료 코드·열기 시간·최대 메모리(PeakWorkingSet64) 기록, `tests/dxf_audit.py`.
GUI 로 평면 표시 확인(캡처). H:\3sm 전후 크기·시각 동일, 부속 파일 없음. `asec_tests` 기준선(104 통과/1 실패 `test_schedule.cpp:39`/1 건너뜀) 대비 새 실패 0.

## 9. 1차에서 하지 않는 것·남는 위험

- 3SM 쓰기, 변환 캐시, 점군·지형 전용 3SM(IsTerrain=1)·텍스처 없는 3SM 의 최적화(열리기는 해야 함 — 텍스처 없으면 회색).
- UV v 방향·LOD 규칙은 화면으로 확인해야 확정. 다른 19개 파일은 같은 도구 출력이라 같을 것으로 추정.
- SQLite 아말감은 새 의존성(2.85 MiB 소스, public domain) — 저장소에 들어간다. MinGW 교차 빌드(`build-win.sh`) 는 이 PC 에서 돌리지 않으므로 미검증.
