#include <iterator>

#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanGraphicsPipeline::VulkanGraphicsPipeline(Ptr<BindingLayout> layout, VkDevice device, VkPipeline pipeline)
    : layout_(std::move(layout)), device_(device), pipeline_(pipeline)
{
    
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    assert(pipeline_);
    vkDestroyPipeline(device_, pipeline_, VK_ALLOC);
}

const Ptr<BindingLayout> &VulkanGraphicsPipeline::GetBindingLayout() const
{
    return layout_;
}

VkPipeline VulkanGraphicsPipeline::GetNativePipeline() const
{
    return pipeline_;
}

RTRC_RHI_VK_END
