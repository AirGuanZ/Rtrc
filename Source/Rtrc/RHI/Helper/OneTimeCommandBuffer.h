#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_RHI_BEGIN

class OneTimeCommandBuffer : public Uncopyable
{
public:

    static OneTimeCommandBuffer Create(Ptr<Queue> queue);

    OneTimeCommandBuffer() = default;

    OneTimeCommandBuffer(OneTimeCommandBuffer &&other) noexcept;

    ~OneTimeCommandBuffer();

    OneTimeCommandBuffer &operator=(OneTimeCommandBuffer &&other) noexcept;

    void Swap(OneTimeCommandBuffer &other) noexcept;

    Ptr<CommandBuffer> operator->();

    void SubmitAndWait();

private:

    OneTimeCommandBuffer(Ptr<Queue> queue, Ptr<CommandPool> pool, Ptr<CommandBuffer> buffer);

#if RTRC_DEBUG
    bool submitted_ = false;
#endif
    Ptr<Queue> queue_;
    Ptr<CommandPool> pool_;
    Ptr<CommandBuffer> buffer_;
};

RTRC_RHI_END
