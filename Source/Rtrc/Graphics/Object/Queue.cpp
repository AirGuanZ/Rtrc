#include <Rtrc/Graphics/Object/CommandBuffer.h>
#include <Rtrc/Graphics/Object/Queue.h>

RTRC_BEGIN

Queue::Queue(RHI::QueuePtr rhiQueue)
    : rhiQueue_(std::move(rhiQueue))
{
    
}

void Queue::Submit(const CommandBuffer &commandBuffer)
{
    rhiQueue_->Submit({}, {}, commandBuffer.GetRHIObject(), {}, {}, {});
}

const RHI::QueuePtr &Queue::GetRHIObject() const
{
    return rhiQueue_;
}

RTRC_END
