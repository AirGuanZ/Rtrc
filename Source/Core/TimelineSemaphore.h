#pragma once

#include <cassert>
#include <mutex>

#include <Core/Common.h>

RTRC_BEGIN

class TimelineSemaphore;

struct TimelineSemaphoreSharedData
{
    std::mutex mutex;
    std::condition_variable cond;
    uint64_t signaledValue = 0;
};

class TimelineSemaphoreWaiter
{
public:

    TimelineSemaphoreWaiter() = default;

    bool IsSignaled() const
    {
        if(!waitValue_)
        {
            return true;
        }
        assert(shared_);
        std::lock_guard lock(shared_->mutex);
        return shared_->signaledValue >= waitValue_;
    }

    void Wait() const
    {
        if(waitValue_)
        {
            assert(shared_);
            std::unique_lock lock(shared_->mutex);
            shared_->cond.wait(lock, [&] { return shared_->signaledValue >= waitValue_; });
        }
    }

private:

    friend class TimelineSemaphore;

    TimelineSemaphoreWaiter(RC<TimelineSemaphoreSharedData> shared, uint64_t waitValue)
        : shared_(std::move(shared)), waitValue_(waitValue)
    {
        
    }

    RC<TimelineSemaphoreSharedData> shared_;
    uint64_t waitValue_ = 0;
};

class TimelineSemaphore
{
public:

    void Signal()
    {
        {
            std::lock_guard lock(shared_->mutex);
            ++shared_->signaledValue;
        }
        shared_->cond.notify_all();
    }

    // Not thread-safe.
    // Make sure 'CreateWaiter' can not be called when another thread is calling 'Signal'
    TimelineSemaphoreWaiter CreateWaiter() const
    {
        return TimelineSemaphoreWaiter(shared_, shared_->signaledValue + 1);
    }

private:

    RC<TimelineSemaphoreSharedData> shared_ = MakeRC<TimelineSemaphoreSharedData>();
};

RTRC_END
