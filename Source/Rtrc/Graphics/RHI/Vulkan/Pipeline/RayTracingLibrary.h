#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanRayTracingLibrary, RayTracingLibrary)
{
public:

    RTRC_VK_SET_OBJECT_NAME(device_, pipeline_, VK_OBJECT_TYPE_PIPELINE)

    VulkanRayTracingLibrary(VulkanDevice *device, VkPipeline pipeline);
    ~VulkanRayTracingLibrary() override;

    VkPipeline _internalGetNativePipeline() const;

private:

    VulkanDevice *device_;
    VkPipeline pipeline_;
};

RTRC_RHI_VK_END
