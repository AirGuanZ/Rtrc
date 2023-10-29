#include <RHI/Vulkan/Context/Device.h>
#include <RHI/Vulkan/RayTracing/Tlas.h>

RTRC_RHI_VK_BEGIN

VulkanTlas::VulkanTlas(VulkanDevice *device, VkAccelerationStructureKHR tlas, BufferRPtr buffer)
    : device_(device), tlas_(tlas), deviceAddress_{}, buffer_(std::move(buffer))
{
    const VkAccelerationStructureDeviceAddressInfoKHR info =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = tlas_
    };
    deviceAddress_.address = vkGetAccelerationStructureDeviceAddressKHR(device_->_internalGetNativeDevice(), &info);
}

VulkanTlas::~VulkanTlas()
{
    assert(device_ && buffer_);
    vkDestroyAccelerationStructureKHR(device_->_internalGetNativeDevice(), tlas_, RTRC_VK_ALLOC);
}

BufferDeviceAddress VulkanTlas::GetDeviceAddress() const
{
    return deviceAddress_;
}

VkAccelerationStructureKHR VulkanTlas::_internalGetNativeTlas() const
{
    return tlas_;
}

RTRC_RHI_VK_END
