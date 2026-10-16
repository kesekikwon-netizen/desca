// 프로젝트 파일(<모델이름>.trench.json) 읽기·쓰기 — 사용자 결정 2026-10-11 「C: 앱 폴더 projects\ 자동 저장 + 저장/열기」.
// 레지스트리(QSettings)에만 남던 작업 상태(단면 목록·단면선·깊이·입면·카메라·조판 설정)를 파일로 옮겨 다른 PC·포터블에서도 이어서 작업한다.
#include "catch_amalgamated.hpp"
#include "asec/project.hpp"

using namespace asec;

static ProjectDoc sample() {
    ProjectDoc p;
    p.modelPath = "H:/3sm/화원 성산리 석곽.3sm";
    p.modelName = "화원 성산리 석곽.3sm";
    p.srsLabel = "EPSG:5187 h=ellipsoidal(GRS80)";
    SavedSection a; a.name = "A–A′"; a.ax = 153528; a.ay = 356975.4; a.bx = 153541; a.by = 356975.4; a.front = 0; a.back = 3; a.backUserSet = false; a.note = "";
    SavedSection b; b.name = "B–B′"; b.ax = 153530; b.ay = 356971; b.bx = 153538; b.by = 356980; b.front = 0.25; b.back = 0.5; b.backUserSet = true; b.note = "북벽";
    p.sections = {a, b};
    p.current = 1;
    p.front = 0.25; p.back = 0.5; p.backUserSet = true;
    p.hasLine = true; p.ax = 153530; p.ay = 356971; p.bx = 153538; p.by = 356980;
    p.opacity = 70; p.lineWidthPx = 3.0; p.smooth = true; p.depthFade = false;
    p.hasCamera = true; p.camX = 153534.5; p.camY = 356975.25; p.metersPerPixel = 0.0123;
    p.hasPlanRotation = true; p.planRotationDeg = -32.5;
    p.sectionSheet.present = true; p.sectionSheet.paper = 1; p.sectionSheet.landscape = false; p.sectionSheet.denom = 60; p.sectionSheet.format = 2;
    p.sectionSheet.dpi = 150; p.sectionSheet.split = true; p.sectionSheet.withImage = false; p.sectionSheet.withLine = true; p.sectionSheet.withLevels = false;
    p.sectionSheet.withTicks = false; p.sectionSheet.withTitle = true; p.sectionSheet.withPlan = false; p.sectionSheet.levelMinorPt = 0.25; p.sectionSheet.levelMajorPt = 0.7;
    p.sectionSheet.hatchKind = 2; p.sectionSheet.hatchMm = 1.5;
    p.planSheet.present = true; p.planSheet.paper = 0; p.planSheet.landscape = true; p.planSheet.denom = 100; p.planSheet.format = 0;
    p.savedAt = "2026-10-11T14:30:00";
    return p;
}

