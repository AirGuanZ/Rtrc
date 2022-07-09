#pragma once

#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupLayout.h>

RTRC_RHI_VK_BEGIN

class VulkanBindingGroupInstance : public BindingGroup
{
public:

    VulkanBindingGroupInstance(VkDevice device, VulkanBindingGroupLayout *layout, VkDescriptorSet set);

    ~VulkanBindingGroupInstance() override;

    const BindingGroupLayout *GetLayout() const override;

    void ModifyMember(int index, const Ptr<BufferSRV> &bufferSRV) override;

    void ModifyMember(int index, const Ptr<BufferUAV> &bufferUAV) override;

    void ModifyMember(int index, const Ptr<Texture2DSRV> &textureSRV) override;

    void ModifyMember(int index, const Ptr<Texture2DUAV> &textureUAV) override;

    void ModifyMember(int index, const Ptr<Sampler> &sampler) override;

    void ModifyMember(int index, const Ptr<Buffer> &uniformBuffer, size_t offset, size_t range) override;

    VkDescriptorSet GetNativeSet() const;

private:

    VkDevice device_;
    VulkanBindingGroupLayout *layout_;
    VkDescriptorSet set_;
};

RTRC_RHI_VK_END
