// 단면 도면 용지 배치(Qt 무관): 용지·축척 → 그림 칸, 넘침 판정, 추천(축척
// 줄이기·A3·나눠 붙이기), 방위·보는 방향 글자, 단면 이름, 최근 목록.
#pragma once
#include <string>
#include <vector>

namespace asec {

enum class Paper { A4 = 0, A3 = 1 };

// ---- 레벨선 굵기(디자인 v5 결정 L): 10 cm 선 0.2 pt · 50 cm 선 0.5 pt. 1 pt
// = 25.4/72 mm. SVG · DXF · PDF · 화면 모두 같은 값
struct LineWeights {
  double minorPt = 0.2;
  double majorPt = 0.5;
};
/// 빨간 잘린 선(단면선) 인쇄 · 내보내기 굵기 = 0.8 pt 고정(사용자 2026-10-16
/// 「단면선 빨간선은 0.8pt고정」) — SVG · PDF · PNG · DXF 모두. 화면 굵기와 무관
constexpr double kCutLinePt = 0.8;
/// pt → 종이 위 mm (1 pt = 25.4 / 72 mm)
double ptToMm(double pt);
/// 입력값을 0.05–3.0 pt 로 자른다. NaN 은 기본 0.2
double clampLineWeightPt(double pt);
LineWeights defaultLineWeights();
/// pt → DXF 선 굵기 코드(그룹 370, 1/100 mm). DXF 가 허용하는 값(0 · 5 · 9 · 13
/// · 15 · 18 · 20 · 25 · 30 · 35 · 40 · 50 · 53 · 60 · 70 · 80 · 90 · 100 · 106
/// · 120 · 140 · 158 · 200 · 211) 중 가장 가까운 것. 입력은 clampLineWeightPt
/// 로 자름
int dxfLineWeight(double pt);

struct SheetSpec {
  Paper paper = Paper::A4;
  bool landscape = true;
  double denom = 40;          // 축척 분모(1:denom)
  double marginMm = 5.0;      // 용지 가장자리 ~ 테두리
  double gutterLeftMm = 11.0; // 표고 숫자 칸
  double gutterRightMm = 9.0;
  double gutterTopMm = 5.0;    // A / A′ 글자
  double gutterBottomMm = 6.0; // 거리 숫자
  double titleHmm = 8.0;       // 표제란·축척 막대 띠(그림 칸 아래)
};

struct SheetLayout {
  double paperW = 0, paperH = 0;                         // mm
  double frameX = 0, frameY = 0, frameW = 0, frameH = 0; // 테두리(mm)
  double plotX = 0, plotY = 0, plotW = 0,
         plotH = 0;                  // 그림 칸(mm, 축척으로 그리는 곳)
  double titleY = 0;                 // 표제란 띠 위쪽(mm)
  double contentW = 0, contentH = 0; // 단면 실제 크기(mm, 축척 적용)
  bool fits = false;
  int cols = 1, rows = 1;             // 나눠 붙이기 장수(가로 × 세로)
  double fitLenM = 0, fitHeightM = 0; // 한 장에 들어가는 길이·높이(m)
};

void paperSizeMm(Paper p, bool landscape, double &w, double &h);
const char *paperName(Paper p); // "A4" / "A3"
/// lenM × heightM(m) 단면을 spec 용지에 놓았을 때 배치
SheetLayout layoutSheet(const SheetSpec &s, double lenM, double heightM);
/// 한 장에 꼭 들어가는 가장 큰 축척의 분모(정수로 올림)
double fitDenominator(const SheetSpec &s, double lenM, double heightM);

/// 인쇄(2026-10-12): 프린터가 지금 고른 용지(mm, 그 방향 그대로) · 못 찍는
/// 가장자리(mm)를 도면 용지와 비교. sizeOk = 가로 · 세로가 각각 tol 안(방향까지
/// 같음). marginOk = 네 가장자리 모두 테두리 바깥 여백 이하(아니면 테두리가
/// 잘림)
struct PrintPaperCheck {
  bool sizeOk = false, marginOk = false;
  double frameMarginMm = 0, maxPrinterMarginMm = 0;
};
PrintPaperCheck checkPrintPaper(const SheetLayout &L, double printerWmm,
                                double printerHmm, double marginLeftMm,
                                double marginTopMm, double marginRightMm,
                                double marginBottomMm, double tolMm = 1.0);

struct SheetAdvice {
  bool overflow = false;
  double fitLenM = 0;      // 지금 축척으로 한 장에 들어가는 길이
  double smallerDenom = 0; // 같은 용지에 들어가는 가장 큰 표준 축척(없으면 0)
  bool a3Fits = false;     // 같은 축척으로 A3(같은 방향)에 들어감 — A4 일 때만
  bool rotateFits = false; // 방향만 바꾸면 들어감
  int splitSheets = 1;     // 지금 용지·축척으로 나눠 붙일 장수
  int splitCols = 1, splitRows = 1;
};
/// 표준 축척(분모 오름차순)에서 추천을 고름
SheetAdvice adviseSheet(const SheetSpec &s, double lenM, double heightM,
                        const std::vector<double> &stdDenoms = {10, 20, 40, 50,
                                                                100, 200, 500});

/// 평면도 축척 분모. 10 단위로 맞추고 최소 10, 상한 없음. 0 이하·비수는 10.
double snapScaleDenom10(double denom);
/// 범위(m)가 그림 칸에 들어가는 가장 작은 10 단위 분모. 1 mm 이내 넘침은 들어간
/// 것으로 본다.
double fitDenomStep10(const SheetSpec &s, double widthM, double heightM);
/// 바깥 눈금 간격(m). 종이에서 한 칸이 minMm 이상이 되는 가장 작은 1·2·5×10ⁿ.
double outsideStepMeters(double mPerMm, double minMm = 28);

/// 평면도 조판 배치(mm, y 아래). 휠·이동은 그림 좌표만 바꾸고 글자 배율은 없다.
struct PlanPlace {
  double mPerMm = 0;
  double cX = 0, cY = 0;
  double dxMm = 0, dyMm = 0;
  double plotX = 0, plotY = 0, plotW = 0, plotH = 0;
  double imgX = 0, imgY = 0, imgW = 0,
         imgH = 0; // 범위와 칸을 함께 담는 영상 사각형
  double worldX0 = 0, worldX1 = 0, worldYBot = 0, worldYTop = 0;
  double visX0 = 0, visX1 = 0, visYBot = 0,
         visYTop = 0; // 그림 칸이 비추는 실좌표
  double tickStep = 1;
  bool imageFillsPlot = false;
  bool rangeInsideImage = false;
  double xMm(double worldX) const {
    return plotX + plotW * 0.5 + (worldX - cX) / mPerMm + dxMm;
  }
  double yMm(double worldY) const {
    return plotY + plotH * 0.5 - (worldY - cY) / mPerMm + dyMm;
  }
};
/// x0..x1, y0..y1 는 맞출 범위(로컬 m). split 이면 그 범위를 쪽으로 자른다.
PlanPlace placePlan(const SheetSpec &spec, double x0, double x1, double y0,
                    double y1, double viewZoom, double dxMm, double dyMm,
                    int col = 0, int row = 0, bool split = false);

/// 단면도 조판의 그림 칸이 비추는 거리·표고. 휠은 m/mm 만 바꾸고 글자 배율은
/// 없다.
struct SectionPaperWindow {
  double mPerMm = 0;
  double s0 = 0;   // 그림 칸 왼쪽의 거리(m, A 가 0)
  double zTop = 0; // 그림 칸 위의 절대 표고(m)
  double visLen = 0;
  double visHeight = 0;
};
SectionPaperWindow
sectionPaperWindow(double lenM, double heightM, double zTopAbs, double plotWmm,
                   double plotHmm, double denom, double viewZoom, double dxMm,
                   double dyMm, int col = 0, int row = 0, bool split = false);

/// 위에서 본 범위(단계 12): 기울어진 3D 평면 화면과 같은 중심 · 같은 m/px로
/// 위에서 본 사각형(로컬 m). 도면은 3D 기울기를 쓰지 않으므로 pitch를 받지
/// 않는다. mPerMm·그림 칸이 0 이하면 중심 한 점. yBot <= yTop, x0 <= x1 로
/// 둔다.
struct TopDownRange {
  double x0 = 0, x1 = 0, yBot = 0, yTop = 0;
};
TopDownRange topDownRange(double cX, double cY, double mPerMm, double plotWmm,
                          double plotHmm);
/// 그림 칸 바깥 좌표 눈금. [lo, hi] 안에 최대 maxCount개(1·2·5×10ⁿ 간격).
struct OutsideTicks {
  double step = 1;
  double first = 0;
  int count = 0;
  double at(int i) const { return first + double(i) * step; }
};
OutsideTicks outsideTicks(double lo, double hi, int maxCount = 6);

// ---- 단면도 조판의 「위치도」(평면 띠): 단면선이 가로로 놓이게 돌린 평면
// 정사영상을 같은 축척으로 단면 위에 둔다(사용자 결정 2026-10-11).
//      띠 반폭 = max(3 m, 앞 깊이, 뒤 깊이). 띠 안에서 가로 = 거리 s(A 가 0,
//      오른쪽 +), 세로 = 법선 t(단면의 '뒤' 쪽이 위).
constexpr double kPlanStripMinHalfM = 3.0;
/// 띠 반폭(m)
double planStripHalfWidth(double frontM, double backM);
/// 띠가 덮는 로컬 XY 외접 상자: s ∈ [−marginM, L+marginM], t ∈ [−halfM, halfM]
/// 인 사각형을 돌린 것
struct PlanStripArea {
  double x0 = 0, x1 = 0, y0 = 0, y1 = 0;
};
PlanStripArea planStripArea(double ax, double ay, double bx, double by,
                            double halfM, double marginM);
/// 영상 픽셀(열 i, 행 j; 왼쪽 위 픽셀 모서리 = (imgX0, imgY1), res m/px, 행은
/// 아래로 = 남쪽) → 종이(mm) 아핀 변환. Qt QTransform 순서: x' = m11·i + m21·j
/// + dx, y' = m12·i + m22·j + dy. pageXAtS0·pageYAtT0 = s=0, t=0 이 놓일 종이
/// mm, mmPerM = 종이 mm / 실제 m
struct PlanStripXf {
  double m11 = 1, m12 = 0, m21 = 0, m22 = 1, dx = 0, dy = 0;
};
PlanStripXf planStripTransform(double ax, double ay, double bx, double by,
                               double imgX0, double imgY1, double res,
                               double pageXAtS0, double pageYAtT0,
                               double mmPerM);

// ---- 평·단면도 한 장 조판(2026-10-16, 사용자 요청 「샘플 도면처럼 한 장에 축척·방향을 맞춰」):
//      첫 단면(기준)의 A→A′ 가 종이 가로(왼쪽→오른쪽)가 되게 평면을 돌린다. 단면도는 저마다
//      「잘린 면을 바라보는 쪽」(A→A′ 의 왼쪽 법선 = 입면 뒤쪽, facingKo 와 같은 규약)에 놓는다:
//      돌린 평면에서 A→A′ 가 →(기준과 같은 방향)이면 위, ←이면 아래, ↑이면 왼쪽(90° 돌려 A 아래 · A′ 위),
//      ↓이면 오른쪽(A 위 · A′ 아래). 위 · 왼쪽 · 오른쪽은 평면의 A 자리와 거리(s)를 맞추고, 아래(반대 방향)는
//      글자를 뒤집지 않으려고 보통 방향으로 두되 선의 가운데를 맞춘다. 같은 축척. 좌표 · 높이 값은 바꾸지 않는다.
struct ComboSection {
  double ax = 0, ay = 0, bx = 0, by = 0; // 단면선 A · A′(로컬 XY, m)
  double heightM = 0;                    // 단면 표시 높이(m, 위아래 여유 포함)
};
enum class ComboSlot { Top = 0, Bottom = 1, Left = 2, Right = 3 };
struct ComboPlaced {
  ComboSlot slot = ComboSlot::Top;
  int rotDeg = 0; // 0 · -90(왼쪽: 반시계 — A 아래 · A′ 위 · 높은 쪽이 왼쪽) · 90(오른쪽: 시계 — A 위 · A′ 아래 · 높은 쪽이 오른쪽)
  double lenM = 0, heightM = 0;
  double sA = 0, tA = 0, sB = 0, tB = 0; // 돌린 평면 좌표(m)의 A · A′
  // 단면 자체 좌표(mm, 돌리기 전: x = 거리 s 오른쪽, y = 아래로 = 높이 낮아짐)의 그림 칸과 글자 여백.
  // 종이에 놓을 때: translate(originX, originY) → rotate(rotDeg) → 이 좌표로 그린다
  double originX = 0, originY = 0;
  double plotX = 0, plotY = 0, plotW = 0, plotH = 0;
  double areaX = 0, areaY = 0, areaW = 0, areaH = 0;
  /// 단면 좌표(mm) → 종이(mm)
  void toPaper(double x, double y, double &px, double &py) const;
};
struct ComboLayout {
  SheetLayout L;
  double mmPerM = 0;
  double ux = 1, uy = 0, nx = 0, ny = 1, ax = 0, ay = 0; // 돌린 틀: 원점 = 기준 A, s 축 u(종이 오른쪽), t 축 n(종이 위)
  double planX = 0, planY = 0, planW = 0, planH = 0;     // 평면 칸(mm)
  double sMin = 0, sMax = 0, tMin = 0, tMax = 0;         // 평면 칸이 비추는 돌린 좌표(m)
  double contentW = 0, contentH = 0;                     // 전체(평면 + 단면 띠 + 여백, mm)
  bool fits = false;
  std::vector<ComboPlaced> sections;
  void toST(double x, double y, double &s, double &t) const {
    s = (x - ax) * ux + (y - ay) * uy;
    t = (x - ax) * nx + (y - ay) * ny;
  }
  /// 돌린 좌표 → 로컬 XY
  void toXY(double s, double t, double &x, double &y) const {
    x = ax + ux * s + nx * t;
    y = ay + uy * s + ny * t;
  }
  double xMm(double s) const { return planX + (s - sMin) * mmPerM; }
  double yMm(double t) const { return planY + (tMax - t) * mmPerM; }
  /// 종이 mm → 돌린 좌표(xMm · yMm 의 거꾸로)
  void paperToST(double mmX, double mmY, double &s, double &t) const {
    s = sMin + (mmX - planX) / mmPerM;
    t = tMax - (mmY - planY) / mmPerM;
  }
  /// 진북 화살표의 종이 각(도, 시계 방향, 0 = 위)
  double northDeg() const;
};
/// 단면들(첫 것이 기준)과 평면 여유(단면선 끝에서 m)로 배치. 단면이 없으면 fits=false.
/// keepFrame 이 있으면 돌린 틀(원점 · s · t 축)을 그것에서 그대로 가져온다 — 도면에서 단면선 끝을 끄는 동안
/// 평면이 돌지 않게(2026-10-16 「도면 안에서 빨간선 끝단을 움직여」). 놓으면 붙들지 않고 다시 배치한다
ComboLayout layoutCombo(const SheetSpec &spec,
                        const std::vector<ComboSection> &secs,
                        double planMarginM, double gapMm = 6.0,
                        double stackGapMm = 3.0,
                        const ComboLayout *keepFrame = nullptr);
// ---- 평면 방향(사용자 2026-10-16 「사용자가 평면을 어떻게 놓을지 생각을 해야 단면을 만들 수 있다」 · 「다이얼을 돌리면 평면도가 돌아가게」)
// 방향 각 rotDeg = 화면 · 종이에서 진북이 위에서 시계 방향으로 돈 각(0 = 북쪽 위). 표시 회전일 뿐 좌표 · Z · 단면 계산은 그대로
/// 각을 (−180, 180] 으로
double normDeg180(double deg);
/// 평면 방향의 돌린 틀: s 축 u = (cos, sin)(종이 오른쪽) · t 축 n = (−sin, cos)(종이 위), 원점 (ax, ay).
/// layoutCombo 의 keepFrame 으로 넘기면 첫 단면 기준 자동 회전 대신 이 방향으로 놓는다. northDeg() == normDeg180(rotDeg)
ComboLayout comboFrameForRotation(double rotDeg, double ax = 0, double ay = 0);
/// 다이얼 한 칸(사용자 2026-10-16 「방향키로도 0.1도씩」): dir 쪽(+ = 시계)으로 step 격자의 다음 칸.
/// 격자 위면 정확히 step 만큼, 격자 밖(끌어서 33.63°)이면 그 방향의 가장 가까운 격자(33.7°)로. 결과는 (−180, 180]
double rotationStep(double deg, int dir, double step = 0.1);
/// 두 점 맞춤: p → q 가 화면 가로(오른쪽으로)가 되는 방향 각. vertical 이면 세로(위로)
double rotationToAlign(double px, double py, double qx, double qy, bool vertical);
/// 단면선 방향 스냅: dirDeg(동 0°, 반시계)를 refsDeg 각각의 ref + k·90° 중 tolDeg 안에서 가장 가까운 것에 붙인다.
/// source = 붙은 refs 자리(-1 = 안 붙음), perpendicular = 그 기준과 직각(k 홀수)
struct AngleSnap { bool snapped = false; double angleDeg = 0; int source = -1; bool perpendicular = false; };
AngleSnap snapAngle(double dirDeg, const std::vector<double> &refsDeg, double tolDeg = 3.0);

/// 한 장에 들어가는 가장 작은 10 단위 분모(fitDenomStep10 와 같은 규칙), 최대 2000
double fitComboDenomStep10(const SheetSpec &spec,
                           const std::vector<ComboSection> &secs,
                           double planMarginM);
/// 용지 없는 한 장(사용자 2026-10-16 「용지 크기는 신경 쓰지 말고 최적 크기로 배치만」): 평면 칸과 모든 단면 칸(글자
/// 여백 포함)의 외접 상자에서 marginMm 띄우고, 아래에 titleMm 띠를 둔 종이로 옮긴다. L.paperW/H · frame · plot ·
/// titleY 를 그 종이로 다시 채우고 fits = true. 축척 · 배치 관계는 그대로(평행 이동만)
void freeComboPage(ComboLayout &c, double marginMm = 10.0,
                   double titleMm = 16.0);
/// 평·단면도 용지(사용자 2026-10-16 「용지방향이안돼」 → 카드 답 「둘 다」): 기본은 내용 크기, 단추를 누를 때마다
/// A4 가로 → A4 세로 → A3 가로 → A3 세로 → 내용 크기
enum class ComboPaper { Content = 0, A4Landscape, A4Portrait, A3Landscape, A3Portrait };
ComboPaper nextComboPaper(ComboPaper p);
/// 정한 용지의 가로 × 세로(mm). 내용 크기면 false
bool comboPaperSize(ComboPaper p, double &wMm, double &hMm);
/// 정한 용지에 놓기: freeComboPage 와 같은 내용 외접 상자를 paperW × paperH 종이의 여백 안(아래 titleMm 띠 위)
/// 가운데로 평행 이동한다. 들어가면 true(fits). 안 들어가도 가운데에 놓고(넘친 곳은 종이 밖) false
bool placeComboOnPaper(ComboLayout &c, double paperW, double paperH,
                       double marginMm = 10.0, double titleMm = 16.0);
/// 그 용지에 들어가는 가장 작은 10 단위 분모(최대 2000, 없으면 0). 나머지 인자는 layoutCombo 와 같음
double fitComboDenomOnPaper(const SheetSpec &spec,
                            const std::vector<ComboSection> &secs,
                            double planMarginM, double paperW, double paperH,
                            double marginMm = 10.0, double titleMm = 16.0,
                            double gapMm = 6.0, double stackGapMm = 3.0,
                            const ComboLayout *keepFrame = nullptr);
/// 평면 영상을 만들 로컬 XY 외접 상자(돌린 평면 칸 네 모서리)
PlanStripArea comboPlanArea(const ComboLayout &c);

/// 방위각(북 0°, 시계 방향) → "N 90.4° E" (180° 넘으면 "N 45.0° W")
std::string formatAzimuth(double azDeg);
/// 단면 보기가 바라보는 쪽(A→A′ 왼쪽 법선 = 두께 '뒤' 쪽): A→A′ 방위각 →
/// "북쪽을 봄" 등 8방위
std::string facingKo(double lineAzDeg);
/// 0 → "A–A′", 1 → "B–B′", … 25 → "Z–Z′", 26 → "A1–A1′"
std::string sectionLetterName(int index);
/// 도면 파일 이름: `모델_도면_1-분모.svg`. 도면 이름의 –(en dash)는 -로, ′는
/// 그대로. Windows 금지 글자 `\ / : * ? " < > |`는 _로, 각 부분의 앞뒤
/// 공백·점을 뗀다. 분모가 정수면 정수로(1-20), 아니면 소수(1-22.5).
std::string sheetFileName(const std::string &model, const std::string &sheet,
                          double denom);
/// 같은 이름이 있으면 이름 뒤(확장자 앞)에 _2, _3… 을 붙인 첫 빈 이름.
/// existing = 이미 있는 파일 이름들. 확장자가 없으면 뒤에 바로 붙인다.
std::string uniqueSheetFileName(const std::string &base,
                                const std::vector<std::string> &existing);
/// 최근 목록 맨 앞에 넣기(같은 경로는 대소문자·구분자 무시하고 하나만), 최대
/// maxN
std::vector<std::string> pushRecent(const std::vector<std::string> &list,
                                    const std::string &item, size_t maxN = 8);

} // namespace asec