TEST_CASE("프로젝트 파일: 쓰고 읽으면 모든 값이 그대로", "[project]") {
    const ProjectDoc p = sample();
    const std::string text = projectToJson(p);
    REQUIRE(text.find("\"format\": \"Trench.project\"") != std::string::npos);
    ProjectDoc q; std::string err;
    REQUIRE(projectFromJson(text, q, &err));
    CHECK(err.empty());
    CHECK(q.modelPath == p.modelPath);
    CHECK(q.modelName == p.modelName);
    CHECK(q.srsLabel == p.srsLabel);
    REQUIRE(q.sections.size() == 2);
    CHECK(q.sections[1].name == "B–B′");
    CHECK(q.sections[1].ax == Catch::Approx(153530));
    CHECK(q.sections[1].by == Catch::Approx(356980));
    CHECK(q.sections[1].front == Catch::Approx(0.25));
    CHECK(q.sections[1].back == Catch::Approx(0.5));
    CHECK(q.sections[1].backUserSet == true);
    CHECK(q.sections[1].note == "북벽");
    CHECK(q.current == 1);
    CHECK(q.front == Catch::Approx(0.25)); CHECK(q.back == Catch::Approx(0.5)); CHECK(q.backUserSet == true);
    CHECK(q.hasLine == true); CHECK(q.ax == Catch::Approx(153530)); CHECK(q.ay == Catch::Approx(356971)); CHECK(q.bx == Catch::Approx(153538)); CHECK(q.by == Catch::Approx(356980));
    CHECK(q.opacity == 70); CHECK(q.lineWidthPx == Catch::Approx(3.0)); CHECK(q.smooth == true); CHECK(q.depthFade == false);
    CHECK(q.hasCamera == true); CHECK(q.camX == Catch::Approx(153534.5)); CHECK(q.camY == Catch::Approx(356975.25)); CHECK(q.metersPerPixel == Catch::Approx(0.0123));
    CHECK(q.hasPlanRotation == true); CHECK(q.planRotationDeg == Catch::Approx(-32.5));   // 평면 방향(2026-10-16)
    CHECK(q.sectionSheet.present == true); CHECK(q.sectionSheet.paper == 1); CHECK(q.sectionSheet.landscape == false); CHECK(q.sectionSheet.denom == Catch::Approx(60));
    CHECK(q.sectionSheet.format == 2); CHECK(q.sectionSheet.dpi == Catch::Approx(150)); CHECK(q.sectionSheet.split == true);
    CHECK(q.sectionSheet.withImage == false); CHECK(q.sectionSheet.withLine == true); CHECK(q.sectionSheet.withLevels == false);
    CHECK(q.sectionSheet.withTicks == false); CHECK(q.sectionSheet.withTitle == true); CHECK(q.sectionSheet.withPlan == false);
    CHECK(q.sectionSheet.levelMinorPt == Catch::Approx(0.25)); CHECK(q.sectionSheet.levelMajorPt == Catch::Approx(0.7));
    CHECK(q.sectionSheet.hatchKind == 2); CHECK(q.sectionSheet.hatchMm == Catch::Approx(1.5));
    CHECK(q.planSheet.present == true); CHECK(q.planSheet.paper == 0); CHECK(q.planSheet.landscape == true); CHECK(q.planSheet.denom == Catch::Approx(100)); CHECK(q.planSheet.format == 0);
    CHECK(q.savedAt == "2026-10-11T14:30:00");
}

TEST_CASE("프로젝트 파일: 단면 목록은 .sections.json 과 같은 모양이라 그대로 가져올 수 있다", "[project]") {
    const std::string text = projectToJson(sample());
    // 단면 목록 부분만 떼어 sectionsFromJson 으로 읽어도 같은 두 단면
    ProjectDoc q; std::string err;
    REQUIRE(projectFromJson(text, q, &err));
    const std::string secOnly = sectionsToJson(q.sections, q.modelName, q.srsLabel, q.current);
    std::vector<SavedSection> v; int cur = -1;
    REQUIRE(sectionsFromJson(secOnly, v, &cur, &err));
    REQUIRE(v.size() == 2);
    CHECK(v[0].name == "A–A′"); CHECK(v[1].note == "북벽"); CHECK(cur == 1);
}

TEST_CASE("프로젝트 파일: 최소 내용(형식 + 모델)만 있어도 읽히고 나머지는 기본값", "[project]") {
    ProjectDoc q; std::string err;
    REQUIRE(projectFromJson("{\"format\":\"Trench.project\",\"version\":1,\"model\":{\"path\":\"D:/x/a.3mx\"}}", q, &err));
    CHECK(q.modelPath == "D:/x/a.3mx");
    CHECK(q.modelName == "a.3mx");   // 경로에서 이름을 채운다
    CHECK(q.sections.empty());
    CHECK(q.current == -1);
    CHECK(q.hasLine == false);
    CHECK(q.hasCamera == false);
    CHECK(q.hasPlanRotation == false);   // 옛 파일 = 방향을 정하지 않음(평·단면도는 첫 단면 기준 자동)
    CHECK(q.back == Catch::Approx(kDefaultBackDepth));
    CHECK(q.opacity == 100);
    CHECK(q.lineWidthPx == Catch::Approx(2.0));
    CHECK(q.smooth == false);
    CHECK(q.sectionSheet.present == false);
    CHECK(q.planSheet.present == false);
}

TEST_CASE("프로젝트 파일: 형식이 다르거나 깨진 JSON 은 거절하고 까닭을 적는다", "[project]") {
    ProjectDoc q; std::string err;
    CHECK_FALSE(projectFromJson("{\"format\":\"ExcavSection.sections\",\"sections\":[]}", q, &err));   // 단면 목록 파일은 프로젝트가 아니다
    CHECK_FALSE(err.empty());
    err.clear();
    CHECK_FALSE(projectFromJson("{ not json", q, &err));
    CHECK_FALSE(err.empty());
    err.clear();
    CHECK_FALSE(projectFromJson("{\"format\":\"Trench.project\",\"version\":1}", q, &err));   // 모델 경로 없음
    CHECK_FALSE(err.empty());
}

