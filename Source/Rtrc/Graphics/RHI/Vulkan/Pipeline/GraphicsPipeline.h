#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanGraphicsPipeline : public GraphicsPipeline
{
public:

    VulkanGraphicsPipeline(Ptr<BindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanGraphicsPipeline() override;

    const Ptr<BindingLayout> &GetBindingLayout() const override;

    VkPipeline GetNativePipeline() const;

private:

    Ptr<BindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

RTRC_RHI_VK_END
