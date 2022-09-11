#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureRTV.h>

RTRC_RHI_VK_BEGIN

VulkanTextureRTV::VulkanTextureRTV(const VulkanTexture *tex, const TextureRTVDesc &desc, VkImageView imageView)
    : tex_(tex), desc_(desc), imageView_(imageView)
{
    
}

const TextureRTVDesc &VulkanTextureRTV::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTextureRTV::GetNativeImageView() const
{
    return imageView_;
}

const VulkanTexture *VulkanTextureRTV::GetTexture() const
{
    return tex_;
}

RTRC_RHI_VK_END