namespace asec {
/// 단면 목록 한 줄(실좌표 = SRSOrigin 적용, 앞/뒤 m). 설정(모델 경로 키)과
/// .sections.json 공용 뒤 깊이(입면 배경) 기본값: 입면도를 그릴 수 있게 3
/// m(1.2.1~). 1.2.0 까지는 0.5 m
constexpr double kDefaultBackDepth = 3.0;
constexpr double kLegacyDefaultBackDepth = 0.5;
constexpr double kMaxBackDepth = 10.0;
/// 저장된 뒤 깊이 → 쓸 값. userSet = 사용자가 직접 고른 값이라는 표시(1.2.1~
/// 저장). 저장 없음 → 기본 3 m. 표시 있음 → 그 값. 표시 없는 옛 기록: 옛 기본값
/// 0.5 면 새 기본 3 m, 다른 값이면 사용자가 바꾼 것이므로 유지.
double resolveBackDepth(bool hasStored, double stored, bool userSet);

struct SavedSection {
  std::string name; // "A–A′"
  double ax = 0, ay = 0, bx = 0, by = 0;
  double front = 0, back = kDefaultBackDepth;
  bool backUserSet =
      false; // 뒤 깊이를 사용자가 직접 고름(아니면 기본값을 따름)
  std::string note;
};
std::string sectionsToJson(const std::vector<SavedSection> &v,
                           const std::string &model,
                           const std::string &srsLabel, int current);
bool sectionsFromJson(const std::string &text, std::vector<SavedSection> &out,
                      int *current, std::string *err);
} // namespace asec
