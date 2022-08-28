#include <Rtrc/RHI/Helper/FrameResourceManager.h>

RTRC_BEGIN

void FrameEndEventManager::DelayedRelease(RHI::Ptr<RHI::RHIObject> ptr)
{
    OnGPUFrameEnd([p = std::move(ptr)] {});
}

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

FrameSynchronizer::FrameSynchronizer(RHI::DevicePtr device, int frameCount)
    : device_(std::move(device)), frameIndex_(frameCount - 1)
#if RTRC_DEBUG
    , getFrameFenceCalled_(true)
#endif
{
    frames_.resize(frameCount);
    for(auto &f : frames_)
    {
        f.fence = device_->CreateFence(true);
    }
}

FrameSynchronizer::~FrameSynchronizer()
{
    assert(device_);
    device_->WaitIdle();
    for(auto &record : frames_)
    {
        for(auto &callback : record.completeCallbacks)
        {
            std::invoke(std::move(callback));
        }
    }
}

void FrameSynchronizer::BeginFrame()
{
    frameIndex_ = (frameIndex_ + 1) % static_cast<int>(frames_.size());
    frames_[frameIndex_].fence->Wait();
    frames_[frameIndex_].fence->Reset();
    for(auto &callback : frames_[frameIndex_].completeCallbacks)
    {
        std::invoke(std::move(callback));
    }
    frames_[frameIndex_].completeCallbacks.clear();

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
    return frames_[frameIndex_].fence;
}

void FrameSynchronizer::OnGPUFrameEnd(std::function<void()> func)
{
    frames_[frameIndex_].completeCallbacks.push_back(std::move(func));
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

void FrameResourceManager::OnGPUFrameEnd(std::function<void()> func)
{
    synchronizer_.OnGPUFrameEnd(std::move(func));
}

const RC<RG::TransientResourceManager> &FrameResourceManager::GetTransicentResourceManager() const
{
    return transientResourceManager_;
}

RTRC_END
