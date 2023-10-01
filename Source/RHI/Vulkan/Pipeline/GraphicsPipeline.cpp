#include <iterator>

#include <RHI/Vulkan/Pipeline/BindingLayout.h>
#include <RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Core/Enumerate.h>

RTRC_RHI_VK_BEGIN

VulkanGraphicsPipeline::VulkanGraphicsPipeline(Ptr<BindingLayout> layout, VkDevice device, VkPipeline pipeline)
    : layout_(std::move(layout)), device_(device), pipeline_(pipeline)
{
    
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    assert(pipeline_);
    vkDestroyPipeline(device_, pipeline_, RTRC_VK_ALLOC);
}

const Ptr<BindingLayout> &VulkanGraphicsPipeline::GetBindingLayout() const
{
    return layout_;
}

VkPipeline VulkanGraphicsPipeline::_internalGetNativePipeline() const
{
    return pipeline_;
}

RTRC_RHI_VK_END
