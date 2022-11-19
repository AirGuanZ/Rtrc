#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>

RTRC_BEGIN

class CommandBuffer;

class Queue
{
public:

    explicit Queue(RHI::QueuePtr rhiQueue = nullptr);

    void Submit(const CommandBuffer &commandBuffer);

    const RHI::QueuePtr &GetRHIObject() const;

private:

    RHI::QueuePtr rhiQueue_;
};

inline Queue::Queue(RHI::QueuePtr rhiQueue)
    : rhiQueue_(std::move(rhiQueue))
{

}

inline void Queue::Submit(const CommandBuffer &commandBuffer)
{
    rhiQueue_->Submit({}, {}, commandBuffer.GetRHIObject(), {}, {}, {});
}

inline const RHI::QueuePtr &Queue::GetRHIObject() const
{
    return rhiQueue_;
}

RTRC_END
