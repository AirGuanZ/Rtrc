#include <Core/Enumerate.h>
#include <RHI/Vulkan/Pipeline/BindingLayout.h>
#include <RHI/Vulkan/Pipeline/BindingGroupLayout.h>

RTRC_RHI_VK_BEGIN

VulkanBindingLayout::VulkanBindingLayout(BindingLayoutDesc desc, VkDevice device, VkPipelineLayout layout)
    : desc_(std::move(desc)), device_(device), layout_(layout)
{

}

VulkanBindingLayout::~VulkanBindingLayout()
{
    assert(device_ && layout_);
    vkDestroyPipelineLayout(device_, layout_, RTRC_VK_ALLOC);
}

VkPipelineLayout VulkanBindingLayout::_internalGetNativeLayout() const
{
    return layout_;
}

RTRC_RHI_VK_END
