cmake_minimum_required(VERSION 3.10)

function(add_rtrc_sample SAMPLE_NAME)
	file(
		GLOB_RECURSE SRCS
		"${CMAKE_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.cpp"
		"${CMAKE_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.h")
	add_executable(${SAMPLE_NAME} ${SRCS})
	set_property(TARGET ${SAMPLE_NAME} PROPERTY CXX_STANDARD 23)
	set_property(TARGET ${SAMPLE_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
    set_property(TARGET ${SAMPLE_NAME} PROPERTY FOLDER "Samples")
	target_link_libraries(${SAMPLE_NAME} PUBLIC Rtrc)
	enable_rtrc_cpp_reflection(${SAMPLE_NAME} Rtrc)
endfunction()

add_rtrc_sample(01.TexturedQuad)
add_rtrc_sample(02.ComputeShader)
add_rtrc_sample(03.BindlessTexture)
add_rtrc_sample(04.RayTracingTriangle)
add_rtrc_sample(05.PathTracing)
