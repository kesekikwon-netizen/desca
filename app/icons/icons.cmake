# 아이콘 SVG 목록(손으로 적는다 — glob 아님). configure 때 kerf_icons.hpp 를 만든다: rcc · moc 없이 SVG 글자를 C++ 문자열로.
# lucide/ 는 Lucide(ISC, LICENSE-lucide.txt)에서 그대로, kerf/ 는 Kerf 전용(make_kerf_svgs.py 가 만듦). 둘 다 24 격자 · stroke 2.
set(KERF_ICON_DIR ${CMAKE_CURRENT_LIST_DIR})
set(KERF_ICON_FILES
  lucide/folder.svg lucide/x.svg lucide/plus.svg lucide/check.svg lucide/clipboard-list.svg lucide/file-text.svg lucide/image.svg
  lucide/ellipsis.svg lucide/house.svg lucide/undo-2.svg lucide/redo-2.svg lucide/maximize.svg lucide/search.svg lucide/move.svg
  lucide/printer.svg lucide/download.svg lucide/pencil.svg lucide/triangle-alert.svg lucide/layers.svg lucide/map.svg lucide/save.svg
  lucide/box.svg lucide/pen-line.svg lucide/square-dashed.svg
  kerf/clock.svg kerf/flip.svg kerf/list.svg kerf/contrast.svg kerf/keyboard.svg kerf/info.svg kerf/zoom-in.svg kerf/zoom-out.svg
  kerf/chevron-down.svg kerf/star.svg kerf/section-line.svg kerf/hatch.svg kerf/outline.svg kerf/profile.svg kerf/points-xyz.svg
  kerf/cloud-las.svg kerf/geotiff.svg kerf/csv.svg kerf/sections-list.svg kerf/vex.svg kerf/top-view.svg kerf/height.svg
  kerf/crosshair.svg kerf/levels.svg kerf/fade.svg kerf/image-layer.svg kerf/north.svg kerf/legend.svg kerf/title-block.svg
  kerf/scalebar.svg kerf/paper.svg kerf/edit-move.svg kerf/copy.svg kerf/enter.svg kerf/esc.svg kerf/gap.svg kerf/menu.svg kerf/sheet.svg)

# out: 만들 헤더 경로. SVG 가 바뀌면 configure 가 다시 돈다(CMAKE_CONFIGURE_DEPENDS).
function(kerf_write_icons_header out)
  set(body "// 자동 생성(app/icons/icons.cmake). 손으로 고치지 않는다 — SVG 파일과 목록을 고친다.\n#pragma once\n\nnamespace kerficons {\nstruct Svg { const char* name; const char* svg; };\ninline const Svg kAll[] = {\n")
  foreach(f IN LISTS KERF_ICON_FILES)
    get_filename_component(name "${f}" NAME_WE)
    file(READ "${KERF_ICON_DIR}/${f}" svg)
    string(APPEND body "  {\"${name}\", R\"SVG(${svg})SVG\"},\n")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${KERF_ICON_DIR}/${f}")
  endforeach()
  list(LENGTH KERF_ICON_FILES n)
  string(APPEND body "};\ninline constexpr int kCount = ${n};\n}  // namespace kerficons\n")
  file(WRITE "${out}" "${body}")
endfunction()
