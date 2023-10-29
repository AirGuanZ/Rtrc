#pragma once

#include <RHI/Vulkan/Pipeline/BindingLayout.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanRayTracingPipeline, RayTracingPipeline)
{
public:

    VulkanRayTracingPipeline(OPtr<BindingLayout> layout, VulkanDevice *device, VkPipeline pipeline);

    ~VulkanRayTracingPipeline() override;

    const OPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE;

    void GetShaderGroupHandles(
        uint32_t                   startGroupIndex,
        uint32_t                   groupCount,
        MutableSpan<unsigned char> outputData) const RTRC_RHI_OVERRIDE;

    VkPipeline _internalGetNativePipeline() const;

private:

    OPtr<BindingLayout> layout_;
    VulkanDevice *device_;
    VkPipeline pipeline_;
};

RTRC_RHI_VK_END
