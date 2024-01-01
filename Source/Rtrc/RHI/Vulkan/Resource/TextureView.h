#pragma once

#include <Rtrc/RHI/Vulkan/Resource/Texture.h>

RTRC_RHI_VK_BEGIN

class VulkanTexture;

RTRC_RHI_IMPLEMENT(VulkanTextureSrv, TextureSrv)
{
public:

    VulkanTextureSrv(const TextureSrvDesc &desc, VkImageView imageView);

    const TextureSrvDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    VkImageView _internalGetNativeImageView() const;

private:

    TextureSrvDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_IMPLEMENT(VulkanTextureUav, TextureUav)
{
public:

    VulkanTextureUav(const TextureUavDesc &desc, VkImageView imageView);

    const TextureUavDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    VkImageView _internalGetNativeImageView() const;

private:

    TextureUavDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_IMPLEMENT(VulkanTextureRtv, TextureRtv)
{
public:

    VulkanTextureRtv(const VulkanTexture *tex, const TextureRtvDesc &desc, VkImageView imageView);

    const TextureRtvDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    VkImageView _internalGetNativeImageView() const;

    const VulkanTexture *_internalGetTexture() const;

private:

    const VulkanTexture *tex_;
    TextureRtvDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_IMPLEMENT(VulkanTextureDsv, TextureDsv)
{
public:

    VulkanTextureDsv(const VulkanTexture *tex, const TextureDsvDesc &desc, VkImageView imageView)
        : tex_(tex), desc_(desc), imageView_(imageView)
    {
        if(desc_.format == Format::Unknown)
        {
            desc_.format = tex_->GetDesc().format;
        }
    }

    const TextureDsvDesc &GetDesc() const RTRC_RHI_OVERRIDE { return desc_; }

    VkImageView _internalGetNativeImageView() const { return imageView_; }

    const VulkanTexture *_internalGetTexture() const { return tex_; }

private:

    const VulkanTexture *tex_;
    TextureDsvDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_VK_END
