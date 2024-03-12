cmake_minimum_required(VERSION 3.10)

project(RtrcShaderPreprocessor)

# ======================= ShaderPreprocesor =======================

set(SRCS "${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp")
    
add_executable(RtrcShaderPreprocessor ${SRCS})

foreach(SRC IN ITEMS ${SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "ShaderPreprocessor" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET RtrcShaderPreprocessor PROPERTY CXX_STANDARD 23)
set_property(TARGET RtrcShaderPreprocessor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${RTRC_SOURCE_DIR}")

target_link_libraries(RtrcShaderPreprocessor PUBLIC RtrcShaderCommon cxxopts)

add_custom_command(TARGET RtrcShaderPreprocessor POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:RtrcShaderPreprocessor>
        $<TARGET_FILE_DIR:RtrcShaderPreprocessor>
    COMMAND_EXPAND_LISTS)

# ======================= Shader registration function =======================
# Register all "*.shader" source from given target for shader preprocessing
# Usage:
#    rtrc_register_shaders(TARGET_NAME)

include(TargetDependency)

define_property(TARGET PROPERTY SHADER_INCLUDE_DIRECTORY)

add_custom_target(RtrcPreprocessShaders)
set_property(TARGET RtrcPreprocessShaders PROPERTY FOLDER "Rtrc/Generated")

function(rtrc_preprocess_shaders FILE_LIST_NAME CUSTOM_TARGET_NAME INCLUDE_DIR_LIST_NAME)
    set(FILE_LIST ${${FILE_LIST_NAME}})
    set(INC_DIR_LIST ${${INCLUDE_DIR_LIST_NAME}})
    set(SHADER_TIMESTAMP_FILE_LIST)
    string(JOIN "," INCLUDE_DIR_ARG ${INC_DIR_LIST})
    foreach(SHADER_FILE IN LISTS FILE_LIST)
        set(IN ${SHADER_FILE})
        set(OUT "${SHADER_FILE}.outh")
        set(TS "${SHADER_FILE}.outh.timestamp")
        set(DF "${SHADER_FILE}.outh.depfile")
        add_custom_command(
            OUTPUT ${TS}
            BYPRODUCTS ${OUT}
            COMMAND
                RtrcShaderPreprocessor -i "${IN}"
                                       -o "${OUT}"
                                       -r "${INCLUDE_DIR_ARG}"
                                       -d "${DF}"
            WORKING_DIRECTORY ${RTRC_SOURCE_DIR}
            COMMAND
                ${CMAKE_COMMAND} -E touch "${TS}"
            DEPFILE ${DF}
            DEPENDS RtrcShaderPreprocessor)
        list(APPEND SHADER_TIMESTAMP_FILE_LIST ${TS})
    endforeach()
    add_custom_target(${CUSTOM_TARGET_NAME} ALL DEPENDS ${SHADER_TIMESTAMP_FILE_LIST})
    add_dependencies(RtrcPreprocessShaders ${CUSTOM_TARGET_NAME})
    set_property(TARGET ${CUSTOM_TARGET_NAME} PROPERTY FOLDER "Rtrc/Generated")
    message("Register rtrc custom target '${CUSTOM_TARGET_NAME}' for shader preprocessing")
endfunction()

function(rtrc_register_shaders TARGET_NAME)

    get_all_dependencies(${TARGET_NAME} TARGET_LIST)
    list(APPEND TARGET_LIST ${TARGET_NAME})
    list(REMOVE_DUPLICATES TARGET_LIST)

    # Preprocess shader files in target source

    set_property(TARGET ${TARGET_NAME} PROPERTY SHADER_INCLUDE_DIRECTORY "${ARGN}")
    set(INCLUDE_DIRS)
    foreach(DEP_TARGET IN LISTS TARGET_LIST)
        get_target_property(DIR ${DEP_TARGET} SHADER_INCLUDE_DIRECTORY)
        if(NOT ${DIR} STREQUAL "DIR-NOTFOUND")
            list(APPEND INCLUDE_DIRS ${DIR})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES INCLUDE_DIRS)
    
    get_target_property(TARGET_SHADERS ${TARGET_NAME} SOURCES)
    list(FILTER TARGET_SHADERS INCLUDE REGEX ".*\\.shader$")
    rtrc_preprocess_shaders(TARGET_SHADERS "RtrcPreprocessShadersFor${TARGET_NAME}" INCLUDE_DIRS)
    add_dependencies(${TARGET_NAME} "RtrcPreprocessShadersFor${TARGET_NAME}")
    
    # Generate 'PreprocessedShaderFiles.inl' if the target is an executable. This file will
    # contain all shaders from the entire target DAG.

    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if (TARGET_TYPE STREQUAL "EXECUTABLE")
        set(ALL_SHADERS)
        foreach(DEP_TARGET IN LISTS TARGET_LIST)
            get_target_property(DEP_SHADERS ${DEP_TARGET} SOURCES)
            list(FILTER DEP_SHADERS INCLUDE REGEX ".*\\.shader")
            list(APPEND ALL_SHADERS ${DEP_SHADERS})
        endforeach()
        list(REMOVE_DUPLICATES ALL_SHADERS)
        set(INCLUDE_CONTENT)
        foreach(F IN LISTS ALL_SHADERS)
            set(INCLUDE_CONTENT "${INCLUDE_CONTENT}#include \"${F}.outh\"\n")
        endforeach()
        file(
            WRITE
            "${CMAKE_BINARY_DIR}/RtrcGenerated/${TARGET_NAME}/Generated/PreprocessedShaderFiles.inl"
            "${INCLUDE_CONTENT}")
        target_include_directories(${TARGET_NAME} PUBLIC "${CMAKE_BINARY_DIR}/RtrcGenerated/${TARGET_NAME}")
        target_sources(${TARGET_NAME} PUBLIC "${RTRC_SOURCE_DIR}/Source/Rtrc/ShaderPreprocessor/RegisterShaders.cpp")
        source_group("Generated" FILES "${RTRC_SOURCE_DIR}/Source/Rtrc/ShaderPreprocessor/RegisterShaders.cpp")
    endif()

endfunction()
