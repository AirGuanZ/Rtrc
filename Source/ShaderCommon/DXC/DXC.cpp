#ifdef _MSC_VER
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include <cassert>
#include <codecvt>
#include <filesystem>
#include <locale>

// TODO: dxc for linux
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <dxc/dxcapi.h>
#include <wrl/client.h>

#include <Core/ScopeGuard.h>
#include <ShaderCommon/DXC/DXC.h>

RTRC_BEGIN

namespace DXCDetail
{

    using Microsoft::WRL::ComPtr;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> gConvertor;

    std::wstring ToWString(const std::string &s)
    {
        return gConvertor.from_bytes(s);
    }

} // namespace DXCDetail

using Microsoft::WRL::ComPtr;

struct DXC::Impl
{
    ComPtr<IDxcUtils>           utils;
    ComPtr<IDxcCompiler3>       compiler;
    ComPtr<IDxcIncludeHandler>  defaultHandler;
};

DXC::DXC()
{
    impl_ = new Impl;
    RTRC_SCOPE_FAIL{ delete impl_; };

    if(FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(impl_->utils.GetAddressOf()))))
    {
        throw Exception("failed to create dxc utils");
    }

    if(FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(impl_->compiler.GetAddressOf()))))
    {
        throw Exception("failed to create dxc compiler");
    }

    if(FAILED(impl_->utils->CreateDefaultIncludeHandler(impl_->defaultHandler.GetAddressOf())))
    {
        throw Exception("failed to create dxc default include handler");
    }
}

DXC::~DXC()
{
    delete impl_;
}

