#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include "catch_amalgamated.hpp"
#include "asec/export.hpp"
#include "common.hpp"

using namespace asec;
using Catch::Approx;

static std::vector<std::pair<int, std::string>> pairsOf(const std::string& s) {
    std::istringstream in(s);
    std::vector<std::pair<int, std::string>> v;
    std::string c, val;
    while (std::getline(in, c) && std::getline(in, val)) v.emplace_back(std::stoi(c), val);
    return v;
}

static SectionResult sampleResult() {
    SectionResult r;
    r.line = SectionLine{{10, 20}, {13, 24}, 0, 0.5};  // 길이 5 m
    r.srs.srs = "EPSG:5186"; r.srs.origin = Vec3(200000, 450000, 40); r.srs.hasOrigin = true;
    r.profile = {{{0, 5.12}, {1.5, 5.30}, {2.0, 4.70}, {5.0, 4.75}}};
    r.zMin = 4.6; r.zMax = 5.4;
    return r;
}

TEST_CASE("DXF 2D: 절대표고 Y, 레이어, 레벨선 10cm, 라벨 겹침 없음, 중복 꼭짓점 없음") {
    auto r = sampleResult();
    DxfExportOptions o; o.scaleDenom = 20;
    o.image = true; o.imageFile = "sec_image.png"; o.imageW = 1250; o.imageH = 200; o.imageS0 = 0; o.imageZ0 = 4.6; o.imageRes = 0.004;
    std::string txt, err;
    REQUIRE(exportSectionDxf(r, o, {}, &err, &txt));
    auto P = pairsOf(txt);
    // 단면선: LWPOLYLINE 4점, Y = 로컬 z + 40
    bool found = false;
    for (size_t i = 0; i < P.size(); ++i) {
        if (P[i] == std::make_pair(0, std::string("LWPOLYLINE"))) {
            std::vector<double> xs, ys; std::string layer;
            for (size_t k = i + 1; k < P.size() && P[k].first != 0; ++k) {
                if (P[k].first == 8) layer = P[k].second;
                if (P[k].first == 10) xs.push_back(std::stod(P[k].second));
                if (P[k].first == 20) ys.push_back(std::stod(P[k].second));
            }
            if (layer == "0.8") {
                found = true;
                REQUIRE(ys.size() == 4);
                CHECK(ys[0] == Approx(45.12)); CHECK(ys[2] == Approx(44.70));
                CHECK(xs[3] == Approx(5.0));
                for (size_t k = 1; k < xs.size(); ++k) CHECK(std::hypot(xs[k] - xs[k - 1], ys[k] - ys[k - 1]) > 1e-6);
            }
        }
    }
    CHECK(found);
    // 레벨선 개수: 44.6 ~ 45.4 → 9개
    int lv = 0; std::set<std::string> labels; int labelCount = 0;
    for (size_t i = 0; i < P.size(); ++i) {
        if (P[i].first == 8 && (P[i].second == "0.2" || P[i].second == "0.5") && P[i - 4].second == "LINE") ++lv;
        if (P[i].first == 1 && P[i].second.size() == 4 && P[i].second[2] == '.') { labels.insert(P[i].second); ++labelCount; }
    }
    CHECK(lv == 9);
    CHECK(labels.count("45.0") == 1);   // 50 cm 마다, 소수 1자리
    CHECK(labels.count("45.3") == 0);   // 10 cm 선에는 라벨 없음
    // 핸들 유일
    std::set<std::string> handles; bool dupHandle = false;
    for (auto& p : P) if (p.first == 5 || p.first == 105) { if (!handles.insert(p.second).second) dupHandle = true; }
    CHECK_FALSE(dupHandle);
    // IMAGE 배치: 삽입점 (0, 44.6), 픽셀 벡터 0.004
    CHECK(txt.find("IMAGE\n") != std::string::npos);
    CHECK(txt.find("IMAGEDEF\n") != std::string::npos);
    CHECK(txt.find("sec_image.png") != std::string::npos);
    CHECK(txt.find("ACAD_IMAGE_DICT") != std::string::npos);
}

TEST_CASE("DXF 라벨 간격: 1/100 이면 10cm 마다 라벨 안 붙임(겹침 방지)") {
    auto r = sampleResult();
    DxfExportOptions o; o.scaleDenom = 100;  // 글자 20 cm
    std::string txt, err;
    REQUIRE(exportSectionDxf(r, o, {}, &err, &txt));
    CHECK(txt.find("\n1\n45.3\n") == std::string::npos);
    CHECK(txt.find("\n1\n45.0\n") != std::string::npos);
}

