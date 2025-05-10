#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>

RTRC_BEGIN

class CommandBuffer;

class Queue
{
public:

    explicit Queue(RHI::QueueRPtr rhiQueue = nullptr);

    RHI::QueueSessionID Submit(CommandBuffer &commandBuffer);
    RHI::QueueSessionID GetCurrentSessionID() const;
    RHI::QueueSessionID GetSynchronizedSessionID() const;

    const RHI::QueueRPtr &GetRHIObject() const;

private:

    RHI::QueueRPtr rhiQueue_;
};

inline Queue::Queue(RHI::QueueRPtr rhiQueue)
    : rhiQueue_(std::move(rhiQueue))
{

}

inline RHI::QueueSessionID Queue::Submit(CommandBuffer &commandBuffer)
{
    assert(commandBuffer.submitSessionID_ == RHI::INITIAL_QUEUE_SESSION_ID);
    rhiQueue_->Submit({}, {}, commandBuffer.GetRHIObject(), {}, {}, {});
    commandBuffer.submitSessionID_ = rhiQueue_->GetCurrentSessionID();
    return commandBuffer.submitSessionID_;
}

inline RHI::QueueSessionID Queue::GetCurrentSessionID() const
{
    return rhiQueue_->GetCurrentSessionID();
}

inline RHI::QueueSessionID Queue::GetSynchronizedSessionID() const
{
    return rhiQueue_->GetSynchronizedSessionID();
}

inline const RHI::QueueRPtr &Queue::GetRHIObject() const
{
    return rhiQueue_;
}

RTRC_END
