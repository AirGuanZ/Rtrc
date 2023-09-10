cmake_minimum_required(VERSION 3.10)

project(RtrcMain)

file(GLOB_RECURSE NON_BACKEND_SRCS "*.cpp" "*.h")
file(GLOB_RECURSE DIRECTX12_SRCS   "Graphics/RHI/DirectX12/*.cpp" "Graphics/RHI/DirectX12/*.h")
file(GLOB_RECURSE VULKAN_SRCS      "Graphics/RHI/Vulkan/*.cpp" "Graphics/RHI/Vulkan/*.h")

list(REMOVE_ITEM NON_BACKEND_SRCS ${DIRECTX12_SRCS})
list(REMOVE_ITEM NON_BACKEND_SRCS ${VULKAN_SRCS})

if(NOT ENABLE_DIRECTX12_BACKEND)
	set(DIRECTX12_SRCS)
endif()
if(NOT ENABLE_VULKAN_BACKEND)
	set(VULKAN_SRCS)
endif()

set(RTRC_SRCS ${NON_BACKEND_SRCS} ${DIRECTX12_SRCS} ${VULKAN_SRCS})
add_library(Rtrc STATIC ${RTRC_SRCS})

foreach(SRC IN ITEMS ${RTRC_SRCS})
    get_filename_component(SRC_PATH "${SRC}" PATH)
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "Rtrc" GROUP_PATH "${SRC_PATH}")
    string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
    source_group("${GROUP_PATH}" FILES "${SRC}")
endforeach()

set_property(TARGET Rtrc PROPERTY CXX_STANDARD 23)

if(ENABLE_DIRECTX12_BACKEND)
	target_compile_definitions(Rtrc PUBLIC RTRC_RHI_DIRECTX12=1)
endif()
if(ENABLE_VULKAN_BACKEND)
	target_compile_definitions(Rtrc PUBLIC RTRC_RHI_VULKAN=1)
endif()
if(ENABLE_STATIC_RHI)
	target_compile_definitions(Rtrc PUBLIC RTRC_STATIC_RHI=1)
endif()

target_include_directories(Rtrc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(Rtrc PUBLIC Core unordered_dense imgui)
target_link_libraries(Rtrc PRIVATE glfw dxc spirv_reflect)

if(ENABLE_DIRECTX12_BACKEND)
	target_link_libraries(Rtrc PUBLIC d3d12_1610 winpix_102 D3D12MemoryAllocator)
endif()
if(ENABLE_VULKAN_BACKEND)
	target_link_libraries(Rtrc PUBLIC volk vk-bootstrap VulkanMemoryAllocatorHeader)
endif()
