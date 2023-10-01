cmake_minimum_required(VERSION 3.10)

project(Reflection)

set(CPP_REFL_SRCS
	"${CMAKE_CURRENT_SOURCE_DIR}/Frontend.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Frontend.cpp"
	"${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp")
add_executable(Reflection ${CPP_REFL_SRCS})

foreach(SRC IN ITEMS ${CPP_REFL_SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "Reflection" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET Reflection PROPERTY CXX_STANDARD 23)

if(MSVC)
	target_link_options(Reflection PUBLIC "/STACK:10485760") # Default 4MB stack is not enough for cista x libtooling
endif()

target_link_libraries(Reflection PUBLIC Core LLVM cxxopts version)

function(get_interface_build_includes OUT TARGET_NAME)
	get_target_property(INCS ${TARGET_NAME} INTERFACE_INCLUDE_DIRECTORIES)
	list(LENGTH INCS INCS_LEN)
	if(INCS_LEN GREATER 1)
		list(GET INCS 0 BUILD_INCS)
		string(REPLACE "$<BUILD_INTERFACE:" "" BUILD_INCS ${BUILD_INCS})
		string(REPLACE ">" "" BUILD_INCS ${BUILD_INCS})
		set(INCS "${BUILD_INCS}")
	endif()
	set(${OUT} ${INCS} PARENT_SCOPE)
endfunction()

macro(get_build_includes_impl OUT TARGET_NAME INCLUDE_PRIVATE)
	if(${INCLUDE_PRIVATE})
		get_target_property(INCS ${TARGET_NAME} INCLUDE_DIRECTORIES)
	else()
		get_interface_build_includes(INCS ${TARGET_NAME})
	endif()
	if(NOT ${INCS} STREQUAL "INCS-NOTFOUND")
		list(APPEND ${OUT} ${INCS})
	endif()
	get_target_property(DEPS ${TARGET_NAME} LINK_LIBRARIES)
	foreach(DEP ${DEPS})
		if(TARGET ${DEP})
			get_build_includes_impl(${OUT} ${DEP} OFF)
		endif()
	endforeach()
endmacro()

function(get_build_includes OUT TARGET_NAME)
	set(LOCAL_INCS)
	get_build_includes_impl(LOCAL_INCS ${TARGET_NAME} ON)
	set(${OUT} ${LOCAL_INCS} PARENT_SCOPE)
endfunction()

function(list_to_string RESULT DELIM)
	list(GET ARGV 2 L)
	math(EXPR N "${ARGC}-1")
	foreach(IDX RANGE 3 ${N})
		list(GET ARGV ${IDX} ITEM)
		set(L "${L}${DELIM}${ITEM}")
	endforeach()
	set(${RESULT} "${L}" PARENT_SCOPE)
endfunction()

function(enable_rtrc_cpp_reflection_helper)

	# collect sources & includes

	set(TARGETS ${ARGV})

	set(ALL_INCLUDES)
	set(ALL_SOURCES)
	foreach(TGT ${TARGETS})
		get_build_includes(TGT_INCS ${TGT})
		list(APPEND ALL_INCLUDES ${TGT_INCS})
		get_target_property(TGT_SRCS ${TGT} SOURCES)
		list(APPEND ALL_SOURCES ${TGT_SRCS})
	endforeach()

	# generate reflection input

	set(MAIN_TARGET ${ARGV0})
	set(GEN_PATH "${CMAKE_SOURCE_DIR}/Asset/RtrcGenerated/${MAIN_TARGET}")

	set(REFL_SOURCE_CONTENT)
	foreach(SRC ${ALL_SOURCES})
		set(REFL_SOURCE_CONTENT "${REFL_SOURCE_CONTENT}#include \"${SRC}\"\n")
	endforeach()
	file(WRITE "${GEN_PATH}/unity_source.cpp" "${REFL_SOURCE_CONTENT}")

	set(REFL_SOURCE_LIST "source ${GEN_PATH}/unity_source.cpp\n")
	foreach(INC ${ALL_INCLUDES})
		set(REFL_SOURCE_LIST "${REFL_SOURCE_LIST}include ${INC}\n")
	endforeach()
	file(WRITE "${GEN_PATH}/sources.txt" "${REFL_SOURCE_LIST}")

	# add generating command

	set(GEN_FILES
			"${GEN_PATH}/Generated/Reflection.h"
			"${GEN_PATH}/Generated/Reflection.hlsl"
			"${GEN_PATH}/Generated/ReflectionList.inc"
			"${GEN_PATH}/Generated/ReflectionPath.inc"
			"${GEN_PATH}/Generated/ReflectionSourceLocation.txt")
	add_custom_command(
		OUTPUT ${GEN_FILES}
		COMMAND
			Reflection -i "${GEN_PATH}/sources.txt"
					   -c "${GEN_PATH}/Generated/Reflection.h"
					   -s "${GEN_PATH}/Generated/Reflection.hlsl"
					   -l "${GEN_PATH}/Generated/ReflectionList.inc"
					   -f "${GEN_PATH}/Generated/ReflectionSourceLocation.txt"
		COMMAND
			${CMAKE_COMMAND} -E echo "\\\"Asset/RtrcGenerated/${MAIN_TARGET}\\\""
							 > "${GEN_PATH}/Generated/ReflectionPath.inc"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		DEPENDS Reflection "${ALL_SOURCES}")

	add_custom_target(
		"RtrcGeneratedFilesFor${MAIN_TARGET}" ALL
		DEPENDS ${GEN_FILES})
	
	# setup target dependencies		
			
	add_dependencies(${MAIN_TARGET} RtrcGeneratedFilesFor${MAIN_TARGET})
	set_property(TARGET RtrcGeneratedFilesFor${MAIN_TARGET} PROPERTY FOLDER "Generated")
	target_include_directories(${MAIN_TARGET} PUBLIC "${GEN_PATH}")
	
	set(CBUFFER_REFL_FILE "${CMAKE_SOURCE_DIR}/Source/Reflection/ReflectedStruct.cpp")
	target_sources(${MAIN_TARGET} PUBLIC "${CBUFFER_REFL_FILE}")
	source_group("Reflection" FILES "${CBUFFER_REFL_FILE}")

endfunction()

function(enable_rtrc_cpp_reflection TARGET_NAME)
	enable_rtrc_cpp_reflection_helper(${TARGET_NAME} Rtrc Graphics)
endfunction()
