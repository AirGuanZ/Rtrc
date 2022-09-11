#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanTexture;

class VulkanTextureRTV : public TextureRTV
{
public:

    VulkanTextureRTV(const VulkanTexture *tex, const TextureRTVDesc &desc, VkImageView imageView);

    const TextureRTVDesc &GetDesc() const override;

    VkImageView GetNativeImageView() const;

    const VulkanTexture *GetTexture() const;

private:

    const VulkanTexture *tex_;
    TextureRTVDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_VK_END
