#include <Rtrc/RHI/Vulkan/Queue.h>

RTRC_RHI_VK_BEGIN

VulkanQueue::VulkanQueue(VkQueue queue, uint32_t queueFamilyIndex)
    : queue_(queue), queueFamilyIndex_(queueFamilyIndex)
{

}

bool VulkanQueue::IsInSameFamily(const Queue &other) const
{
    return queueFamilyIndex_ == dynamic_cast<const VulkanQueue &>(other).queueFamilyIndex_;
}

RTRC_RHI_VK_END
