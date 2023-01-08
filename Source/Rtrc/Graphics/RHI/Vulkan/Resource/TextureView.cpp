#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureView.h>

RTRC_RHI_VK_BEGIN

VulkanTextureSrv::VulkanTextureSrv(const TextureSrvDesc &desc, VkImageView imageView)
    : desc_(desc), imageView_(imageView)
{

}

const TextureSrvDesc &VulkanTextureSrv::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTextureSrv::_internalGetNativeImageView() const
{
    return imageView_;
}

VulkanTextureUav::VulkanTextureUav(const TextureUavDesc &desc, VkImageView imageView)
    : desc_(desc), imageView_(imageView)
{

}

const TextureUavDesc &VulkanTextureUav::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTextureUav::_internalGetNativeImageView() const
{
    return imageView_;
}

VulkanTextureRtv::VulkanTextureRtv(const VulkanTexture *tex, const TextureRtvDesc &desc, VkImageView imageView)
    : tex_(tex), desc_(desc), imageView_(imageView)
{
    if(desc_.format == Format::Unknown)
    {
        desc_.format = tex_->GetDesc().format;
    }
}

const TextureRtvDesc &VulkanTextureRtv::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTextureRtv::_internalGetNativeImageView() const
{
    return imageView_;
}

const VulkanTexture *VulkanTextureRtv::_internalGetTexture() const
{
    return tex_;
}

RTRC_RHI_VK_END
