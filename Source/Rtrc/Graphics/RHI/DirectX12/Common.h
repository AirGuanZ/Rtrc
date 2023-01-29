#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3D12MemAlloc.h>
#include <wrl/client.h>

#include <Rtrc/Graphics/RHI/RHIDeclaration.h>

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

#define RTRC_D3D12_CHECK(RESULT) ::Rtrc::RHI::D3D12::D3D12ResultChecker{(RESULT)}+[&](HRESULT errorCode)->void
#define RTRC_D3D12_FAIL_MSG(RESULT, MSG) do { RTRC_D3D12_CHECK(RESULT) { throw ::Rtrc::Exception(MSG); }; } while(false)

struct DirectX12MemoryAllocation
{
    D3D12MA::Allocator *allocator;
    D3D12MA::Allocation *allocation;
};

enum class ResourceOwnership
{
    Allocation,
    Resource,
    None
};

RTRC_RHI_D3D12_END
