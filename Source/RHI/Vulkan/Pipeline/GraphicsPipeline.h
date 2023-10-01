#pragma once

#include <RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanGraphicsPipeline, GraphicsPipeline)
{
public:

    VulkanGraphicsPipeline(Ptr<BindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanGraphicsPipeline() override;

    const Ptr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE;

    VkPipeline _internalGetNativePipeline() const;

private:

    Ptr<BindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

RTRC_RHI_VK_END
