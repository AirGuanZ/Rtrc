#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferSRV.h>

RTRC_RHI_VK_BEGIN

VulkanBufferSRV::VulkanBufferSRV(const VulkanBuffer *buffer, const BufferSRVDesc &desc, VkBufferView view)
    : buffer_(buffer), desc_(desc), view_(view)
{
    
}

const BufferSRVDesc &VulkanBufferSRV::GetDesc() const
{
    return desc_;
}

const VulkanBuffer *VulkanBufferSRV::GetBuffer() const
{
    return buffer_;
}

VkBufferView VulkanBufferSRV::GetNativeView() const
{
    assert(view_);
    return view_;
}

RTRC_RHI_VK_END
