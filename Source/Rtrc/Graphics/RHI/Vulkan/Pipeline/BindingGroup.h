#pragma once

#include <Core/Memory/Arena.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroupLayout.h>

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
        VkDevice                        device,
        const VulkanBindingGroupLayout *layout,
        uint32_t                        variableArraySize,
        VkDescriptorPool                pool,
        VkDescriptorSet                 set);
    ~VulkanBindingGroup() override;

    const BindingGroupLayout *GetLayout() const RTRC_RHI_OVERRIDE;
    uint32_t GetVariableArraySize() const RTRC_RHI_OVERRIDE;

    void ModifyMember(int index, int arrayElem, const BufferSrv            *bufferSrv)  RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const BufferUav            *bufferUav)  RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const TextureSrv           *textureSrv) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const TextureUav           *textureUav) RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const Sampler              *sampler)    RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const ConstantBufferUpdate &cbuffer)    RTRC_RHI_OVERRIDE;
    void ModifyMember(int index, int arrayElem, const Tlas                 *tlas)       RTRC_RHI_OVERRIDE;

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
    uint32_t variableArraySize_;
    VkDescriptorPool pool_;
    VkDescriptorSet set_;
};

RTRC_RHI_VK_END
