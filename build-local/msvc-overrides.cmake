# Local MSVC build only: the code uses [=, this] captures (needs C++20 in MSVC) and u8string() as std::string (C++17),
# so map the C++17 request to C++20 with char8_t off. Product code is unchanged.
set(CMAKE_CXX17_STANDARD_COMPILE_OPTION "-std:c++20;-Zc:char8_t-")
set(CMAKE_CXX17_EXTENSION_COMPILE_OPTION "-std:c++20;-Zc:char8_t-")
# Local MSVC build only: embed the app icon in SectionViewer.exe (the repo adds packaging/app.rc only in the MinGW branch).
# Deferred to the end of the top-level directory so the target exists; repo CMakeLists unchanged.
get_property(_asec_icon_deferred GLOBAL PROPERTY ASEC_ICON_DEFERRED)
if(MSVC AND NOT _asec_icon_deferred)
  set_property(GLOBAL PROPERTY ASEC_ICON_DEFERRED ON)
  # DEFER evaluates its arguments late, so bake the path in now via EVAL
  cmake_language(EVAL CODE "cmake_language(DEFER DIRECTORY [[${CMAKE_SOURCE_DIR}]] CALL
    target_sources SectionViewer PRIVATE [[${CMAKE_CURRENT_LIST_DIR}/icon.rc]])")
endif()
