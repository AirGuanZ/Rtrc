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
        const BindingGroupLayoutDesc &desc,
        std::vector<VkDescriptorSetLayoutBinding> bindings,
        VkDevice device,
        VkDescriptorSetLayout layout);

    ~VulkanBindingGroupLayout() override;

    RC<BindingGroupInstance> CreateBindingGroup(bool updateAfterBind) override;

    VkDescriptorSetLayout GetLayout() const;

    void ReleaseSet(VkDescriptorSet set);

private:

    void TransferNode(std::list<PoolInfo> &from, std::list<PoolInfo> &to, std::list<PoolInfo>::iterator iter);

    void AllocateNewDescriptorPool(bool updateAfterBind);

    BindingGroupLayoutDesc desc_;
    std::vector<VkDescriptorSetLayoutBinding> bindings_;

    VkDevice device_;
    VkDescriptorSetLayout layout_;
    std::vector<VkDescriptorPoolSize> singleSetPoolSizes_;

    std::vector<VkDescriptorPool> pools_;
    int poolCountRegular_;
    int poolCountUpdateAfterBind_;

    std::vector<VkDescriptorSet> freeSetsRegular_;
    std::vector<VkDescriptorSet> freeSetsUpdateAfterBind_;
};

RTRC_RHI_VK_END
