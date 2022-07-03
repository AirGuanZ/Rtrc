#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Utils/Enumerate.h>

RTRC_RHI_VK_BEGIN

VulkanBindingLayout::VulkanBindingLayout(const BindingLayoutDesc &desc, VkDevice device, VkPipelineLayout layout)
    : desc_(desc), device_(device), layout_(layout)
{

}

VulkanBindingLayout::~VulkanBindingLayout()
{
    assert(device_ && layout_);
    vkDestroyPipelineLayout(device_, layout_, VK_ALLOC);
}

VkPipelineLayout VulkanBindingLayout::GetNativeLayout() const
{
    return layout_;
}

RTRC_RHI_VK_END