TEST_CASE("DXF 3D: 실좌표(EPSG) 수직면 배치") {
    auto r = sampleResult();
    DxfExportOptions o; o.mode = DxfCoordMode::World3D;
    o.image = true; o.imageFile = "x.png"; o.imageW = 100; o.imageH = 20; o.imageS0 = 0; o.imageZ0 = 4.6; o.imageRes = 0.05;
    std::string txt, err;
    REQUIRE(exportSectionDxf(r, o, {}, &err, &txt));
    Vec3 w = sectionToWorld(r, 5.0, 4.75);
    CHECK(w.x == Approx(200013.0)); CHECK(w.y == Approx(450024.0)); CHECK(w.z == Approx(44.75));
    CHECK(txt.find("AcDb3dPolyline") != std::string::npos);
    CHECK(txt.find("200013.0") != std::string::npos);
    CHECK(txt.find("\n210\n") != std::string::npos);  // 수직면 문자(OCS)
}

TEST_CASE("DXF 파일 저장(ezdxf 감사는 tests/dxf_audit.py)") {
    auto dir = tmpDir("dxf");
    auto r = sampleResult();
    DxfExportOptions o; o.image = true; o.imageFile = "sec_image.png"; o.imageW = 10; o.imageH = 10; o.imageRes = 0.1; o.imageZ0 = 4.6;
    std::string err;
    REQUIRE(exportSectionDxf(r, o, dir / "sec.dxf", &err));
    o.mode = DxfCoordMode::World3D;
    REQUIRE(exportSectionDxf(r, o, dir / "sec3d.dxf", &err));
    CHECK(std::filesystem::file_size(dir / "sec.dxf") > 1000);
}

TEST_CASE("DXF: 닫힌 단면 고리는 닫힘 플래그 LWPOLYLINE, 단면선 레이어 색 = 빨강(1)") {
    SectionResult r;
    r.line = SectionLine{{0, 0}, {10, 0}, 0, 0.5};
    r.zMin = 4; r.zMax = 6;
    Polyline loop{{4, 5}, {5, 5}, {5, 5.5}, {4, 5.5}, {4, 5}};
    r.profile = {Polyline{{0, 4.5}, {10, 4.5}}, loop};
    DxfExportOptions o;
    std::string txt, err;
    auto dir = tmpDir("dxfloop");
    REQUIRE(exportSectionDxf(r, o, dir / "l.dxf", &err, &txt));
    CHECK(txt.find("\n2\n0.8\n70\n0\n62\n1\n") != std::string::npos);
    // 두 번째 LWPOLYLINE: 꼭짓점 4개, 70 = 1
    CHECK(txt.find("AcDbPolyline\n90\n4\n70\n1\n") != std::string::npos);
}

TEST_CASE("DXF 그리기 순서: 레벨선 → IMAGE → 단면선, 라벨은 50 cm 마다 소수 1자리") {
    SectionResult r;
    r.line = SectionLine{{0, 0}, {10, 0}, 0, 0.5};
    r.zMin = 4.0; r.zMax = 6.0;
    r.srs.origin = Vec3(0, 0, 74.0);  // 표고 78.0 ~ 80.0
    r.profile = {Polyline{{0, 5.0}, {10, 5.2}}};
    DxfExportOptions o; o.image = true; o.imageFile = "x.png"; o.imageW = 10; o.imageH = 10; o.imageRes = 0.1; o.imageZ0 = 4.0;
    std::string txt, err;
    auto dir = tmpDir("dxforder");
    REQUIRE(exportSectionDxf(r, o, dir / "o.dxf", &err, &txt));
    size_t ent = txt.find("\n2\nENTITIES\n");
    size_t firstLevel = txt.find("\n8\n0.2\n", ent), img = txt.find("\nIMAGE\n", ent), prof = txt.find("\n8\n0.8\n", ent);
    REQUIRE(firstLevel != std::string::npos); REQUIRE(img != std::string::npos); REQUIRE(prof != std::string::npos);
    CHECK(firstLevel < img);
    CHECK(img < prof);
    CHECK(txt.find("\n1\n78.5\n", ent) != std::string::npos);
    CHECK(txt.find("\n1\n79.0\n", ent) != std::string::npos);
    CHECK(txt.find("\n1\n78.6\n", ent) == std::string::npos);   // 10 cm 선에는 라벨 없음
    CHECK(txt.find("\n1\n78.50\n", ent) == std::string::npos);  // 소수 2자리 아님
}

// 디자인 v5 결정 L — 레벨선 레이어 굵기: 10 cm = levelMinorPt(기본 0.2 pt → 9), 50 cm · 1 m = levelMajorPt(기본 0.5 pt → 18).
// 레이어 이름은 0.2 · 0.5 · 0.8(사용자 2026-10-16 「0.2 0.5 0.8로 레이어를 잡아 내보내기」)
static int layerLw(const std::vector<std::pair<int, std::string>>& P, const std::string& name) {
    for (size_t i = 0; i + 1 < P.size(); ++i)
        if (P[i] == std::make_pair(2, name))
            for (size_t j = i; j < P.size() && j < i + 12; ++j) if (P[j].first == 370) return std::stoi(P[j].second);
    return -99;
}
TEST_CASE("DXF 레벨선 레이어 굵기는 pt 설정을 따른다(기본 0.2 / 0.5 pt)", "[dxf][pt]") {
    auto r = sampleResult();
    std::string txt, err;
    {
        DxfExportOptions o;
        REQUIRE(exportSectionDxf(r, o, {}, &err, &txt));
        auto P = pairsOf(txt);
        CHECK(layerLw(P, "0.2") == 9);
        CHECK(layerLw(P, "0.5") == 18);   // 1 m 선도 0.5 레이어
    }
    {
        DxfExportOptions o; o.levelMinorPt = 1.0; o.levelMajorPt = 2.0;
        REQUIRE(exportSectionDxf(r, o, {}, &err, &txt));
        auto P = pairsOf(txt);
        CHECK(layerLw(P, "0.2") == 35);
        CHECK(layerLw(P, "0.5") == 70);
    }
}

