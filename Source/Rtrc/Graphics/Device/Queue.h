#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>

RTRC_BEGIN

class CommandBuffer;

class Queue
{
public:

    explicit Queue(RHI::QueueRPtr rhiQueue = nullptr);

    void Submit(const CommandBuffer &commandBuffer);

    const RHI::QueueRPtr &GetRHIObject() const;

private:

    RHI::QueueRPtr rhiQueue_;
};

inline Queue::Queue(RHI::QueueRPtr rhiQueue)
    : rhiQueue_(std::move(rhiQueue))
{

}

inline void Queue::Submit(const CommandBuffer &commandBuffer)
{
    rhiQueue_->Submit({}, {}, commandBuffer.GetRHIObject(), {}, {}, {});
}

inline const RHI::QueueRPtr &Queue::GetRHIObject() const
{
    return rhiQueue_;
}

RTRC_END
