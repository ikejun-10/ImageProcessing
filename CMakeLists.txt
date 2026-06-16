cmake_minimum_required(VERSION 3.20)

project(ImageProcessing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC ランタイムを静的リンク (/MT) して，VC++ 再頒布パッケージ不要の単一 EXE にする
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable(ImageProcessing WIN32
  # app entry point
  src/app/main.cpp
  src/app/window_proc.cpp
  # core: shared state, geometry primitives
  src/modules/core/state.cpp
  src/modules/core/geometry.cpp
  # io: image import/export
  src/modules/io/image_io.cpp
  # view: display geometry, hit testing, rendering
  src/modules/view/view.cpp
  src/modules/view/render.cpp
  # controls: UI layout, edit-box handling, mouse input
  src/modules/controls/ui.cpp
  src/modules/controls/edit_apply.cpp
  src/modules/controls/input_handlers.cpp
  # correction: rebuilds the corrected preview
  src/modules/correction/corrector.cpp
)

target_include_directories(ImageProcessing PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/app
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/core
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/io
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/view
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/controls
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modules/correction
)

target_link_libraries(ImageProcessing PRIVATE
  gdiplus
  comctl32
)

# MSVC: UTF-8 ソースを正しく解釈させる (`°` 等のラベル文字化け対策)
if(MSVC)
  target_compile_options(ImageProcessing PRIVATE /utf-8)
endif()

# Put the .exe into ./bin (keep build artifacts in ./build).
set_target_properties(ImageProcessing PROPERTIES
  OUTPUT_NAME "ImageProcessing"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)
foreach(cfg IN ITEMS DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
  set_target_properties(ImageProcessing PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_${cfg} "${CMAKE_CURRENT_SOURCE_DIR}/bin"
  )
endforeach()

