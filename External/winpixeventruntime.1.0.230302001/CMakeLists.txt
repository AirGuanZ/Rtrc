cmake_minimum_required(VERSION 3.10)

project(winpix_102)

add_library(winpix_102_lib SHARED IMPORTED)
set_property(TARGET winpix_102_lib PROPERTY
             IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll")
set_property(TARGET winpix_102_lib PROPERTY
             IMPORTED_IMPLIB "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib")

add_library(winpix_102 INTERFACE)
target_link_libraries(winpix_102 INTERFACE winpix_102_lib)
target_include_directories(winpix_102 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Include")
