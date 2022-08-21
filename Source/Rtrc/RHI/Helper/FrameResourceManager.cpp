#include <Rtrc/RHI/Helper/FrameResourceManager.h>

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
        pool = device_->GetQueue(type)->CreateCommandPool();
    }
    return pool->NewCommandBuffer();
}

FrameSynchronizer::FrameSynchronizer(const RHI::DevicePtr &device, int frameCount)
    : frameIndex_(frameCount - 1)
#if RTRC_DEBUG
    , getFrameFenceCalled_(true)
#endif
{
    fences_.resize(frameCount);
    for(auto &f : fences_)
    {
        f = device->CreateFence(true);
    }
}

void FrameSynchronizer::BeginFrame()
{
    frameIndex_ = (frameIndex_ + 1) % static_cast<int>(fences_.size());
    fences_[frameIndex_]->Wait();
    fences_[frameIndex_]->Reset();
#if RTRC_DEBUG
    assert(getFrameFenceCalled_ && "FrameSynchronizer::GetFrameFence haven't been called at last frame");
    getFrameFenceCalled_ = false;
#endif
}

int FrameSynchronizer::GetFrameIndex() const
{
#if RTRC_DEBUG
    getFrameFenceCalled_ = true;
#endif
    return frameIndex_;
}

RHI::FencePtr FrameSynchronizer::GetFrameFence() const
{
    return fences_[frameIndex_];
}

FrameResourceManager::FrameResourceManager(RHI::DevicePtr device, int frameCount)
    : synchronizer_(device, frameCount), commandBufferManager_(device, frameCount)
{
    transientResourceManager_ = MakeRC<RG::TransientResourceManager>(
        device, RG::TransientResourceManager::Strategy::ReuseAcrossFrame, frameCount);
}

void FrameResourceManager::BeginFrame()
{
    synchronizer_.BeginFrame();
    commandBufferManager_.BeginFrame(synchronizer_.GetFrameIndex());
    transientResourceManager_->BeginFrame();
}

int FrameResourceManager::GetFrameIndex() const
{
    return synchronizer_.GetFrameIndex();
}

RHI::FencePtr FrameResourceManager::GetFrameFence() const
{
    return synchronizer_.GetFrameFence();
}

RHI::CommandBufferPtr FrameResourceManager::AllocateCommandBuffer(RHI::QueueType type)
{
    return commandBufferManager_.AllocateCommandBuffer(type);
}

const RC<RG::TransientResourceManager> &FrameResourceManager::GetTransicentResourceManager() const
{
    return transientResourceManager_;
}

RTRC_END
