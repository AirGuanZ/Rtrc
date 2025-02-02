#pragma once

#include <Rtrc/RHI/RHI.h>

#include <d3d12_agility/d3d12.h>
#include <dxgi1_4.h>
#include <D3D12MemAlloc.h>
#include <wrl/client.h>

#define RTRC_RHI_D3D12_BEGIN RTRC_RHI_BEGIN namespace D3D12 {
#define RTRC_RHI_D3D12_END } RTRC_RHI_END

RTRC_RHI_D3D12_BEGIN

using Microsoft::WRL::ComPtr;

struct D3D12ResultChecker
{
    HRESULT result;
    template<typename F>
    void operator+(F &&f)
    {
        if(FAILED(result))
        {
            std::invoke(std::forward<F>(f), result);
        }
    }
};

std::string HResultToString(HRESULT hr);

#define RTRC_D3D12_CHECK(RESULT) ::Rtrc::RHI::D3D12::D3D12ResultChecker{(RESULT)}+[&](HRESULT errorCode)->void
#define RTRC_D3D12_FAIL_MSG(RESULT, MSG)                                                                            \
    do                                                                                                              \
    {                                                                                                               \
        const auto r = (RESULT);                                                                                    \
        RTRC_D3D12_CHECK(r)                                                                                         \
        {                                                                                                           \
            throw ::Rtrc::Exception(std::format(                                                                    \
                "{} (Error = {:#X}, {})", (MSG), uint32_t(r), ::Rtrc::RHI::D3D12::HResultToString(r)));             \
        };                                                                                                          \
    } while(false)

struct D3D12MemoryAllocation
{
    ComPtr<D3D12MA::Allocation> allocation;
};

extern D3D12MA::ALLOCATION_CALLBACKS RtrcGlobalDirectX12AllocationCallbacks;
#define RTRC_D3D12_ALLOC (&RtrcGlobalDirectX12AllocationCallbacks)

std::wstring Utf8ToWin32W(std::string_view s);

#define RTRC_D3D12_IMPL_SET_NAME(OBJPTR)               \
    void SetName(std::string name) override            \
    {                                                  \
        (OBJPTR)->SetName(Utf8ToWin32W(name).c_str()); \
        RHIObject::SetName(std::move(name));           \
    }

D3D12_COMMAND_LIST_TYPE QueueTypeToD3D12CommandListType(QueueType type);

DXGI_FORMAT FormatToD3D12TextureFormat(Format format);
DXGI_FORMAT FormatToD3D12SRVFormat(Format format);
DXGI_FORMAT FormatToD3D12UAVFormat(Format format);
DXGI_FORMAT FormatToD3D12RTVFormat(Format format);
DXGI_FORMAT FormatToD3D12DSVFormat(Format format);
DXGI_FORMAT FormatToD3D12BufferFormat(Format format);

RTRC_RHI_D3D12_END
