#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

class StandaloneCommandBuffer : public Uncopyable
{
public:

    static StandaloneCommandBuffer Create(const RHI::DevicePtr &device, RHI::Ptr<RHI::Queue> queue);

    StandaloneCommandBuffer() = default;

    StandaloneCommandBuffer(StandaloneCommandBuffer &&other) noexcept;

    ~StandaloneCommandBuffer();

    StandaloneCommandBuffer &operator=(StandaloneCommandBuffer &&other) noexcept;

    void Swap(StandaloneCommandBuffer &other) noexcept;

    RHI::Ptr<RHI::CommandBuffer> operator->();

    void SubmitAndWait();

private:

    StandaloneCommandBuffer(
        RHI::Ptr<RHI::Queue> queue, RHI::Ptr<RHI::CommandPool> pool, RHI::Ptr<RHI::CommandBuffer> buffer);

#if RTRC_DEBUG
    bool submitted_ = false;
#endif
    RHI::Ptr<RHI::Queue> queue_;
    RHI::Ptr<RHI::CommandPool> pool_;
    RHI::Ptr<RHI::CommandBuffer> buffer_;
};

RTRC_END
