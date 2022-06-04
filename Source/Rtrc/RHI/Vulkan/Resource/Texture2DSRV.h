#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanTexture2DSRV : public Texture2DSRV
{
public:

    VulkanTexture2DSRV(const Texture2DSRVDesc &desc, VkImageView imageView);

    const Texture2DSRVDesc &GetDesc() const override;

    VkImageView GetImageView() const;

private:

    Texture2DSRVDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_VK_END
