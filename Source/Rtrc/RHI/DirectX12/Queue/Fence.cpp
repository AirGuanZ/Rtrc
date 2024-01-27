#include <Rtrc/RHI/DirectX12/Queue/Fence.h>

RTRC_RHI_D3D12_BEGIN

void DirectX12Fence::Wait()
{
    const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    RTRC_D3D12_FAIL_MSG(
        fence_->SetEventOnCompletion(signalValue_, eventHandle),
        "Fail to set event on completion for directx12 queue fence");
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);

    if(syncSessionIDRecevier_)
    {
        auto &maxv = *syncSessionIDRecevier_;
        QueueSessionID prevValue = maxv;
        while(prevValue < syncSessionID_ && !maxv.compare_exchange_weak(prevValue, syncSessionID_))
        {

        }
        syncSessionID_ = 0;
        syncSessionIDRecevier_ = nullptr;
    }
}

RTRC_RHI_D3D12_END
