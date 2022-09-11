#pragma once

#include <Rtrc/Graphics/Resource/Interface/CommandBufferAllocator.h>

RTRC_BEGIN

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

RTRC_END
