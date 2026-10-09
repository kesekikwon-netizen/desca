# 발굴 단면뷰어 (Qt Widgets + OpenGL). moc 없이 빌드(Q_OBJECT 미사용) → 교차 빌드에서 Qt 도구 불필요
set(APP_SRC main.cpp mainwindow.cpp mainwindow_ui.cpp sheetexport.cpp planview.cpp sectionview.cpp)
# 아이콘: SVG 글자를 configure 때 헤더로 굽는다(icons/icons.cmake). rcc 없이 두 빌드 분기 모두 같은 헤더
include(${CMAKE_CURRENT_LIST_DIR}/icons/icons.cmake)
set(KERF_ICONS_HPP ${CMAKE_BINARY_DIR}/generated/kerf_icons.hpp)
kerf_write_icons_header(${KERF_ICONS_HPP})
if(ASEC_QT_WIN_DIR)
  # Windows 교차 빌드: MinGW 용 Qt 가져오기 라이브러리 직접 연결
  set(Q ${ASEC_QT_WIN_DIR})
  enable_language(RC)
  add_executable(SectionViewer WIN32 ${APP_SRC} ${CMAKE_SOURCE_DIR}/packaging/app.rc)
  target_include_directories(SectionViewer PRIVATE ${CMAKE_BINARY_DIR}/generated ${Q}/include ${Q}/include/QtCore ${Q}/include/QtGui ${Q}/include/QtWidgets
                             ${Q}/include/QtOpenGL ${Q}/include/QtOpenGLWidgets ${Q}/mkspecs/win32-g++)
  target_link_libraries(SectionViewer PRIVATE archsection asec_stb
    ${Q}/lib/libQt6OpenGLWidgets.a ${Q}/lib/libQt6OpenGL.a ${Q}/lib/libQt6Widgets.a ${Q}/lib/libQt6Gui.a ${Q}/lib/libQt6Core.a
    opengl32 shell32 ole32 uuid)
  target_compile_definitions(SectionViewer PRIVATE UNICODE _UNICODE _USE_MATH_DEFINES QT_NO_DEBUG QT_CORE_LIB QT_GUI_LIB QT_WIDGETS_LIB
                             QT_OPENGL_LIB QT_OPENGLWIDGETS_LIB)
  target_compile_options(SectionViewer PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8 -O2)
  target_link_options(SectionViewer PRIVATE -mwindows -static)
  set_source_files_properties(${CMAKE_SOURCE_DIR}/packaging/app.rc PROPERTIES COMPILE_FLAGS "-c 65001")
else()
  find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets OpenGL Svg Sql)
  add_executable(SectionViewer ${APP_SRC} sm3convert.cpp)   # .3sm(ScalableMesh, SQLite) 읽기 = Qt Sql(QSQLITE)
  target_link_libraries(SectionViewer PRIVATE archsection asec_stb Qt6::Widgets Qt6::OpenGLWidgets Qt6::OpenGL Qt6::Svg Qt6::Sql)
  target_include_directories(SectionViewer PRIVATE ${CMAKE_BINARY_DIR}/generated)
  target_compile_definitions(SectionViewer PRIVATE _USE_MATH_DEFINES KERF_HAS_3SM)
  set_target_properties(SectionViewer PROPERTIES WIN32_EXECUTABLE ON)
  if(WIN32)   # 아이콘·버전 정보(실행 파일 자체 아이콘 = 탐색기·작업 표시줄)
    target_sources(SectionViewer PRIVATE ${CMAKE_SOURCE_DIR}/packaging/app.rc)
  endif()
  if(MSVC)
    target_compile_options(SectionViewer PRIVATE /utf-8)
    target_link_options(SectionViewer PRIVATE /MANIFEST:NO)   # app.rc 가 app.manifest 를 넣으므로 링커 기본 매니페스트는 끔
  else()
    target_compile_options(SectionViewer PRIVATE -O2)
  endif()
endif()
