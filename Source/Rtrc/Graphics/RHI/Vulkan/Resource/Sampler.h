#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanSampler : public Sampler
{
public:

    VK_SET_OBJECT_NAME(device_, sampler_, VK_OBJECT_TYPE_SAMPLER)

    VulkanSampler(const SamplerDesc &desc, VulkanDevice *device, VkSampler sampler);

    ~VulkanSampler() override;

    const SamplerDesc &GetDesc() const override;

    VkSampler GetNativeSampler() const;

private:

    SamplerDesc desc_;
    VulkanDevice *device_;
    VkSampler sampler_;
};

RTRC_RHI_VK_END
