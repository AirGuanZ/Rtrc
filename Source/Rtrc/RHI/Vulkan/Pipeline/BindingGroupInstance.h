#pragma once

#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupLayout.h>

RTRC_RHI_VK_BEGIN

class VulkanBindingGroupInstance : public BindingGroupInstance
{
public:

    VulkanBindingGroupInstance(VulkanBindingGroupLayout *layout, VkDescriptorSet set);

    ~VulkanBindingGroupInstance() override;

private:

    VulkanBindingGroupLayout *layout_;
    VkDescriptorSet set_;
};

RTRC_RHI_VK_END
