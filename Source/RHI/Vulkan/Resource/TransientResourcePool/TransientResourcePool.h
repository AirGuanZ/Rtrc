#pragma once

#include <RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanTransientResourcePool, TransientResourcePool)
{
public:

    VulkanTransientResourcePool(VulkanDevice* device, const TransientResourcePoolDesc &desc);

    int StartHostSynchronizationSession() RTRC_RHI_OVERRIDE;

    void NotifyExternalHostSynchronization(int session) RTRC_RHI_OVERRIDE;

    void Allocate(
        MutSpan<TransientResourceDeclaration> resources,
        std::vector<AliasedTransientResourcePair> &aliasRelation) RTRC_RHI_OVERRIDE;
};

RTRC_RHI_VK_END
