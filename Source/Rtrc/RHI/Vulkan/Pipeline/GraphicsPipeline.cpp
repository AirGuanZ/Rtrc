#include <iterator>

#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/Core/Enumerate.h>

RTRC_RHI_VK_BEGIN

VulkanGraphicsPipeline::VulkanGraphicsPipeline(OPtr<BindingLayout> layout, VkDevice device, VkPipeline pipeline)
    : layout_(std::move(layout)), device_(device), pipeline_(pipeline)
{
    
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    assert(pipeline_);
    vkDestroyPipeline(device_, pipeline_, RTRC_VK_ALLOC);
}

const OPtr<BindingLayout> &VulkanGraphicsPipeline::GetBindingLayout() const
{
    return layout_;
}

VkPipeline VulkanGraphicsPipeline::_internalGetNativePipeline() const
{
    return pipeline_;
}

RTRC_RHI_VK_END
