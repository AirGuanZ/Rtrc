#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Pipeline/RayTracingPipeline.h>

RTRC_RHI_VK_BEGIN

VulkanRayTracingPipeline::VulkanRayTracingPipeline(OPtr<BindingLayout> layout, VulkanDevice *device, VkPipeline pipeline)
    : layout_(std::move(layout)), device_(device), pipeline_(pipeline)
{
    
}

VulkanRayTracingPipeline::~VulkanRayTracingPipeline()
{
    if(pipeline_)
    {
        vkDestroyPipeline(device_->_internalGetNativeDevice(), pipeline_, RTRC_VK_ALLOC);
    }
}

const OPtr<BindingLayout> &VulkanRayTracingPipeline::GetBindingLayout() const
{
    return layout_;
}

void VulkanRayTracingPipeline::GetShaderGroupHandles(
    uint32_t               startGroupIndex,
    uint32_t               groupCount,
    MutSpan<unsigned char> outputData) const
{
    RTRC_VK_FAIL_MSG(
        vkGetRayTracingShaderGroupHandlesKHR(
            device_->_internalGetNativeDevice(), pipeline_,
            startGroupIndex, groupCount, outputData.size(), outputData.GetData()),
        "Failed to get Vulkan ray tracing shader group handles");
}

VkPipeline VulkanRayTracingPipeline::_internalGetNativePipeline() const
{
    return pipeline_;
}

RTRC_RHI_VK_END
