#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanSampler : public Sampler
{
public:

    VulkanSampler(const SamplerDesc &desc, VkDevice device, VkSampler sampler);

    ~VulkanSampler() override;

    const SamplerDesc &GetDesc() const override;

    VkSampler GetNativeSampler() const;

private:

    SamplerDesc desc_;
    VkDevice device_;
    VkSampler sampler_;
};

RTRC_RHI_VK_END
