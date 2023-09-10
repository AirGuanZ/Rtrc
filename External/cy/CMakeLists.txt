cmake_minimum_required(VERSION 3.12)

project(cy)

add_library(cy INTERFACE)

target_include_directories(avir INTERFACE "${PROJECT_SOURCE_DIR}")
