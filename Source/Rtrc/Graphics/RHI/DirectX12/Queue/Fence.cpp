#include <Rtrc/Graphics/RHI/DirectX12/Queue/Fence.h>

RTRC_RHI_D3D12_BEGIN

void DirectX12Fence::Wait()
{
    const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    RTRC_D3D12_FAIL_MSG(
        fence_->SetEventOnCompletion(signalValue_, eventHandle),
        "Fail to set event on completion for directx12 queue fence");
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
}

RTRC_RHI_D3D12_END
