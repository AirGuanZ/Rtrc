#include <Rtrc/Graphics/Resource/StandaloneCommandBuffer.h>

RTRC_BEGIN

using namespace RHI;

StandaloneCommandBuffer StandaloneCommandBuffer::Create(const DevicePtr &device, Ptr<Queue> queue)
{
    auto pool = device->CreateCommandPool(queue);
    auto commandBuffer = pool->NewCommandBuffer();
    return StandaloneCommandBuffer(std::move(queue), std::move(pool), std::move(commandBuffer));
}

StandaloneCommandBuffer::StandaloneCommandBuffer(StandaloneCommandBuffer &&other) noexcept
    : StandaloneCommandBuffer()
{
    Swap(other);
}

StandaloneCommandBuffer::~StandaloneCommandBuffer()
{
#if RTRC_DEBUG
    assert((submitted_ || !buffer_) && "unsubmitted one-time command buffer");
#endif
}

StandaloneCommandBuffer &StandaloneCommandBuffer::operator=(StandaloneCommandBuffer &&other) noexcept
{
    Swap(other);
    return *this;
}

void StandaloneCommandBuffer::Swap(StandaloneCommandBuffer &other) noexcept
{
    queue_.Swap(other.queue_);
    pool_.Swap(other.pool_);
    buffer_.Swap(other.buffer_);
#if RTRC_DEBUG
    std::swap(submitted_, other.submitted_);
#endif
}

Ptr<CommandBuffer> StandaloneCommandBuffer::operator->()
{
    return buffer_;
}

void StandaloneCommandBuffer::SubmitAndWait()
{
#if RTRC_DEBUG
    submitted_ = true;
#endif
    queue_->Submit({}, {}, buffer_, {}, {}, {});
    queue_->WaitIdle();
}

StandaloneCommandBuffer::StandaloneCommandBuffer(Ptr<Queue> queue, Ptr<CommandPool> pool, Ptr<CommandBuffer> buffer)
    : queue_(std::move(queue)), pool_(std::move(pool)), buffer_(std::move(buffer))
{
    
}

RTRC_END
