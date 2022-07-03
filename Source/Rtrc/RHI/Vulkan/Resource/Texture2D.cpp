#include <Rtrc/RHI/Vulkan/Resource/Texture2D.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2DRTV.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2DSRV.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2DUAV.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanTexture2D::VulkanTexture2D(
    const Texture2DDesc          &desc,
    VkDevice                      device,
    VkImage                       image,
    const VulkanMemoryAllocation &alloc,
    ResourceOwnership             ownership)
    : desc_(desc), device_(device), image_(image), alloc_(alloc), ownership_(ownership)
{

}

VulkanTexture2D::~VulkanTexture2D()
{
    for(auto &[_, view] : views_)
    {
        vkDestroyImageView(device_, view, VK_ALLOC);
    }

    if(ownership_ == ResourceOwnership::Allocation)
    {
        vmaDestroyImage(alloc_.allocator, image_, alloc_.allocation);
    }
    else if(ownership_ == ResourceOwnership::Resource)
    {
        vkDestroyImage(device_, image_, VK_ALLOC);
    }
}

TextureDimension VulkanTexture2D::GetDimension() const
{
    return TextureDimension::Tex2D;
}

const Texture2DDesc &VulkanTexture2D::Get2DDesc() const
{
    return desc_;
}

RC<Texture2DRTV> VulkanTexture2D::Create2DRTV(const Texture2DRTVDesc &desc) const
{
    auto imageView = CreateImageView(ViewKey{
        .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
        .format         = TranslateTexelFormat(desc.format == Format::Unknown ? desc_.format : desc.format),
        .baseMipLevel   = desc.mipLevel,
        .levelCount     = 1,
        .baseArrayLayer = desc.arrayLayer,
        .layerCount     = 1
    });
    return MakeRC<VulkanTexture2DRTV>(this, desc, imageView);
}

RC<Texture2DSRV> VulkanTexture2D::Create2DSRV(const Texture2DSRVDesc &desc) const
{
    auto imageView = CreateImageView(ViewKey{
        .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
        .format         = TranslateTexelFormat(desc.format == Format::Unknown ? desc_.format : desc.format),
        .baseMipLevel   = desc.baseMipLevel,
        .levelCount     = desc.levelCount,
        .baseArrayLayer = desc.baseArrayLayer,
        .layerCount     = desc.layerCount
    });
    return MakeRC<VulkanTexture2DSRV>(desc, imageView);
}

RC<Texture2DUAV> VulkanTexture2D::Create2DUAV(const Texture2DUAVDesc &desc) const
{
    auto imageView = CreateImageView(ViewKey{
        .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
        .format         = TranslateTexelFormat(desc.format == Format::Unknown ? desc_.format : desc.format),
        .baseMipLevel   = desc.baseMipLevel,
        .levelCount     = desc.levelCount,
        .baseArrayLayer = desc.baseArrayLayer,
        .layerCount     = desc.layerCount
    });
    return MakeRC<VulkanTexture2DUAV>(desc, imageView);
}

VkImage VulkanTexture2D::GetNativeImage() const
{
    return image_;
}

VkImageView VulkanTexture2D::CreateImageView(const ViewKey &key) const
{
    if(auto it = views_.find(key); it != views_.end())
    {
        return it->second;
    }

    const VkImageViewCreateInfo createInfo = {
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = image_,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
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
        vkCreateImageView(device_, &createInfo, VK_ALLOC, &imageView),
        "failed to create vulkan image view");
    RTRC_SCOPE_FAIL{ vkDestroyImageView(device_, imageView, VK_ALLOC); };
    
    views_[key] = imageView;
    return imageView;
}

RTRC_RHI_VK_END
