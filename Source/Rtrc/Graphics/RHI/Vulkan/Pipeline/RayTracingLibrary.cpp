#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/RayTracingLibrary.h>

RTRC_RHI_VK_BEGIN

VulkanRayTracingLibrary::VulkanRayTracingLibrary(VulkanDevice *device, VkPipeline pipeline)
    : device_(device), pipeline_(pipeline)
{
    
}

VulkanRayTracingLibrary::~VulkanRayTracingLibrary()
{
    vkDestroyPipeline(device_->_internalGetNativeDevice(), pipeline_, RTRC_VK_ALLOC);
}

VkPipeline VulkanRayTracingLibrary::_internalGetNativePipeline() const
{
    return pipeline_;
}

RTRC_RHI_VK_END
