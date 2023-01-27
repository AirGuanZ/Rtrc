#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/RayTracingPipeline.h>

RTRC_RHI_VK_BEGIN

VulkanRayTracingPipeline::VulkanRayTracingPipeline(Ptr<BindingLayout> layout, VulkanDevice *device, VkPipeline pipeline)
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

const Ptr<BindingLayout> &VulkanRayTracingPipeline::GetBindingLayout() const
{
    return layout_;
}

VkPipeline VulkanRayTracingPipeline::_internalGetNativePipeline() const
{
    return pipeline_;
}

RTRC_RHI_VK_END
