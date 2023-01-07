#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferSRV.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferUAV.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureView.h>

RTRC_RHI_VK_BEGIN

VulkanBindingGroupInstance::VulkanBindingGroupInstance(
    VkDevice device, const VulkanBindingGroupLayout *layout, VkDescriptorSet set)
    : device_(device), layout_(layout), set_(set)
{
    
}

VulkanBindingGroupInstance::~VulkanBindingGroupInstance()
{
    layout_->ReleaseSet(set_);
}

const BindingGroupLayout *VulkanBindingGroupInstance::GetLayout() const
{
    return layout_;
}

void VulkanBindingGroupInstance::ModifyMember(int index, BufferSRV *bufferSRV)
{
    auto rawBufferSRV = static_cast<VulkanBufferSRV *>(bufferSRV);
    auto &desc = rawBufferSRV->GetDesc();

    if(layout_->IsSlotTexelBuffer(index))
    {
        VkBufferView bufferView = rawBufferSRV->GetNativeView();
        assert(bufferView);
        const VkWriteDescriptorSet write = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = set_,
            .dstBinding       = static_cast<uint32_t>(index),
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            .pTexelBufferView = &bufferView
        };
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
    else
    {
        assert(layout_->IsSlotStructuredBuffer(index));
        assert(desc.stride != 0);
        const VkDescriptorBufferInfo bufferInfo = {
            .buffer = rawBufferSRV->GetBuffer()->GetNativeBuffer(),
            .offset = desc.offset,
            .range  = desc.range
        };
        const VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set_,
            .dstBinding      = static_cast<uint32_t>(index),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &bufferInfo
        };
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
}

void VulkanBindingGroupInstance::ModifyMember(int index, BufferUAV *bufferUAV)
{
    auto rawBufferUAV = static_cast<VulkanBufferUAV *>(bufferUAV);
    auto &desc = rawBufferUAV->GetDesc();

    if(layout_->IsSlotStorageTexelBuffer(index))
    {
        VkBufferView bufferView = rawBufferUAV->GetNativeView();
        assert(bufferView);
        const VkWriteDescriptorSet write = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = set_,
            .dstBinding       = static_cast<uint32_t>(index),
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .pTexelBufferView = &bufferView
        };
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
    else
    {
        assert(layout_->IsSlotRWStructuredBuffer(index));
        assert(desc.stride != 0);
        const VkDescriptorBufferInfo bufferInfo = {
            .buffer = rawBufferUAV->GetBuffer()->GetNativeBuffer(),
            .offset = desc.offset,
            .range  = desc.range
        };
        const VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set_,
            .dstBinding      = static_cast<uint32_t>(index),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &bufferInfo
        };
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
}

