cmake_minimum_required(VERSION 3.12)

find_package(TBB REQUIRED tbb PATHS "${CMAKE_CURRENT_SOURCE_DIR}/../oneapi-tbb-2021.11.0/lib/cmake/tbb")
add_library(tbbinterface INTERFACE)
target_link_libraries(tbbinterface INTERFACE TBB::tbb)
