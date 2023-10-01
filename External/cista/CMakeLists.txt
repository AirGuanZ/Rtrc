cmake_minimum_required(VERSION 3.12)

project(cista)

add_library(cista INTERFACE)

target_include_directories(cista INTERFACE "${PROJECT_SOURCE_DIR}")
