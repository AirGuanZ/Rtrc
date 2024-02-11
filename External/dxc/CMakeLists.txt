cmake_minimum_required(VERSION 3.19)

project(DXC)

add_library(dxc_lib SHARED IMPORTED)
set_property(
	TARGET dxc_lib PROPERTY IMPORTED_LOCATION
	"${CMAKE_CURRENT_SOURCE_DIR}/bin/dxcompiler.dll"
	"${CMAKE_CURRENT_SOURCE_DIR}/bin/dxil.dll")
set_property(
	TARGET dxc_lib PROPERTY IMPORTED_IMPLIB
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/dxcompiler.lib")

add_library(dxc INTERFACE)
target_include_directories(dxc INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_link_libraries(dxc INTERFACE dxc_lib)
