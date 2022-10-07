#pragma once

#include <map>

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

/*
    Usage:
    
        mgr.OnCurrentFrameComplete(...);
        mgr.WaitIdle();

        mgr.BeginRenderLoop(3);
        while(true)
        {
            mgr.BeginFrame();
            mgr.OnCurrentFrameComplete(...);
        }
        mgr.EndRenderLoop();

    Thread Safety:

        TS(1):
            OnCurrentFrameComplete
            GetFrameFence
            GetCurrentBatchIdentifier
            GetRenderThreadID
            IsInRenderThread
        TS(0):
            SetRenderThreadID
            WaitIdle
            BeginRenderLoop
            EndRenderLoop
            BeginFrame
            RegisterNewBatchEvent
            UnregisterNewBatchEvent

        Callbacks specified by OnCurrentFrameComplete are executed during WaitIdle/BeginFrame/Destruction
            
*/
class HostSynchronizer : public Uncopyable
{
public:

    using RegisterKey = uint32_t;

    HostSynchronizer(RHI::DevicePtr device, RHI::QueuePtr queue);
    ~HostSynchronizer();

    const RHI::QueuePtr &GetQueue() const;

    void SetRenderThreadID(std::thread::id id);
    std::thread::id GetRenderThreadID() const;
    bool IsInRenderThread() const;

    // Common

    void OnCurrentFrameComplete(std::function<void()> callback);
    void WaitIdle();

    // Render loop
    
    void BeginRenderLoop(int maxFlightFrameCount);
    void EndRenderLoop();

    void BeginFrame();
    const RHI::FencePtr &GetFrameFence();

    // Batch info

    RegisterKey RegisterNewBatchEvent(std::function<void()> callback);
    void UnregisterNewBatchEvent(RegisterKey key);

private:

    void OnNewBatch();

    struct FrameRecord;
    class CallbackSet;

    RHI::DevicePtr device_;
    RHI::QueuePtr queue_;

    std::atomic<std::thread::id> renderThreadID_;

    Box<CallbackSet> nonLoopCallbacks_;

    int frameIndex_;
    std::vector<Box<FrameRecord>> frameRecords_;

    std::atomic<RegisterKey> nextRegisterKey_;
    std::map<RegisterKey, std::function<void()>> newBatchCallbacks_;
};

RTRC_END
