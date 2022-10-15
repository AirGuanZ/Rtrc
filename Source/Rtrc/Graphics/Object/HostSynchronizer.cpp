#include <ranges>

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
    Box<CallbackSet> callbacks;
};

HostSynchronizer::HostSynchronizer(RHI::DevicePtr device, RHI::QueuePtr queue)
    : device_(std::move(device)), queue_(std::move(queue)), frameIndex_(0), nextRegisterKey_(0)
{
    nonLoopCallbacks_ = MakeBox<CallbackSet>();
    backupNonLoopCallbacks_ = MakeBox<CallbackSet>();
}

HostSynchronizer::~HostSynchronizer()
{
    assert(frameRecords_.empty());
}

const RHI::QueuePtr &HostSynchronizer::GetQueue() const
{
    return queue_;
}

void HostSynchronizer::SetOwnerThreadID(std::thread::id id)
{
    renderThreadID_ = id;
}

std::thread::id HostSynchronizer::GetOwnerThreadID() const
{
    return renderThreadID_;
}

bool HostSynchronizer::IsInOwnerThread() const
{
    return std::this_thread::get_id() == GetOwnerThreadID();
}

void HostSynchronizer::OnCurrentFrameComplete(std::function<void()> callback)
{
    if(frameRecords_.empty())
    {
        nonLoopCallbacks_->Add(std::move(callback));
    }
    else
    {
        frameRecords_[frameIndex_]->callbacks->Add(std::move(callback));
    }
}

void HostSynchronizer::WaitIdle()
{
    queue_->WaitIdle();

    backupNonLoopCallbacks_.swap(nonLoopCallbacks_);
    for(size_t i = 0; i < frameRecords_.size(); ++i)
    {
        backUpFrameRecords_[i].swap(frameRecords_[i]->callbacks);
    }

    OnNewBatch();

    backupNonLoopCallbacks_->ExecuteAndClear();
    for(auto &f : backUpFrameRecords_)
    {
        f->ExecuteAndClear();
    }
}

void HostSynchronizer::End()
{
    queue_->WaitIdle();
    nonLoopCallbacks_->ExecuteAndClear();
    for(auto &f : frameRecords_)
    {
        f->callbacks->ExecuteAndClear();
    }
}

void HostSynchronizer::BeginRenderLoop(int maxFlightFrameCount)
{
    assert(frameRecords_.empty());
    WaitIdle();
    frameRecords_.resize(maxFlightFrameCount);
    for(auto &f : frameRecords_)
    {
        f = MakeBox<FrameRecord>();
        f->fence = device_->CreateFence(true);
        f->callbacks = MakeBox<CallbackSet>();
    }
    backUpFrameRecords_.resize(maxFlightFrameCount);
    for(auto &f : backUpFrameRecords_)
        f = MakeBox<CallbackSet>();
}

void HostSynchronizer::EndRenderLoop()
{
    assert(!frameRecords_.empty());
    WaitIdle();
    frameRecords_.clear();
    backUpFrameRecords_.clear();
}

void HostSynchronizer::WaitForOldFrame()
{
    frameIndex_ = (frameIndex_ + 1) % static_cast<int>(frameRecords_.size());
    auto &frame = frameRecords_[frameIndex_];
    frame->fence->Wait();

    backUpFrameRecords_[frameIndex_].swap(frame->callbacks);
    OnNewBatch();
    backUpFrameRecords_[frameIndex_]->ExecuteAndClear();
}

void HostSynchronizer::BeginNewFrame()
{
    auto &frame = frameRecords_[frameIndex_];
    frame->fence->Reset();
}

const RHI::FencePtr &HostSynchronizer::GetFrameFence()
{
    return frameRecords_[frameIndex_]->fence;
}

HostSynchronizer::RegisterKey HostSynchronizer::RegisterNewBatchEvent(std::function<void()> callback)
{
    const RegisterKey key = ++nextRegisterKey_;
    newBatchCallbacks_.insert({ key, std::move(callback) });
    return key;
}

void HostSynchronizer::UnregisterNewBatchEvent(RegisterKey key)
{
    newBatchCallbacks_.erase(key);
}

void HostSynchronizer::OnNewBatch()
{
    for(auto &c : std::ranges::views::values(newBatchCallbacks_))
    {
        c();
    }
}

RTRC_END
