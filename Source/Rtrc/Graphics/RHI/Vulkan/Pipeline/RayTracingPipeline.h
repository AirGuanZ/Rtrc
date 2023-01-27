#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingLayout.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanRayTracingPipeline, RayTracingPipeline)
{
public:

    VulkanRayTracingPipeline(Ptr<BindingLayout> layout, VulkanDevice *device, VkPipeline pipeline);

    ~VulkanRayTracingPipeline() override;

    const Ptr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE;

    VkPipeline _internalGetNativePipeline() const;

private:

    Ptr<BindingLayout> layout_;
    VulkanDevice *device_;
    VkPipeline pipeline_;
};

RTRC_RHI_VK_END
