#pragma once

#include <tbb/spin_mutex.h>

#include <queue>

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanBindingGroupLayout, BindingGroupLayout)
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
        BindingGroupLayoutDesc                    desc,
        std::vector<VkDescriptorSetLayoutBinding> bindings,
        VkDevice                                  device,
        VkDescriptorSetLayout                     layout,
        bool                                      hasBindlessBinding);

    ~VulkanBindingGroupLayout() override;

    const BindingGroupLayoutDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    VkDescriptorSetLayout _internalGetNativeLayout() const;

    void _internalReleaseSet(VkDescriptorPool pool, VkDescriptorSet set) const;

    bool _internalIsSlotTexelBuffer(int index) const;
    bool _internalIsSlotStorageTexelBuffer(int index) const;

    bool _internalIsSlotStructuredBuffer(int index) const;
    bool _internalIsSlotRWStructuredBuffer(int index) const;

    bool _internalIsSlotTexture2D(int index) const;
    bool _internalIsSlotRWTexture2D(int index) const;

    bool _internalIsSlotTexture3D(int index) const;
    bool _internalIsSlotRWTexture3D(int index) const;

    bool _internalIsSlotTexture2DArray(int index) const;
    bool _internalIsSlotRWTexture2DArray(int index) const;

    bool _internalIsSlotTexture3DArray(int index) const;
    bool _internalIsSlotRWTexture3DArray(int index) const;

    Ptr<BindingGroup> _internalCreateBindingGroupImpl(uint32_t variableArraySize) const;

private:

    void TransferNode(std::list<PoolInfo> &from, std::list<PoolInfo> &to, std::list<PoolInfo>::iterator iter);

    void AllocateNewDescriptorPool() const;

    const BindingGroupLayoutDesc desc_;
    bool hasBindlessBinding_;
    std::vector<VkDescriptorSetLayoutBinding> bindings_;

    VkDevice device_;
    VkDescriptorSetLayout layout_;
    std::vector<VkDescriptorPoolSize> singleSetPoolSizes_;

    mutable std::vector<VkDescriptorPool> pools_;
    mutable int poolCount_;
    mutable std::vector<VkDescriptorSet> freeSets_;
    mutable tbb::spin_mutex poolMutex_;
};

RTRC_RHI_VK_END
