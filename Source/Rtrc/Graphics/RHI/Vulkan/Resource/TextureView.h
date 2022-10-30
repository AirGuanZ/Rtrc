#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

#include "Texture.h"

RTRC_RHI_VK_BEGIN
    class VulkanTexture;

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

class VulkanTextureRTV : public TextureRTV
{
public:

    VulkanTextureRTV(const VulkanTexture *tex, const TextureRTVDesc &desc, VkImageView imageView);

    const TextureRTVDesc &GetDesc() const override;

    VkImageView GetNativeImageView() const;

    const VulkanTexture *GetTexture() const;

private:

    const VulkanTexture *tex_;
    TextureRTVDesc desc_;
    VkImageView imageView_;
};

class VulkanTextureDSV : public TextureDSV
{
public:

    VulkanTextureDSV(const VulkanTexture *tex, const TextureDSVDesc &desc, VkImageView imageView)
        : tex_(tex), desc_(desc), imageView_(imageView)
    {
        if(desc_.format == Format::Unknown)
        {
            desc_.format = tex_->GetDesc().format;
        }
    }

    const TextureDSVDesc &GetDesc() const override { return desc_; }

    VkImageView GetNativeImageView() const { return imageView_; }

    const VulkanTexture *GetTexture() const { return tex_; }

private:

    const VulkanTexture *tex_;
    TextureDSVDesc desc_;
    VkImageView imageView_;
};

RTRC_RHI_VK_END
