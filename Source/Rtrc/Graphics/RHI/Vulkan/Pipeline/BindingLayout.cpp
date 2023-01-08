#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingGroupLayout.h>
#include <Rtrc/Utility/Enumerate.h>

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

VkPipelineLayout VulkanBindingLayout::_internalGetNativeLayout() const
{
    return layout_;
}

RTRC_RHI_VK_END