// 사용자 2026-10-16 「내보내기할때 0.2 0.5 0.8로 레이어를 잡아 내보내기」 — SVG 와 같은 이름. 빨간 잘린 선 = 0.8(선 굵기 30),
// 50 cm(1 m 포함) 해발선 = 0.5(18), 10 cm 해발선 = 0.2(9). 옛 이름(SECTION_PROFILE · LEVEL_10CM · LEVEL_50CM · LEVEL_1M)은 없다
TEST_CASE("DXF 레이어 이름 0.2 · 0.5 · 0.8: 2D · 3D 모두, 1 m 선은 0.5", "[dxf][layers]") {
    auto r = sampleResult();   // 44.6 – 45.4 m: 45.0 이 1 m 선
    for (auto mode : {DxfCoordMode::Drawing2D, DxfCoordMode::World3D}) {
        DxfExportOptions o; o.mode = mode;
        std::string txt, err;
        REQUIRE(exportSectionDxf(r, o, {}, &err, &txt));
        auto P = pairsOf(txt);
        CHECK(layerLw(P, "0.8") == 30);
        CHECK(layerLw(P, "0.5") == 18);
        CHECK(layerLw(P, "0.2") == 9);
        for (const char* old : {"SECTION_PROFILE", "LEVEL_10CM", "LEVEL_50CM", "LEVEL_1M"})
            CHECK(txt.find(std::string("\n") + old + "\n") == std::string::npos);
        // 1 m(45.0) 선은 0.5, 45.1 은 0.2
        std::map<std::string, std::string> layerAtZ;
        for (size_t i = 0; i < P.size(); ++i) {
            if (P[i] != std::make_pair(0, std::string("LINE"))) continue;
            std::string layer, z;
            for (size_t k = i + 1; k < P.size() && P[k].first != 0; ++k) {
                if (P[k].first == 8) layer = P[k].second;
                if (P[k].first == (mode == DxfCoordMode::World3D ? 30 : 20)) z = P[k].second;
            }
            layerAtZ[z.substr(0, 4)] = layer;
        }
        CHECK(layerAtZ["45.0"] == "0.5");
        CHECK(layerAtZ["45.5"].empty());   // 범위 밖
        CHECK(layerAtZ["45.1"] == "0.2");
        std::string rep;
        CHECK(checkDrawingDxfLayers(txt, true, &rep));
        INFO(rep);
    }
}

TEST_CASE("DXF 레이어 점검(--dxf-layers-check): 옛 이름 · 빠진 레이어 · 굵기 틀림 · 해발선 없음을 잡는다", "[dxf][layers]") {
    auto mk = [](const char* cut, const char* major, const char* minor, int cutLw, bool levels) {
        DxfWriter d;
        d.addLayer(cut, 1, cutLw);
        d.addLayer(major, 8, 18);
        d.addLayer(minor, 9, 9);
        d.addLayer("LEVEL_TEXT", 7, 18);
        if (levels) { d.line(major, Vec3(0, 45.0, 0), Vec3(5, 45.0, 0)); d.line(minor, Vec3(0, 45.1, 0), Vec3(5, 45.1, 0)); }
        d.polyline2d(cut, {Vec2(0, 45.2), Vec2(5, 45.3)});
        return d.str();
    };
    std::string rep;
    CHECK(checkDrawingDxfLayers(mk("0.8", "0.5", "0.2", 30, true), true, &rep));
    CHECK_FALSE(checkDrawingDxfLayers(mk("SECTION_PROFILE", "LEVEL_50CM", "LEVEL_10CM", 30, true), true, &rep));
    CHECK_FALSE(checkDrawingDxfLayers(mk("0.8", "0.5", "0.2", 18, true), true, &rep));      // 0.8 굵기 틀림
    CHECK_FALSE(checkDrawingDxfLayers(mk("0.8", "0.5", "0.2", 30, false), true, &rep));     // 해발선 없음
    CHECK(checkDrawingDxfLayers(mk("0.8", "0.5", "0.2", 30, false), false, &rep));          // 해발선 끈 도면
    // 줄 끝 CRLF(Windows 텍스트 모드로 읽은 파일)도 같게 읽는다
    std::string crlf;
    for (char ch : mk("0.8", "0.5", "0.2", 30, true)) { if (ch == '\n') crlf += '\r'; crlf += ch; }
    CHECK(checkDrawingDxfLayers(crlf, true, &rep));
    INFO(rep);
}
