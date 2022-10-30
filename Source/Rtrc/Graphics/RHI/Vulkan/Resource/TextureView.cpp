#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureView.h>

RTRC_RHI_VK_BEGIN

VulkanTextureSRV::VulkanTextureSRV(const TextureSRVDesc &desc, VkImageView imageView)
    : desc_(desc), imageView_(imageView)
{

}

const TextureSRVDesc &VulkanTextureSRV::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTextureSRV::GetNativeImageView() const
{
    return imageView_;
}

VulkanTextureUAV::VulkanTextureUAV(const TextureUAVDesc &desc, VkImageView imageView)
    : desc_(desc), imageView_(imageView)
{

}

const TextureUAVDesc &VulkanTextureUAV::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTextureUAV::GetNativeImageView() const
{
    return imageView_;
}

VulkanTextureRTV::VulkanTextureRTV(const VulkanTexture *tex, const TextureRTVDesc &desc, VkImageView imageView)
    : tex_(tex), desc_(desc), imageView_(imageView)
{
    if(desc_.format == Format::Unknown)
    {
        desc_.format = tex_->GetDesc().format;
    }
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