void VulkanBindingGroupInstance::ModifyMember(int index, TextureSRV *textureSRV)
{
    auto rawTexSRV = static_cast<VulkanTextureSRV *>(textureSRV);
    assert(layout_->IsSlotTexture2D(index) || layout_->IsSlotTexture3D(index) ||
           layout_->IsSlotTexture2DArray(index) || layout_->IsSlotTexture3DArray(index));
    const VkDescriptorImageInfo imageInfo = {
        .imageView   = rawTexSRV->GetNativeImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo      = &imageInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroupInstance::ModifyMember(int index, TextureUAV *textureUAV)
{
    auto rawTexUAV = static_cast<VulkanTextureUAV *>(textureUAV);
    assert(layout_->IsSlotRWTexture2D(index) || layout_->IsSlotRWTexture3D(index) ||
           layout_->IsSlotRWTexture2DArray(index) || layout_->IsSlotRWTexture3DArray(index));
    const VkDescriptorImageInfo imageInfo = {
        .imageView   = rawTexUAV->GetNativeImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo      = &imageInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroupInstance::ModifyMember(int index, Sampler *sampler)
{
    auto rawSampler = static_cast<VulkanSampler *>(sampler);
    const VkDescriptorImageInfo samplerInfo = {
        .sampler = rawSampler->GetNativeSampler()
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo      = &samplerInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroupInstance::ModifyMember(int index, const ConstantBufferUpdate &cbuffer)
{
    const VkDescriptorBufferInfo bufferInfo = {
        .buffer = static_cast<const VulkanBuffer *>(cbuffer.buffer)->GetNativeBuffer(),
        .offset = cbuffer.offset,
        .range  = cbuffer.range
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo     = &bufferInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

VkDescriptorSet VulkanBindingGroupInstance::GetNativeSet() const
{
    return set_;
}

void VulkanBindingGroupInstance::Translate(
    LinearAllocator &arena, int index, const VulkanBufferSRV *bufferSrv, VkWriteDescriptorSet &write) const
{
    if(layout_->IsSlotTexelBuffer(index))
    {
        auto bufferView = arena.Create<VkBufferView>();
        *bufferView = bufferSrv->GetNativeView();
        write = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = set_,
            .dstBinding       = static_cast<uint32_t>(index),
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            .pTexelBufferView = bufferView
        };
    }
    else
    {
        assert(layout_->IsSlotStructuredBuffer(index));
        auto &desc = bufferSrv->GetDesc();
        assert(desc.stride != 0);
        auto bufferInfo = arena.Create<VkDescriptorBufferInfo>();
        *bufferInfo = {
            .buffer = bufferSrv->GetBuffer()->GetNativeBuffer(),
            .offset = desc.offset,
            .range = desc.range
        };
        write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set_,
            .dstBinding      = static_cast<uint32_t>(index),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = bufferInfo
        };
    }
}

void VulkanBindingGroupInstance::Translate(
    LinearAllocator &arena, int index, const VulkanBufferUAV *bufferUAV, VkWriteDescriptorSet &write) const
{
    auto &desc = bufferUAV->GetDesc();

    if(layout_->IsSlotStorageTexelBuffer(index))
    {
        auto bufferView = arena.Create<VkBufferView>();
        *bufferView = bufferUAV->GetNativeView();
        write = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = set_,
            .dstBinding       = static_cast<uint32_t>(index),
            .dstArrayElement  = 0,
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .pTexelBufferView = bufferView
        };
    }
    else
    {
        assert(layout_->IsSlotRWStructuredBuffer(index));
        assert(desc.stride != 0);
        auto bufferInfo = arena.Create<VkDescriptorBufferInfo>();
        *bufferInfo = {
            .buffer = bufferUAV->GetBuffer()->GetNativeBuffer(),
            .offset = desc.offset,
            .range  = desc.range
        };
        write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set_,
            .dstBinding      = static_cast<uint32_t>(index),
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = bufferInfo
        };
    }
}

void VulkanBindingGroupInstance::Translate(
    LinearAllocator &arena, int index, const VulkanTextureSRV *textureSrv, VkWriteDescriptorSet &write) const
{
    assert(layout_->IsSlotTexture2D(index) || layout_->IsSlotTexture3D(index) ||
           layout_->IsSlotTexture2DArray(index) || layout_->IsSlotTexture3DArray(index));
    auto imageInfo = arena.Create<VkDescriptorImageInfo>();
    *imageInfo = {
        .imageView   = textureSrv->GetNativeImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };
    write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo      = imageInfo
    };
}

void VulkanBindingGroupInstance::Translate(
    LinearAllocator &arena, int index, const VulkanTextureUAV *textureUAV, VkWriteDescriptorSet &write) const
{
    assert(layout_->IsSlotRWTexture2D(index) || layout_->IsSlotRWTexture3D(index) ||
           layout_->IsSlotRWTexture2DArray(index) || layout_->IsSlotRWTexture3DArray(index));
    auto imageInfo = arena.Create<VkDescriptorImageInfo>();
    *imageInfo = {
        .imageView   = textureUAV->GetNativeImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo      = imageInfo
    };
}

void VulkanBindingGroupInstance::Translate(
    LinearAllocator &arena, int index, const VulkanSampler *sampler, VkWriteDescriptorSet &write) const
{
    auto samplerInfo = arena.Create<VkDescriptorImageInfo>();
    *samplerInfo = {
        .sampler = sampler->GetNativeSampler()
    };
    write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo      = samplerInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroupInstance::Translate(
    LinearAllocator &arena, int index,
    const VulkanBuffer *cbuffer, size_t offset, size_t range, VkWriteDescriptorSet &write) const
{
    auto bufferInfo = arena.Create<VkDescriptorBufferInfo>();
    *bufferInfo = {
        .buffer = cbuffer->GetNativeBuffer(),
        .offset = offset,
        .range  = range
    };
    write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo     = bufferInfo
    };
}

RTRC_RHI_VK_END
