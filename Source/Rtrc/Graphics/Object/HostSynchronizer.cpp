#include <tbb/enumerable_thread_specific.h>

#include <Rtrc/Graphics/Object/HostSynchronizer.h>

RTRC_BEGIN

class HostSynchronizer::CallbackSet
{
public:

    void Add(std::function<void()> callback)
    {
        callbacks_.local().push_back(std::move(callback));
    }

    void ExecuteAndClear()
    {
        for(auto &tls : callbacks_)
        {
            for(auto &c : tls)
                c();
            tls.clear();
        }
    }

private:

    tbb::enumerable_thread_specific<std::vector<std::function<void()>>> callbacks_;
};

struct HostSynchronizer::FrameRecord
{
    RHI::FencePtr fence;
    CallbackSet callbacks;
};

HostSynchronizer::HostSynchronizer(RHI::DevicePtr device, RHI::QueuePtr queue)
    : device_(std::move(device)), queue_(std::move(queue)), frameIndex_(0)
{
    nonLoopCallbacks_ = MakeBox<CallbackSet>();
}

HostSynchronizer::~HostSynchronizer()
{
    WaitIdle();
}

void HostSynchronizer::OnCurrentFrameComplete(std::function<void()> callback)
{
    if(frameRecords_.empty())
    {
        nonLoopCallbacks_->Add(std::move(callback));
    }
    else
    {
        frameRecords_[frameIndex_]->callbacks.Add(std::move(callback));
    }
}

void HostSynchronizer::WaitIdle()
{
    queue_->WaitIdle();
    nonLoopCallbacks_->ExecuteAndClear();
    for(auto &f : frameRecords_)
    {
        f->callbacks.ExecuteAndClear();
    }
}

void HostSynchronizer::BeginRenderLoop(int maxFlightFrameCount)
{
    queue_->WaitIdle();
    nonLoopCallbacks_->ExecuteAndClear();

    assert(frameRecords_.empty());
    frameRecords_.resize(maxFlightFrameCount);
    for(auto &f : frameRecords_)
    {
        f = MakeBox<FrameRecord>();
        f->fence = device_->CreateFence(true);
    }
}

void HostSynchronizer::EndRenderLoop()
{
    assert(!frameRecords_.empty());
    for(auto &f : frameRecords_)
    {
        f->fence->Wait();
        f->callbacks.ExecuteAndClear();
    }
    frameRecords_.clear();
}

void HostSynchronizer::BeginFrame()
{
    frameIndex_ = (frameIndex_ + 1) % static_cast<int>(frameRecords_.size());
    auto &frame = frameRecords_[frameIndex_];
    frame->fence->Wait();
    frame->callbacks.ExecuteAndClear();
}

const RHI::FencePtr &HostSynchronizer::GetFrameFence()
{
    return frameRecords_[frameIndex_]->fence;
}

RTRC_END
