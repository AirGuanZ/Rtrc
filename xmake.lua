set_project("Rtrc")

option("vulkan_backend")
    set_default(true)
    set_showmenu(true)
option("directx12_backend")
    set_default(true)
    set_showmenu(true)
option("static_rhi")
    set_default(false)
    set_showmenu(true)
option_end()

-- Global settings

set_arch("x64")
set_languages("c++23")
add_rules("mode.debug", "mode.release")
set_policy("build.across_targets_in_parallel", false)

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

includes("External/abseil")
add_requires("abseil", { configs = { shared = false } })

includes("External/glfw")
if has_config("vulkan_backend") then
    add_requires("glfw", { configs = { glfw_include = "vulkan" } })
else
    add_requires("glfw")
end

includes("External/mimalloc")
add_requires("mimalloc", { configs = { shared = false } })

includes("External/tbb")
add_requires("mytbb", { configs = { debug = is_mode("debug") } })

includes("External/dxc")
add_requires("mydxc")

add_requires("stb 2021.09.10", "tinyexr v1.0.1")
add_requires("spirv-reflect 1.2.189+1")
add_requires("catch2 3.1.0")
add_requires("fmt 9.1.0", { configs = { header_only = true } })
add_requires("spdlog v1.11.0")
add_requires("cxxopts v3.1.1")

includes("External/llvm16")
add_requires("myclang")

if has_config("vulkan_backend") then
    add_requires("vk-bootstrap v0.5", "vulkan-memory-allocator v3.0.0")
    add_requires("volk 1.3.231", { configs = { header_only = true } })
end

if has_config("directx12_backend") then
    add_requires("d3d12-memory-allocator v2.0.1")
    includes("External/microsoft.direct3d.d3d12.1.610.3")
	includes("External/winpixeventruntime.1.0.230302001")
    add_requires("d3d12_1610")
	add_requires("winpix_102")
end

includes("External/tinyobjloader")
includes("External/sigslot")
includes("External/imgui")
includes("External/avir")
includes("External/cy")

-- Targets

target("Reflection")
    set_kind("binary")
    set_rundir(".")
    set_strip("all")
    set_optimize("fastest")
    -- Source files
    add_includedirs("Source", { public = true })
    add_headerfiles("Source/Reflection/**.h")
    add_files("Source/Reflection/**.cpp")
    add_filegroups("Reflection", { rootdir = "Source/Reflection" })
    -- Dependencies
    add_packages("myclang", "fmt", "cxxopts")
    if is_plat("windows") then
        add_syslinks("Version")
    end
target_end()

rule("reflect_cpp")
    on_load(function (target)
        local outdir = path.join(target:autogendir(), "AutoGen")
        target:add("includedirs", outdir, { public = true })
        target:add("deps", "Reflection")
    end)
    before_build(function (target)
        local sourcebatch = nil
        for _, batch in pairs(target:sourcebatches()) do
            if batch.sourcekind == "cxx" and batch.rulename == "c++.build" then
                sourcebatch = batch
                break
            end
        end
        if sourcebatch == nil then
            return
        end

        local outdir = path.join(target:autogendir(), "AutoGen")
        local outcppfile = path.join(outdir, "Rtrc/Generated/Reflection.h")
        outcppfile = outcppfile:gsub("\\", "/")
        local outhlslfile = path.join(os.scriptdir(), "Asset/Builtin/Material/Rtrc/Generated/Reflection.hlsl")
        outhlslfile = outhlslfile:gsub("\\", "/")
        local outlistfile = path.join(outdir, "Rtrc/Generated/ReflectionList.inc")
        outlistfile = outlistfile:gsub("\\", "/")

        local unity_source = ""
        for _, sourcefile in ipairs(sourcebatch.sourcefiles) do
            local absfile = path.absolute(sourcefile)
            local relfile = path.relative(absfile, path.join("$(projectdir)", Source))
			unity_source = unity_source.."#include <"..relfile..">\n"
		end
        
        local unity_source_file = path.join(target:autogendir(), "unity_source.cpp")
        io.writefile(unity_source_file, unity_source)

        local source_txt_file = path.join(target:autogendir(), "rtrc_sources.txt")
        source_txt_file = source_txt_file:gsub("\\", "/")
        
        local source_txt = "source "..unity_source_file.."\n"
        for _, includedir in ipairs(target:get("includedirs")) do
            source_txt = source_txt.."include "..includedir.."\n"
        end
    
        refltarget = target:dep("Reflection")
        io.writefile(source_txt_file, source_txt)

        local cmd = refltarget:targetfile()
        local cmdi = " -i "..source_txt_file
        local cmdc = " -c "..outcppfile
        local cmds = " -s "..outhlslfile
        local cmdl = " -l "..outlistfile

        print("generating reflection info...command is")
        print("    "..cmd)
        print("       "..cmdi)
        print("       "..cmdc)
        print("       "..cmds)
        print("       "..cmdl)
        os.run(cmd..cmdi..cmdc..cmds..cmdl)
    end)
