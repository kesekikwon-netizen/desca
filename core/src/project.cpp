#include "asec/project.hpp"

#include <algorithm>
#include <cctype>

#include "json.hpp"

namespace asec {
namespace {

nlohmann::json sheetToJson(const ProjectSheet& s) {
    return {{"paper", s.paper}, {"landscape", s.landscape}, {"denom", s.denom}, {"format", s.format}, {"dpi", s.dpi}, {"split", s.split},
            {"withImage", s.withImage}, {"withLine", s.withLine}, {"withLevels", s.withLevels}, {"withTicks", s.withTicks}, {"withTitle", s.withTitle}, {"withPlan", s.withPlan},
            {"levelMinorPt", s.levelMinorPt}, {"levelMajorPt", s.levelMajorPt}, {"hatchKind", s.hatchKind}, {"hatchMm", s.hatchMm}};
}

ProjectSheet sheetFromJson(const nlohmann::json& j) {
    ProjectSheet s;
    if (!j.is_object()) return s;
    s.present = true;
    s.paper = j.value("paper", s.paper); s.landscape = j.value("landscape", s.landscape); s.denom = j.value("denom", s.denom);
    s.format = j.value("format", s.format); s.dpi = j.value("dpi", s.dpi); s.split = j.value("split", s.split);
    s.withImage = j.value("withImage", s.withImage); s.withLine = j.value("withLine", s.withLine); s.withLevels = j.value("withLevels", s.withLevels);
    s.withTicks = j.value("withTicks", s.withTicks); s.withTitle = j.value("withTitle", s.withTitle); s.withPlan = j.value("withPlan", s.withPlan);
    s.levelMinorPt = j.value("levelMinorPt", s.levelMinorPt); s.levelMajorPt = j.value("levelMajorPt", s.levelMajorPt);
    s.hatchKind = j.value("hatchKind", s.hatchKind); s.hatchMm = j.value("hatchMm", s.hatchMm);
    return s;
}

std::string baseName(const std::string& path) {
    const size_t cut = path.find_last_of("/\\");
    return cut == std::string::npos ? path : path.substr(cut + 1);
}

}  // namespace

std::string projectToJson(const ProjectDoc& p) {
    nlohmann::json j;
    j["format"] = "Trench.project";
    j["version"] = 1;
    j["savedAt"] = p.savedAt;
    j["model"] = {{"path", p.modelPath}, {"name", p.modelName.empty() ? baseName(p.modelPath) : p.modelName}, {"srs", p.srsLabel}};
    j["coords"] = "world (SRSOrigin applied), metres";
    j["current"] = p.current;
    auto& a = j["sections"] = nlohmann::json::array();
    for (const SavedSection& s : p.sections)
        a.push_back({{"name", s.name}, {"a", {s.ax, s.ay}}, {"b", {s.bx, s.by}}, {"front", s.front}, {"back", s.back}, {"backUserSet", s.backUserSet}, {"note", s.note}});
    j["elevation"] = {{"front", p.front}, {"back", p.back}, {"backUserSet", p.backUserSet}, {"opacity", p.opacity}, {"lineWidthPx", p.lineWidthPx}, {"smooth", p.smooth}, {"depthFade", p.depthFade}};
    if (p.hasLine) j["line"] = {{"a", {p.ax, p.ay}}, {"b", {p.bx, p.by}}};
    if (p.hasCamera) j["camera"] = {{"x", p.camX}, {"y", p.camY}, {"metersPerPixel", p.metersPerPixel}};
    if (p.sectionSheet.present) j["sectionSheet"] = sheetToJson(p.sectionSheet);
    if (p.planSheet.present) j["planSheet"] = sheetToJson(p.planSheet);
    return j.dump(2);
}

bool projectFromJson(const std::string& text, ProjectDoc& out, std::string* err) {
    out = ProjectDoc();
    nlohmann::json j;
    try { j = nlohmann::json::parse(text); }
    catch (const std::exception& ex) { if (err) *err = std::string("JSON 이 깨졌습니다: ") + ex.what(); return false; }
    if (!j.is_object() || j.value("format", std::string()) != "Trench.project") { if (err) *err = "프로젝트 파일(Trench.project)이 아닙니다"; return false; }
    const auto m = j.value("model", nlohmann::json::object());
    out.modelPath = m.is_object() ? m.value("path", std::string()) : std::string();
    if (out.modelPath.empty()) { if (err) *err = "모델 경로(model.path)가 없습니다"; return false; }
    out.modelName = m.value("name", std::string());
    if (out.modelName.empty()) out.modelName = baseName(out.modelPath);
    out.srsLabel = m.value("srs", std::string());
    out.savedAt = j.value("savedAt", std::string());
    if (j.contains("sections") && j["sections"].is_array()) {
        for (const auto& e : j["sections"]) {
            try {
                SavedSection s;
                s.name = e.value("name", std::string());
                const auto A = e.at("a"), B = e.at("b");
                s.ax = A.at(0).get<double>(); s.ay = A.at(1).get<double>();
                s.bx = B.at(0).get<double>(); s.by = B.at(1).get<double>();
                s.front = e.value("front", 0.0);
                s.backUserSet = e.value("backUserSet", false);
                s.back = resolveBackDepth(e.contains("back") && e["back"].is_number(), e.value("back", kDefaultBackDepth), s.backUserSet);
                s.note = e.value("note", std::string());
                out.sections.push_back(s);
            } catch (const std::exception&) { /* 깨진 줄은 건너뛴다 */ }
        }
    }
    const int cur = j.value("current", -1);
    out.current = out.sections.empty() ? -1 : (cur >= 0 && cur < int(out.sections.size()) ? cur : 0);
    const auto el = j.value("elevation", nlohmann::json::object());
    if (el.is_object()) {
        out.front = el.value("front", out.front);
        out.backUserSet = el.value("backUserSet", false);
        out.back = resolveBackDepth(el.contains("back") && el["back"].is_number(), el.value("back", kDefaultBackDepth), out.backUserSet);
        out.opacity = std::clamp(el.value("opacity", out.opacity), 10, 100);
        out.lineWidthPx = el.value("lineWidthPx", out.lineWidthPx);
        out.smooth = el.value("smooth", out.smooth);
        out.depthFade = el.value("depthFade", out.depthFade);
    }
    const auto ln = j.value("line", nlohmann::json::object());
    if (ln.is_object() && ln.contains("a") && ln.contains("b")) {
        try {
            out.ax = ln["a"].at(0).get<double>(); out.ay = ln["a"].at(1).get<double>();
            out.bx = ln["b"].at(0).get<double>(); out.by = ln["b"].at(1).get<double>();
            out.hasLine = true;
        } catch (const std::exception&) { out.hasLine = false; }
    }
    const auto cam = j.value("camera", nlohmann::json::object());
    if (cam.is_object() && cam.contains("x") && cam.contains("y")) {
        out.camX = cam.value("x", 0.0); out.camY = cam.value("y", 0.0); out.metersPerPixel = cam.value("metersPerPixel", 0.0);
        out.hasCamera = out.metersPerPixel > 0;
    }
    if (j.contains("sectionSheet")) out.sectionSheet = sheetFromJson(j["sectionSheet"]);
    if (j.contains("planSheet")) out.planSheet = sheetFromJson(j["planSheet"]);
    return true;
}

std::string projectFileName(const std::string& modelFileName) {
    std::string stem = modelFileName;   // 이름만 받는다(경로 구분자가 섞여 있어도 떼지 않고 '_' 로)
    const size_t dot = stem.find_last_of('.');
    if (dot != std::string::npos && dot > 0) stem = stem.substr(0, dot);
    static const std::string bad = "\\/:*?\"<>|";
    for (char& c : stem) if (bad.find(c) != std::string::npos) c = '_';
    return stem + ".trench.json";
}

bool isProjectFileName(const std::string& name) {
    static const std::string suf = ".trench.json";
    if (name.size() < suf.size()) return false;
    const std::string tail = name.substr(name.size() - suf.size());
    return std::equal(tail.begin(), tail.end(), suf.begin(), [](char a, char b) { return std::tolower(static_cast<unsigned char>(a)) == b; });
}

}  // namespace asec
