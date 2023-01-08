#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/Utility/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanComputePipeline::VulkanComputePipeline(Ptr<BindingLayout> layout, VkDevice device, VkPipeline pipeline)
    : layout_(std::move(layout)), device_(device), pipeline_(pipeline)
{
    
}

VulkanComputePipeline::~VulkanComputePipeline()
{
    assert(device_ && pipeline_);
    vkDestroyPipeline(device_, pipeline_, VK_ALLOC);
}

const Ptr<BindingLayout> &VulkanComputePipeline::GetBindingLayout() const
{
    return layout_;
}

VkPipeline VulkanComputePipeline::_internalGetNativePipeline() const
{
    return pipeline_;
}

RTRC_RHI_VK_END
