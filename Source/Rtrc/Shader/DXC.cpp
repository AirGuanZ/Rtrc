#ifdef _MSC_VER
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include <codecvt>
#include <filesystem>
#include <locale>

// TODO: dxc for linux
#include <Windows.h>
#include <dxc/dxcapi.h>
#include <fmt/format.h>
#include <wrl/client.h>

#include <Rtrc/Shader/DXC.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_BEGIN

namespace
{

    using Microsoft::WRL::ComPtr;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> gConvertor;

    std::wstring ToWString(const std::string &s)
    {
        return gConvertor.from_bytes(s);
    }

    std::string ToString(const std::wstring &s)
    {
        return gConvertor.to_bytes(s);
    }

    class CustomIncludeHandler : public IDxcIncludeHandler
    {
    public:

        virtual ~CustomIncludeHandler() = default;

        HRESULT STDMETHODCALLTYPE LoadSource(
            _In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob **ppIncludeSource) override
        {
            std::string result;
            if(!dxcHandler->Handle(ToString(pFilename), result))
            {
                return S_FALSE;
            }
            ComPtr<IDxcBlobEncoding> pEncoding;
            if(FAILED(utils->CreateBlob(
                result.data(), static_cast<uint32_t>(result.size()), CP_UTF8, pEncoding.GetAddressOf())))
            {
                return S_FALSE;
            }
            *ppIncludeSource = pEncoding.Detach();
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(
            REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject) override
        {
            return defaultHandler->QueryInterface(riid, ppvObject);
        }

        ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
        ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

        ComPtr<IDxcUtils> utils;
        ComPtr<IDxcIncludeHandler> defaultHandler;
        const DXC::IncludeHandler *dxcHandler = nullptr;
    };

} // namespace anonymous

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
    const ShaderInfo     &shaderInfo,
    Target                target,
    bool                  debugMode,
    const IncludeHandler *includeHandler,
    std::string          *preprocessOutput) const
{
    ComPtr<IDxcBlobEncoding> sourceBlob;
    if(FAILED(impl_->utils->CreateBlob(
        shaderInfo.source.data(), static_cast<uint32_t>(shaderInfo.source.size()), CP_UTF8, sourceBlob.GetAddressOf())))
    {
        throw Exception("failed to create shader source blob");
    }

    std::vector<LPCWSTR> arguments;

    std::wstring filename;
    if(!shaderInfo.sourceFilename.empty())
    {
        filename = ToWString(shaderInfo.sourceFilename);
        arguments.push_back(filename.c_str());
    }

    std::vector<std::wstring> includeDirs;
    for(auto &inc : shaderInfo.includeDirs)
    {
        includeDirs.push_back(ToWString(inc));
    }
    for(auto &inc : includeDirs)
    {
        arguments.push_back(L"-I");
        arguments.push_back(inc.c_str());
    }

    const auto entryPoint = ToWString(shaderInfo.entryPoint);
    arguments.push_back(L"-E");
    arguments.push_back(entryPoint.c_str());

    std::wstring targetProfile;
    switch(target)
    {
    case Target::Vulkan_1_3_VS_6_0: targetProfile = L"vs_6_0"; break;
    case Target::Vulkan_1_3_FS_6_0: targetProfile = L"ps_6_0"; break;
    case Target::Vulkan_1_3_CS_6_0: targetProfile = L"cs_6_0"; break;
    }
    arguments.push_back(L"-T");
    arguments.push_back(targetProfile.c_str());

    std::vector<std::wstring> macros;
    for(auto &m : shaderInfo.macros)
    {
        macros.push_back(ToWString("-D" + m.first));
        macros.push_back(ToWString(m.second));
    }
    std::vector<DxcDefine> defines;
    for(size_t i = 0; i < macros.size(); i += 2)
    {
        defines.push_back(DxcDefine{
            .Name = macros[i].c_str(),
            .Value = macros[i + 1].c_str()
        });
    }

    arguments.push_back(L"-spirv");
    arguments.push_back(L"-fvk-use-dx-layout");
    arguments.push_back(L"-fvk-use-dx-position-w");
    arguments.push_back(L"-fspv-target-env=vulkan1.3");
    arguments.push_back(L"-fvk-stage-io-order=alpha");

    CustomIncludeHandler customIncludeHandler;
    customIncludeHandler.utils = impl_->utils;
    customIncludeHandler.defaultHandler = impl_->defaultHandler;
    customIncludeHandler.dxcHandler = includeHandler;

    if(debugMode)
    {
        arguments.push_back(L"-O0");
        arguments.push_back(L"-Zi");
        arguments.push_back(L"-fspv-debug=vulkan-with-source");
        //arguments.push_back(L"-fspv-extension=SPV_GOOGLE_hlsl_functionality1");
        //arguments.push_back(L"-fspv-extension=SPV_GOOGLE_user_type");
        //arguments.push_back(L"-fspv-extension=SPV_KHR_non_semantic_info");
        //arguments.push_back(L"-fspv-reflect");
    }
    else
    {
        arguments.push_back(L"-O3");
    }

    if(preprocessOutput)
    {
        arguments.push_back(L"-P");
        arguments.push_back(L"~");
    }

    const DxcBuffer sourceBuffer = {
        .Ptr = sourceBlob->GetBufferPointer(),
        .Size = sourceBlob->GetBufferSize()
    };
    ComPtr<IDxcResult> compileResult;
    if(FAILED(impl_->compiler->Compile(
        &sourceBuffer, arguments.data(), static_cast<uint32_t>(arguments.size()),
        &customIncludeHandler, IID_PPV_ARGS(compileResult.GetAddressOf()))))
    {
        throw Exception("failed to call DXC::Compile");
    }

    HRESULT status;
    if(FAILED(compileResult->GetStatus(&status)))
    {
        throw Exception("failed to get dxc operation status");
    }

    if(FAILED(status))
    {
        ComPtr<IDxcBlobUtf8> errors;
        if(FAILED(compileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr)))
        {
            throw Exception("failed to get error message from DXC");
        }
        if(errors && errors->GetStringLength() > 0)
        {
            throw Exception(fmt::format("dxc compile error. {}", errors->GetStringPointer()));
        }
        throw Exception("dxc: an unknown error occurred");
    }

    ComPtr<IDxcBlob> result;
    if(FAILED(compileResult->GetResult(result.GetAddressOf())))
    {
        throw Exception("failed to get dxc compile result");
    }

    if(preprocessOutput)
    {
        *preprocessOutput = reinterpret_cast<const char *>(result->GetBufferPointer());
        return {};
    }

    std::vector<unsigned char> ret(result->GetBufferSize());
    std::memcpy(ret.data(), result->GetBufferPointer(), result->GetBufferSize());
    return ret;
}

RTRC_END
