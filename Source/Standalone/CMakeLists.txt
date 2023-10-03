cmake_minimum_required(VERSION 3.10)

project(Standalone)

set(SRCS
	"${CMAKE_CURRENT_SOURCE_DIR}/Common.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/StandaloneApplication.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/StandaloneApplication.cpp")

add_executable(Standalone ${SRCS})

foreach(SRC IN ITEMS ${SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "Standalone" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET Standalone PROPERTY CXX_STANDARD 23)
set_property(TARGET Standalone PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(Standalone PUBLIC Rtrc)

enable_rtrc_cpp_reflection(Standalone)
enable_preprocessed_shaders(Standalone "")
