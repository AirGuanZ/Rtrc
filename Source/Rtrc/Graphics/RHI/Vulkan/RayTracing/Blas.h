#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanBlas, Blas)
{
public:

    VulkanBlas(VulkanDevice *device, VkAccelerationStructureKHR blas, BufferPtr buffer);

    ~VulkanBlas() override;

    VkAccelerationStructureKHR _internalGetNativeBlas() const;

private:

    VulkanDevice *device_;
    VkAccelerationStructureKHR blas_;
    BufferPtr buffer_;
};

RTRC_RHI_VK_END
