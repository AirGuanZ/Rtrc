#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupInstance.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanBindingGroupLayout::VulkanBindingGroupLayout(
    const BindingGroupLayoutDesc             &desc,
    std::vector<VkDescriptorSetLayoutBinding> bindings,
    VkDevice                                  device,
    VkDescriptorSetLayout                     layout)
    : desc_(desc), bindings_(std::move(bindings)),
      device_(device), layout_(layout),
      poolCountRegular_(0), poolCountUpdateAfterBind_(0)
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

RC<BindingGroupInstance> VulkanBindingGroupLayout::CreateBindingGroup(bool updateAfterBind)
{
    auto &freeSets = updateAfterBind ? freeSetsUpdateAfterBind_ : freeSetsRegular_;
    if(freeSets.empty())
    {
        AllocateNewDescriptorPool(updateAfterBind);
    }
    auto set = freeSets.back();
    freeSets.pop_back();
    return MakeRC<VulkanBindingGroupInstance>(this, set);
}

VkDescriptorSetLayout VulkanBindingGroupLayout::GetLayout() const
{
    return layout_;
}

void VulkanBindingGroupLayout::ReleaseSet(VkDescriptorSet set)
{
    freeSetsRegular_.push_back(set);
}

void VulkanBindingGroupLayout::TransferNode(std::list<PoolInfo> &from, std::list<PoolInfo> &to, std::list<PoolInfo>::iterator iter)
{
    to.splice(to.begin(), from, iter);
}

void VulkanBindingGroupLayout::AllocateNewDescriptorPool(bool updateAfterBind)
{
    const int existedPoolCount = updateAfterBind ? poolCountUpdateAfterBind_ : poolCountRegular_;
    const int maxSets = (std::min)(8192, 64 << (existedPoolCount >> 1));

    VkDescriptorPoolCreateFlags flags = 0;
    if(updateAfterBind)
    {
        flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    }

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
        .flags         = flags,
        .maxSets       = static_cast<uint32_t>(maxSets),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes    = poolSizes.data()
    };
    VkDescriptorPool pool;
    VK_FAIL_MSG(
        vkCreateDescriptorPool(device_, &createInfo, VK_ALLOC, &pool),
        "failed to create vulkan descriptor pool");
    RTRC_SCOPE_FAIL{ vkDestroyDescriptorPool(device_, pool, VK_ALLOC); };

    auto &freeSets = updateAfterBind ? freeSetsRegular_ : freeSetsUpdateAfterBind_;
    freeSets.reserve(freeSets.size() + maxSets);
    const size_t oldSize = freeSets.size();
    RTRC_SCOPE_FAIL{ freeSets.resize(oldSize); };

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
        freeSets.push_back(set);
    }

    pools_.push_back(pool);

    if(updateAfterBind)
    {
        ++poolCountUpdateAfterBind_;
    }
    else
    {
        ++poolCountRegular_;
    }
}

RTRC_RHI_VK_END
