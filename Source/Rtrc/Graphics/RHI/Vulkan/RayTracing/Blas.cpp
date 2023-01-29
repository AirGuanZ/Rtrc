#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/Blas.h>

RTRC_RHI_VK_BEGIN

VulkanBlas::VulkanBlas(VulkanDevice *device, VkAccelerationStructureKHR blas, BufferPtr buffer)
    : device_(device), blas_(blas), buffer_(std::move(buffer))
{
    
}

VulkanBlas::~VulkanBlas()
{
    assert(device_ && buffer_);
    vkDestroyAccelerationStructureKHR(device_->_internalGetNativeDevice(), blas_, RTRC_VK_ALLOC);
}

VkAccelerationStructureKHR VulkanBlas::_internalGetNativeBlas() const
{
    return blas_;
}

RTRC_RHI_VK_END
