#ifdef _MSC_VER
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#endif

#include <locale>
#include <codecvt>

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
    ComPtr<IDxcUtils>          utils;
    ComPtr<IDxcCompiler>       compiler;
    ComPtr<IDxcIncludeHandler> defaultHandler;
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

    const auto entryPoint = ToWString(shaderInfo.entryPoint);

    std::vector<std::wstring> macros;
    for(auto &m : shaderInfo.macros)
    {
        macros.push_back(ToWString(m.first));
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
    
    std::wstring targetProfile;
    switch(target)
    {
    case Target::Vulkan_1_3_VS_6_0: targetProfile = L"vs_6_0"; break;
    case Target::Vulkan_1_3_PS_6_0: targetProfile = L"ps_6_0"; break;
    case Target::Vulkan_1_3_CS_6_0: targetProfile = L"cs_6_0"; break;
    }

    CustomIncludeHandler customIncludeHandler;
    customIncludeHandler.utils = impl_->utils;
    customIncludeHandler.defaultHandler = impl_->defaultHandler;
    customIncludeHandler.dxcHandler = includeHandler;

    if(debugMode)
    {
        arguments.push_back(L"-Od");
    }
    else
    {
        arguments.push_back(L"-O3");
    }

    if(preprocessOutput)
    {
        arguments.push_back(L"-P ~");
    }

    ComPtr<IDxcOperationResult> oprResult;
    if(FAILED(impl_->compiler->Compile(
        sourceBlob.Get(), ToWString(shaderInfo.sourceFilename).c_str(),
        entryPoint.c_str(), targetProfile.c_str(),
        arguments.data(), static_cast<uint32_t>(arguments.size()),
        defines.data(), static_cast<uint32_t>(defines.size()),
        &customIncludeHandler, oprResult.GetAddressOf())))
    {
        throw Exception("failed to call DXC::Compile");
    }

    HRESULT status;
    if(FAILED(oprResult->GetStatus(&status)))
    {
        throw Exception("failed to get dxc operation status");
    }

    if(FAILED(status))
    {
        ComPtr<IDxcBlobEncoding> errorBuffer;
        if(FAILED(oprResult->GetErrorBuffer(errorBuffer.GetAddressOf())))
        {
            throw Exception("failed to get dxc error buffer");
        }
        if(errorBuffer && errorBuffer->GetBufferSize() > 0)
        {
            throw Exception(fmt::format(
                "dxc compile error. {}", static_cast<const char *>(errorBuffer->GetBufferPointer())));
        }
        throw Exception("dxc: an unknown error occurred");
    }

    ComPtr<IDxcBlob> result;
    if(FAILED(oprResult->GetResult(result.GetAddressOf())))
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
