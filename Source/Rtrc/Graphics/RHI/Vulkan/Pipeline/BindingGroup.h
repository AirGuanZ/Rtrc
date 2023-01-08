#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Utility/Arena.h>

RTRC_RHI_VK_BEGIN

class VulkanBufferSrv;
class VulkanBufferUav;
class VulkanTextureSrv;
class VulkanTextureUav;
class VulkanSampler;
class VulkanBuffer;

RTRC_RHI_IMPLEMENT(VulkanBindingGroup, BindingGroup)
{
public:

    VulkanBindingGroup(VkDevice device, const VulkanBindingGroupLayout *layout, VkDescriptorSet set);
    ~VulkanBindingGroup() override;

    const BindingGroupLayout *GetLayout() const RTRC_RHI_OVERRIDE;

    void ModifyMember(int index, BufferSrv  *bufferSrv) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, BufferUav  *bufferUav) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, TextureSrv *textureSrv) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, TextureUav *textureUav) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, Sampler    *sampler) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, const ConstantBufferUpdate &cbuffer) RTRC_RHI_OVERRIDE;

    VkDescriptorSet _internalGetNativeSet() const;

    void _internalTranslate(
        LinearAllocator &arena, int index, const VulkanBufferSrv *bufferSrv, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, const VulkanBufferUav *bufferUav, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, const VulkanTextureSrv *textureSrv, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, const VulkanTextureUav *textureUav, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, const VulkanSampler *sampler, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index,
        const VulkanBuffer *cbuffer, size_t offset, size_t range, VkWriteDescriptorSet &write) const;

private:

    VkDevice device_;
    const VulkanBindingGroupLayout *layout_;
    VkDescriptorSet set_;
};

RTRC_RHI_VK_END
