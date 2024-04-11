cmake_minimum_required(VERSION 3.10)

project(RtrcRHI)

file(GLOB_RECURSE NON_BACKEND_SRCS "*.cpp" "*.h")
file(GLOB_RECURSE DIRECTX12_SRCS   "DirectX12/*.cpp" "DirectX12/*.h")
file(GLOB_RECURSE VULKAN_SRCS      "Vulkan/*.cpp" "Vulkan/*.h")

list(REMOVE_ITEM NON_BACKEND_SRCS ${DIRECTX12_SRCS})
list(REMOVE_ITEM NON_BACKEND_SRCS ${VULKAN_SRCS})

if(NOT RTRC_ENABLE_DIRECTX12_BACKEND)
	set(DIRECTX12_SRCS)
endif()
if(NOT RTRC_ENABLE_VULKAN_BACKEND)
	set(VULKAN_SRCS)
endif()

set(SRCS ${NON_BACKEND_SRCS} ${DIRECTX12_SRCS} ${VULKAN_SRCS})
add_library(RtrcRHI STATIC ${SRCS})

foreach(SRC IN ITEMS ${SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "RHI" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET RtrcRHI PROPERTY CXX_STANDARD 23)

if(RTRC_ENABLE_DIRECTX12_BACKEND)
	target_compile_definitions(RtrcRHI PUBLIC RTRC_RHI_DIRECTX12=1)
else()
	target_compile_definitions(RtrcRHI PUBLIC RTRC_RHI_DIRECTX12=0)
endif()
if(RTRC_ENABLE_VULKAN_BACKEND)
	target_compile_definitions(RtrcRHI PUBLIC RTRC_RHI_VULKAN=1)
else()
	target_compile_definitions(RtrcRHI PUBLIC RTRC_RHI_VULKAN=0)
endif()
if(RTRC_ENABLE_STATIC_RHI)
	target_compile_definitions(RtrcRHI PUBLIC RTRC_STATIC_RHI=1)
else()
	target_compile_definitions(RtrcRHI PUBLIC RTRC_STATIC_RHI=0)
endif()

target_link_libraries(RtrcRHI PUBLIC RtrcCore)
target_link_libraries(RtrcRHI PRIVATE glfw)

if(RTRC_ENABLE_DIRECTX12_BACKEND)
	target_link_libraries(RtrcRHI PUBLIC d3d12_1613 winpix_102 D3D12MemoryAllocator)
endif()

if(RTRC_ENABLE_VULKAN_BACKEND)
	target_link_libraries(RtrcRHI PUBLIC volk vk-bootstrap VulkanMemoryAllocatorHeader)
endif()
