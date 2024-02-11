cmake_minimum_required(VERSION 3.10)

project(RtrcShaderPreprocessor)

# ======================= ShaderPreprocesor =======================

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

add_custom_command(TARGET ShaderPreprocessor POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:ShaderPreprocessor>
    $<TARGET_FILE_DIR:ShaderPreprocessor>
  COMMAND_EXPAND_LISTS)

# ======================= Shader Registration Macro =======================
# Usage:
#    rtrc_register_shaders(TARGET_NAME)

# Conditionally set a variable to its name or undefined, for forwarding options.
macro(set_if target condition)
    if(${condition})
        set(${target} "${target}")
    else()
        set(${target})
    endif()
endmacro()

# Recursively traverse LINK_LIBRARIES to find all link dependencies.
# Options:
#   NO_STATIC - prune static libraries
#   NO_SYSTEM - skip SYSTEM targets (in >=3.25)
#       ^ REQUIRED if you want to install() the result, due to non-existent IMPORTED targets
# Caveats:
#   Non-targets in LINK_LIBRARIES like "m" (as in "libm") are ignored.
#   ALIAS target names are resolved.
function(get_all_dependencies target output_list)
    # Check if the NO_STATIC or NO_SYSTEM flag is provided
    set(options NO_STATIC NO_SYSTEM)
    set(oneValueArgs)
    set(multiValueArgs)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")
    set_if(NO_STATIC ARG_NO_STATIC)
    set_if(NO_SYSTEM ARG_NO_SYSTEM)

    # Get dependencies of the target
    get_target_property(current_deps ${target} LINK_LIBRARIES)

    if(NOT current_deps)
        set(current_deps "") # if no dependencies, replace "current_deps-NOTFOUND" with empty list
    endif()

    # Remove entries between ::@(directory-id) and ::@
    # Such entries are added by target_link_libraries() calls outside the target's directory
    set(filtered_deps "")
    set(in_special_block FALSE)
    foreach(dep IN LISTS current_deps)
        if("${dep}" MATCHES "^::@\\(.*\\)$")
            set(in_special_block TRUE)  # Latch on
        elseif("${dep}" STREQUAL "::@")
            set(in_special_block FALSE)  # Latch off
        elseif(NOT in_special_block)
            if(TARGET ${dep})  # Exclude non-targets like m (= libm)
                # Exclude SYSTEM targets (prevents  "install TARGETS given target "Perl" which does not exist")
                get_target_property(_is_system ${dep} SYSTEM)
                if(NOT _is_system OR NOT NO_SYSTEM)
                    # Resolve ALIAS targets (CMake issue #20979)
                    get_target_property(_aliased_dep ${dep} ALIASED_TARGET)
                    if(_aliased_dep)
                        list(APPEND filtered_deps ${_aliased_dep})
                    else()
                        list(APPEND filtered_deps ${dep})
                    endif()
                else()
                    message(VERBOSE "get_all_dependencies ignoring ${target} -> ${dep} (system)")
                endif()
            else()
                message(VERBOSE "get_all_dependencies ignoring ${target} -> ${dep} (not a target)")
            endif()
        else()
            message(VERBOSE "get_all_dependencies ignoring ${target} -> ${dep} (added externally)")
        endif()
    endforeach()

    set(all_deps ${filtered_deps})

    foreach(dep IN LISTS filtered_deps)
        # Avoid infinite recursion if the target has a cyclic dependency
        if(NOT "${dep}" IN_LIST ${output_list})
            get_all_dependencies(${dep} dep_child_deps ${NO_STATIC} ${NO_SYSTEM})
            list(APPEND all_deps ${dep_child_deps})
        endif()
    endforeach()

    # Remove duplicates
    list(REMOVE_DUPLICATES all_deps)

    # Remove static libraries if the NO_STATIC flag is set
    if(ARG_NO_STATIC)
        foreach(dep IN LISTS all_deps)
            get_target_property(dep_type ${dep} TYPE)
            if(dep_type STREQUAL "STATIC_LIBRARY")
                message(VERBOSE "get_all_dependencies pruning ${target} -> ${dep} (static)")
                list(REMOVE_ITEM all_deps ${dep})
            endif()
        endforeach()
    endif()

    # Set the result in parent scope
    set(${output_list} "${all_deps}" PARENT_SCOPE)
endfunction()

function(rtrc_generate_custom_target_for_preprocessing_shaders FILE_LIST_NAME CUSTOM_TARGET_NAME)
    #set(SHADER_OUT_FILE_LIST)
    set(FILE_LIST ${${FILE_LIST_NAME}})
    set(SHADER_TIMESTAMP_FILE_LIST)
    foreach(SHADER_FILE IN LISTS FILE_LIST)
        set(IN ${SHADER_FILE})
        set(OUT "${SHADER_FILE}.outh")
        set(TS "${SHADER_FILE}.outh.timestamp")
        set(DF "${SHADER_FILE}.outh.depfile")
        add_custom_command(
            OUTPUT ${TS}
            BYPRODUCTS ${OUT}
            COMMAND
                ShaderPreprocessor -i "${IN}"
                                   -o "${OUT}"
                                   -r "${CMAKE_SOURCE_DIR}/Source"
                                   -d "${DF}"
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND
                ${CMAKE_COMMAND} -E touch "${TS}"
            DEPFILE ${DF}
            DEPENDS ShaderPreprocessor)
        list(APPEND SHADER_TIMESTAMP_FILE_LIST ${TS})
    endforeach()
    add_custom_target(${CUSTOM_TARGET_NAME} ALL DEPENDS ${SHADER_TIMESTAMP_FILE_LIST})
    set_property(TARGET ${CUSTOM_TARGET_NAME} PROPERTY FOLDER "Generated")
    message("Register rtrc custom target '${CUSTOM_TARGET_NAME}' for shader preprocessing")
endfunction()

function(rtrc_register_shaders TARGET_NAME)
    get_target_property(TARGET_SHADERS ${TARGET_NAME} SOURCES)
    list(FILTER TARGET_SHADERS INCLUDE REGEX ".*\\.shader$")
    rtrc_generate_custom_target_for_preprocessing_shaders(TARGET_SHADERS "RtrcPreprocessShadersFor${TARGET_NAME}")
    add_dependencies(${TARGET_NAME} "RtrcPreprocessShadersFor${TARGET_NAME}")

    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if (TARGET_TYPE STREQUAL "EXECUTABLE")
        set(ALL_SHADERS)
        get_all_dependencies(${TARGET_NAME} TARGET_LIST)
        list(APPEND TARGET_LIST ${TARGET_NAME})
        list(REMOVE_DUPLICATES TARGET_LIST)
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
        target_sources(${TARGET_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/Source/Rtrc/ShaderPreprocessor/RegisterShaders.cpp")
        source_group("Generated" FILES "${CMAKE_SOURCE_DIR}/Source/Rtrc/ShaderPreprocessor/RegisterShaders.cpp")
    endif()
endfunction()
