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
            mgr.WaitForOldFrame();
            mgr.OnCurrentFrameComplete(...);
        }
        mgr.EndRenderLoop();

    Thread Safety:

        TS(1):
            OnCurrentFrameComplete
            GetFrameFence
            GetCurrentBatchIdentifier
            GetOwnerThreadID
            IsInOwnerThread
        TS(0):
            SetOwnerThreadID
            WaitIdle
            BeginRenderLoop
            EndRenderLoop
            WaitForOldFrame
            RegisterNewBatchEvent
            UnregisterNewBatchEvent

        Callbacks specified by OnCurrentFrameComplete are executed during WaitIdle/WaitForOldFrame/Destruction
            
*/
class HostSynchronizer : public Uncopyable
{
public:

    using RegisterKey = uint32_t;

    HostSynchronizer(RHI::DevicePtr device, RHI::QueuePtr queue);
    ~HostSynchronizer();

    const RHI::QueuePtr &GetQueue() const;

    void SetOwnerThreadID(std::thread::id id);
    std::thread::id GetOwnerThreadID() const;
    bool IsInOwnerThread() const;

    // Common

    void OnCurrentFrameComplete(std::function<void()> callback);
    void WaitIdle();
    void End();

    // Render loop
    
    void BeginRenderLoop(int maxFlightFrameCount);
    void EndRenderLoop();

    void WaitForOldFrame();
    void BeginNewFrame();
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
    Box<CallbackSet> backupNonLoopCallbacks_;

    int frameIndex_;
    std::vector<Box<FrameRecord>> frameRecords_;
    std::vector<Box<CallbackSet>> backUpFrameRecords_;

    std::atomic<RegisterKey> nextRegisterKey_;
    std::map<RegisterKey, std::function<void()>> newBatchCallbacks_;
};

RTRC_END
