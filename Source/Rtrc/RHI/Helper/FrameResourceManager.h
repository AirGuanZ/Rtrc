#pragma once

#include <Rtrc/RenderGraph/TransientResourceManager.h>

RTRC_BEGIN

class FrameEndEventManager
{
public:

    virtual ~FrameEndEventManager() = default;

    virtual void OnGPUFrameEnd(std::function<void()> func) = 0;
};

class CommandBufferAllocator
{
public:

    virtual ~CommandBufferAllocator() = default;

    virtual RHI::CommandBufferPtr AllocateCommandBuffer(RHI::QueueType type) = 0;
};

class FrameCommandBufferManager : public CommandBufferAllocator, public Uncopyable
{
public:

    FrameCommandBufferManager(RHI::DevicePtr device, int frameCount);

    // call BeginFrame(i) after frame i is synchronized with host
    void BeginFrame(int index);

    RHI::CommandBufferPtr AllocateCommandBuffer(RHI::QueueType type) override;

private:

    RHI::DevicePtr device_;
    int frameIndex_;
    std::vector<std::vector<RHI::CommandPoolPtr>> pools_; // usage: pools_[frame][queue]
};

class FrameSynchronizer : public Uncopyable, public FrameEndEventManager
{
public:

    FrameSynchronizer(RHI::DevicePtr device, int frameCount);

    ~FrameSynchronizer() override;

    void BeginFrame();

    int GetFrameIndex() const;

    // call queue->submit(frameFence) at the end of each frame
    RHI::FencePtr GetFrameFence() const;

    void OnGPUFrameEnd(std::function<void()> func) override;

private:

    struct FrameRecord
    {
        RHI::FencePtr fence;
        std::vector<std::function<void()>> completeCallbacks;
    };

    RHI::DevicePtr device_;
    int frameIndex_;
    std::vector<FrameRecord> frames_;

#if RTRC_DEBUG
    mutable bool getFrameFenceCalled_;
#endif
};

class FrameResourceManager : public Uncopyable, public CommandBufferAllocator, public FrameEndEventManager
{
public:

    FrameResourceManager(RHI::DevicePtr device, int frameCount);

    void BeginFrame();

    int GetFrameIndex() const;

    RHI::FencePtr GetFrameFence() const;

    RHI::CommandBufferPtr AllocateCommandBuffer(RHI::QueueType type) override;

    void OnGPUFrameEnd(std::function<void()> func) override;

    const RC<RG::TransientResourceManager> &GetTransicentResourceManager() const;

private:

    FrameSynchronizer                synchronizer_;
    FrameCommandBufferManager        commandBufferManager_;
    RC<RG::TransientResourceManager> transientResourceManager_;
};

RTRC_END
