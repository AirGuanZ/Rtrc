cmake_minimum_required(VERSION 3.12)

project(avir)

add_library(avir INTERFACE)

target_include_directories(avir INTERFACE "${PROJECT_SOURCE_DIR}")
