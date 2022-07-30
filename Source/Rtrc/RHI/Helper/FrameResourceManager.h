#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

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

class FrameSynchronizer : public Uncopyable
{
public:

    FrameSynchronizer(const RHI::DevicePtr &device, int frameCount);

    void BeginFrame();

    int GetFrameIndex() const;

    // call queue->submit(frameFence) at the end of each frame
    RHI::FencePtr GetFrameFence() const;

private:

    int frameIndex_;
    std::vector<RHI::FencePtr> fences_;

#if RTRC_DEBUG
    mutable bool getFrameFenceCalled_;
#endif
};

class FrameResourceManager : public Uncopyable, public CommandBufferAllocator
{
public:

    FrameResourceManager(RHI::DevicePtr device, int frameCount);

    void BeginFrame();

    int GetFrameIndex() const;

    RHI::FencePtr GetFrameFence() const;

    RHI::CommandBufferPtr AllocateCommandBuffer(RHI::QueueType type) override;

private:

    FrameSynchronizer         synchronizer_;
    FrameCommandBufferManager commandBufferManager_;
};

RTRC_END
