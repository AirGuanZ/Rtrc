#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanGraphicsPipeline, GraphicsPipeline)
{
public:

    VulkanGraphicsPipeline(OPtr<BindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanGraphicsPipeline() override;

    const OPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE;

    VkPipeline _internalGetNativePipeline() const;

private:

    OPtr<BindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

RTRC_RHI_VK_END
