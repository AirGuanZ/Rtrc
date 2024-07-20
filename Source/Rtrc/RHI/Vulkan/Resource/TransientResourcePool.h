#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanTransientResourcePool, TransientResourcePool)
{
public:
    
    void Recycle() RTRC_RHI_OVERRIDE
    {

    }

    RC<QueueSyncQuery> Allocate(
        QueueOPtr                                  queue,
        MutSpan<TransientResourceDeclaration>      resources,
        std::vector<AliasedTransientResourcePair> &aliasRelation) RTRC_RHI_OVERRIDE
    {
        throw Exception("Not implemented");
    }
};

RTRC_RHI_VK_END
