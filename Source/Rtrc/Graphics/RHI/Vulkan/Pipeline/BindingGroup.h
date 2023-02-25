#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_RHI_VK_BEGIN

class VulkanBufferSrv;
class VulkanBufferUav;
class VulkanTextureSrv;
class VulkanTextureUav;
class VulkanSampler;
class VulkanBuffer;
class VulkanTlas;

RTRC_RHI_IMPLEMENT(VulkanBindingGroup, BindingGroup)
{
public:

#ifdef RTRC_STATIC_RHI
    RTRC_RHI_BINDING_GROUP_COMMON
#endif

    VulkanBindingGroup(
        VkDevice device, const VulkanBindingGroupLayout *layout, VkDescriptorPool pool, VkDescriptorSet set);
    ~VulkanBindingGroup() override;

    const BindingGroupLayout *GetLayout() const RTRC_RHI_OVERRIDE;

    void ModifyMember(int index, int arrayElem, BufferSrv                  *bufferSrv)  RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, BufferUav                  *bufferUav)  RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, TextureSrv                 *textureSrv) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, TextureUav                 *textureUav) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, Sampler                    *sampler)    RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const ConstantBufferUpdate &cbuffer)    RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, Tlas                       *tlas)       RTRC_RHI_OVERRIDE;

    VkDescriptorSet _internalGetNativeSet() const;

    void _internalTranslate(
        LinearAllocator &arena, int index, int arrayElem,
        const VulkanBufferSrv *bufferSrv, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, int arrayElem,
        const VulkanBufferUav *bufferUav, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, int arrayElem,
        const VulkanTextureSrv *textureSrv, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, int arrayElem,
        const VulkanTextureUav *textureUav, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, int arrayElem,
        const VulkanSampler *sampler, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, int arrayElem,
        const VulkanBuffer *cbuffer, size_t offset, size_t range, VkWriteDescriptorSet &write) const;
    void _internalTranslate(
        LinearAllocator &arena, int index, int arrayElem,
        const VulkanTlas *tlas, VkWriteDescriptorSet &write) const;

private:

    VkDevice device_;
    const VulkanBindingGroupLayout *layout_;
    VkDescriptorPool pool_;
    VkDescriptorSet set_;
};

RTRC_RHI_VK_END
