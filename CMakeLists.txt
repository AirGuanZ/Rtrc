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
add_subdirectory(Source/ShaderCompiler)
add_subdirectory(Source/ShaderPreprocessor)
add_subdirectory(Source/Rtrc)
add_subdirectory(Source/TempTest)

set_property(TARGET Core               PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET RHI                PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET Graphics           PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET ShaderCompiler     PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET Rtrc               PROPERTY FOLDER "Rtrc/Library")
set_property(TARGET Reflection         PROPERTY FOLDER "Rtrc/Tool")
set_property(TARGET ShaderPreprocessor PROPERTY FOLDER "Rtrc/Tool")

set_property(TARGET TempTest PROPERTY FOLDER "Rtrc/Temp")

############## standalone

add_subdirectory(Source/Standalone)

############## samples

add_subdirectory(Samples)
