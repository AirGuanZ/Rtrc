cmake_minimum_required(VERSION 3.10)

project(half)

add_library(half INTERFACE)
target_include_directories(half INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
