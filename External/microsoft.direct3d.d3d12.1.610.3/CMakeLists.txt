cmake_minimum_required(VERSION 3.10)

project(d3d12_1610)

add_custom_command(
	OUTPUT
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12/D3D12Core.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12/D3D12Core.pdb"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12/d3d12SDKLayers.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12/d3d12SDKLayers.pdb"
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/build/native/bin/x64/D3D12Core.dll"
		"${CMAKE_CURRENT_SOURCE_DIR}/build/native/bin/x64/D3D12Core.pdb"
		"${CMAKE_CURRENT_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.dll"
		"${CMAKE_CURRENT_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.pdb"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12"
	DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/build/native/bin/x64/D3D12Core.dll"
		"${CMAKE_CURRENT_SOURCE_DIR}/build/native/bin/x64/D3D12Core.pdb"
		"${CMAKE_CURRENT_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.dll"
		"${CMAKE_CURRENT_SOURCE_DIR}/build/native/bin/x64/d3d12SDKLayers.pdb")

add_custom_target(
	CopyD3D12Dlls
	DEPENDS
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12/D3D12Core.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12/D3D12Core.pdb"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12/d3d12SDKLayers.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/D3D12/d3d12SDKLayers.pdb")

add_library(d3d12_1610 INTERFACE)
target_include_directories(d3d12_1610 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/build/native/include")
add_dependencies(d3d12_1610 CopyD3D12Dlls)
