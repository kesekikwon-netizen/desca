#pragma once
// 프로젝트 파일(<모델이름>.trench.json) — 한 모델의 작업 상태 전부(사용자 결정 2026-10-11 「C: 앱 폴더 projects\ 자동 저장 + 저장/열기」).
// 앱은 모델을 열고 닫을 때·단면이 바뀔 때마다 이 파일을 쓰고, 모델을 열 때 있으면 먼저 읽는다. Qt 없음(core).
#include <string>
#include <vector>
#include "asec/sheet.hpp"

namespace asec {

/// 조판 설정 중 저장할 부분(앱 SheetParams/PlanSheetParams 의 값 칸). present=false 면 저장된 적 없음
struct ProjectSheet {
    bool present = false;
    int paper = 0;              // 0 A4 · 1 A3
    bool landscape = true;
    double denom = 40;          // 축척 분모
    int format = 4;             // 0 PNG · 1 TIFF · 2 PDF · 3 DXF · 4 SVG(단면도) / 평면도는 0–3
    double dpi = 300;
    bool split = false;
    bool withImage = true, withLine = true, withLevels = true, withTicks = true, withTitle = true, withPlan = true;
    double levelMinorPt = 0.2, levelMajorPt = 0.5;
    int hatchKind = 0; double hatchMm = 1.0;
};

struct ProjectDoc {
    std::string modelPath;      // 모델 파일 절대 경로(UTF-8, '/' 구분자)
    std::string modelName;      // 파일 이름(경로에서 채움)
    std::string srsLabel;
    std::vector<SavedSection> sections;   // 단면 목록(.sections.json 과 같은 칸)
    int current = -1;           // 고른 단면(목록 색인), 없으면 -1
    double front = 0, back = kDefaultBackDepth; bool backUserSet = false;   // 리본 입면 깊이
    bool hasLine = false; double ax = 0, ay = 0, bx = 0, by = 0;            // 지금 단면선(실좌표 m)
    int opacity = 100;          // 입면 영상 불투명도 %
    double lineWidthPx = 2.0;   // 단면선 굵기(화면 px: 1.5 · 2 · 3)
    bool smooth = false, depthFade = false;
    bool hasCamera = false; double camX = 0, camY = 0, metersPerPixel = 0;   // 평면 카메라(실좌표)
    // 평면 방향(진북이 화면 위에서 시계 방향으로 돈 각, 사용자 2026-10-16). 표시 회전일 뿐 좌표는 그대로.
    // hasPlanRotation = 사용자가 방향을 정함(평·단면도가 이 방향을 쓴다). 없으면 첫 단면 기준 자동
    bool hasPlanRotation = false; double planRotationDeg = 0;
    ProjectSheet sectionSheet, planSheet;
    std::string savedAt;        // ISO 8601(앱이 채움)
};

/// 파일 내용(JSON, 들여쓰기 2) ← 문서
std::string projectToJson(const ProjectDoc& p);
/// 문서 ← 파일 내용. 형식이 "Trench.project" 가 아니거나 모델 경로가 없으면 false + err. 깨진 단면 줄은 건너뛴다
bool projectFromJson(const std::string& text, ProjectDoc& out, std::string* err);
/// 모델 파일 이름 → 프로젝트 파일 이름(확장자 뗀 이름 + ".trench.json", 금지 글자는 '_')
std::string projectFileName(const std::string& modelFileName);
/// ".trench.json" 으로 끝나는가(대소문자 무시)
bool isProjectFileName(const std::string& name);

}  // namespace asec
