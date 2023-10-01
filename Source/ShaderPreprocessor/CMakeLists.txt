cmake_minimum_required(VERSION 3.10)

project(RtrcShaderPreprocessor)

set(SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp")
    
add_executable(ShaderPreprocessor ${SRCS})

foreach(SRC IN ITEMS ${SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "ShaderPreprocessor" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET ShaderPreprocessor PROPERTY CXX_STANDARD 23)
set_property(TARGET ShaderPreprocessor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

target_link_libraries(ShaderPreprocessor PUBLIC ShaderCompiler cxxopts)

file(
    GLOB_RECURSE ALL_SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Source/*.shader"
    "${CMAKE_SOURCE_DIR}/Source/*.material")

set(ALL_SHADER_OUT_FILES)
set(INCLUDE_ALL_GENERATED_FILES)
foreach(SHADER_FILE IN ITEMS ${ALL_SHADER_FILES})
    set(IN ${SHADER_FILE})
    set(OUT "${SHADER_FILE}.h")
    add_custom_command(
        OUTPUT ${OUT}
        COMMAND
            ShaderPreprocessor -i "${IN}"
                               -o "${OUT}"
                               -r "${CMAKE_SOURCE_DIR}"
                               -r "${CMAKE_CURRENT_SOURCE_DIR}/DummyShader"
        DEPENDS
            ShaderPreprocessor "${IN}")
    list(APPEND ALL_SHADER_OUT_FILES ${OUT})
    set(INCLUDE_ALL_GENERATED_FILES "${INCLUDE_ALL_GENERATED_FILES}#include \"${OUT}\"\n")
endforeach()

file(
    WRITE
    "${CMAKE_BINARY_DIR}/RtrcGenerated/ShaderDatabase/RtrcPreprocessedShaderFiles.inl"
    "${INCLUDE_ALL_GENERATED_FILES}")

add_custom_target(RtrcPreprocessedShaderCustomTarget ALL DEPENDS ${ALL_SHADER_OUT_FILES})

add_library(RtrcPreprocessedShader INTERFACE)
set_property(TARGET RtrcPreprocessedShader PROPERTY FOLDER "Generated")
target_include_directories(RtrcPreprocessedShader INTERFACE "${CMAKE_BINARY_DIR}/RtrcGenerated/ShaderDatabase")
add_dependencies(RtrcPreprocessedShader RtrcPreprocessedShaderCustomTarget)
