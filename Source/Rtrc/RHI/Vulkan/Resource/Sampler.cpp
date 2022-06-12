#include <Rtrc/RHI/Vulkan/Resource/Sampler.h>

RTRC_RHI_VK_BEGIN

VulkanSampler::VulkanSampler(const SamplerDesc &desc, VkDevice device, VkSampler sampler)
    : desc_(desc), device_(device), sampler_(sampler)
{
    
}

VulkanSampler::~VulkanSampler()
{
    assert(device_ && sampler_);
    vkDestroySampler(device_, sampler_, VK_ALLOC);
}

const SamplerDesc &VulkanSampler::GetDesc() const
{
    return desc_;
}

VkSampler VulkanSampler::GetNativeSampler() const
{
    return sampler_;
}

RTRC_RHI_VK_END
