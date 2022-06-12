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
            if(auto it = virtualFiles->find(ToString(pFilename)); it != virtualFiles->end())
            {
                ComPtr<IDxcBlobEncoding> pEncoding;
                if(FAILED(utils->CreateBlob(
                    it->second.data(), static_cast<uint32_t>(it->second.size()), CP_UTF8, pEncoding.GetAddressOf())))
                {
                    return S_FALSE;
                }
                *ppIncludeSource = pEncoding.Detach();
                return S_OK;
            }
            return defaultHandler->LoadSource(pFilename, ppIncludeSource);
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
        const std::map<std::string, std::string> *virtualFiles;
    };

} // namespace anonymous

using Microsoft::WRL::ComPtr;

struct DXC::Impl
{
    ComPtr<IDxcUtils>          utils;
    ComPtr<IDxcCompiler3>      compiler;
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
    const ShaderInfo                         &shaderInfo,
    Target                                    target,
    bool                                      debugMode,
    const std::map<std::string, std::string> &virtualFiles) const
{
    ComPtr<IDxcBlobEncoding> sourceBlob;
    if(FAILED(impl_->utils->CreateBlob(
        shaderInfo.source.data(), static_cast<uint32_t>(shaderInfo.source.size()), CP_UTF8, sourceBlob.GetAddressOf())))
    {
        throw Exception("failed to create shader source blob");
    }

    std::vector<LPCWSTR> arguments;

    const auto entryPoint = ToWString(shaderInfo.entryPoint);
    arguments.push_back(L"-E");
    arguments.push_back(entryPoint.c_str());

    std::vector<std::wstring> macros;
    for(auto &m : shaderInfo.macros)
    {
        macros.push_back(ToWString(m.first + "=" + m.second));
    }
    for(auto &m : macros)
    {
        arguments.push_back(L"-D");
        arguments.push_back(m.c_str());
    }

    arguments.push_back(L"-spirv");
    arguments.push_back(L"-fvk-use-dx-layout");
    arguments.push_back(L"-fvk-use-dx-position-w");
    arguments.push_back(L"-fspv-target-env=vulkan1.3");

    arguments.push_back(L"-T");
    switch(target)
    {
    case Target::Vulkan_1_3_VS_6_0:
        arguments.push_back(L"vs_6_0");
        break;
    case Target::Vulkan_1_3_PS_6_0:
        arguments.push_back(L"ps_6_0");
        break;
    }

    CustomIncludeHandler includeHandler;
    includeHandler.utils = impl_->utils;
    includeHandler.defaultHandler = impl_->defaultHandler;
    includeHandler.virtualFiles = &virtualFiles;

    if(debugMode)
    {
        arguments.push_back(L"-Od");
    }
    else
    {
        arguments.push_back(L"-O3");
    }

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = 0;

    ComPtr<IDxcResult> result;
    if(FAILED(impl_->compiler->Compile(
        &sourceBuffer,
        arguments.data(), static_cast<uint32_t>(arguments.size()),
        &includeHandler, IID_PPV_ARGS(result.GetAddressOf()))))
    {
        throw Exception("failed to call DXC::Compile");
    }

    ComPtr<IDxcBlobUtf8> errors;
    if(FAILED(result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr)))
    {
        throw Exception("failed to get dxc compile errors");
    }

    if(errors && errors->GetStringLength() > 0)
    {
        throw Exception(fmt::format("dxc compile error. {}", static_cast<const char*>(errors->GetBufferPointer())));
    }

    ComPtr<IDxcBlob> code;
    if(FAILED(result->GetResult(code.GetAddressOf())))
    {
        throw Exception("failed to get dxc compile result");
    }

    std::vector<unsigned char> ret(code->GetBufferSize());
    std::memcpy(ret.data(), code->GetBufferPointer(), code->GetBufferSize());
    return ret;
}

RTRC_END
