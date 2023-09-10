#include <mutex>

#include <Core/ScopeGuard.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Sampler.h>

RTRC_RHI_VK_BEGIN

VulkanBindingGroupLayout::VulkanBindingGroupLayout(
    BindingGroupLayoutDesc                    desc,
    std::vector<VkDescriptorSetLayoutBinding> bindings,
    VkDevice                                  device,
    VkDescriptorSetLayout                     layout,
    bool                                      hasBindlessBinding)
    : desc_(std::move(desc)), hasBindlessBinding_(hasBindlessBinding), bindings_(std::move(bindings)),
      device_(device), layout_(layout), poolCount_(0)
{
    for(auto &binding : bindings_)
    {
        bool found = false;
        for(auto &poolSize : singleSetPoolSizes_)
        {
            if(poolSize.type == binding.descriptorType)
            {
                if(!(desc_.variableArraySize && &binding == &bindings_.back()))
                {
                    poolSize.descriptorCount += binding.descriptorCount;
                }
                found = true;
                break;
            }
        }
        if(!found)
        {
            singleSetPoolSizes_.push_back(VkDescriptorPoolSize{
                .type = binding.descriptorType,
                .descriptorCount = binding.descriptorCount
            });
        }
    }
}

VulkanBindingGroupLayout::~VulkanBindingGroupLayout()
{
    assert(layout_);
    for(auto pool : pools_)
    {
        vkDestroyDescriptorPool(device_, pool, RTRC_VK_ALLOC);
    }
    vkDestroyDescriptorSetLayout(device_, layout_, RTRC_VK_ALLOC);
}

const BindingGroupLayoutDesc &VulkanBindingGroupLayout::GetDesc() const
{
    return desc_;
}

VkDescriptorSetLayout VulkanBindingGroupLayout::_internalGetNativeLayout() const
{
    return layout_;
}

void VulkanBindingGroupLayout::_internalReleaseSet(VkDescriptorPool pool, VkDescriptorSet set) const
{
    if(desc_.variableArraySize)
    {
        vkDestroyDescriptorPool(device_, pool, RTRC_VK_ALLOC);
    }
    else
    {
        std::lock_guard lock(poolMutex_);
        freeSets_.push_back(set);
    }
}

bool VulkanBindingGroupLayout::_internalIsSlotTexelBuffer(int index) const
{
    return desc_.bindings[index].type == BindingType::Buffer;
}

bool VulkanBindingGroupLayout::_internalIsSlotStorageTexelBuffer(int index) const
{
    return desc_.bindings[index].type == BindingType::RWBuffer;
}

bool VulkanBindingGroupLayout::_internalIsSlotStructuredBuffer(int index) const
{
    return desc_.bindings[index].type == BindingType::StructuredBuffer;
}

bool VulkanBindingGroupLayout::_internalIsSlotRWStructuredBuffer(int index) const
{
    return desc_.bindings[index].type == BindingType::RWStructuredBuffer;
}

bool VulkanBindingGroupLayout::_internalIsSlotTexture(int index) const
{
    return desc_.bindings[index].type == BindingType::Texture;
}

bool VulkanBindingGroupLayout::_internalIsSlotRWTexture(int index) const
{
    return desc_.bindings[index].type == BindingType::RWTexture;
}

Ptr<BindingGroup> VulkanBindingGroupLayout::_internalCreateBindingGroupImpl(uint32_t variableArraySize) const
{
    assert(!desc_.variableArraySize || variableArraySize != 0);
    if(!variableArraySize)
    {
        std::lock_guard lock(poolMutex_);
        if(freeSets_.empty())
        {
            AllocateNewDescriptorPool();
        }
        auto set = freeSets_.back();
        freeSets_.pop_back();
        return MakePtr<VulkanBindingGroup>(device_, this, 0, nullptr, set);
    }

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(singleSetPoolSizes_.size());
    for(auto &poolSize : singleSetPoolSizes_)
    {
        poolSizes.push_back(VkDescriptorPoolSize
        {
            .type = poolSize.type,
            .descriptorCount = poolSize.descriptorCount
        });
        if(poolSize.type == bindings_.back().descriptorType)
        {
            poolSizes.back().descriptorCount += variableArraySize;
        }
    }

    const VkDescriptorPoolCreateFlags flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    const VkDescriptorPoolCreateInfo createInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = flags,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    VkDescriptorPool pool;
    RTRC_VK_FAIL_MSG(
        vkCreateDescriptorPool(device_, &createInfo, RTRC_VK_ALLOC, &pool),
        "Failed to create vulkan descriptor pool for descriptor set with variable descriptor count");
    RTRC_SCOPE_FAIL{ vkDestroyDescriptorPool(device_, pool, RTRC_VK_ALLOC); };
    
    const VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &variableArraySize
    };
    const VkDescriptorSetAllocateInfo allocInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variableDescriptorCountInfo,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout_
    };
    VkDescriptorSet set;
    RTRC_VK_FAIL_MSG(
        vkAllocateDescriptorSets(device_, &allocInfo, &set),
        "Failed to allocate vulkan descriptor set with variable descriptor count");

    return MakePtr<VulkanBindingGroup>(device_, this, variableArraySize, pool, set);
}

void VulkanBindingGroupLayout::TransferNode(
    std::list<PoolInfo> &from, std::list<PoolInfo> &to, std::list<PoolInfo>::iterator iter)
{
    to.splice(to.begin(), from, std::move(iter));
}

void VulkanBindingGroupLayout::AllocateNewDescriptorPool() const
{
    const int maxSets = (std::min)(8192, 64 << (poolCount_ >> 1));

    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(singleSetPoolSizes_.size());
    for(auto &poolSize : singleSetPoolSizes_)
    {
        poolSizes.push_back(VkDescriptorPoolSize{
            .type = poolSize.type,
            .descriptorCount = poolSize.descriptorCount * maxSets
        });
    }

    const VkDescriptorPoolCreateFlags flags = hasBindlessBinding_ ? VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT : 0;
    const VkDescriptorPoolCreateInfo createInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags         = flags,
        .maxSets       = static_cast<uint32_t>(maxSets),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes    = poolSizes.data()
    };
    VkDescriptorPool pool;
    RTRC_VK_FAIL_MSG(
        vkCreateDescriptorPool(device_, &createInfo, RTRC_VK_ALLOC, &pool),
        "Failed to create vulkan descriptor pool");
    RTRC_SCOPE_FAIL{ vkDestroyDescriptorPool(device_, pool, RTRC_VK_ALLOC); };

    freeSets_.reserve(freeSets_.size() + maxSets);
    const size_t oldSize = freeSets_.size();
    RTRC_SCOPE_FAIL{ freeSets_.resize(oldSize); };

    const VkDescriptorSetAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &layout_
    };
    for(int i = 0; i < maxSets; ++i)
    {
        VkDescriptorSet set;
        RTRC_VK_FAIL_MSG(
            vkAllocateDescriptorSets(device_, &allocInfo, &set),
            "failed to allocate vulkan descriptor set");
        freeSets_.push_back(set);
    }

    pools_.push_back(pool);
    ++poolCount_;
}

RTRC_RHI_VK_END
