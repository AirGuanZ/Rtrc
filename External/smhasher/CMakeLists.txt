cmake_minimum_required(VERSION 3.10)

project(smhasher)

add_library(smhasher STATIC "MurmurHash3.h" "MurmurHash3.cpp")

target_include_directories(smhasher PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
