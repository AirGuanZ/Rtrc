set_project("Rtrc")

set_arch("x64")
set_languages("c++23")
add_rules("mode.debug", "mode.release")

-- Global definitions

if is_mode("debug") then
    add_defines("DEBUG", "_DEBUG")
    set_runtimes("MTd")
    set_symbols("debug")
else
    set_runtimes("MT")
end

if is_plat("windows") then
    add_defines("WIN32")
end

-- Dependencies

includes("External/glfw")
add_requires("glfw", {configs = { glfw_include = "vulkan" }})

includes("External/mimalloc")
add_requires("mimalloc")

includes("External/oneapi-tbb-2021.6.0")

add_requires("fmt 9.1.0", "stb 2021.09.10", "tinyexr v1.0.1")
add_requires("vk-bootstrap v0.5", "spirv-reflect 1.2.189+1", "vulkan-memory-allocator v3.0.0")
add_requires("volk 1.3.204", {configs = {header_only = true}})

includes("External/tinyobjloader")
includes("External/dxc")

-- Targets

target("Rtrc")
    set_kind("static")
    -- Enable vulkan rhi
    add_defines("RTRC_RHI_VULKAN", {public = true})
    add_defines("VMA_STATIC_VULKAN_FUNCTIONS=0", "VMA_DYNAMIC_VULKAN_FUNCTIONS=1", {public = false})
    -- Source files
    add_includedirs("Source", {public = true})
    add_headerfiles("Source/**.h")
    add_files("Source/**.cpp")
    remove_headerfiles("Source/Rtrc/Graphics/RHI/DirectX12/**.h")
    remove_files("Source/Rtrc/Graphics/RHI/DirectX12/**.cpp")
    -- Dependencies
    add_includedirs("External/avir", {public = false})
    add_packages("glfw", "stb", "tinyexr", "spirv-reflect", "volk", "vk-bootstrap", "vulkan-memory-allocator")
    add_packages("fmt", "mimalloc", {public = true})
    add_deps("dxc", "tinyobjloader", "tbb")

target("01.TexturedQuad")
    set_kind("binary")
    set_group("Samples")
    set_rundir(".")
    add_files("Samples/01.TexturedQuad/*.cpp")
    add_deps("Rtrc")
    
target("02.ComputeShader")
    set_kind("binary")
    set_group("Samples")
    set_rundir(".")
    add_files("Samples/02.ComputeShader/*.cpp")
    add_deps("Rtrc")
    
target("03.DeferredShading")
    set_kind("binary")
    set_group("Samples")
    set_rundir(".")
    add_files("Samples/03.DeferredShading/*.cpp")
    add_deps("Rtrc")
