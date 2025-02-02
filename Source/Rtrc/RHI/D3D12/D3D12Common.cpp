#include <Rtrc/RHI/D3D12/D3D12Common.h>

#include <comdef.h>
#include <mimalloc.h>

RTRC_RHI_D3D12_BEGIN

static void *D3D12AllocateCallback(size_t size, size_t alignment, void *)
{
    return mi_aligned_alloc(alignment, size);
}

static void D3D12FreeCallback(void *ptr, void *)
{
    mi_free(ptr);
}

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 715; }
extern "C" { __declspec(dllexport) extern const char8_t *D3D12SDKPath = u8".\\D3D12\\"; }

std::string HResultToString(HRESULT hr)
{
    return _com_error(hr).ErrorMessage();
}

D3D12MA::ALLOCATION_CALLBACKS RtrcGlobalDirectX12AllocationCallbacks =
{
    .pAllocate    = D3D12AllocateCallback,
    .pFree        = D3D12FreeCallback,
    .pPrivateData = nullptr
};

std::wstring Utf8ToWin32W(std::string_view s)
{
    if(s.empty())
    {
        return {};
    }

    auto ThrowError = []
    {
        const DWORD lastError = GetLastError();
        throw Exception(std::format(
            "Fail to convert utf8 string to wstring. Error = {}",
            std::system_category().message(static_cast<int>(lastError))));
    };

    int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.length()), nullptr, 0);
    if(!len)
    {
        ThrowError();
    }

    std::wstring ret;
    ret.resize(len);
    len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), static_cast<int>(s.length()), ret.data(), len);
    if(!len)
    {
        ThrowError();
    }

    return ret;
}

D3D12_COMMAND_LIST_TYPE QueueTypeToD3D12CommandListType(QueueType type)
{
    switch(type)
    {
    case QueueType::Graphics: return D3D12_COMMAND_LIST_TYPE_DIRECT;
    case QueueType::Compute:  return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    case QueueType::Copy:     return D3D12_COMMAND_LIST_TYPE_COPY;
    }
    Unreachable();
}

static DXGI_FORMAT FormatToD3D12FormatHelper(Format format)
{
    using enum Format;
    switch(format)
    {
    case B8G8R8A8_UNorm:     return DXGI_FORMAT_B8G8R8A8_UNORM;
    case R8G8B8A8_UNorm:     return DXGI_FORMAT_R8G8B8A8_UNORM;
    case R32_Float:          return DXGI_FORMAT_R32_FLOAT;
    case R32G32_Float:       return DXGI_FORMAT_R32G32_FLOAT;
    case R32G32B32A32_Float: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case R32G32B32_Float:    return DXGI_FORMAT_R32G32B32_FLOAT;
    case R32G32B32A32_UInt:  return DXGI_FORMAT_R32G32B32A32_UINT;
    case A2R10G10B10_UNorm:  return DXGI_FORMAT_R10G10B10A2_UNORM;
    case R16_UInt:           return DXGI_FORMAT_R16_UINT;
    case R32_UInt:           return DXGI_FORMAT_R32_UINT;
    case R8_UNorm:           return DXGI_FORMAT_R8_UNORM;
    case R16_UNorm:          return DXGI_FORMAT_R16_UNORM;
    case R16G16_UNorm:       return DXGI_FORMAT_R16G16_UNORM;
    case R16G16_Float:       return DXGI_FORMAT_R16G16_FLOAT;
    }
    Unreachable();
}

DXGI_FORMAT FormatToD3D12TextureFormat(Format format)
{
    switch(format)
    {
    case Format::D24S8: return DXGI_FORMAT_R24G8_TYPELESS;
    case Format::D32:   return DXGI_FORMAT_R32_TYPELESS;
    default:            return FormatToD3D12FormatHelper(format);
    }
}

DXGI_FORMAT FormatToD3D12SRVFormat(Format format)
{
    switch(format)
    {
    case Format::D24S8: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case Format::D32:   return DXGI_FORMAT_R32_FLOAT;
    default:            return FormatToD3D12FormatHelper(format);
    }
}

DXGI_FORMAT FormatToD3D12UAVFormat(Format format)
{
    return FormatToD3D12FormatHelper(format);
}

DXGI_FORMAT FormatToD3D12RTVFormat(Format format)
{
    return FormatToD3D12FormatHelper(format);
}

DXGI_FORMAT FormatToD3D12DSVFormat(Format format)
{
    switch(format)
    {
    case Format::D24S8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case Format::D32:   return DXGI_FORMAT_D32_FLOAT;
    default:            Unreachable();
    }
}

DXGI_FORMAT FormatToD3D12BufferFormat(Format format)
{
    return FormatToD3D12FormatHelper(format);
}

RTRC_RHI_D3D12_END
