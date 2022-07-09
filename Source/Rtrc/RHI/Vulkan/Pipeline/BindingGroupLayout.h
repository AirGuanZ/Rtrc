#pragma once

#include <queue>

#include <Rtrc/RHI/Vulkan/Common.h>

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
        const BindingGroupLayoutDesc             *desc,
        std::vector<VkDescriptorSetLayoutBinding> bindings,
        VkDevice                                  device,
        VkDescriptorSetLayout                     layout);

    ~VulkanBindingGroupLayout() override;

    const BindingGroupLayoutDesc *GetDesc() const override;

    Ptr<BindingGroup> CreateBindingGroup() override;

    VkDescriptorSetLayout GetLayout() const;

    void ReleaseSet(VkDescriptorSet set);

    bool IsSlotTexelBuffer(int index) const;

    bool IsSlotStorageTexelBuffer(int index) const;

    bool IsSlotStructuredBuffer(int index) const;

    bool IsSlotRWStructuredBuffer(int index) const;

    bool IsSlotTexture2D(int index) const;

    bool IsSlotRWTexture2D(int index) const;

private:

    void TransferNode(std::list<PoolInfo> &from, std::list<PoolInfo> &to, std::list<PoolInfo>::iterator iter);

    void AllocateNewDescriptorPool();

    const BindingGroupLayoutDesc *desc_;
    std::vector<VkDescriptorSetLayoutBinding> bindings_;

    VkDevice device_;
    VkDescriptorSetLayout layout_;
    std::vector<VkDescriptorPoolSize> singleSetPoolSizes_;

    std::vector<VkDescriptorPool> pools_;
    int poolCount_;

    std::vector<VkDescriptorSet> freeSets_;
};

RTRC_RHI_VK_END
