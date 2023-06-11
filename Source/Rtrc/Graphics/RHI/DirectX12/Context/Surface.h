#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Surface, Surface)
{
public:

    explicit DirectX12Surface(HWND window) : window_(window) { } 

    HWND _internalGetWindowHandle() const { return window_; }

private:

    HWND window_;
};

RTRC_RHI_D3D12_END
