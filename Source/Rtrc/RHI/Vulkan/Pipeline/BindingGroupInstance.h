#pragma once

#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupLayout.h>

RTRC_RHI_VK_BEGIN

class VulkanBindingGroupInstance : public BindingGroup
{
public:

    VulkanBindingGroupInstance(VkDevice device, VulkanBindingGroupLayout *layout, VkDescriptorSet set);

    ~VulkanBindingGroupInstance() override;

    void ModifyMember(int index, const RC<BufferSRV> &bufferSRV) override;

    void ModifyMember(int index, const RC<Texture2DSRV> &textureSRV) override;

    VkDescriptorSet GetNativeSet() const;

private:

    VkDevice device_;
    VulkanBindingGroupLayout *layout_;
    VkDescriptorSet set_;
};

RTRC_RHI_VK_END
