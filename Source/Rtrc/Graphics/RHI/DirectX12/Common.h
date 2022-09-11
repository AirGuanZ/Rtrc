#pragma once

#include <wrl/client.h>

#include <D3D12MemAlloc.h>

#include <Rtrc/Graphics/RHI/RHI.h>

/*
 * IMPORTANT: waiting for driver implementation of 'Enhanced Barriers'
 */

#define RTRC_RHI_DIRECTX12_BEGIN RTRC_RHI_BEGIN namespace DirectX12 {
#define RTRC_RHI_DIRECTX12_END } RTRC_RHI_END

RTRC_RHI_DIRECTX12_BEGIN

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

#define D3D12_CHECK(RESULT) ::Rtrc::RHI::DirectX12::D3D12ResultChecker{ RESULT }+[&](HRESULT result)
#define D3D12_FAIL_MSG(RESULT, MSG) do { D3D12_CHECK(RESULT) { throw Exception(MSG); }; } while(false)

RTRC_RHI_DIRECTX12_END
