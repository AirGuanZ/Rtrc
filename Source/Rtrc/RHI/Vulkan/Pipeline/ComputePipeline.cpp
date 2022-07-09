#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/Utils/ScopeGuard.h>

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

VkPipeline VulkanComputePipeline::GetNativePipeline() const
{
    return pipeline_;
}

VulkanComputePipelineBuilder::VulkanComputePipelineBuilder(VkDevice device)
    : device_(device)
{
    
}

ComputePipelineBuilder &VulkanComputePipelineBuilder::SetComputeShader(Ptr<RawShader> shader)
{
    computeShader_ = std::move(shader);
    return *this;
}

ComputePipelineBuilder &VulkanComputePipelineBuilder::SetBindingLayout(Ptr<BindingLayout> layout)
{
    bindingLayout_ = std::move(layout);
    return *this;
}

Ptr<ComputePipeline> VulkanComputePipelineBuilder::CreatePipeline() const
{
    const VkComputePipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = static_cast<VulkanShader *>(computeShader_.Get())->GetStageCreateInfo(),
        .layout = static_cast<VulkanBindingLayout *>(bindingLayout_.Get())->GetNativeLayout()
    };
    VkPipeline pipeline;
    VK_FAIL_MSG(
        vkCreateComputePipelines(device_, nullptr, 1, &pipelineCreateInfo, VK_ALLOC, &pipeline),
        "failed to create vulkan compute pipeline");
    RTRC_SCOPE_FAIL{ vkDestroyPipeline(device_, pipeline, VK_ALLOC); };
    return MakePtr<VulkanComputePipeline>(bindingLayout_, device_, pipeline);
}

RTRC_RHI_VK_END
