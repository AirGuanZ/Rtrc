#include <Rtrc/Graphics/Resource/Frame/FrameCommandBufferManager.h>

RTRC_BEGIN

FrameCommandBufferManager::FrameCommandBufferManager(RHI::DevicePtr device, int frameCount)
    : device_(std::move(device)), frameIndex_(0)
{
    pools_.resize(frameCount, std::vector<RHI::CommandPoolPtr>(EnumValueCount<RHI::QueueType>));
}

void FrameCommandBufferManager::BeginFrame(int index)
{
    assert(0 <= index && index < static_cast<int>(pools_.size()));
    frameIndex_ = index;
    for(auto &pool : pools_[frameIndex_])
    {
        if(pool)
        {
            pool->Reset();
        }
    }
}

RHI::CommandBufferPtr FrameCommandBufferManager::AllocateCommandBuffer(RHI::QueueType type)
{
    auto &pool = pools_[frameIndex_][static_cast<int>(type)];
    if(!pool)
    {
        pool = device_->CreateCommandPool(device_->GetQueue(type));
    }
    return pool->NewCommandBuffer();
}

RTRC_END
