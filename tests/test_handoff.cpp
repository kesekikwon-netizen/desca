// vector(VectorCraft/Trowel)로 도면 넘기기 — 순수 부분(파일 이름 · 실행 파일 찾기 · 실행 명령). 사용자 2026-10-16
// 「ui버튼추가 누르면 내보내기를 이앱으로 들어가게」 · 「내보내기 경로나 바로가기를 지정할 수 있게」
#include "asec/handoff.hpp"
#include "catch_amalgamated.hpp"
#include <set>

using namespace asec;

TEST_CASE("handoff: 파일 이름은 도면명 + 시각, 금지 글자는 '_'", "[handoff]") {
    CHECK(handoffFileName("Production_1 (22) 평·단면도", "20261016-173200", "svg") ==
          "Production_1 (22) 평·단면도_20261016-173200.svg");
    CHECK(handoffFileName("A–A′ / B:B\"?*<>|\\", "s", "svg") == "A–A′ _ B_B________s.svg");
    CHECK(handoffFileName("  . 이름 . ", "s", "dxf") == "이름_s.dxf");
    CHECK(handoffFileName("", "s", "svg") == "sheet_s.svg");
    CHECK(handoffFileName("tab\there", "s", "svg") == "tab_here_s.svg");
    CHECK(handoffFileName("이름", "", "svg") == "이름.svg");   // 시각이 없으면 이름만
}

TEST_CASE("handoff: 긴 도면명은 UTF-8 글자 가운데를 자르지 않고 80 바이트 안으로", "[handoff]") {
    std::string longKo;
    for (int i = 0; i < 40; ++i) longKo += "가";   // 120 바이트
    const std::string f = handoffFileName(longKo, "s", "svg");
    const std::string stem = f.substr(0, f.size() - std::string("_s.svg").size());
    CHECK(stem.size() <= 80);
    CHECK(stem.size() % 3 == 0);   // '가' 는 3 바이트 — 글자 경계에서 잘림
    CHECK(stem == std::string(longKo, 0, stem.size()));
}

TEST_CASE("handoff: 실행 파일 후보는 설정값 → 앱 옆 → 옆 폴더 순서", "[handoff]") {
    const std::vector<std::string> names = {"vectorcraft.exe"};
    const auto c = vectorExeCandidates("C:/Trench/", "", names);
    REQUIRE(c.size() >= 4);
    CHECK(c[0] == "C:/Trench/vectorcraft.exe");
    CHECK(c[1] == "C:/Trench/vector/vectorcraft.exe");
    CHECK(c[2] == "C:/Trench/../vector/vectorcraft.exe");
    CHECK(std::set<std::string>(c.begin(), c.end()).size() == c.size());   // 겹침 없음
    const auto d = vectorExeCandidates("C:/Trench", "D:/Apps/Trowel/vectorcraft.exe", names);
    CHECK(d[0] == "D:/Apps/Trowel/vectorcraft.exe");
    CHECK(d[1] == "C:/Trench/vectorcraft.exe");
    // 이름이 둘이면 같은 폴더의 두 이름이 먼저, 그다음 폴더
    const auto e = vectorExeCandidates("/app", "", {"vectorcraft", "trowel"});
    CHECK(e[0] == "/app/vectorcraft");
    CHECK(e[1] == "/app/trowel");
    CHECK(e[2] == "/app/vector/vectorcraft");
}

TEST_CASE("handoff: 있는 첫 후보를 고르고, 없으면 빈 문자열", "[handoff]") {
    const std::vector<std::string> names = {"vectorcraft.exe"};
    std::set<std::string> present = {"C:/Trench/../vector/vectorcraft.exe"};
    auto exists = [&](const std::string& p) { return present.count(p) > 0; };
    CHECK(findVectorExe("C:/Trench", "", exists, names) == "C:/Trench/../vector/vectorcraft.exe");
    // 설정값이 사라졌으면 다음 후보로
    CHECK(findVectorExe("C:/Trench", "D:/old/vectorcraft.exe", exists, names) == "C:/Trench/../vector/vectorcraft.exe");
    present.insert("D:/old/vectorcraft.exe");
    CHECK(findVectorExe("C:/Trench", "D:/old/vectorcraft.exe", exists, names) == "D:/old/vectorcraft.exe");
    CHECK(findVectorExe("C:/Trench", "", [](const std::string&) { return false; }, names).empty());
}

TEST_CASE("handoff: 실행 명령은 파일 하나를 위치 인자로, 작업 폴더는 실행 파일 폴더", "[handoff]") {
    const HandoffCommand c = vectorCommand("D:/Apps/vector/vectorcraft.exe", "D:/Trench/exports/A_s.svg");
    CHECK(c.program == "D:/Apps/vector/vectorcraft.exe");
    REQUIRE(c.args.size() == 1);
    CHECK(c.args[0] == "D:/Trench/exports/A_s.svg");
    CHECK(c.workDir == "D:/Apps/vector");
    CHECK(vectorCommand("vectorcraft.exe", "a.svg").workDir.empty());   // 폴더 없는 이름이면 비움
    CHECK(vectorCommand("C:\\v\\vectorcraft.exe", "a.svg").workDir == "C:\\v");
    CHECK(handoffDefaultDir("D:/Trench") == "D:/Trench/exports");
    CHECK(handoffDefaultDir("D:/Trench/") == "D:/Trench/exports");
}

TEST_CASE("handoff: 플랫폼 실행 파일 이름", "[handoff]") {
    const auto n = vectorExeNames();
    REQUIRE_FALSE(n.empty());
#ifdef _WIN32
    CHECK(n[0] == "vectorcraft.exe");
#else
    CHECK(n[0] == "vectorcraft");
#endif
}
