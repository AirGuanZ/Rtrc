#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanSampler, Sampler)
{
public:

    RTRC_VK_SET_OBJECT_NAME(device_, sampler_, VK_OBJECT_TYPE_SAMPLER)

    VulkanSampler(const SamplerDesc &desc, VulkanDevice *device, VkSampler sampler);

    ~VulkanSampler() override;

    const SamplerDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    VkSampler _internalGetNativeSampler() const;

private:

    SamplerDesc desc_;
    VulkanDevice *device_;
    VkSampler sampler_;
};

RTRC_RHI_VK_END
