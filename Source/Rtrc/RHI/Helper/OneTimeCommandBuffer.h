#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_RHI_BEGIN

class OneTimeCommandBuffer : public Uncopyable
{
public:

    static OneTimeCommandBuffer Create(RC<Queue> queue);

    OneTimeCommandBuffer() = default;

    OneTimeCommandBuffer(OneTimeCommandBuffer &&other) noexcept;

    ~OneTimeCommandBuffer();

    OneTimeCommandBuffer &operator=(OneTimeCommandBuffer &&other) noexcept;

    void Swap(OneTimeCommandBuffer &other) noexcept;

    RC<CommandBuffer> operator->();

    void SubmitAndWait();

private:

    OneTimeCommandBuffer(RC<Queue> queue, RC<CommandPool> pool, RC<CommandBuffer> buffer);

#if defined(_DEBUG) || defined(DEBUG)
    bool submitted_ = false;
#endif
    RC<Queue> queue_;
    RC<CommandPool> pool_;
    RC<CommandBuffer> buffer_;
};

RTRC_RHI_END
