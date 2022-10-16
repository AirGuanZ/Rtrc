#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>

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

RTRC_END
