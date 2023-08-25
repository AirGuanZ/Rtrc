#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>
#include <Rtrc/Core/Enumerate.h>

RTRC_BEGIN

DeviceSynchronizer::DeviceSynchronizer(RHI::DevicePtr device, RHI::QueuePtr queue)
    : device_(std::move(device))
    , queue_(std::move(queue))
    , renderLoopFrameIndex_(0)
{
    
}

DeviceSynchronizer::~DeviceSynchronizer()
{
    device_->WaitIdle();
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

    if(renderLoopFrames_.empty())
    {
        Callbacks callbacks;
        {
            std::lock_guard lock(currentFrameCallbacksMutex_);
            callbacks.swap(currentFrameCallbacks_);
        }
        for(auto &e : callbacks)
        {
            e();
        }
    }
}

void DeviceSynchronizer::PrepareDestruction()
{
    if(!renderLoopFrames_.empty())
    {
        if(!std::uncaught_exceptions())
        {
            LogError("DeviceSynchronizer is destructed in renderloop");
        }
        EndRenderLoop();
    }
    else
    {
        WaitIdle();
    }
}

void DeviceSynchronizer::BeginRenderLoop(int frameCountInFlight)
{
    assert(
        renderLoopFrames_.empty() &&
        "DeviceSynchronizer::BeginRenderLoop cannot be called again before exit the renderloop");
    WaitIdle();

    renderLoopFrames_.resize(frameCountInFlight);
    for(auto &&[i, frame] : Enumerate(renderLoopFrames_))
    {
        frame.fence = device_->CreateFence(true);
        frame.fence->SetName(fmt::format("DeviceSynchronizer.Fence{}", i));
    }
    renderLoopFrameIndex_ = 0;
}

void DeviceSynchronizer::EndRenderLoop()
{
    assert(!renderLoopFrames_.empty());
    queue_->WaitIdle();
    {
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
            for(auto &e : frame.callbacks)
            {
                e();
            }
            frame.callbacks.clear();
        }
    }
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
