#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanTexture2D;

class VulkanTexture2DRTV : public Texture2DRTV
{
public:

    VulkanTexture2DRTV(const VulkanTexture2D *tex, const Texture2DRTVDesc &desc, VkImageView imageView);

    const Texture2DRTVDesc &GetDesc() const override;

    VkImageView GetNativeImageView() const;

    const VulkanTexture2D *GetTexture() const;

private:

    const VulkanTexture2D *tex_;
    Texture2DRTVDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_VK_END
