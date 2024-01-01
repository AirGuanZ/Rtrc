#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanComputePipeline, ComputePipeline)
{
public:

    VulkanComputePipeline(OPtr<BindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanComputePipeline() override;

    const OPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE;

    VkPipeline _internalGetNativePipeline() const;

private:

    OPtr<BindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

RTRC_RHI_VK_END
