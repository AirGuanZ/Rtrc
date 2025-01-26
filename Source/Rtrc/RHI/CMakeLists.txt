cmake_minimum_required(VERSION 3.10)

project(RtrcRHI)

file(GLOB_RECURSE SRCS "*.cpp" "*.h")
add_library(RtrcRHI STATIC ${SRCS})

foreach(SRC IN ITEMS ${SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "RHI" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET RtrcRHI PROPERTY CXX_STANDARD 23)

target_link_libraries(RtrcRHI PUBLIC RtrcCore)
target_link_libraries(RtrcRHI PRIVATE glfw)

target_link_libraries(RtrcRHI PUBLIC d3d12_1715 winpix_102 D3D12MemoryAllocator)
