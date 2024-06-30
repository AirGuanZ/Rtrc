cmake_minimum_required(VERSION 3.20)

project(RtrcTest)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "*.cpp" "*.h" "*.inl")
file(GLOB_RECURSE SHADER_SRCS CONFIGURE_DEPENDS "*.shader" "*.hlsl")

set(ALL_SRCS ${SRCS} ${SHADER_SRCS})
add_executable(RtrcTest ${ALL_SRCS})

foreach(SRC IN ITEMS ${ALL_SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "Test" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_source_files_properties(${SHADER_SRCS} PROPERTIES HEADER_FILE_ONLY TRUE)

set_property(TARGET RtrcTest PROPERTY CXX_STANDARD 23)

target_link_libraries(RtrcTest PUBLIC RtrcToolKit RtrcGraphics RtrcGeometry Eigen3::Eigen Catch2)

rtrc_register_target(RtrcTest "${RTRC_SOURCE_DIR}/Source")
