#pragma once
// 도면을 다른 앱(vector = VectorCraft/Trowel, https://github.com/kesekikwon-netizen/vector)으로 넘기기 —
// 순수 계산(파일 이름 · 실행 파일 후보 · 실행 명령)만. Qt 없음(core). 실제 내보내기 · 실행 · 설정 저장은
// app/sheetexport.cpp 가 한다. vector(vectorcraft.exe)는 파일 경로를 위치 인자로 받아 그대로 연다(SVG 는 대화 상자 없이).
#include <functional>
#include <string>
#include <vector>

namespace asec {

/// 넘길 파일 이름 "<도면명>_<stamp>.<ext>": Windows 금지 글자(\ / : * ? " < > |) · 제어 문자는 '_',
/// 앞뒤 공백 · 점은 뗀다. 도면명이 비면 "sheet". 도면명은 UTF-8 바이트 80 까지만(글자 가운데에서 자르지 않음)
std::string handoffFileName(const std::string& title, const std::string& stamp, const std::string& ext);

/// 이 플랫폼의 vector 실행 파일 이름 후보(Windows 는 .exe)
std::vector<std::string> vectorExeNames();

/// vector 실행 파일을 찾아볼 경로(순서대로): configured(비어 있지 않으면 맨 앞, 그대로) →
/// appDir/<이름> → appDir/vector/<이름> → appDir/../vector/<이름> → appDir/VectorCraft/… → appDir/../VectorCraft/… →
/// appDir/Trowel/… → appDir/../Trowel/…  ('/' 구분자, appDir 끝의 '/' 는 뗀다)
std::vector<std::string> vectorExeCandidates(const std::string& appDir, const std::string& configured,
                                             const std::vector<std::string>& exeNames = vectorExeNames());

/// 후보 가운데 exists 가 참인 첫 경로. 없으면 ""
std::string findVectorExe(const std::string& appDir, const std::string& configured,
                          const std::function<bool(const std::string&)>& exists,
                          const std::vector<std::string>& exeNames = vectorExeNames());

/// 실행 명령: 프로그램 · 인자(파일 하나) · 작업 폴더(실행 파일이 있는 폴더 — 옆의 자원을 찾게)
struct HandoffCommand {
    std::string program;
    std::vector<std::string> args;
    std::string workDir;
};
HandoffCommand vectorCommand(const std::string& exe, const std::string& file);

/// 보낼 파일을 두는 기본 폴더: <root>/exports (root = 앱 폴더 또는 --settings 폴더)
std::string handoffDefaultDir(const std::string& root);

}  // namespace asec
