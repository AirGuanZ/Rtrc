#include <Rtrc/RHI/Vulkan/Resource/Texture2DSRV.h>

RTRC_RHI_VK_BEGIN

VulkanTexture2DSRV::VulkanTexture2DSRV(const Texture2DSRVDesc &desc, VkImageView imageView)
    : desc_(desc), imageView_(imageView)
{
    
}

const Texture2DSRVDesc &VulkanTexture2DSRV::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTexture2DSRV::GetNativeImageView() const
{
    return imageView_;
}

RTRC_RHI_VK_END