TEST_CASE("프로젝트 파일: 단면 한 줄이 깨져도 나머지는 읽고 current 는 범위 안으로", "[project]") {
    ProjectDoc q; std::string err;
    const std::string text =
        "{\"format\":\"Trench.project\",\"version\":1,\"model\":{\"path\":\"D:/x/a.3mx\"},"
        "\"current\":7,"
        "\"sections\":[{\"name\":\"A–A′\",\"a\":[1,2],\"b\":[3,4]},{\"name\":\"깨짐\",\"a\":\"x\"},{\"name\":\"C–C′\",\"a\":[5,6],\"b\":[7,8],\"back\":1.5}]}";
    REQUIRE(projectFromJson(text, q, &err));
    REQUIRE(q.sections.size() == 2);
    CHECK(q.sections[0].name == "A–A′"); CHECK(q.sections[0].back == Catch::Approx(kDefaultBackDepth));
    CHECK(q.sections[1].name == "C–C′"); CHECK(q.sections[1].back == Catch::Approx(1.5));
    CHECK(q.current == 0);   // 7 은 범위 밖 → 첫 단면
}

TEST_CASE("프로젝트 파일 이름: 모델 이름 + .trench.json, 경로 구분자·금지 글자는 _", "[project]") {
    CHECK(projectFileName("화원 성산리 석곽.3sm") == "화원 성산리 석곽.trench.json");
    CHECK(projectFileName("Production_2.3mx") == "Production_2.trench.json");
    CHECK(projectFileName("a:b/c?.obj") == "a_b_c_.trench.json");
    CHECK(projectFileName("noext") == "noext.trench.json");
    CHECK(isProjectFileName("x.trench.json"));
    CHECK(isProjectFileName("X.TRENCH.JSON"));
    CHECK_FALSE(isProjectFileName("x.sections.json"));
    CHECK_FALSE(isProjectFileName("x.3mx"));
}

TEST_CASE("프로젝트 파일: 0개 단면 저장 및 복원 시 빈 목록 유지", "[project]") {
    ProjectDoc p;
    p.modelPath = "D:/x/test.3mx";
    p.modelName = "test.3mx";
    p.sections.clear();
    p.current = -1;
    p.hasLine = false;
    const std::string text = projectToJson(p);
    ProjectDoc q; std::string err;
    REQUIRE(projectFromJson(text, q, &err));
    CHECK(q.sections.empty());
    CHECK(q.current == -1);
    CHECK(q.hasLine == false);
}

TEST_CASE("프로젝트 파일: 10 m 입면 깊이 및 단면별 10 m 뒤 깊이 보존", "[project]") {
    ProjectDoc p;
    p.modelPath = "D:/x/test.3mx";
    p.modelName = "test.3mx";
    p.front = 10.0;
    p.back = 10.0;
    p.backUserSet = true;
    SavedSection s;
    s.name = "A–A′";
    s.ax = 1.0; s.ay = 2.0; s.bx = 3.0; s.by = 4.0;
    s.front = 10.0;
    s.back = 10.0;
    s.backUserSet = true;
    p.sections.push_back(s);
    p.current = 0;
    p.hasLine = true;
    p.ax = 1.0; p.ay = 2.0; p.bx = 3.0; p.by = 4.0;
    const std::string text = projectToJson(p);
    ProjectDoc q; std::string err;
    REQUIRE(projectFromJson(text, q, &err));
    CHECK(q.front == Catch::Approx(10.0));
    CHECK(q.back == Catch::Approx(10.0));
    CHECK(q.backUserSet == true);
    REQUIRE(q.sections.size() == 1);
    CHECK(q.sections[0].front == Catch::Approx(10.0));
    CHECK(q.sections[0].back == Catch::Approx(10.0));
    CHECK(q.sections[0].backUserSet == true);
}

TEST_CASE("프로젝트 파일: 0개 단면일 때 current가 0 이상으로 기록되어 있어도 -1로 강제", "[project]") {
    const std::string text =
        "{\"format\":\"Trench.project\",\"version\":1,\"model\":{\"path\":\"D:/x/test.3mx\"},"
        "\"current\":5,"
        "\"sections\":[]}";
    ProjectDoc q; std::string err;
    REQUIRE(projectFromJson(text, q, &err));
    CHECK(q.sections.empty());
    CHECK(q.current == -1);
}
