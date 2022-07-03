#include <Rtrc/RHI/Vulkan/Resource/Texture2DUAV.h>

RTRC_RHI_VK_BEGIN

VulkanTexture2DUAV::VulkanTexture2DUAV(const Texture2DUAVDesc &desc, VkImageView imageView)
    : desc_(desc), imageView_(imageView)
{
    
}

const Texture2DUAVDesc &VulkanTexture2DUAV::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTexture2DUAV::GetNativeImageView() const
{
    return imageView_;
}

RTRC_RHI_VK_END
