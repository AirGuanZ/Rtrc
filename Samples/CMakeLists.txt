cmake_minimum_required(VERSION 3.22)

function(add_rtrc_sample SAMPLE_NAME)
	file(
		GLOB_RECURSE SRCS CONFIGURE_DEPENDS
		"${RTRC_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.cpp"
		"${RTRC_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.h")
	file(
		GLOB_RECURSE SHADERS CONFIGURE_DEPENDS
		"${RTRC_SOURCE_DIR}/Samples/${SAMPLE_NAME}/*.shader")
	set(ALL_SRCS ${SRCS} ${SHADERS})
	add_executable(${SAMPLE_NAME} ${ALL_SRCS})
	foreach(SRC IN ITEMS ${ALL_SRCS})
		get_filename_component(SRC_PATH "${SRC}" PATH)
		string(REPLACE "${RTRC_SOURCE_DIR}/Samples/${SAMPLE_NAME}" "${SAMPLE_NAME}" GROUP_PATH "${SRC_PATH}")
		string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
		source_group("${GROUP_PATH}" FILES "${SRC}")
	endforeach()
	set_source_files_properties(${SHADERS} PROPERTIES HEADER_FILE_ONLY TRUE)
	set_property(TARGET ${SAMPLE_NAME} PROPERTY CXX_STANDARD 23)
	set_property(TARGET ${SAMPLE_NAME} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${RTRC_SOURCE_DIR}")
    set_property(TARGET ${SAMPLE_NAME} PROPERTY FOLDER "Samples")
	target_link_libraries(${SAMPLE_NAME} PUBLIC ToolKit)
	rtrc_register_target(${SAMPLE_NAME})
endfunction()

add_rtrc_sample(01.TexturedQuad)
add_rtrc_sample(02.ComputeShader)
add_rtrc_sample(03.BindlessTexture)
add_rtrc_sample(04.RayTracingTriangle)
add_rtrc_sample(05.PathTracing)
add_rtrc_sample(06.DistortionFreeDisplacementMap)
add_rtrc_sample(07.Gizmo)
#add_rtrc_sample(08.DisplacementMapFilter)
