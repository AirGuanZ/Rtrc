cmake_minimum_required(VERSION 3.12)

project(nlohmann-json)

add_library(nlohmann-json INTERFACE)

target_include_directories(nlohmann-json INTERFACE "${PROJECT_SOURCE_DIR}")
