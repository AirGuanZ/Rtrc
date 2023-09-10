cmake_minimum_required(VERSION 3.12)

add_custom_command(
	OUTPUT
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/tbb12$<$<CONFIG:Debug>:_debug>.dll"
	COMMAND
		${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/../oneapi-tbb-2021.10.0/redist/intel64/vc14/tbb12$<$<CONFIG:Debug>:_debug>.dll"
		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/tbb12$<$<CONFIG:Debug>:_debug>.dll"
	DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/../oneapi-tbb-2021.10.0/redist/intel64/vc14/tbb12$<$<CONFIG:Debug>:_debug>.dll")

add_custom_target(
	CopyTBBDlls
	DEPENDS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<IF:$<CONFIG:Debug>,Debug,Release>/tbb12$<$<CONFIG:Debug>:_debug>.dll")
	
find_package(TBB REQUIRED tbb PATHS "${CMAKE_CURRENT_SOURCE_DIR}/oneapi-tbb-2021.10.0/lib/cmake/tbb")

add_library(tbbinterface INTERFACE)
target_link_libraries(tbbinterface INTERFACE TBB::tbb)
add_dependencies(tbbinterface CopyTBBDlls)
