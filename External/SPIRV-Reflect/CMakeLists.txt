CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

PROJECT(SPIRV-REFLECT)

ADD_LIBRARY(
	spirv_reflect STATIC
	"spirv_reflect.h"
	"spirv_reflect.c")

TARGET_INCLUDE_DIRECTORIES(spirv_reflect PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
