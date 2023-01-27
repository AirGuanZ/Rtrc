#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Pipeline/RayTracingLibrary.h>

RTRC_RHI_VK_BEGIN

VulkanRayTracingLibrary::VulkanRayTracingLibrary(
    VulkanDevice *device, VkPipeline pipeline, uint32_t maxPayloadSize, uint32_t maxHitAttribSize)
    : device_(device), pipeline_(pipeline), maxPayloadSize_(maxPayloadSize), maxHitAttribSize_(maxHitAttribSize)
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

uint32_t VulkanRayTracingLibrary::_internalGetMaxPayloadSize() const
{
    return maxPayloadSize_;
}

uint32_t VulkanRayTracingLibrary::_internalGetMaxHitAttributeSize() const
{
    return maxHitAttribSize_;
}

RTRC_RHI_VK_END
