#include <Rtrc/Graphics/Resource/Frame/FrameSynchronizer.h>

RTRC_BEGIN

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

RTRC_END
