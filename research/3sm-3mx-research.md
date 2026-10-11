# 3SM·3MX·iTwin Capture·Descartes 조사 (2026-10-11)

목적: "3SM 이 안 열리고 너무 느리다" 를 풀기 위해 두 형식과 생산 도구를 조사하고, 지금 design-v2 의 3SM 처리 방식이 느린 이유를 측정으로 확인한 뒤 설계를 다시 정한다.
확인(출처·실측) 과 추정을 구분한다. 코드는 어떤 외부 저장소에서도 복사하지 않았다.

## 1. 3MX (공개 형식)

- Bentley 가 공개한 형식. "a lightweight open format that facilitates the distribution of our ContextCapture data, both online and toward 3rd party applications". 목표: 웹 친화·압축·스트리밍 최적화·확장성·다해상도(대용량).
  — [3MX Introduction](https://docs.bentley.com/LiveContent/web/ContextCapture%20Help-v18/en/GUID-FD56AC5F-8485-457F-83B5-26A269F25237.html)
- 구조: JSON 장면 파일(.3mx, layers[].type=meshPyramid, SRS, SRSOrigin, root) + 이진 .3mxb 트리. 3MXB = `3MXBO` + uint32 헤더 길이 + JSON 헤더(version, nodes[id,bbMin,bbMax,maxScreenDiameter,children,resources], resources) + 버퍼. 기하 = OpenCTM, 텍스처 = JPG(그대로 포장).
  — [3MX format](https://docs.bentley.com/LiveContent/web/ContextCapture%20Help-v18/en/GUID-CED0ABE6-2EE3-458D-9810-D87EC3C521BD.html), [3MXB export](https://docs.bentley.com/LiveContent/web/ContextCapture%20Help-v18/en/GUID-986AA6A2-4566-4846-9E0E-A09BE3225432.html), [3MXB 구조](https://docs.bentley.com/LiveContent/web/ContextCapture%20Help-v18/en/GUID-2A3C8541-EF29-4F17-8999-D3398C8E7192.html)
- LOD 규칙: `maxScreenDiameter` = "the maximum diameter, in pixels, for which this node should be visible. Once passed this size on screen, the node should be replaced by its children". 자식 없는 잎은 그 크기를 넘어도 그대로 보인다. 리소스가 빈 노드는 "부모를 보이지 않게 하되 다른 기하로 바꾸지 않는다" 는 뜻.
  — 위 3MX 문서군(검색 요약). ContextCapture 내보내기는 한 노드의 자식 전부를 한 .3mxb 에 넣는다("to speed-up loading, and to reduce the used memory").
  — [Current implementation](https://docs.bentley.com/LiveContent/web/ContextCapture%20Help-v18/en/GUID-E063264D-0CF4-4A42-8A68-955D4A646E5C.html)
- 이 앱(코어 `tmx.hpp`/`lod.hpp`/`stream.hpp`)은 이미 이 규칙대로 3MX 를 읽는다. **노드의 bbMin/bbMax 는 화면 지름 계산의 근거**라서, 상자가 비면(±DBL_MAX) 스트리밍이 그 노드에서 멈춘다(`core/src/stream.cpp:63,77` — `Box3::diag()` 는 빈 상자에 0).

## 2. 3SM (Bentley Scalable Mesh, 비공개 형식)

- Bentley 설명: "file format suitable for display, analysis and editing of large 3D meshes in Bentley design applications"(MicroStation·OpenRoads·Descartes·LumenRT 계열). 좌표계(GCS) 가 파일 안에 들어 있다("Geographic Coordinate System (GCS) information is embedded in the file itself").
  — [Scalable Mesh File Support (OpenRoads)](https://prd-aws-docs.bentley.com/LiveContent/web/OpenRoads%20Designer%20Help-v20/en/GUID-007CF251-C5BD-4D71-8F6E-273F6201C2EC.html), [ContextCapture 3SM Meshes (LumenRT)](https://docs.bentley.com/LiveContent/web/LumenRT%20CONNECT%20Edition%20Help-v1/en/GUID-1C52482D-4434-4C23-85C8-34543133873F.html)
- iTwin Capture Modeler(옛 ContextCapture) 의 내보내기 형식 목록에서 3MX 와 3SM 은 둘 다 "natively support LOD" 묶음에 있다(OBJ·FBX·STL 은 단일 해상도).
  — [Formats](https://docs.bentley.com/LiveContent/web/ContextCapture%20Help-v18/en/GUID-049FE15A-18DE-4314-B3B2-DF8ADD05C848.html)
- 다해상도 메시를 요청에 따라 스트리밍하는 용도("stream multi-resolution meshes on demand … to desktop or mobile devices"), 3MX → 3SM 변환은 ContextCapture Editor/Descartes 에서 한 방향으로만 된다는 사용자 보고.
  — [AEC Magazine](https://aecmag.com/features/future-reality-from-contextcapture-to-ai/), [Virtuosity 블로그](https://blog.virtuosity.com/contextcapture-offers-scalable-mesh-technology), [Bentley Communities 스레드](https://communities.bentley.com/products/3d_imaging_and_point_cloud_software/f/contextcapture-descartes-pointools-forum/133491/bentley-s-new-scaleable-reality-meshes-3sm)(새 커뮤니티로 이전되어 본문은 읽지 못함)
- **내부 구조의 공개 사양은 없다.** Bentley 의 공개 저장소 [iTwin/imodel-native](https://github.com/iTwin/imodel-native)(Apache-2.0) 에는 ScalableMesh 판독 코드가 없다(코드 검색: `SMNodeHeader` 0건, `ScalableMesh` 는 스키마·유틸 이름 6건뿐).
- 독립 구현 하나가 공개돼 있다: [Di0105/sl-digital-twin `tools/sm_export/sm_to_obj.py`](https://github.com/Di0105/sl-digital-twin/blob/main/tools/sm_export/sm_to_obj.py)(2026-06, 라이선스 표기 없음 → 코드 복사 안 함, 대조용으로만 읽음). 그 설명과 우리 실측이 일치한다:
  "The .3sm container is a SQLite database. Leaf nodes hold the full-resolution mesh. Geometry vertices are stored as zlib-compressed absolute float64 X/Y/Z in the master GCS … Triangle indices are zlib-compressed uint32 … Each node carries one JPEG texture atlas." 인덱스는 1 기반, UV 는 double, TexID → SMTexture. 그 쪽 예시 파일은 UTM 44N + 지오이드 높이라 **GCS 는 파일마다 다를 수 있고 WKT 원문을 읽어야 한다**는 점도 확인된다.
- 우리 실측(`research/3sm-format.md`, H:\3sm 3개): 표 `SMMasterHeader`(GCS WKT, RootNodeId)·`SMNodeHeader`(부모·레벨·Extent double 6·SubNode int32[]·NodeCount·TexID·GeometryResolution)·`SMPoint`(zlib double xyz 실좌표, zlib int32 1 기반)·`SMUVs`(zlib double uv, zlib int32 1 기반 — 확인한 파일에선 점 인덱스와 동일)·`SMTexture`(16 B 머리 + JPEG). 노드 색인은 4.3 GB 파일(55,364 노드)에서도 수 MB.

## 3. design-v2 가 느리고 평면이 비는 이유 (측정)

design-v2(3e3dfcc) 의 `app/sm3convert.cpp` 는 .3sm 을 **처음 열 때 통째로 읽어 .3mxb 수천 개로 변환**한 뒤 3MX 로 연다.

| 파일 | 크기 | 변환(첫 열기) | 최대 메모리 | 캐시 크기 | 결과 |
|---|---|---|---|---|---|
| 화원 성산리 석곽 | 120 MB | 2.3 s | 0.62 GB | 120 MB | 단면·DXF OK, **평면 빈 화면** |
| 북문지 | 346 MB | 6.7 s | 1.88 GB | 424 MB | 같음 |
| 1차발굴최종 | 4,150 MB | **69.6 s** | **4.33 GB** | **4,086 MB** | 같음 |

- 느린 이유: `app/sm3convert.cpp:151-192` 가 모든 노드의 PointData·IndexData·UV·텍스처 BLOB 을 `std::map<qint64, SmNode>` 와 `tex` 맵에 전부 올린다(파일 크기만큼 메모리) → `:222-246` 모든 노드를 CTM 으로 다시 인코딩해 디스크에 쓴다(파일 크기만큼 캐시). 파일 크기에 비례해 수십 초·수 GB.
- 평면이 비는 이유: `app/sm3convert.cpp:106-112` `writeNode` 가 메시 없는 뿌리 노드에 상자를 주지 않고, `core/src/tmx.cpp` `writeTmxTile` 은 메시로만 상자를 만들어 root.3mxb 에 bbMin=+DBL_MAX/bbMax=−DBL_MAX 가 기록된다(캐시 파일 헤더로 확인). 그러면 `TmxSource::open` 의 `bounds` 가 비어 평면 카메라가 1e+07 m 축척으로 맞춰지고(캡처: 축척 막대 "1e+07 m", 평면 흰 화면), 스트리머는 뿌리에서 멈춘다. "로컬 좌표가 큽니다(최대 1.797e308)" 경고도 같은 원인(`analyzeSrs` 에 빈 상자). 원점은 정상(예 153534, 356975, 0) — 원점 문제가 아니다.

## 4. 결론 → 설계

- 3SM 은 이미 LOD 트리 + 노드별 독립 BLOB(SQLite PK 조회) 이라 **변환 없이 노드 단위로 바로 읽을 수 있다**. 변환은 파일 크기에 비례하는 시간·메모리·디스크를 쓰고, 앱이 쓰는 3MX 규칙(상자·maxScreenDiameter)으로 옮기면서 오류가 들어갈 자리도 만든다.
- 그래서 `docs/superpowers/specs/2026-10-11-3sm-reader-design.md` 의 C안(가상 타일 스트리밍 판독기, SQLite 아말감, Qt Sql 불필요) 을 design-v2 위에 구현한다. 상자는 3SM `Extent` 에서 직접 오므로 빈 상자 문제가 생기지 않고, 원점은 뿌리 Extent 중심(double) 으로 잡아 float32 로컬에 넣는다.
- 성능 목표: 4.3 GB 파일도 첫 화면(저해상도 전체 모습)까지 5 s 이내(노드 색인 읽기 + 뿌리 근처 노드 몇 개), 변환 캐시 0, 최대 메모리는 TileCache 예산(1.5 GB) 안.
- 좌표계: 파일 WKT 를 그대로 읽어 EPSG:5186/5187 을 매개변수로 판별(이름 "Korea 2000 / East Belt" 는 5183 과 혼동되므로 매개변수 우선). 높이는 WKT 의 `VERT_DATUM["Ellipsoid",2002]` 근거로 타원체고 표기. 값 변환 없음.
