#pragma once

#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanTlas, Tlas)
{
public:

    VulkanTlas(VulkanDevice *device, VkAccelerationStructureKHR tlas, BufferRPtr buffer);

    ~VulkanTlas() override;

    BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_OVERRIDE;

    VkAccelerationStructureKHR _internalGetNativeTlas() const;

private:

    VulkanDevice *device_;
    VkAccelerationStructureKHR tlas_;
    BufferDeviceAddress deviceAddress_;
    BufferRPtr buffer_;
};

RTRC_RHI_VK_END
