#include <RHI/Vulkan/Resource/BufferView.h>

RTRC_RHI_VK_BEGIN

VulkanBufferSrv::VulkanBufferSrv(const VulkanBuffer *buffer, const BufferSrvDesc &desc, VkBufferView view)
    : buffer_(buffer), desc_(desc), view_(view)
{

}

const BufferSrvDesc &VulkanBufferSrv::GetDesc() const
{
    return desc_;
}

const VulkanBuffer *VulkanBufferSrv::_internalGetBuffer() const
{
    return buffer_;
}

VkBufferView VulkanBufferSrv::_internalGetNativeView() const
{
    assert(view_);
    return view_;
}

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
