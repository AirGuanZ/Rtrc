cmake_minimum_required(VERSION 3.12)

project(ShaderCompiler)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "*.cpp" "*.h")

add_library(ShaderCompiler STATIC ${SRCS})

foreach(SRC IN ITEMS ${SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "ShaderCompiler" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET ShaderCompiler PROPERTY CXX_STANDARD 23)

target_link_libraries(ShaderCompiler PUBLIC Graphics dxc)
