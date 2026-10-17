// vector(VectorCraft/Trowel)로 넘기기 — 순수 계산. 설명은 asec/handoff.hpp
#include "asec/handoff.hpp"
#include <set>

namespace asec {

namespace {
bool isTrimChar(char c) { return c == ' ' || c == '.'; }
std::string stripTrailingSlash(std::string s) {
    while (s.size() > 1 && (s.back() == '/' || s.back() == '\\')) s.pop_back();
    return s;
}
}  // namespace

std::string handoffFileName(const std::string& title, const std::string& stamp, const std::string& ext) {
    static const std::string bad = "\\/:*?\"<>|";
    std::string stem = title;
    for (char& c : stem) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (u < 0x20 || u == 0x7F || bad.find(c) != std::string::npos) c = '_';
    }
    auto trim = [](std::string& s) {
        size_t b = 0;
        while (b < s.size() && isTrimChar(s[b])) ++b;
        size_t e = s.size();
        while (e > b && isTrimChar(s[e - 1])) --e;
        s = s.substr(b, e - b);
    };
    trim(stem);
    const size_t kMaxBytes = 80;
    if (stem.size() > kMaxBytes) {   // UTF-8 이어지는 바이트(10xxxxxx) 앞에서 끊지 않는다
        size_t cut = kMaxBytes;
        while (cut > 0 && (static_cast<unsigned char>(stem[cut]) & 0xC0) == 0x80) --cut;
        stem.resize(cut);
        trim(stem);
    }
    if (stem.empty()) stem = "sheet";
    std::string name = stem;
    if (!stamp.empty()) name += "_" + stamp;
    if (!ext.empty()) name += "." + ext;
    return name;
}

std::vector<std::string> vectorExeNames() {
#ifdef _WIN32
    return {"vectorcraft.exe", "trowel.exe"};
#else
    return {"vectorcraft", "trowel"};
#endif
}

std::vector<std::string> vectorExeCandidates(const std::string& appDir, const std::string& configured,
                                             const std::vector<std::string>& exeNames) {
    static const char* const kDirs[] = {"", "vector", "../vector", "VectorCraft", "../VectorCraft", "Trowel", "../Trowel"};
    std::vector<std::string> out;
    std::set<std::string> seen;
    auto add = [&](const std::string& p) {
        if (!p.empty() && seen.insert(p).second) out.push_back(p);
    };
    add(configured);
    const std::string base = stripTrailingSlash(appDir);
    for (const char* d : kDirs) {
        const std::string dir = *d ? base + "/" + d : base;
        for (const std::string& n : exeNames) add(dir + "/" + n);
    }
    return out;
}

std::string findVectorExe(const std::string& appDir, const std::string& configured,
                          const std::function<bool(const std::string&)>& exists,
                          const std::vector<std::string>& exeNames) {
    for (const std::string& p : vectorExeCandidates(appDir, configured, exeNames))
        if (exists(p)) return p;
    return {};
}

HandoffCommand vectorCommand(const std::string& exe, const std::string& file) {
    HandoffCommand c;
    c.program = exe;
    c.args = {file};
    const size_t slash = exe.find_last_of("/\\");
    if (slash != std::string::npos && slash > 0) c.workDir = exe.substr(0, slash);
    return c;
}

std::string handoffDefaultDir(const std::string& root) { return stripTrailingSlash(root) + "/exports"; }

}  // namespace asec
