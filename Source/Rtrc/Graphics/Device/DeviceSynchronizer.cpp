#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>

RTRC_BEGIN

DeviceSynchronizer::DeviceSynchronizer(RHI::DevicePtr device, RHI::QueuePtr queue)
    : device_(std::move(device))
    , queue_(std::move(queue))
    , renderLoopFrameIndex_(0)
{
    
}

DeviceSynchronizer::~DeviceSynchronizer()
{
    queue_->WaitIdle();

    assert(renderLoopFrames_.empty() && "DeviceSynchronizer is destructed in renderloop");
    for(auto &e : currentFrameCallbacks_)
    {
        e();
    }
}

const RHI::QueuePtr &DeviceSynchronizer::GetQueue() const
{
    return queue_;
}

void DeviceSynchronizer::OnFrameComplete(std::move_only_function<void()> callback)
{
    std::lock_guard lock(currentFrameCallbacksMutex_);
    currentFrameCallbacks_.push_back(std::move(callback));
}

void DeviceSynchronizer::WaitIdle()
{
    queue_->WaitIdle();

    Callbacks callbacks;
    {
        std::lock_guard lock(currentFrameCallbacksMutex_);
        callbacks.swap(currentFrameCallbacks_);
    }
    for(auto &e : callbacks)
    {
        e();
    }

    for(auto &frame : renderLoopFrames_)
    {
        for(auto &e :frame.callbacks)
        {
            e();
        }
        frame.callbacks.clear();
    }
}

void DeviceSynchronizer::BeginRenderLoop(int frameCountInFlight)
{
    assert(
        renderLoopFrames_.empty() &&
        "DeviceSynchronizer::BeginRenderLoop cannot be called again before exit the renderloop");
    WaitIdle();

    renderLoopFrames_.resize(frameCountInFlight);
    for(auto &frame : renderLoopFrames_)
    {
        frame.fence = device_->CreateFence(true);
    }
    renderLoopFrameIndex_ = 0;
}

void DeviceSynchronizer::EndRenderLoop()
{
    WaitIdle();
    renderLoopFrames_.clear();
}

void DeviceSynchronizer::WaitForOldFrame()
{
    {
        std::lock_guard lock(currentFrameCallbacksMutex_);
        assert(renderLoopFrames_[renderLoopFrameIndex_].callbacks.empty());
        renderLoopFrames_[renderLoopFrameIndex_].callbacks.swap(currentFrameCallbacks_);
    }

    renderLoopFrameIndex_ = (renderLoopFrameIndex_ + 1) % static_cast<int>(renderLoopFrames_.size());
    auto &frame = renderLoopFrames_[renderLoopFrameIndex_];
    frame.fence->Wait();

    for(auto &e : frame.callbacks)
    {
        e();
    }
    frame.callbacks.clear();
}

void DeviceSynchronizer::BeginNewFrame()
{
    renderLoopFrames_[renderLoopFrameIndex_].fence->Reset();
}

const RHI::FencePtr &DeviceSynchronizer::GetFrameFence()
{
    return renderLoopFrames_[renderLoopFrameIndex_].fence;
}

RTRC_END