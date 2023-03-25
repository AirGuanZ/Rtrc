set_project("Rtrc")

option("vulkan_backend")
    set_default(true)
option("directx12_backend")
    set_default(false)
option("static_rhi")
    set_default(false)
option_end()

-- Global settings

set_arch("x64")
set_languages("c++23")
add_rules("mode.debug", "mode.release")

if is_mode("release") then
    add_rules("c++.unity_build", { batchsize = 16 })
    set_policy("build.optimization.lto", true)
end

option("is_msvc")
    add_csnippets("is_msvc", "return (_MSC_VER)?0:-1;", { tryrun = true })
option_end()

set_runtimes(is_mode("debug") and "MTd" or "MT")
set_targetdir("Build/Bin/"..(is_mode("debug") and "debug" or "release"))

if is_mode("debug") then
    add_defines("DEBUG", "_DEBUG")
end

if is_plat("windows") then
    add_defines("WIN32")
end

if has_config("is_msvc") then
    add_cxflags("/Zc:preprocessor")
    add_defines("_CRT_SECURE_NO_WARNINGS")
end

-- External dependencies

--includes("External/abseil")
--add_requires("abseil", { configs = { shared = false } })

includes("External/glfw")
add_requires("glfw", { configs = { glfw_include = "vulkan" } })

includes("External/mimalloc")
add_requires("mimalloc")

includes("External/tbb")
add_requires("mytbb", { configs = { debug = is_mode("debug") } })

includes("External/dxc")
add_requires("mydxc")

add_requires("stb 2021.09.10", "tinyexr v1.0.1")
add_requires("spirv-reflect 1.2.189+1")
add_requires("catch2 3.1.0")
add_requires("fmt 9.1.0", { configs = { header_only = true } })
add_requires("spdlog v1.11.0")

if has_config("vulkan_backend") then
    add_requires("vk-bootstrap v0.5", "vulkan-memory-allocator v3.0.0")
    add_requires("volk 1.3.231", { configs = { header_only = true } })
end

if has_config("directx12_backend") then
    add_requires("d3d12-memory-allocator v2.0.1")
end

includes("External/tinyobjloader")
includes("External/sigslot")
includes("External/imgui")
includes("External/avir")
includes("External/cy")

-- Targets

target("Rtrc")
    set_kind("static")
    -- Source files
    add_includedirs("Source", { public = true })
    add_headerfiles("Source/**.h|Rtrc/Graphics/RHI/**.h", "Source/Rtrc/Graphics/RHI/*.h")
    add_headerfiles()
    add_files("Source/**.cpp|Rtrc/Graphics/RHI/**.cpp", "Source/Rtrc/Graphics/RHI/*.cpp")
    -- Source group
    add_filegroups("Rtrc", { rootdir = "Source/Rtrc" })
    -- Vulkan RHI
    add_options("vulkan_backend")
    if has_config("vulkan_backend") then
        add_headerfiles("Source/Rtrc/Graphics/RHI/Vulkan/**.h")
        add_files("Source/Rtrc/Graphics/RHI/Vulkan/**.cpp", { unity_group = "VulkanRHI" })
        add_defines("RTRC_RHI_VULKAN=1", { public = true })
        add_defines("VMA_STATIC_VULKAN_FUNCTIONS=0", "VMA_DYNAMIC_VULKAN_FUNCTIONS=1", { public = false })
        add_packages("spirv-reflect")
        add_packages("volk", "vk-bootstrap", "vulkan-memory-allocator", { public = has_config("static_rhi") })
    end
    -- DirectX12 RHI
    add_options("directx12_backend")
    if has_config("directx12_backend") then
        add_headerfiles("Source/Rtrc/Graphics/RHI/DirectX12/**.h")
        add_files("Source/Rtrc/Graphics/RHI/DirectX12/**.cpp", { unity_group = "DirectX12RHI" })
        add_defines("RTRC_RHI_DIRECTX12=1", { public = true })
        add_packages("d3d12-memory-allocator")
    end
    -- Static RHI
    if has_config("static_rhi") then
        add_defines("RTRC_STATIC_RHI=1", { public = true })
    end
    -- Dependencies
    add_packages("mydxc", "glfw", "stb", "tinyexr")
    add_packages("spdlog", "fmt", "mimalloc", "mytbb", --[["abseil",--]] { public = true })
    add_deps("tinyobjloader", "sigslot", "imgui", "avir", "cy")
target_end()

target("Test")
    set_kind("binary")
    set_rundir(".")
    add_files("Test/**.cpp")
    add_packages("catch2")
    add_deps("Rtrc")
target_end()

function add_sample(name)
    target(name)
        set_kind("binary")
        set_rundir(".")
        local source_dir = "Samples/"..name;
        add_headerfiles(source_dir.."/**.h")
        add_files(source_dir.."/**.cpp")
        add_filegroups("Source", { rootdir = source_dir })
        add_deps("Rtrc")
    target_end()
end

add_sample("01.TexturedQuad")
add_sample("02.ComputeShader")
add_sample("03.DeferredShading")
add_sample("04.BindlessTexture")
add_sample("05.RayTracingTriangle")
add_sample("06.PathTracing")
add_sample("07.Application")
