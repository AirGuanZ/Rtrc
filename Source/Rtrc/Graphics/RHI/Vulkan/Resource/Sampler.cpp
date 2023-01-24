#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Sampler.h>

RTRC_RHI_VK_BEGIN

VulkanSampler::VulkanSampler(const SamplerDesc &desc, VulkanDevice *device, VkSampler sampler)
    : desc_(desc), device_(device), sampler_(sampler)
{
    
}

VulkanSampler::~VulkanSampler()
{
    assert(device_ && sampler_);
    vkDestroySampler(device_->_internalGetNativeDevice(), sampler_, RTRC_VK_ALLOC);
}

const SamplerDesc &VulkanSampler::GetDesc() const
{
    return desc_;
}

VkSampler VulkanSampler::_internalGetNativeSampler() const
{
    return sampler_;
}

RTRC_RHI_VK_END
