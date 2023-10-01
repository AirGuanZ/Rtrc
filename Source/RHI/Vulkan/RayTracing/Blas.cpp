#include <RHI/Vulkan/Context/Device.h>
#include <RHI/Vulkan/RayTracing/Blas.h>

RTRC_RHI_VK_BEGIN

VulkanBlas::VulkanBlas(VulkanDevice *device, VkAccelerationStructureKHR blas, BufferPtr buffer)
    : device_(device), blas_(blas), deviceAddress_{}, buffer_(std::move(buffer))
{
    const VkAccelerationStructureDeviceAddressInfoKHR info =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = blas_
    };
    deviceAddress_.address = vkGetAccelerationStructureDeviceAddressKHR(device_->_internalGetNativeDevice(), &info);
}

VulkanBlas::~VulkanBlas()
{
    assert(device_ && buffer_);
    vkDestroyAccelerationStructureKHR(device_->_internalGetNativeDevice(), blas_, RTRC_VK_ALLOC);
}

BufferDeviceAddress VulkanBlas::GetDeviceAddress() const
{
    return deviceAddress_;
}

VkAccelerationStructureKHR VulkanBlas::_internalGetNativeBlas() const
{
    return blas_;
}

RTRC_RHI_VK_END
