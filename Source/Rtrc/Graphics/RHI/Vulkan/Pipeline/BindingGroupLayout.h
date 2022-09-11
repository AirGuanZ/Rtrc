#pragma once

#include <queue>

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanBindingGroupLayout : public BindingGroupLayout
{
public:

    struct PoolInfo
    {
        int capacity;
        int rest;
        VkDescriptorPool pool;
    };

    using PoolInfoIterator = std::list<PoolInfo>::iterator;

    VulkanBindingGroupLayout(
        const BindingGroupLayoutDesc             &desc,
        std::vector<VkDescriptorSetLayoutBinding> bindings,
        VkDevice                                  device,
        VkDescriptorSetLayout                     layout);

    ~VulkanBindingGroupLayout() override;

    const BindingGroupLayoutDesc &GetDesc() const override;

    VkDescriptorSetLayout GetLayout() const;

    void ReleaseSet(VkDescriptorSet set) const;

    bool IsSlotTexelBuffer(int index) const;
    bool IsSlotStorageTexelBuffer(int index) const;

    bool IsSlotStructuredBuffer(int index) const;
    bool IsSlotRWStructuredBuffer(int index) const;

    bool IsSlotTexture2D(int index) const;
    bool IsSlotRWTexture2D(int index) const;

    bool IsSlotTexture3D(int index) const;
    bool IsSlotRWTexture3D(int index) const;

    bool IsSlotTexture2DArray(int index) const;
    bool IsSlotRWTexture2DArray(int index) const;

    bool IsSlotTexture3DArray(int index) const;
    bool IsSlotRWTexture3DArray(int index) const;

    Ptr<BindingGroup> CreateBindingGroupImpl() const;

private:

    void TransferNode(std::list<PoolInfo> &from, std::list<PoolInfo> &to, std::list<PoolInfo>::iterator iter);

    void AllocateNewDescriptorPool() const;

    const BindingGroupLayoutDesc desc_;
    std::vector<VkDescriptorSetLayoutBinding> bindings_;

    VkDevice device_;
    VkDescriptorSetLayout layout_;
    std::vector<VkDescriptorPoolSize> singleSetPoolSizes_;

    mutable std::vector<VkDescriptorPool> pools_;
    mutable int poolCount_;

    mutable std::vector<VkDescriptorSet> freeSets_;
};

RTRC_RHI_VK_END
