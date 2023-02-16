#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/Tlas.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferView.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureView.h>

RTRC_RHI_VK_BEGIN

VulkanBindingGroup::VulkanBindingGroup(
    VkDevice device, const VulkanBindingGroupLayout *layout, VkDescriptorPool pool, VkDescriptorSet set)
    : device_(device), layout_(layout), pool_(pool), set_(set)
{
    
}

VulkanBindingGroup::~VulkanBindingGroup()
{
    layout_->_internalReleaseSet(pool_, set_);
}

const BindingGroupLayout *VulkanBindingGroup::GetLayout() const
{
    return layout_;
}

void VulkanBindingGroup::ModifyMember(int index, int arrayElem, BufferSrv *bufferSrv)
{
    auto rawBufferSrv = static_cast<VulkanBufferSrv *>(bufferSrv);
    auto &desc = rawBufferSrv->GetDesc();

    if(layout_->_internalIsSlotTexelBuffer(index))
    {
        VkBufferView bufferView = rawBufferSrv->_internalGetNativeView();
        assert(bufferView);
        const VkWriteDescriptorSet write = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = set_,
            .dstBinding       = static_cast<uint32_t>(index),
            .dstArrayElement  = static_cast<uint32_t>(arrayElem),
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            .pTexelBufferView = &bufferView
        };
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
    else
    {
        assert(layout_->_internalIsSlotStructuredBuffer(index));
        assert(desc.stride != 0);
        const VkDescriptorBufferInfo bufferInfo = {
            .buffer = rawBufferSrv->_internalGetBuffer()->_internalGetNativeBuffer(),
            .offset = desc.offset,
            .range  = desc.range
        };
        const VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set_,
            .dstBinding      = static_cast<uint32_t>(index),
            .dstArrayElement = static_cast<uint32_t>(arrayElem),
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &bufferInfo
        };
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
}

void VulkanBindingGroup::ModifyMember(int index, int arrayElem, BufferUav *bufferUav)
{
    auto rawBufferUav = static_cast<VulkanBufferUav *>(bufferUav);
    auto &desc = rawBufferUav->GetDesc();

    if(layout_->_internalIsSlotStorageTexelBuffer(index))
    {
        VkBufferView bufferView = rawBufferUav->_internalGetNativeView();
        assert(bufferView);
        const VkWriteDescriptorSet write = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = set_,
            .dstBinding       = static_cast<uint32_t>(index),
            .dstArrayElement  = static_cast<uint32_t>(arrayElem),
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .pTexelBufferView = &bufferView
        };
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
    else
    {
        assert(layout_->_internalIsSlotRWStructuredBuffer(index));
        assert(desc.stride != 0);
        const VkDescriptorBufferInfo bufferInfo = {
            .buffer = rawBufferUav->GetBuffer()->_internalGetNativeBuffer(),
            .offset = desc.offset,
            .range  = desc.range
        };
        const VkWriteDescriptorSet write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set_,
            .dstBinding      = static_cast<uint32_t>(index),
            .dstArrayElement = static_cast<uint32_t>(arrayElem),
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = &bufferInfo
        };
        vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
    }
}

