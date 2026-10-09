// 단면 도면 용지 배치(Qt 무관): 용지·축척 → 그림 칸, 넘침 판정, 추천(축척 줄이기·A3·나눠 붙이기), 방위·보는 방향 글자, 단면 이름, 최근 목록.
#pragma once
#include <string>
#include <vector>

namespace asec {

enum class Paper { A4 = 0, A3 = 1 };

struct SheetSpec {
    Paper paper = Paper::A4;
    bool landscape = true;
    double denom = 40;          // 축척 분모(1:denom)
    double marginMm = 10;       // 용지 가장자리 ~ 테두리
    double gutterLeftMm = 17;   // 표고 숫자 칸
    double gutterRightMm = 15;
    double gutterTopMm = 9;     // A / A′ 글자
    double gutterBottomMm = 11; // 거리 숫자
    double titleHmm = 24;       // 표제란·축척 막대 띠(그림 칸 아래)
};

struct SheetLayout {
    double paperW = 0, paperH = 0;              // mm
    double frameX = 0, frameY = 0, frameW = 0, frameH = 0;   // 테두리(mm)
    double plotX = 0, plotY = 0, plotW = 0, plotH = 0;       // 그림 칸(mm, 축척으로 그리는 곳)
    double titleY = 0;                          // 표제란 띠 위쪽(mm)
    double contentW = 0, contentH = 0;          // 단면 실제 크기(mm, 축척 적용)
    bool fits = false;
    int cols = 1, rows = 1;                     // 나눠 붙이기 장수(가로 × 세로)
    double fitLenM = 0, fitHeightM = 0;         // 한 장에 들어가는 길이·높이(m)
};

void paperSizeMm(Paper p, bool landscape, double& w, double& h);
const char* paperName(Paper p);                 // "A4" / "A3"
/// lenM × heightM(m) 단면을 spec 용지에 놓았을 때 배치
SheetLayout layoutSheet(const SheetSpec& s, double lenM, double heightM);
/// 한 장에 꼭 들어가는 가장 큰 축척의 분모(정수로 올림)
double fitDenominator(const SheetSpec& s, double lenM, double heightM);

struct SheetAdvice {
    bool overflow = false;
    double fitLenM = 0;          // 지금 축척으로 한 장에 들어가는 길이
    double smallerDenom = 0;     // 같은 용지에 들어가는 가장 큰 표준 축척(없으면 0)
    bool a3Fits = false;         // 같은 축척으로 A3(같은 방향)에 들어감 — A4 일 때만
    bool rotateFits = false;     // 방향만 바꾸면 들어감
    int splitSheets = 1;         // 지금 용지·축척으로 나눠 붙일 장수
    int splitCols = 1, splitRows = 1;
};
/// 표준 축척(분모 오름차순)에서 추천을 고름
SheetAdvice adviseSheet(const SheetSpec& s, double lenM, double heightM, const std::vector<double>& stdDenoms = {10, 20, 40, 50, 100, 200, 500});

/// 평면도 축척 분모. 10 단위로 맞추고 최소 10, 상한 없음. 0 이하·비수는 10.
double snapScaleDenom10(double denom);
/// 범위(m)가 그림 칸에 들어가는 가장 작은 10 단위 분모. 1 mm 이내 넘침은 들어간 것으로 본다.
double fitDenomStep10(const SheetSpec& s, double widthM, double heightM);
/// 바깥 눈금 간격(m). 종이에서 한 칸이 minMm 이상이 되는 가장 작은 1·2·5×10ⁿ.
double outsideStepMeters(double mPerMm, double minMm = 28);

/// 평면도 조판 배치(mm, y 아래). 휠·이동은 그림 좌표만 바꾸고 글자 배율은 없다.
struct PlanPlace {
    double mPerMm = 0;
    double cX = 0, cY = 0;
    double dxMm = 0, dyMm = 0;
    double plotX = 0, plotY = 0, plotW = 0, plotH = 0;
    double imgX = 0, imgY = 0, imgW = 0, imgH = 0;   // 범위와 칸을 함께 담는 영상 사각형
    double worldX0 = 0, worldX1 = 0, worldYBot = 0, worldYTop = 0;
    double visX0 = 0, visX1 = 0, visYBot = 0, visYTop = 0;   // 그림 칸이 비추는 실좌표
    double tickStep = 1;
    bool imageFillsPlot = false;
    bool rangeInsideImage = false;
    double xMm(double worldX) const { return plotX + plotW * 0.5 + (worldX - cX) / mPerMm + dxMm; }
    double yMm(double worldY) const { return plotY + plotH * 0.5 - (worldY - cY) / mPerMm + dyMm; }
};
/// x0..x1, y0..y1 는 맞출 범위(로컬 m). split 이면 그 범위를 쪽으로 자른다.
PlanPlace placePlan(const SheetSpec& spec, double x0, double x1, double y0, double y1,
                    double viewZoom, double dxMm, double dyMm, int col = 0, int row = 0, bool split = false);

/// 단면도 조판의 그림 칸이 비추는 거리·표고. 휠은 m/mm 만 바꾸고 글자 배율은 없다.
struct SectionPaperWindow {
    double mPerMm = 0;
    double s0 = 0;       // 그림 칸 왼쪽의 거리(m, A 가 0)
    double zTop = 0;     // 그림 칸 위의 절대 표고(m)
    double visLen = 0;
    double visHeight = 0;
};
SectionPaperWindow sectionPaperWindow(double lenM, double heightM, double zTopAbs, double plotWmm, double plotHmm,
                                      double denom, double viewZoom, double dxMm, double dyMm, int col = 0, int row = 0, bool split = false);

/// 그림 칸 바깥 좌표 눈금. [lo, hi] 안에 최대 maxCount개(1·2·5×10ⁿ 간격).
struct OutsideTicks {
    double step = 1;
    double first = 0;
    int count = 0;
    double at(int i) const { return first + double(i) * step; }
};
OutsideTicks outsideTicks(double lo, double hi, int maxCount = 6);

/// 방위각(북 0°, 시계 방향) → "N 90.4° E" (180° 넘으면 "N 45.0° W")
std::string formatAzimuth(double azDeg);
/// 단면 보기가 바라보는 쪽(A→A′ 왼쪽 법선 = 두께 '뒤' 쪽): A→A′ 방위각 → "북쪽을 봄" 등 8방위
std::string facingKo(double lineAzDeg);
/// 0 → "A–A′", 1 → "B–B′", … 25 → "Z–Z′", 26 → "A1–A1′"
std::string sectionLetterName(int index);
/// 도면 파일 이름: `모델_도면_1-분모.svg`. 도면 이름의 –(en dash)는 -로, ′는 그대로.
/// Windows 금지 글자 `\ / : * ? " < > |`는 _로, 각 부분의 앞뒤 공백·점을 뗀다.
/// 분모가 정수면 정수로(1-20), 아니면 소수(1-22.5).
std::string sheetFileName(const std::string& model, const std::string& sheet, double denom);
/// 같은 이름이 있으면 이름 뒤(확장자 앞)에 _2, _3… 을 붙인 첫 빈 이름.
/// existing = 이미 있는 파일 이름들. 확장자가 없으면 뒤에 바로 붙인다.
std::string uniqueSheetFileName(const std::string& base, const std::vector<std::string>& existing);
/// 최근 목록 맨 앞에 넣기(같은 경로는 대소문자·구분자 무시하고 하나만), 최대 maxN
std::vector<std::string> pushRecent(const std::vector<std::string>& list, const std::string& item, size_t maxN = 8);

}  // namespace asec

