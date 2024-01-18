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

target_link_libraries(ShaderPreprocessor PUBLIC ShaderCommon cxxopts)

file(
    GLOB_RECURSE ALL_SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Source/*.shader"
    "${CMAKE_SOURCE_DIR}/Samples/*.shader")

file(GLOB_RECURSE ALL_HLSL_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/Source/*.hlsl"
    "${CMAKE_SOURCE_DIR}/Samples/*.hlsl")

set(ALL_SHADER_OUT_FILES)
set(ALL_SHADER_TIMESTAMP_FILES)
foreach(SHADER_FILE IN ITEMS ${ALL_SHADER_FILES})
    set(IN ${SHADER_FILE})
    set(OUT "${SHADER_FILE}.outh")
    set(TS "${SHADER_FILE}.outh.timestamp")
    add_custom_command(
        OUTPUT ${TS}
        BYPRODUCTS ${OUT}
        COMMAND
            ShaderPreprocessor -i "${IN}"
                               -o "${OUT}"
                               -r "${CMAKE_SOURCE_DIR}/Source"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND
            ${CMAKE_COMMAND} -E touch "${TS}"
        DEPENDS
            ShaderPreprocessor "${IN}" "${ALL_HLSL_FILES}") # TODO: use depfile to optimize this
    list(APPEND ALL_SHADER_OUT_FILES ${OUT})
    list(APPEND ALL_SHADER_TIMESTAMP_FILES ${TS})
endforeach()

add_custom_target(RtrcPreprocessedShaderCustomTarget ALL DEPENDS ${ALL_SHADER_TIMESTAMP_FILES})
set_property(TARGET RtrcPreprocessedShaderCustomTarget PROPERTY FOLDER "Generated")

add_library(RtrcPreprocessedShader INTERFACE)
set_property(TARGET RtrcPreprocessedShader PROPERTY FOLDER "Generated")

add_dependencies(RtrcPreprocessedShader RtrcPreprocessedShaderCustomTarget)
  
function(enable_preprocessed_shaders TARGET_NAME ADDITIONAL_SHADERS)
    file(
        GLOB_RECURSE FIXED_SHADER_FILES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/Source/*.shader")
    set(SHADERS "${ADDITIONAL_SHADERS}" "${FIXED_SHADER_FILES}")
    set(INCLUDE_CONTENT)
    get_target_property(TARGET_DIR ${TARGET_NAME} SOURCE_DIR)
    foreach(F IN ITEMS ${SHADERS})
        set(INCLUDE_CONTENT "${INCLUDE_CONTENT}#include \"${F}.outh\"\n")
    endforeach()
    file(
        WRITE
        "${CMAKE_SOURCE_DIR}/Asset/RtrcGenerated/${TARGET_NAME}/Generated/PreprocessedShaderFiles.inl"
        "${INCLUDE_CONTENT}")
    target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/Asset/RtrcGenerated/${TARGET_NAME}")
    target_link_libraries(${TARGET_NAME} PUBLIC RtrcPreprocessedShader)
    target_sources(${TARGET_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/Source/Rtrc/ShaderPreprocessor/RegisterShaders.cpp")
    source_group("Reflection" FILES "${CMAKE_SOURCE_DIR}/Source/Rtrc/ShaderPreprocessor/RegisterShaders.cpp")
endfunction()