void VulkanBindingGroup::ModifyMember(int index, int arrayElem, TextureSrv *textureSrv)
{
    auto rawTexSrv = static_cast<VulkanTextureSrv *>(textureSrv);
    assert(layout_->_internalIsSlotTexture2D(index) || layout_->_internalIsSlotTexture3D(index) ||
           layout_->_internalIsSlotTexture2DArray(index) || layout_->_internalIsSlotTexture3DArray(index));
    VkImageLayout imageLayout;
    if(textureSrv->GetDesc().flags.contains(TextureSrvFlagBit::SpecialLayout_DepthSrv_StencilAttachment))
    {
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
    }
    else if(textureSrv->GetDesc().flags.contains(TextureSrvFlagBit::SpecialLayout_DepthSrv_StencilAttachmentReadOnly))
    {
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    else
    {
        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    const VkDescriptorImageInfo imageInfo = {
        .imageView   = rawTexSrv->_internalGetNativeImageView(),
        .imageLayout = imageLayout
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo      = &imageInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroup::ModifyMember(int index, int arrayElem, TextureUav *textureUav)
{
    auto rawTexUav = static_cast<VulkanTextureUav *>(textureUav);
    assert(layout_->_internalIsSlotRWTexture2D(index) || layout_->_internalIsSlotRWTexture3D(index) ||
           layout_->_internalIsSlotRWTexture2DArray(index) || layout_->_internalIsSlotRWTexture3DArray(index));
    const VkDescriptorImageInfo imageInfo = {
        .imageView   = rawTexUav->_internalGetNativeImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo      = &imageInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroup::ModifyMember(int index, int arrayElem, Sampler *sampler)
{
    auto rawSampler = static_cast<VulkanSampler *>(sampler);
    const VkDescriptorImageInfo samplerInfo = {
        .sampler = rawSampler->_internalGetNativeSampler()
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo      = &samplerInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroup::ModifyMember(int index, int arrayElem, const ConstantBufferUpdate &cbuffer)
{
    const VkDescriptorBufferInfo bufferInfo = {
        .buffer = static_cast<const VulkanBuffer *>(cbuffer.buffer)->_internalGetNativeBuffer(),
        .offset = cbuffer.offset,
        .range  = cbuffer.range
    };
    const VkWriteDescriptorSet write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo     = &bufferInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroup::ModifyMember(int index, int arrayElem, Tlas *tlas)
{
    VkAccelerationStructureKHR vkTlas = static_cast<VulkanTlas *>(tlas)->_internalGetNativeTlas();
    const VkWriteDescriptorSetAccelerationStructureKHR tlasWrite =
    {
        .sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures    = &vkTlas
    };
    const VkWriteDescriptorSet write =
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext           = &tlasWrite,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

VkDescriptorSet VulkanBindingGroup::_internalGetNativeSet() const
{
    return set_;
}

void VulkanBindingGroup::_internalTranslate(
    LinearAllocator &arena, int index, int arrayElem,
    const VulkanBufferSrv *bufferSrv, VkWriteDescriptorSet &write) const
{
    if(layout_->_internalIsSlotTexelBuffer(index))
    {
        auto bufferView = arena.Create<VkBufferView>();
        *bufferView = bufferSrv->_internalGetNativeView();
        write = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = set_,
            .dstBinding       = static_cast<uint32_t>(index),
            .dstArrayElement  = static_cast<uint32_t>(arrayElem),
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
            .pTexelBufferView = bufferView
        };
    }
    else
    {
        assert(layout_->_internalIsSlotStructuredBuffer(index));
        auto &desc = bufferSrv->GetDesc();
        assert(desc.stride != 0);
        auto bufferInfo = arena.Create<VkDescriptorBufferInfo>();
        *bufferInfo = {
            .buffer = bufferSrv->_internalGetBuffer()->_internalGetNativeBuffer(),
            .offset = desc.offset,
            .range = desc.range
        };
        write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set_,
            .dstBinding      = static_cast<uint32_t>(index),
            .dstArrayElement = static_cast<uint32_t>(arrayElem),
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = bufferInfo
        };
    }
}

void VulkanBindingGroup::_internalTranslate(
    LinearAllocator &arena, int index, int arrayElem,
    const VulkanBufferUav *bufferUav, VkWriteDescriptorSet &write) const
{
    auto &desc = bufferUav->GetDesc();

    if(layout_->_internalIsSlotStorageTexelBuffer(index))
    {
        auto bufferView = arena.Create<VkBufferView>();
        *bufferView = bufferUav->_internalGetNativeView();
        write = {
            .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet           = set_,
            .dstBinding       = static_cast<uint32_t>(index),
            .dstArrayElement  = static_cast<uint32_t>(arrayElem),
            .descriptorCount  = 1,
            .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
            .pTexelBufferView = bufferView
        };
    }
    else
    {
        assert(layout_->_internalIsSlotRWStructuredBuffer(index));
        assert(desc.stride != 0);
        auto bufferInfo = arena.Create<VkDescriptorBufferInfo>();
        *bufferInfo = {
            .buffer = bufferUav->GetBuffer()->_internalGetNativeBuffer(),
            .offset = desc.offset,
            .range  = desc.range
        };
        write = {
            .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet          = set_,
            .dstBinding      = static_cast<uint32_t>(index),
            .dstArrayElement = static_cast<uint32_t>(arrayElem),
            .descriptorCount = 1,
            .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo     = bufferInfo
        };
    }
}

void VulkanBindingGroup::_internalTranslate(
    LinearAllocator &arena, int index, int arrayElem,
    const VulkanTextureSrv *textureSrv, VkWriteDescriptorSet &write) const
{
    assert(layout_->_internalIsSlotTexture2D(index) || layout_->_internalIsSlotTexture3D(index) ||
           layout_->_internalIsSlotTexture2DArray(index) || layout_->_internalIsSlotTexture3DArray(index));
    VkImageLayout imageLayout;
    if(textureSrv->GetDesc().flags.contains(TextureSrvFlagBit::SpecialLayout_DepthSrv_StencilAttachment))
    {
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
    }
    else if(textureSrv->GetDesc().flags.contains(TextureSrvFlagBit::SpecialLayout_DepthSrv_StencilAttachmentReadOnly))
    {
        imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    }
    else
    {
        imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    auto imageInfo = arena.Create<VkDescriptorImageInfo>();
    *imageInfo = {
        .imageView   = textureSrv->_internalGetNativeImageView(),
        .imageLayout = imageLayout
    };
    write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo      = imageInfo
    };
}

void VulkanBindingGroup::_internalTranslate(
    LinearAllocator &arena, int index, int arrayElem,
    const VulkanTextureUav *textureUav, VkWriteDescriptorSet &write) const
{
    assert(layout_->_internalIsSlotRWTexture2D(index) || layout_->_internalIsSlotRWTexture3D(index) ||
           layout_->_internalIsSlotRWTexture2DArray(index) || layout_->_internalIsSlotRWTexture3DArray(index));
    auto imageInfo = arena.Create<VkDescriptorImageInfo>();
    *imageInfo = {
        .imageView   = textureUav->_internalGetNativeImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };
    write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo      = imageInfo
    };
}

void VulkanBindingGroup::_internalTranslate(
    LinearAllocator &arena, int index, int arrayElem,
    const VulkanSampler *sampler, VkWriteDescriptorSet &write) const
{
    auto samplerInfo = arena.Create<VkDescriptorImageInfo>();
    *samplerInfo = {
        .sampler = sampler->_internalGetNativeSampler()
    };
    write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo      = samplerInfo
    };
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

void VulkanBindingGroup::_internalTranslate(
    LinearAllocator &arena, int index, int arrayElem,
    const VulkanBuffer *cbuffer, size_t offset, size_t range, VkWriteDescriptorSet &write) const
{
    auto bufferInfo = arena.Create<VkDescriptorBufferInfo>();
    *bufferInfo = {
        .buffer = cbuffer->_internalGetNativeBuffer(),
        .offset = offset,
        .range  = range
    };
    write = {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo     = bufferInfo
    };
}

void VulkanBindingGroup::_internalTranslate(
    LinearAllocator &arena, int index, int arrayElem, const VulkanTlas *tlas, VkWriteDescriptorSet &write) const
{
    auto vkTlas = arena.Create<VkAccelerationStructureKHR>();
    *vkTlas = tlas->_internalGetNativeTlas();
    auto tlasWrite = arena.Create<VkWriteDescriptorSetAccelerationStructureKHR >();
    *tlasWrite =
    {
        .sType                      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .accelerationStructureCount = 1,
        .pAccelerationStructures    = vkTlas
    };
    write =
    {
        .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext           = tlasWrite,
        .dstSet          = set_,
        .dstBinding      = static_cast<uint32_t>(index),
        .dstArrayElement = static_cast<uint32_t>(arrayElem),
        .descriptorCount = 1,
        .descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR
    };
}

RTRC_RHI_VK_END
