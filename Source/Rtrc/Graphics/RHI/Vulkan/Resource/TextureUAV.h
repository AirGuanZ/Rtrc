#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanTextureUAV : public TextureUAV
{
public:

    VulkanTextureUAV(const TextureUAVDesc &desc, VkImageView imageView);

    const TextureUAVDesc &GetDesc() const override;

    VkImageView GetNativeImageView() const;

private:

    TextureUAVDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_VK_END