std::vector<unsigned char> DXC::Compile(
    const ShaderInfo       &shaderInfo,
    Target                  target,
    bool                    debugMode,
    std::string            *preprocessOutput,
    std::vector<std::byte> *reflectionData) const
{
    ComPtr<IDxcBlobEncoding> sourceBlob;
    if(FAILED(impl_->utils->CreateBlob(
        shaderInfo.source.data(), static_cast<uint32_t>(shaderInfo.source.size()), CP_UTF8, sourceBlob.GetAddressOf())))
    {
        throw Exception("Failed to create shader source blob");
    }

    std::vector<LPCWSTR> arguments;

    std::wstring filename;
    if(!shaderInfo.sourceFilename.empty())
    {
        filename = DXCDetail::ToWString(shaderInfo.sourceFilename);
        arguments.push_back(filename.c_str());
    }

    std::vector<std::wstring> includeDirs;
    includeDirs.reserve(shaderInfo.includeDirs.size());
    for(auto &inc : shaderInfo.includeDirs)
    {
        includeDirs.push_back(DXCDetail::ToWString(inc));
    }
    for(auto &inc : includeDirs)
    {
        arguments.push_back(L"-I");
        arguments.push_back(inc.c_str());
    }

    std::wstring entryPoint;
    if(target != Target::Vulkan_1_3_RT_6_6 && target != Target::DirectX12_RT_6_6 && !shaderInfo.entryPoint.empty())
    {
        entryPoint = DXCDetail::ToWString(shaderInfo.entryPoint);
        arguments.push_back(L"-E");
        arguments.push_back(entryPoint.c_str());
    }

    std::wstring targetProfile;
    switch(target)
    {
    case Target::Vulkan_1_3_VS_6_6:
    case Target::DirectX12_VS_6_6:
        targetProfile = L"vs_6_6";
        break;
    case Target::Vulkan_1_3_FS_6_6:
    case Target::DirectX12_FS_6_6:
        targetProfile = L"ps_6_6";
        break;
    case Target::Vulkan_1_3_CS_6_6:
    case Target::DirectX12_CS_6_6:
        targetProfile = L"cs_6_6";
        break;
    case Target::Vulkan_1_3_RT_6_6:
    case Target::DirectX12_RT_6_6:
        targetProfile = L"lib_6_6";
        break;
    }
    arguments.push_back(L"-T");
    arguments.push_back(targetProfile.c_str());

    if(target == Target::Vulkan_1_3_RT_6_6)
    {
        arguments.push_back(L"-Vd"); // DXC crashes when ray tracing is enabled for spirv
    }

    std::vector<std::wstring> macros;
    macros.reserve(shaderInfo.macros.size());
    for(auto &m : shaderInfo.macros)
    {
        macros.push_back(DXCDetail::ToWString("-D" + m.first + "=" + m.second));
    }
    for(auto &m : macros)
    {
        arguments.push_back(m.c_str());
    }

    arguments.push_back(L"-Zpr");
    arguments.push_back(L"-enable-16bit-types");
    arguments.push_back(L"-HV");
    arguments.push_back(L"2021");

    const bool isVulkan = std::to_underlying(target) < std::to_underlying(Target::DirectX12_VS_6_6);
    if(isVulkan)
    {
        arguments.push_back(L"-spirv");
        arguments.push_back(L"-fvk-use-dx-layout");
        arguments.push_back(L"-fvk-use-dx-position-w");
        arguments.push_back(L"-fspv-target-env=vulkan1.3");
        arguments.push_back(L"-fvk-stage-io-order=alpha");
    }

    const bool enableDebugInfo =
        debugMode &&
        target != Target::Vulkan_1_3_RT_6_6 && // DXC crashes when enabling debug info for spirv ray tracing shader
        (!shaderInfo.rayQuery || SupportRayQueryDebugInfo(target));
    if(enableDebugInfo)
    {
        if(isVulkan)
        {
            arguments.push_back(L"-O0");
            arguments.push_back(L"-fspv-extension=SPV_KHR_non_semantic_info");
            arguments.push_back(L"-fspv-debug=vulkan-with-source");
        }
        else
        {
            arguments.push_back(L"-Od");
            arguments.push_back(L"-Zi");
            arguments.push_back(L"-Qembed_debug");
        }
    }
    else
    {
        arguments.push_back(L"-O3");
    }

    if(isVulkan)
    {
        if(shaderInfo.bindless)
        {
            arguments.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
        }
        if(shaderInfo.rayQuery)
        {
            arguments.push_back(L"-fspv-extension=SPV_KHR_ray_query");
        }
        if(shaderInfo.rayTracing)
        {
            arguments.push_back(L"-fspv-extension=SPV_KHR_ray_tracing");
        }
    }

    if(preprocessOutput)
    {
        arguments.push_back(L"-P");
        arguments.push_back(L"-Fi");
        arguments.push_back(L"~");
    }

    const DxcBuffer sourceBuffer = { sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), 0 };
    ComPtr<IDxcResult> compileResult;
    if(FAILED(impl_->compiler->Compile(
        &sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()),
        impl_->defaultHandler.Get(), IID_PPV_ARGS(compileResult.GetAddressOf()))))
    {
        throw Exception("Failed to call DXC::Compile");
    }

    HRESULT status;
    if(FAILED(compileResult->GetStatus(&status)))
    {
        throw Exception("Failed to get dxc operation status");
    }

    const bool hasCompileError = FAILED(status);
    if(hasCompileError || RTRC_DEBUG)
    {
        ComPtr<IDxcBlobUtf8> errors;
        if(FAILED(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr)))
        {
            throw Exception("Failed to get error message from DXC");
        }
        if(errors && errors->GetStringLength() > 0)
        {
            const std::string msg = errors->GetStringPointer();
            if(hasCompileError)
            {
                throw Exception(msg);
            }
#if RTRC_DEBUG
            LogWarn(msg);
#endif
        }
        if(hasCompileError)
        {
            throw Exception("DXC: an unknown error occurred");
        }
    }

    ComPtr<IDxcBlob> result;
    if(FAILED(compileResult->GetResult(result.GetAddressOf())))
    {
        throw Exception("Failed to get dxc compile result");
    }

    if(preprocessOutput)
    {
        *preprocessOutput = static_cast<const char *>(result->GetBufferPointer());
        return {};
    }

    if(reflectionData)
    {
        assert(!preprocessOutput);
        assert(std::to_underlying(target) >= std::to_underlying(Target::DirectX12_VS_6_6));
        ComPtr<IDxcBlob> reflectionBlob;
        if(FAILED(compileResult->GetOutput(
            DXC_OUT_REFLECTION, IID_PPV_ARGS(reflectionBlob.GetAddressOf()), nullptr)))
        {
            throw Exception("Fail to get dxc reflection result");
        }
        reflectionData->resize(reflectionBlob->GetBufferSize());
        std::memcpy(reflectionData->data(), reflectionBlob->GetBufferPointer(), reflectionBlob->GetBufferSize());
    }

    std::vector<unsigned char> ret(result->GetBufferSize());
    std::memcpy(ret.data(), result->GetBufferPointer(), result->GetBufferSize());
    return ret;
}

IDxcUtils *DXC::GetDxcUtils() const
{
    return impl_->utils.Get();
}

bool DXC::SupportRayQueryDebugInfo(Target target)
{
    return target == Target::DirectX12_CS_6_6 ||
           target == Target::DirectX12_FS_6_6 ||
           target == Target::DirectX12_VS_6_6 ||
           target == Target::DirectX12_RT_6_6;
}

RTRC_END
