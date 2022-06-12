#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Utils/Enumerate.h>

RTRC_RHI_VK_BEGIN

VulkanBindingLayout::VulkanBindingLayout(const BindingLayoutDesc &desc, VkDevice device, VkPipelineLayout layout)
    : desc_(desc), device_(device), layout_(layout)
{
    for(auto &&[i, g] : Enumerate(desc.groups))
    {
        auto vkg = static_cast<const VulkanBindingGroupLayout *>(g.get());
        auto &typeIndex = vkg->GetGroupStructTypeIndex();
        if(typeIndex)
        {
            groupType2Index_[typeIndex] = static_cast<int>(i);
        }
    }
}

VulkanBindingLayout::~VulkanBindingLayout()
{
    assert(device_ && layout_);
    vkDestroyPipelineLayout(device_, layout_, VK_ALLOC);
}

int VulkanBindingLayout::GetGroupIndex(const TypeIndex &groupStructType) const
{
    auto it = groupType2Index_.find(groupStructType);
    return it != groupType2Index_.end() ? it->second : -1;
}

VkPipelineLayout VulkanBindingLayout::GetNativeLayout() const
{
    return layout_;
}

RTRC_RHI_VK_END
