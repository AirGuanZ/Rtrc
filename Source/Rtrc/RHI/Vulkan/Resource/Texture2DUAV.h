#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanTexture2DUAV : public Texture2DUAV
{
public:

    VulkanTexture2DUAV(const Texture2DUAVDesc &desc, VkImageView imageView);

    const Texture2DUAVDesc &GetDesc() const override;

    VkImageView GetNativeImageView() const;

private:

    Texture2DUAVDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_VK_END
