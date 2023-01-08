#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferUav.h>

RTRC_RHI_VK_BEGIN

VulkanBufferUav::VulkanBufferUav(const VulkanBuffer *buffer, const BufferUavDesc &desc, VkBufferView view)
    : buffer_(buffer), desc_(desc), view_(view)
{
    
}

const BufferUavDesc &VulkanBufferUav::GetDesc() const
{
    return desc_;
}

const VulkanBuffer *VulkanBufferUav::GetBuffer() const
{
    return buffer_;
}

VkBufferView VulkanBufferUav::_internalGetNativeView() const
{
    assert(view_);
    return view_;
}

RTRC_RHI_VK_END
