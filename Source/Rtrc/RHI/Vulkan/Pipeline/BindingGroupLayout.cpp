#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroup.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanBindingGroupLayout::VulkanBindingGroupLayout(
    const BindingGroupLayoutDesc             *desc,
    std::vector<VkDescriptorSetLayoutBinding> bindings,
    VkDevice                                  device,
    VkDescriptorSetLayout                     layout)
    : desc_(desc), bindings_(std::move(bindings)),
      device_(device), layout_(layout), poolCount_(0)
{
    for(auto &binding : bindings_)
    {
        bool found = false;
        for(auto &poolSize : singleSetPoolSizes_)
        {
            if(poolSize.type == binding.descriptorType)
            {
                poolSize.descriptorCount += binding.descriptorCount;
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
        vkDestroyDescriptorPool(device_, pool, VK_ALLOC);
    }
    vkDestroyDescriptorSetLayout(device_, layout_, VK_ALLOC);
}

const BindingGroupLayoutDesc *VulkanBindingGroupLayout::GetDesc() const
{
    return desc_;
}

RC<BindingGroup> VulkanBindingGroupLayout::CreateBindingGroup()
{
    if(freeSets_.empty())
    {
        AllocateNewDescriptorPool();
    }
    auto set = freeSets_.back();
    freeSets_.pop_back();
    return MakeRC<VulkanBindingGroupInstance>(device_, this, set);
}

VkDescriptorSetLayout VulkanBindingGroupLayout::GetLayout() const
{
    return layout_;
}

void VulkanBindingGroupLayout::ReleaseSet(VkDescriptorSet set)
{
    freeSets_.push_back(set);
}

bool VulkanBindingGroupLayout::IsSlotTexelBuffer(int index) const
{
    return desc_->bindings[index].front().type == BindingType::Buffer;
}

bool VulkanBindingGroupLayout::IsSlotStorageTexelBuffer(int index) const
{
    return desc_->bindings[index].front().type == BindingType::RWBuffer;
}

bool VulkanBindingGroupLayout::IsSlotStructuredBuffer(int index) const
{
    return desc_->bindings[index].front().type == BindingType::StructuredBuffer;
}

bool VulkanBindingGroupLayout::IsSlotRWStructuredBuffer(int index) const
{
    return desc_->bindings[index].front().type == BindingType::RWStructuredBuffer;
}

bool VulkanBindingGroupLayout::IsSlotTexture2D(int index) const
{
    return desc_->bindings[index].front().type == BindingType::Texture2D;
}

bool VulkanBindingGroupLayout::IsSlotRWTexture2D(int index) const
{
    return desc_->bindings[index].front().type == BindingType::RWTexture2D;
}

void VulkanBindingGroupLayout::TransferNode(std::list<PoolInfo> &from, std::list<PoolInfo> &to, std::list<PoolInfo>::iterator iter)
{
    to.splice(to.begin(), from, iter);
}

void VulkanBindingGroupLayout::AllocateNewDescriptorPool()
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

    const VkDescriptorPoolCreateInfo createInfo = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = static_cast<uint32_t>(maxSets),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes    = poolSizes.data()
    };
    VkDescriptorPool pool;
    VK_FAIL_MSG(
        vkCreateDescriptorPool(device_, &createInfo, VK_ALLOC, &pool),
        "failed to create vulkan descriptor pool");
    RTRC_SCOPE_FAIL{ vkDestroyDescriptorPool(device_, pool, VK_ALLOC); };

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
        VK_FAIL_MSG(
            vkAllocateDescriptorSets(device_, &allocInfo, &set),
            "failed to allocate vulkan descriptor set");
        freeSets_.push_back(set);
    }

    pools_.push_back(pool);
    ++poolCount_;
}

RTRC_RHI_VK_END
