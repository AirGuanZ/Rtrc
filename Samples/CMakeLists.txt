cmake_minimum_required(VERSION 3.10)

function(add_rtrc_sample SAMPLE_NAME)
	file(
		GLOB_RECURSE SRCS CONFIGURE_DEPENDS
		"${CMAKE_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.cpp"
		"${CMAKE_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.h")
	add_executable(${SAMPLE_NAME} ${SRCS})
	foreach(SRC IN ITEMS ${SRCS})
		get_filename_component(SRC_PATH "${SRC}" PATH)
		string(REPLACE "${CMAKE_SOURCE_DIR}/Samples/${SAMPLE_NAME}" "${SAMPLE_NAME}" GROUP_PATH "${SRC_PATH}")
		string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
		source_group("${GROUP_PATH}" FILES "${SRC}")
	endforeach()
	set_property(TARGET ${SAMPLE_NAME} PROPERTY CXX_STANDARD 23)
	set_property(TARGET ${SAMPLE_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    set_property(TARGET ${SAMPLE_NAME} PROPERTY FOLDER "Samples")
	target_link_libraries(${SAMPLE_NAME} PUBLIC Rtrc)
	enable_rtrc_cpp_reflection(${SAMPLE_NAME} Rtrc)
	
	file(
		GLOB_RECURSE SHADERS CONFIGURE_DEPENDS
		"${CMAKE_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.shader"
		"${CMAKE_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.material")
	enable_preprocessed_shaders(${SAMPLE_NAME} "${SHADERS}")
endfunction()

add_rtrc_sample(01.TexturedQuad)
add_rtrc_sample(02.ComputeShader)
add_rtrc_sample(03.BindlessTexture)
add_rtrc_sample(04.RayTracingTriangle)
add_rtrc_sample(05.PathTracing)
add_rtrc_sample(06.BasicRenderLoop)
