cmake_minimum_required(VERSION 3.15)

project(Rtrc)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(RTRC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "RTRC_SOURCE_DIR" FORCE)

list(APPEND CMAKE_MODULE_PATH "${RTRC_SOURCE_DIR}/CMake")

############## optional modules

option(RTRC_ENABLE_DIRECTX12_BACKEND "Enable DirectX12 backend"        ON)
option(RTRC_ENABLE_VULKAN_BACKEND    "Enable Vulkan backend"	       ON)
option(RTRC_ENABLE_STATIC_RHI        "Enable static backend selection" OFF)
option(RTRC_ENABLE_MIMALLOC          "Enable mimalloc"                 ON)

option(RTRC_BUILD_SAMPLES "Build Rtrc samples" ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(RTRC_ENABLE_TRACY_DEFAULT_VALUE ON)
else()
    set(RTRC_ENABLE_TRACY_DEFAULT_VALUE OFF)
endif()

option(RTRC_ENABLE_TRACY       "Enable tracy for profiling" ${RTRC_ENABLE_TRACY_DEFAULT_VALUE})
option(RTRC_USE_EXTERNAL_TRACY "Use external tracy target"  OFF)

############## build type

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Options: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

############## 3rd-party libs

add_subdirectory(External)

############## core

add_subdirectory(Source/Rtrc/Core)
add_subdirectory(Source/Rtrc/RHI)
add_subdirectory(Source/Rtrc/Geometry)
add_subdirectory(Source/Rtrc/Graphics)
add_subdirectory(Source/Rtrc/ShaderCommon)
add_subdirectory(Source/Rtrc/ShaderPreprocessor)

function(rtrc_register_target TARGET_NAME)
    rtrc_register_shaders(${TARGET_NAME} ${ARGN})
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if (TARGET_TYPE STREQUAL "EXECUTABLE")
        if(RTRC_ENABLE_DIRECTX12_BACKEND)
            rtrc_deploy_d3d12_sdk(${TARGET_NAME})
	    endif()
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
	        COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${TARGET_NAME}>
                $<TARGET_FILE_DIR:${TARGET_NAME}>
	        COMMAND_EXPAND_LISTS)
    endif()
endfunction()

add_subdirectory(Source/Rtrc/ToolKit)
add_subdirectory(Test)

set_property(TARGET RtrcCore               PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET RtrcRHI                PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET RtrcGeometry           PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET RtrcGraphics           PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET RtrcShaderCommon       PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET RtrcShaderPreprocessor PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET RtrcToolKit            PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET RtrcTest               PROPERTY FOLDER "Rtrc/Library")

############## samples

if(RTRC_BUILD_SAMPLES)
    add_subdirectory(Samples)
endif()