rule_end()

target("Rtrc")
    set_kind("static")
    -- Source files
    add_includedirs("Source", { public = true })
    add_headerfiles("Source/Rtrc/**.h|Graphics/RHI/**.h", "Source/Rtrc/Graphics/RHI/*.h")
    add_headerfiles()
    add_files("Source/Rtrc/**.cpp|Graphics/RHI/**.cpp", "Source/Rtrc/Graphics/RHI/*.cpp")
    -- Source group
    add_filegroups("Rtrc", { rootdir = "Source/Rtrc" })
    -- Dependencies
    add_packages("mimalloc", "spdlog", "fmt", "mytbb", "abseil", { public = true })
    add_packages("mydxc", "glfw", "stb", "tinyexr", "spirv-reflect")
    add_deps("tinyobjloader", "sigslot", "imgui", "avir", "cy")
	add_includedirs("External/half/include", { public = true })
    -- Vulkan RHI
    add_options("vulkan_backend")
    if has_config("vulkan_backend") then
        add_headerfiles("Source/Rtrc/Graphics/RHI/Vulkan/**.h")
        add_files("Source/Rtrc/Graphics/RHI/Vulkan/**.cpp", { unity_group = "VulkanRHI" })
        add_defines("RTRC_RHI_VULKAN=1", { public = true })
        add_defines("VMA_STATIC_VULKAN_FUNCTIONS=0", "VMA_DYNAMIC_VULKAN_FUNCTIONS=1", { public = false })
        add_packages("volk", "vk-bootstrap", "vulkan-memory-allocator", { public = has_config("static_rhi") })
    end
    -- DirectX12 RHI
    add_options("directx12_backend")
    if has_config("directx12_backend") then
        add_headerfiles("Source/Rtrc/Graphics/RHI/DirectX12/**.h")
        add_files("Source/Rtrc/Graphics/RHI/DirectX12/**.cpp", { unity_group = "DirectX12RHI" })
        add_defines("RTRC_RHI_DIRECTX12=1", { public = true })
        add_syslinks("d3d12", "dxgi", "dxguid")
        add_packages("d3d12-memory-allocator", "d3d12_1610", "winpix_102", { public = has_config("static_rhi") })
    end
    -- RHI Helper
    add_headerfiles("Source/Rtrc/Graphics/RHI/Helper/**.h")
    add_files("Source/Rtrc/Graphics/RHI/Helper/**.cpp", { unity_group = "DirectX12RHI" })
    -- Static RHI
    if has_config("static_rhi") then
        add_defines("RTRC_STATIC_RHI=1", { public = true })
    end
    -- Reflection
    add_rules("reflect_cpp")
target_end()

-- Standalone renderer

target("Standalone")
    set_kind("binary")
    set_rundir(".")
    add_headerfiles("Source/Standalone/**.h")
    add_files("Source/Standalone/**.cpp")
    add_filegroups("Standalone", { rootdir = "Source/Standalone" })
    add_deps("Rtrc")
target_end()

-- Tests

target("Test")
    set_kind("binary")
    set_rundir(".")
    add_files("Test/**.cpp")
    add_packages("catch2")
    add_deps("Rtrc")
target_end()

-- Samples

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
add_sample("03.BindlessTexture")
add_sample("04.RayTracingTriangle")
add_sample("05.PathTracing")
