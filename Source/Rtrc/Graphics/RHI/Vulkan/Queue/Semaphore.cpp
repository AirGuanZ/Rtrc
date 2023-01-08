#include <Rtrc/Graphics/RHI/Vulkan/Queue/Semaphore.h>

RTRC_RHI_VK_BEGIN

VulkanSemaphore::VulkanSemaphore(VkDevice device, VkSemaphore semaphore)
    : device_(device), semaphore_(semaphore)
{
    
}

VulkanSemaphore::~VulkanSemaphore()
{
    assert(device_ && semaphore_);
    vkDestroySemaphore(device_, semaphore_, VK_ALLOC);
}

uint64_t VulkanSemaphore::GetValue() const
{
    uint64_t value;
    VK_FAIL_MSG(
        vkGetSemaphoreCounterValue(device_, semaphore_, &value),
        "failed to get vulkan semaphore value");
    return value;
}

VkSemaphore VulkanSemaphore::_internalGetNativeSemaphore() const
{
    return semaphore_;
}

RTRC_RHI_VK_END
