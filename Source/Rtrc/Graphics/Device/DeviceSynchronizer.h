#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

/**
 * Synchronize host & device via one specific queue.
 * Generally there should be no more than one device synchronizer associated to a device object.
 *
 * A device synchronizer can be in one of the two states: FreeMode and RenderLoopMode. In RenderLoopMode, you can use
 * `Begin/EndRenderLoop` to synchronize device work submitted several frames ago. This allows multiple frames to be
 * in-flight simultaneously. In any mode, WaitIdle will always block the caller until all gpu works are completed.
 */
class DeviceSynchronizer : public Uncopyable
{
public:

    DeviceSynchronizer(RHI::DeviceOPtr device, RHI::QueueRPtr queue);
    ~DeviceSynchronizer();

    const RHI::QueueRPtr &GetQueue() const;

    void OnFrameComplete(std::move_only_function<void()> callback);

    void WaitIdle();
    void PrepareDestruction();

    void BeginRenderLoop(int frameCountInFlight);
    void EndRenderLoop();
    bool IsInRenderLoop() const;

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
    RHI::QueueRPtr queue_;

    Callbacks currentFrameCallbacks_;
    std::mutex currentFrameCallbacksMutex_; // TODO: tls optimization

    std::vector<RenderLoopFrame> renderLoopFrames_;
    int renderLoopFrameIndex_;

    bool isInDestruction_ = false;
};

RTRC_END
