#include <Rtrc/Graphics/RHI/Vulkan/Context/BackBufferSemaphore.h>

RTRC_RHI_VK_BEGIN

VulkanBackBufferSemaphore::VulkanBackBufferSemaphore(VkDevice device, VkSemaphore binarySemaphore)
    : device_(device), binarySemaphore_(binarySemaphore)
{
    
}

VulkanBackBufferSemaphore::~VulkanBackBufferSemaphore()
{
    assert(binarySemaphore_);
    vkDestroySemaphore(device_, binarySemaphore_, VK_ALLOC);
}

VkSemaphore VulkanBackBufferSemaphore::_internalGetBinarySemaphore() const
{
    return binarySemaphore_;
}

RTRC_RHI_VK_END
