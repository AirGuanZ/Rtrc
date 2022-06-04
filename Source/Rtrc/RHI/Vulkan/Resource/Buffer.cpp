#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>

RTRC_RHI_VK_BEGIN

VulkanBuffer::VulkanBuffer(
    const BufferDesc      &desc,
    VkDevice               device,
    VkBuffer               buffer,
    VulkanMemoryAllocation alloc,
    ResourceOwnership      ownership)
    : desc_(desc), device_(device), buffer_(buffer), alloc_(alloc), ownership_(ownership)
{
    
}

VulkanBuffer::~VulkanBuffer()
{
    if(ownership_ == ResourceOwnership::Allocation)
    {
        vmaDestroyBuffer(alloc_.allocator, buffer_, alloc_.allocation);
    }
    else if(ownership_ == ResourceOwnership::Image)
    {
        vkDestroyBuffer(device_, buffer_, VK_ALLOC);
    }
}

VkBuffer VulkanBuffer::GetNativeBuffer() const
{
    return buffer_;
}

RTRC_RHI_VK_END
