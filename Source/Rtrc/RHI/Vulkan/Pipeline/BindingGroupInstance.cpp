#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupInstance.h>
#include <Rtrc/RHI/Vulkan/Resource/BufferSRV.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2DSRV.h>

RTRC_RHI_VK_BEGIN

VulkanBindingGroupInstance::VulkanBindingGroupInstance(
    VkDevice device, VulkanBindingGroupLayout *layout, VkDescriptorSet set)
    : device_(device), layout_(layout), set_(set)
{
    
}

VulkanBindingGroupInstance::~VulkanBindingGroupInstance()
{
    layout_->ReleaseSet(set_);
}

void VulkanBindingGroupInstance::ModifyMember(int index, const RC<BufferSRV> &bufferSRV)
{
    auto rawBufferSRV = static_cast<VulkanBufferSRV *>(bufferSRV.get());
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

void VulkanBindingGroupInstance::ModifyMember(int index, const RC<Texture2DSRV> &textureSRV)
{
    auto rawTexSRV = static_cast<VulkanTexture2DSRV *>(textureSRV.get());
    assert(layout_->IsSlotTexture2D(index));
    const VkDescriptorImageInfo imageInfo = {
        .imageView   = rawTexSRV->GetImageView(),
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

VkDescriptorSet VulkanBindingGroupInstance::GetNativeSet() const
{
    return set_;
}

RTRC_RHI_VK_END
