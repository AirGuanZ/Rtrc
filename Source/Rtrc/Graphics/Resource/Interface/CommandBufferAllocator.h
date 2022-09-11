#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

class CommandBufferAllocator
{
public:

    virtual ~CommandBufferAllocator() = default;

    virtual RHI::CommandBufferPtr AllocateCommandBuffer(RHI::QueueType type) = 0;
};

RTRC_END
