#include <Rtrc/RHI/Helper/OneTimeCommandBuffer.h>

RTRC_RHI_BEGIN

OneTimeCommandBuffer OneTimeCommandBuffer::Create(RC<Queue> queue)
{
    auto pool = queue->CreateCommandPool();
    auto commandBuffer = pool->NewCommandBuffer();
    return OneTimeCommandBuffer(std::move(queue), std::move(pool), std::move(commandBuffer));
}

OneTimeCommandBuffer::OneTimeCommandBuffer(OneTimeCommandBuffer &&other) noexcept
    : OneTimeCommandBuffer()
{
    Swap(other);
}

OneTimeCommandBuffer::~OneTimeCommandBuffer()
{
#if RTRC_DEBUG
    assert((submitted_ || !buffer_) && "unsubmitted one-time command buffer");
#endif
}

OneTimeCommandBuffer &OneTimeCommandBuffer::operator=(OneTimeCommandBuffer &&other) noexcept
{
    Swap(other);
    return *this;
}

void OneTimeCommandBuffer::Swap(OneTimeCommandBuffer &other) noexcept
{
    queue_.swap(other.queue_);
    pool_.swap(other.pool_);
    buffer_.swap(other.buffer_);
#if RTRC_DEBUG
    std::swap(submitted_, other.submitted_);
#endif
}

RC<CommandBuffer> OneTimeCommandBuffer::operator->()
{
    return buffer_;
}

void OneTimeCommandBuffer::SubmitAndWait()
{
#if RTRC_DEBUG
    submitted_ = true;
#endif
    queue_->Submit({}, {}, buffer_, {}, {}, {});
    queue_->WaitIdle();
}

OneTimeCommandBuffer::OneTimeCommandBuffer(RC<Queue> queue, RC<CommandPool> pool, RC<CommandBuffer> buffer)
    : queue_(std::move(queue)), pool_(std::move(pool)), buffer_(std::move(buffer))
{
    
}

RTRC_RHI_END
