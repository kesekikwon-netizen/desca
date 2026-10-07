// Bentley ScalableMesh(.3sm) 읽기: SQLite 안의 LOD 트리(SMNodeHeader·SMPoint·SMUVs·SMTexture)를
// 한 번 3MX 타일(.3mx + 노드별 .3mxb)로 바꿔 사용자 캐시 폴더에 둔다. 원본은 읽기 전용으로만 연다.
#pragma once
#include <QString>
#include <functional>

/// file3sm → 캐시 폴더의 .3mx 경로(out3mx). 같은 파일(크기·수정 시각)을 전에 바꿨으면 바로 돌려준다.
bool convert3smTo3mx(const QString& file3sm, QString& out3mx, QString* err, const std::function<void(double)>& progress = {});
