#include <Rtrc/RHI/Vulkan/Resource/TransientResourcePool/TransientResourcePool.h>

RTRC_RHI_VK_BEGIN

VulkanTransientResourcePool::VulkanTransientResourcePool(VulkanDevice *device, const TransientResourcePoolDesc &desc)
{
    throw Exception("TODO: VulkanTransientResourcePool is not implemented");
}

int VulkanTransientResourcePool::StartHostSynchronizationSession()
{
    return 0;
}

void VulkanTransientResourcePool::NotifyExternalHostSynchronization(int session)
{

}

void VulkanTransientResourcePool::Allocate(
    MutSpan<TransientResourceDeclaration> resources,
    std::vector<AliasedTransientResourcePair> &aliasRelation)
{

}

RTRC_RHI_VK_END
