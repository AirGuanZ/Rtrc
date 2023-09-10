cmake_minimum_required(VERSION 3.10)

project(winpix_102)

add_custom_command(
	OUTPUT
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/WinPixEventRuntime.dll"
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/WinPixEventRuntime.dll"
	DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll")

add_custom_target(
	CopyPIXDlls
	DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/WinPixEventRuntime.dll")

add_library(winpix_102 INTERFACE)
target_include_directories(winpix_102 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Include")
target_link_libraries(winpix_102 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib")
add_dependencies(winpix_102 CopyPIXDlls)
