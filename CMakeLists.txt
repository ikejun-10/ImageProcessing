cmake_minimum_required(VERSION 3.20)

project(ImageProcessing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ImageProcessing WIN32
  src/app/main.cpp
  src/app/window_proc.cpp
  src/modules/input_handlers.cpp
  src/modules/image_io.cpp
  src/modules/edit_apply.cpp
  src/modules/state.cpp
  src/modules/ui.cpp
  src/modules/view.cpp
  src/modules/render.cpp
  src/modules/corrector.cpp
  src/modules/geometry.cpp
)

target_include_directories(ImageProcessing PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/app
  ${CMAKE_CURRENT_SOURCE_DIR}/src/modules
)

target_link_libraries(ImageProcessing PRIVATE
  gdiplus
  comctl32
)

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

