cmake_minimum_required(VERSION 3.12)

project(Core)

file(GLOB_RECURSE CORE_SRCS CONFIGURE_DEPENDS "*.cpp" "*.h")

add_library(RtrcCore STATIC ${CORE_SRCS})

foreach(SRC IN ITEMS ${CORE_SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "Core" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET RtrcCore PROPERTY CXX_STANDARD 23)

if(MSVC)
	target_compile_options(RtrcCore PUBLIC /Zc:preprocessor /MP)
	target_compile_definitions(RtrcCore PUBLIC _CRT_SECURE_NO_WARNINGS _SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)
endif()

target_include_directories(RtrcCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_link_libraries(
	RtrcCore PUBLIC
	mimalloc-static spdlog fmt tbbinterface stb tinyexr nlohmann-json
	half tinyobjloader sigslot avir cy smhasher cista magic_enum)
