cmake_minimum_required(VERSION 3.10)

project(LLVM)

add_library(LLVM INTERFACE)

file(GLOB LLVM_DEBUG_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/install_debug/lib/*.lib")
file(GLOB LLVM_RELEASE_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/install_release/lib/*.lib")

target_include_directories(
	LLVM INTERFACE
	$<$<CONFIG:Debug>:${CMAKE_CURRENT_SOURCE_DIR}/install_release/include>
	$<$<CONFIG:Release>:${CMAKE_CURRENT_SOURCE_DIR}/install_release/include>)

target_link_libraries(
	LLVM INTERFACE
	$<$<CONFIG:Debug>:${LLVM_RELEASE_LIBS}>
	$<$<CONFIG:Release>:${LLVM_RELEASE_LIBS}>)
