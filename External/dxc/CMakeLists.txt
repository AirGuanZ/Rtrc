cmake_minimum_required(VERSION 3.19)

project(DXC)

add_custom_command(
	OUTPUT
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/dxcompiler.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/dxil.dll"
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/bin/dxcompiler.dll"
		"${CMAKE_CURRENT_SOURCE_DIR}/bin/dxil.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>"
	DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/bin/dxcompiler.dll"
		"${CMAKE_CURRENT_SOURCE_DIR}/bin/dxil.dll")

add_custom_target(
	CopyDXCDlls
	DEPENDS
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/dxcompiler.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/dxil.dll")

add_library(dxc INTERFACE)
target_include_directories(dxc INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/inc")
target_link_libraries(dxc INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/lib/dxcompiler.lib")
add_dependencies(dxc CopyDXCDlls)
