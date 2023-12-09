cmake_minimum_required(VERSION 3.15)

project(Rtrc)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

############## optional submodules

option(ENABLE_DIRECTX12_BACKEND "Enable DirectX12 backend"        ON)
option(ENABLE_VULKAN_BACKEND    "Enable Vulkan backend"	          ON)
option(ENABLE_STATIC_RHI        "Enable static backend selection" OFF)

############## build type

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Options: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

############## 3rd-party libs

add_subdirectory(External)

############## core

add_subdirectory(Source/Core)
add_subdirectory(Source/Reflection)
add_subdirectory(Source/RHI)
add_subdirectory(Source/Graphics)
add_subdirectory(Source/ShaderCommon)
add_subdirectory(Source/ShaderPreprocessor)
add_subdirectory(Source/Rtrc)

set_property(TARGET RHI                PROPERTY FOLDER "RtrcLibrary/RHI")
set_property(TARGET Graphics           PROPERTY FOLDER "RtrcLibrary/RHI")
set_property(TARGET ShaderCommon       PROPERTY FOLDER "RtrcLibrary/Shader")
set_property(TARGET Core               PROPERTY FOLDER "RtrcLibrary")
set_property(TARGET Reflection         PROPERTY FOLDER "RtrcTool")
set_property(TARGET ShaderPreprocessor PROPERTY FOLDER "RtrcTool")

############## samples

add_subdirectory(Samples)