namespace asec {
/// 단면 목록 한 줄(실좌표 = SRSOrigin 적용, 앞/뒤 m). 설정(모델 경로 키)과 .sections.json 공용
/// 뒤 깊이(입면 배경) 기본값: 입면도를 그릴 수 있게 3 m(1.2.1~). 1.2.0 까지는 0.5 m
constexpr double kDefaultBackDepth = 3.0;
constexpr double kLegacyDefaultBackDepth = 0.5;
constexpr double kMaxBackDepth = 5.0;
/// 저장된 뒤 깊이 → 쓸 값. userSet = 사용자가 직접 고른 값이라는 표시(1.2.1~ 저장).
/// 저장 없음 → 기본 3 m. 표시 있음 → 그 값. 표시 없는 옛 기록: 옛 기본값 0.5 면 새 기본 3 m, 다른 값이면 사용자가 바꾼 것이므로 유지.
double resolveBackDepth(bool hasStored, double stored, bool userSet);

struct SavedSection {
    std::string name;      // "A–A′"
    double ax = 0, ay = 0, bx = 0, by = 0;
    double front = 0, back = kDefaultBackDepth;
    bool backUserSet = false;   // 뒤 깊이를 사용자가 직접 고름(아니면 기본값을 따름)
    std::string note;
};
std::string sectionsToJson(const std::vector<SavedSection>& v, const std::string& model, const std::string& srsLabel, int current);
bool sectionsFromJson(const std::string& text, std::vector<SavedSection>& out, int* current, std::string* err);
}  // namespace asec
