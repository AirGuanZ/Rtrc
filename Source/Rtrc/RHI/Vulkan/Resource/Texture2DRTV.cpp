#include <Rtrc/RHI/Vulkan/Resource/Texture2DRTV.h>

RTRC_RHI_VK_BEGIN

VulkanTexture2DRTV::VulkanTexture2DRTV(const VulkanTexture2D *tex, const Texture2DRTVDesc &desc, VkImageView imageView)
    : tex_(tex), desc_(desc), imageView_(imageView)
{
    
}

const Texture2DRTVDesc &VulkanTexture2DRTV::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTexture2DRTV::GetNativeImageView() const
{
    return imageView_;
}

const VulkanTexture2D *VulkanTexture2DRTV::GetTexture() const
{
    return tex_;
}

RTRC_RHI_VK_END
