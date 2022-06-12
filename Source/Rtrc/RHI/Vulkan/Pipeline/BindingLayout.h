#pragma once

#include <map>

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanBindingLayout : public BindingLayout
{
public:

    VulkanBindingLayout(const BindingLayoutDesc &desc, VkDevice device, VkPipelineLayout layout);

    ~VulkanBindingLayout() override;

    int GetGroupIndex(const TypeIndex &groupStructType) const override;

    VkPipelineLayout GetNativeLayout() const;

private:

    BindingLayoutDesc desc_;
    VkDevice device_;
    VkPipelineLayout layout_;

    std::map<TypeIndex, int> groupType2Index_;
};

RTRC_RHI_VK_END
