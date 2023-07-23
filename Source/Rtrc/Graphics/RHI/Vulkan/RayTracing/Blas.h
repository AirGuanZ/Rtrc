#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Resource/Buffer.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanBlas, Blas)
{
public:

    VulkanBlas(VulkanDevice *device, VkAccelerationStructureKHR blas, BufferPtr buffer);

    ~VulkanBlas() override;

    BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_OVERRIDE;

    VkAccelerationStructureKHR _internalGetNativeBlas() const;

private:

    VulkanDevice *device_;
    VkAccelerationStructureKHR blas_;
    BufferDeviceAddress deviceAddress_;
    BufferPtr buffer_;
};

RTRC_RHI_VK_END
