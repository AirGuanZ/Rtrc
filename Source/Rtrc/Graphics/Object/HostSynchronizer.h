#pragma once

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
        TS(0):
            WaitIdle
            BeginRenderLoop
            EndRenderLoop
            BeginFrame
            
*/
class HostSynchronizer : public Uncopyable
{
public:

    HostSynchronizer(RHI::DevicePtr device, RHI::QueuePtr queue);
    ~HostSynchronizer();

    // Common

    void OnCurrentFrameComplete(std::function<void()> callback);
    void WaitIdle();

    // Render loop
    
    void BeginRenderLoop(int maxFlightFrameCount);
    void EndRenderLoop();

    void BeginFrame();
    const RHI::FencePtr &GetFrameFence();

private:

    struct FrameRecord;
    class CallbackSet;

    RHI::DevicePtr device_;
    RHI::QueuePtr queue_;

    Box<CallbackSet> nonLoopCallbacks_;

    int frameIndex_;
    std::vector<Box<FrameRecord>> frameRecords_;
};

RTRC_END
