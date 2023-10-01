#pragma once

#include <RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanRayTracingLibrary, RayTracingLibrary)
{
public:

    RTRC_VK_SET_OBJECT_NAME(device_, pipeline_, VK_OBJECT_TYPE_PIPELINE)

    VulkanRayTracingLibrary(
        VulkanDevice *device, VkPipeline pipeline, uint32_t maxPayloadSize, uint32_t maxHitAttribSize);
    ~VulkanRayTracingLibrary() override;

    VkPipeline _internalGetNativePipeline() const;
    uint32_t _internalGetMaxPayloadSize() const;
    uint32_t _internalGetMaxHitAttributeSize() const;

private:

    VulkanDevice *device_;
    VkPipeline pipeline_;
    uint32_t maxPayloadSize_;
    uint32_t maxHitAttribSize_;
};

RTRC_RHI_VK_END
