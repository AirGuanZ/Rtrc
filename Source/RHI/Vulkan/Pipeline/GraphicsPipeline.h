#pragma once

#include <RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanGraphicsPipeline, GraphicsPipeline)
{
public:

    VulkanGraphicsPipeline(RPtr<BindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanGraphicsPipeline() override;

    const RPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE;

    VkPipeline _internalGetNativePipeline() const;

private:

    RPtr<BindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

RTRC_RHI_VK_END
