# 3SM(Bentley ScalableMesh) 파일 구조 조사

조사일 2026-10-11. 대상 H:\3sm (22개, 125 MB–4.3 GB, 읽기 전용). 확인 방법: Python 3.12 `sqlite3`,
URI `file:...?mode=ro&immutable=1`(잠금·저널 없음). 열기 전후 크기·수정 시각 동일, `-journal/-wal/-shm` 부속 파일 없음(확인).
공개 사양은 찾지 못했다(Bentley 비공개 형식). 아래는 **실제 파일 3개**(현장 A 125 MB, 현장 B 345 MB, 현장 C 4.3 GB)에서
직접 읽어 확인한 것과, 그로부터 추정한 것을 구분해 적는다. 나머지 19개는 SQLite 머리만 같음을 확인했고 내부는 보지 않았다.

## 확인된 것

- SQLite 3 파일. `page_size 4096`, `application_id 1113932898`(0x42654462 "BeDb"), `user_version 10`.
  `SMFileMetadata.Version = {"major":1,"minor":1,"sub1":0,"sub2":5}`, `be_Prop SchemaVersion = 3.1.0.1` (3개 모두 같음).
- 표(3개 모두 같은 스키마):

  | 표 | 역할 | 비고 |
  |---|---|---|
  | `SMMasterHeader` | 1행. `RootNodeId`(=2), `Depth`, `GCS`(WKT1 COMPD_CS 문자열), `IsTerrain`=0, `DataResolution` | `IsTextured`=0 이지만 노드에는 텍스처 있음 |
  | `SMNodeHeader` | 노드(LOD 트리) 색인. `NodeId` PK, `ParentNodeId`(-1 = 뿌리), `Resolution`(0..Depth 레벨), `Extent`·`ContentExtent`(BLOB 48 B = LE double 6: minx,miny,minz,maxx,maxy,maxz, **실좌표**), `NodeCount`(꼭짓점 수), `NbFaceIndexes`(인덱스 수 = 3×삼각형), `SubNode`(BLOB = LE int32 자식 NodeId 배열, 0–8개; 비어 있으면 잎), `TexID`(→ `SMTexture.NodeId`, -1 = 없음), `GeometryResolution`(m, 레벨마다 절반) | 뿌리(2)에는 메시 없음 |
  | `SMPoint` | `NodeId` PK, `PointData`, `IndexData`, `SizePts`, `SizeIndices` | 아래 |
  | `SMUVs` | `NodeId` PK, `UVData`, `SizeUVs`, `UVIndexData`, `SizeUVIndex` | 아래 |
  | `SMTexture` | `NodeId`(=TexID) PK, `TexData`, `SizeTexture`(=w·h·채널), `Codec`·`NOfChannels`(NULL) | 아래 |
  | `SMSources`, `SMImportSequences`, `be_Local`, `be_Prop`, `sqlite_sequence` | 가져오기 이력·DB GUID 등 | 읽기에 불필요 |

- BLOB 인코딩: 모두 **zlib**(머리 `78 5E`, 표준 inflate). 풀린 크기 = `SizePts`/`SizeIndices`/`SizeUVs`/`SizeUVIndex`(바이트).
  - `PointData` → LE **double** x,y,z × `NodeCount`. 절대 실좌표(예 x=XXXXXX.XXX, y=XXXXXX.XXX, z=XX.XXX — 공개 저장소에서는 실제 값 생략). 로컬 원점 없음.
  - `IndexData` → LE int32 × `NbFaceIndexes`, 삼각형 목록, **1 기반**(최솟값 1, 최댓값 = NodeCount).
  - `UVData` → LE double u,v × `NodeCount`(0..1). `UVIndexData` → LE int32, 1 기반. 125 MB 파일의 2,551개 메시 노드 전부에서
    `UVIndexData == IndexData`, UV 수 == 꼭짓점 수(확인). 즉 꼭짓점마다 UV 하나.
  - `TexData` → 16 B 머리(LE int32: 가로, 세로, 채널(3), 0) + **JPEG**(FF D8 … ). 노드당 1장, 128×128·128×64·1024×512 등.
- 규모: 125 MB = 노드 2,553(잎 769, 메시 2,551), 345 MB = 5,298, 4.3 GB = 55,364(잎 17,308, Depth 16). 노드당 최대 PointData 원본 579 KB.
  노드 색인(`SMNodeHeader` 전체)은 4.3 GB 파일에서도 수 MB 수준이라 열 때 다 읽어도 된다. 기하·텍스처는 노드별로 PK 조회(지연 읽기 가능).
- 좌표계: `SMMasterHeader.GCS` =
  `COMPD_CS["Korea 2000 / East Belt ",PROJCS[...GEOGCS["Korean2000.LL",DATUM["Korean2000",SPHEROID["GRS1980",6378137,298.2572221]],...],
  PROJECTION["Transverse Mercator"],PARAMETER["False Easting",200000],PARAMETER["False Northing",600000],PARAMETER["Scale Reduction",1],
  PARAMETER["Central Meridian",129],PARAMETER["Origin Latitude",38],UNIT["Meter",1]],VERT_CS["Ellipsoid Height",VERT_DATUM["Ellipsoid",2002],UNIT["Meter",1]]]`
  — 3개 파일 모두 같다(글자 길이 649). 단위 m. 매개변수(38/129/1/200000/600000, GRS80)는 내장 표의 **EPSG:5187 Korea 2000 / East Belt 2010** 과 일치.
  수직: `VERT_DATUM["Ellipsoid",2002]`(OGC WKT1 의 2002 = 타원체) → 타원체고 표기.
  실제 좌표 범위도 동경 129° 기준 TM(East Belt)과 맞는다(현장 위치·좌표는 공개 저장소에서 생략).

## 추정(아직 확인 안 됨)

- UV 의 v 방향: 앱 `Mesh.uv` 규약(v=0 = 영상 아래)과 같다고 가정. 이전 시도(kerf 브랜치 `app/sm3convert.cpp`)가 뒤집지 않고 썼고
  "유구 사진 메시가 보인다"고 기록했으나, 이 PC 에서 화면으로 다시 확인해야 한다.
- LOD 선택 규칙: 3MX 의 `maxScreenDiameter` 에 대응해 `노드 대각선 / GeometryResolution` 을 쓴다(kerf 와 같은 식). 화면 품질로 확인.
- `ContentExtent`(실제 내용 상자)와 `Extent`(분할 상자)의 차이·`Neighbor`·`AllComponent` 의 의미는 쓰지 않으므로 해석하지 않았다.
- `Resolution` 열이 0 인 노드(가장 깊은 레벨 일부, `GeometryResolution` 0.0)가 있다 → 잎으로 다룬다(자식 없음).
- 다른 19개 파일의 좌표계·스키마 버전은 같은 생산 도구(iTwin Capture Modeler)라 같을 것으로 추정 — 열 때 WKT 를 그대로 읽으므로 다르더라도 표기만 달라진다.

## 이전 시도(참고, 라이선스 문제 없음 — 같은 저장소)

`origin/kerf-portable-3sm`·`origin/feature/design-v2` 의 `app/sm3convert.cpp`: Qt Sql 로 **모든 노드를 메모리에 올린 뒤** 3MX 캐시(.3mxb 수천 개)로
변환해 저장하고 그걸 연다. 수 GB 파일에는 맞지 않는다(전체 메모리 적재·같은 크기의 캐시 생성). 이번 설계는 코어에서 지연 읽기한다.
