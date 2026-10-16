@echo off
rem SectionViewer local Windows build (MSVC 2022 + OSGeo4W Qt 6). Output: D:\desca\build-msvc\app\SectionViewer.exe
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d D:\desca
if not exist build-msvc\build.ninja (
  cmake -S . -B build-msvc -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=D:/OSGeo4W/apps/Qt6;D:/OSGeo4W -DZLIB_ROOT=D:/OSGeo4W "-DCMAKE_CXX_FLAGS=/utf-8 /EHsc" "-DCMAKE_C_FLAGS=/utf-8" -DCMAKE_PROJECT_INCLUDE=D:/desca/build-local/msvc-overrides.cmake -DASEC_BUILD_TESTS=OFF -DASEC_BUILD_TOOLS=OFF || exit /b 1
)
cmake --build build-msvc || exit /b 1
if not exist build-msvc\app\Qt6Core.dll (
  D:\OSGeo4W\apps\Qt6\bin\windeployqt.exe --release --no-translations --no-system-d3d-compiler --no-opengl-sw build-msvc\app\SectionViewer.exe >nul || exit /b 1
  python -I build-local\copy-deps.py "%VCToolsInstallDir%bin\Hostx64\x64\dumpbin.exe" || exit /b 1
)
echo OK: D:\desca\build-msvc\app\SectionViewer.exe
