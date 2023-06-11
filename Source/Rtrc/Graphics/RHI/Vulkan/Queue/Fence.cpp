#include <Rtrc/Graphics/RHI/Vulkan/Queue/Fence.h>

RTRC_RHI_VK_BEGIN

VulkanFence::VulkanFence(VkDevice device, VkFence fence)
    : device_(device), fence_(fence)
{
    
}

VulkanFence::~VulkanFence()
{
    vkDestroyFence(device_, fence_, RTRC_VK_ALLOC);
}

void VulkanFence::Reset()
{
    RTRC_VK_FAIL_MSG(
        vkResetFences(device_, 1, &fence_),
        "Failed to reset vulkan fence");
}

void VulkanFence::Wait()
{
    vkWaitForFences(device_, 1, &fence_, true, UINT64_MAX);
}

VkFence VulkanFence::_internalGetNativeFence() const
{
    return fence_;
}

RTRC_RHI_VK_END
