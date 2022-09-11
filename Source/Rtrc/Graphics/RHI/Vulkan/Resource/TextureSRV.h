#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanTextureSRV : public TextureSRV
{
public:

    VulkanTextureSRV(const TextureSRVDesc &desc, VkImageView imageView);

    const TextureSRVDesc &GetDesc() const override;

    VkImageView GetNativeImageView() const;

private:

    TextureSRVDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_VK_END
