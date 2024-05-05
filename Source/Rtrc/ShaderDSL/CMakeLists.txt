cmake_minimum_required(VERSION 3.10)

project(RtrcShaderDSL)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "*.cpp" "*.h" "*.inl")

add_library(RtrcShaderDSL STATIC ${SRCS})

foreach(SRC IN ITEMS ${SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "Geometry" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET RtrcShaderDSL PROPERTY CXX_STANDARD 23)

target_link_libraries(RtrcShaderDSL PUBLIC RtrcCore)
