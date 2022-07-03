#include <Rtrc/RHI/Vulkan/Resource/BufferUAV.h>

RTRC_RHI_VK_BEGIN

VulkanBufferUAV::VulkanBufferUAV(const VulkanBuffer *buffer, const BufferUAVDesc &desc, VkBufferView view)
    : buffer_(buffer), desc_(desc), view_(view)
{
    
}

const BufferUAVDesc &VulkanBufferUAV::GetDesc() const
{
    return desc_;
}

const VulkanBuffer *VulkanBufferUAV::GetBuffer() const
{
    return buffer_;
}

VkBufferView VulkanBufferUAV::GetNativeView() const
{
    assert(view_);
    return view_;
}

RTRC_RHI_VK_END
