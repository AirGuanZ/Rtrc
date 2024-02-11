cmake_minimum_required(VERSION 3.15)

project(Rtrc)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(RTRC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "RTRC_SOURCE_DIR" FORCE)

############## optional modules

option(RTRC_ENABLE_DIRECTX12_BACKEND "Enable DirectX12 backend"        ON)
option(RTRC_ENABLE_VULKAN_BACKEND    "Enable Vulkan backend"	       ON)
option(RTRC_ENABLE_STATIC_RHI        "Enable static backend selection" OFF)

option(RTRC_BUILD_SAMPLES "Build Rtrc samples" ON)

############## build type

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Options: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

############## 3rd-party libs

add_subdirectory(External)

############## core

add_subdirectory(Source/Rtrc/Core)
add_subdirectory(Source/Rtrc/RHI)
add_subdirectory(Source/Rtrc/Graphics)
add_subdirectory(Source/Rtrc/ShaderCommon)
add_subdirectory(Source/Rtrc/ShaderPreprocessor)

function(rtrc_register_target TARGET_NAME)
    rtrc_register_shaders(${TARGET_NAME})
    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    if (TARGET_TYPE STREQUAL "EXECUTABLE")
        rtrc_deploy_d3d12_sdk(${TARGET_NAME})
	    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
	        COMMAND
                ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${TARGET_NAME}>
                $<TARGET_FILE_DIR:${TARGET_NAME}>
	        COMMAND_EXPAND_LISTS)
    endif()
endfunction()

add_subdirectory(Source/Rtrc/ToolKit)

set_property(TARGET Core               PROPERTY FOLDER "RtrcLibrary")
set_property(TARGET RHI                PROPERTY FOLDER "RtrcLibrary")
set_property(TARGET Graphics           PROPERTY FOLDER "RtrcLibrary")
set_property(TARGET ShaderCommon       PROPERTY FOLDER "RtrcLibrary")
set_property(TARGET ShaderPreprocessor PROPERTY FOLDER "RtrcLibrary")
set_property(TARGET ToolKit            PROPERTY FOLDER "RtrcLibrary")

############## samples

if(RTRC_BUILD_SAMPLES)
    add_subdirectory(Samples)
endif()
