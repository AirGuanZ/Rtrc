#pragma once

#include <deque>
#include <mutex>

#include <RHI/RHI.h>

RTRC_BEGIN

class DeviceSynchronizer : public Uncopyable
{
public:

    DeviceSynchronizer(RHI::DeviceOPtr device, RHI::QueuePtr queue);
    ~DeviceSynchronizer();

    const RHI::QueuePtr &GetQueue() const;

    void OnFrameComplete(std::move_only_function<void()> callback);

    void WaitIdle();
    void PrepareDestruction();

    void BeginRenderLoop(int frameCountInFlight);
    void EndRenderLoop();

    void WaitForOldFrame();
    void BeginNewFrame();
    const RHI::FenceUPtr &GetFrameFence(); // Must be signaled if BeginNewFrame is called

private:

    using Callbacks = std::vector<std::move_only_function<void()>>;

    struct RenderLoopFrame
    {
        RHI::FenceUPtr fence;
        Callbacks callbacks;
    };

    RHI::DeviceOPtr device_;
    RHI::QueuePtr queue_;

    Callbacks currentFrameCallbacks_;
    std::mutex currentFrameCallbacksMutex_;

    std::vector<RenderLoopFrame> renderLoopFrames_;
    int renderLoopFrameIndex_;

    bool isInDestruction_ = false;
};

RTRC_END