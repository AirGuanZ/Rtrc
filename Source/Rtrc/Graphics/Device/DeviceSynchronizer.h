#pragma once

#include <deque>
#include <mutex>

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

class DeviceSynchronizer : public Uncopyable
{
public:

    DeviceSynchronizer(RHI::DevicePtr device, RHI::QueuePtr queue);
    ~DeviceSynchronizer();

    const RHI::QueuePtr &GetQueue() const;

    void OnFrameComplete(std::move_only_function<void()> callback);

    void WaitIdle();

    void BeginRenderLoop(int frameCountInFlight);
    void EndRenderLoop();

    void WaitForOldFrame();
    void BeginNewFrame();
    const RHI::FencePtr &GetFrameFence(); // Must be signaled if BeginNewFrame is called

private:

    using Callbacks = std::vector<std::move_only_function<void()>>;

    struct RenderLoopFrame
    {
        RHI::FencePtr fence;
        Callbacks callbacks;
    };

    RHI::DevicePtr device_;
    RHI::QueuePtr queue_;

    Callbacks currentFrameCallbacks_;
    std::mutex currentFrameCallbacksMutex_;

    std::vector<RenderLoopFrame> renderLoopFrames_;
    int renderLoopFrameIndex_;
};

RTRC_END
