#include <ranges>
#include <shared_mutex>

#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureRTV.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureSRV.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureUAV.h>
#include <Rtrc/Utils/ScopeGuard.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_VK_BEGIN

VulkanTexture::VulkanTexture(
    const TextureDesc          &desc,
    VulkanDevice                 *device,
    VkImage                       image,
    const VulkanMemoryAllocation &alloc,
    ResourceOwnership             ownership)
    : desc_(desc), device_(device), image_(image), alloc_(alloc), ownership_(ownership)
{

}

VulkanTexture::~VulkanTexture()
{
    for(VkImageView view : std::ranges::views::values(views_))
    {
        vkDestroyImageView(device_->GetNativeDevice(), view, VK_ALLOC);
    }
    if(ownership_ == ResourceOwnership::Allocation)
    {
        vmaDestroyImage(alloc_.allocator, image_, alloc_.allocation);
    }
    else if(ownership_ == ResourceOwnership::Resource)
    {
        vkDestroyImage(device_->GetNativeDevice(), image_, VK_ALLOC);
    }
}

const TextureDesc & VulkanTexture::GetDesc() const
{
    return desc_;
}

Ptr<TextureRTV> VulkanTexture::CreateRTV(const TextureRTVDesc &desc) const
{
    auto imageView = CreateImageView(ViewKey{
        .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
        .format         = TranslateTexelFormat(desc.format == Format::Unknown ? desc_.format : desc.format),
        .baseMipLevel   = desc.mipLevel,
        .levelCount     = 1,
        .baseArrayLayer = desc.arrayLayer,
        .layerCount     = 1
    });
    return MakePtr<VulkanTextureRTV>(this, desc, imageView);
}

Ptr<TextureSRV> VulkanTexture::CreateSRV(const TextureSRVDesc &desc) const
{
    auto imageView = CreateImageView(ViewKey{
        .isArray        = desc.isArray,
        .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
        .format         = TranslateTexelFormat(desc.format == Format::Unknown ? desc_.format : desc.format),
        .baseMipLevel   = desc.baseMipLevel,
        .levelCount     = desc.levelCount > 0 ? desc.levelCount : desc_.mipLevels,
        .baseArrayLayer = desc.baseArrayLayer,
        .layerCount     = desc.layerCount > 0 ? desc.layerCount : desc_.arraySize
    });
    return MakePtr<VulkanTextureSRV>(desc, imageView);
}

Ptr<TextureUAV> VulkanTexture::CreateUAV(const TextureUAVDesc &desc) const
{
    auto imageView = CreateImageView(ViewKey{
        .isArray        = desc.isArray,
        .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
        .format         = TranslateTexelFormat(desc.format == Format::Unknown ? desc_.format : desc.format),
        .baseMipLevel   = desc.mipLevel,
        .levelCount     = 1,
        .baseArrayLayer = desc.baseArrayLayer,
        .layerCount     = desc.layerCount > 0 ? desc.layerCount : desc_.arraySize
    });
    return MakePtr<VulkanTextureUAV>(desc, imageView);
}

VkImage VulkanTexture::GetNativeImage() const
{
    return image_;
}

VkImageView VulkanTexture::CreateImageView(const ViewKey &key) const
{
    {
        std::shared_lock lock(viewsMutex_);
        if(auto it = views_.find(key); it != views_.end())
        {
            return it->second;
        }
    }

    std::unique_lock lock(viewsMutex_);

    if(auto it = views_.find(key); it != views_.end())
    {
        return it->second;
    }

    VkImageViewType viewType;
    switch(desc_.dim)
    {
    case TextureDimension::Tex2D:
        viewType = key.isArray ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        break;
    case TextureDimension::Tex3D:
        assert(!key.isArray);
        viewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    default:
        Unreachable();
    }

    const VkImageViewCreateInfo createInfo = {
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = image_,
        .viewType   = viewType,
        .format     = key.format,
        .components = VkComponentMapping{
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY
        },
        .subresourceRange = VkImageSubresourceRange{
            .aspectMask     = key.aspect,
            .baseMipLevel   = key.baseMipLevel,
            .levelCount     = key.levelCount,
            .baseArrayLayer = key.baseArrayLayer,
            .layerCount     = key.layerCount
        }
    };

    VkImageView imageView;
    VK_FAIL_MSG(
        vkCreateImageView(device_->GetNativeDevice(), &createInfo, VK_ALLOC, &imageView),
        "failed to create vulkan image view");
    RTRC_SCOPE_FAIL{ vkDestroyImageView(device_->GetNativeDevice(), imageView, VK_ALLOC); };
    
    views_[key] = imageView;
    return imageView;
}

RTRC_RHI_VK_END
