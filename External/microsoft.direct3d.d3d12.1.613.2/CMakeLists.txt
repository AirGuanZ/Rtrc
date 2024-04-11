cmake_minimum_required(VERSION 3.10)

project(d3d12_1613)

add_library(d3d12_1613 INTERFACE)
target_include_directories(d3d12_1613 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/build/native/include")

set(RTRC_D3D12_SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "d3d12 sdk dir")

function(rtrc_deploy_d3d12_sdk TARGET_NAME)
	set(STAMP_FILE "${CMAKE_BINARY_DIR}/DeployD3D12SDK/${TARGET_NAME}_deploy_d3d12_sdk_timestamp")
	add_custom_command(
		OUTPUT ${STAMP_FILE}
		COMMAND
			${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12"
		COMMAND
			${CMAKE_COMMAND} -E copy_if_different
			"${RTRC_D3D12_SDK_DIR}/build/native/bin/x64/D3D12Core.dll"
			"${RTRC_D3D12_SDK_DIR}/build/native/bin/x64/D3D12Core.pdb"
			"${RTRC_D3D12_SDK_DIR}/build/native/bin/x64/d3d12SDKLayers.dll"
			"${RTRC_D3D12_SDK_DIR}/build/native/bin/x64/d3d12SDKLayers.pdb"
			"$<TARGET_FILE_DIR:${TARGET_NAME}>/D3D12"
		COMMAND ${CMAKE_COMMAND} -E touch "${STAMP_FILE}"
		DEPENDS
			"${RTRC_D3D12_SDK_DIR}/build/native/bin/x64/D3D12Core.dll"
			"${RTRC_D3D12_SDK_DIR}/build/native/bin/x64/D3D12Core.pdb"
			"${RTRC_D3D12_SDK_DIR}/build/native/bin/x64/d3d12SDKLayers.dll"
			"${RTRC_D3D12_SDK_DIR}/build/native/bin/x64/d3d12SDKLayers.pdb")
	add_custom_target(CopyD3D12DllsFor${TARGET_NAME} DEPENDS ${STAMP_FILE})
	add_dependencies(${TARGET_NAME} CopyD3D12DllsFor${TARGET_NAME})
	set_property(TARGET CopyD3D12DllsFor${TARGET_NAME} PROPERTY FOLDER "Rtrc/Generated")
endfunction()
