#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Utility/Arena.h>

RTRC_RHI_VK_BEGIN

class VulkanBufferSRV;
class VulkanBufferUAV;
class VulkanTextureSRV;
class VulkanTextureUAV;
class VulkanSampler;
class VulkanBuffer;

class VulkanBindingGroupInstance : public BindingGroup
{
public:

    VulkanBindingGroupInstance(VkDevice device, const VulkanBindingGroupLayout *layout, VkDescriptorSet set);
    ~VulkanBindingGroupInstance() override;

    const BindingGroupLayout *GetLayout() const override;

    void ModifyMember(int index, BufferSRV  *bufferSRV) override;
    void ModifyMember(int index, BufferUAV  *bufferUAV) override;
    void ModifyMember(int index, TextureSRV *textureSRV) override;
    void ModifyMember(int index, TextureUAV *textureUAV) override;
    void ModifyMember(int index, Sampler    *sampler) override;
    void ModifyMember(int index, const ConstantBufferUpdate &cbuffer) override;

    VkDescriptorSet GetNativeSet() const;

    void Translate(
        LinearAllocator &arena, int index, const VulkanBufferSRV *bufferSrv, VkWriteDescriptorSet &write) const;
    void Translate(
        LinearAllocator &arena, int index, const VulkanBufferUAV *bufferUAV, VkWriteDescriptorSet &write) const;
    void Translate(
        LinearAllocator &arena, int index, const VulkanTextureSRV *textureSrv, VkWriteDescriptorSet &write) const;
    void Translate(
        LinearAllocator &arena, int index, const VulkanTextureUAV *textureUAV, VkWriteDescriptorSet &write) const;
    void Translate(
        LinearAllocator &arena, int index, const VulkanSampler *sampler, VkWriteDescriptorSet &write) const;
    void Translate(
        LinearAllocator &arena, int index,
        const VulkanBuffer *cbuffer, size_t offset, size_t range, VkWriteDescriptorSet &write) const;

private:

    VkDevice device_;
    const VulkanBindingGroupLayout *layout_;
    VkDescriptorSet set_;
};

RTRC_RHI_VK_END
