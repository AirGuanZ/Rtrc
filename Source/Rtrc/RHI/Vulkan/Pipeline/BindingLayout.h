#pragma once

#include <map>

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanBindingLayout, BindingLayout)
{
public:

    VulkanBindingLayout(BindingLayoutDesc desc, VkDevice device, VkPipelineLayout layout);

    ~VulkanBindingLayout() override;

    VkPipelineLayout _internalGetNativeLayout() const;

    const BindingLayoutDesc &_internalGetDesc() const { return desc_; }

private:

    BindingLayoutDesc desc_;
    VkDevice device_;
    VkPipelineLayout layout_;
};

RTRC_RHI_VK_END
