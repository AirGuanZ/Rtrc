#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanBindingLayout : public BindingLayout
{
public:

    VulkanBindingLayout(const BindingLayoutDesc &desc, VkDevice device, VkPipelineLayout layout);

    ~VulkanBindingLayout() override;

    VkPipelineLayout GetNativeLayout() const;

private:

    BindingLayoutDesc desc_;
    VkDevice device_;
    VkPipelineLayout layout_;
};

RTRC_RHI_VK_END
