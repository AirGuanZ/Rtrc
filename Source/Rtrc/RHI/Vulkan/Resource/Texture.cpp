#include <ranges>
#include <shared_mutex>

#include <Rtrc/Core/ScopeGuard.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture.h>
#include <Rtrc/RHI/Vulkan/Resource/TextureView.h>

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
    for(VkImageView view : std::views::values(views_))
    {
        vkDestroyImageView(device_->_internalGetNativeDevice(), view, RTRC_VK_ALLOC);
    }
    if(ownership_ == ResourceOwnership::Allocation)
    {
        vmaDestroyImage(alloc_.allocator, image_, alloc_.allocation);
    }
    else if(ownership_ == ResourceOwnership::Resource)
    {
        vkDestroyImage(device_->_internalGetNativeDevice(), image_, RTRC_VK_ALLOC);
    }
}

const TextureDesc & VulkanTexture::GetDesc() const
{
    return desc_;
}

RPtr<TextureRtv> VulkanTexture::CreateRtv(const TextureRtvDesc &_desc) const
{
    auto desc = _desc;
    if(desc.format == Format::Unknown)
    {
        desc.format = desc_.format;
    }

    auto imageView = CreateImageView(ViewKey{
        .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
        .format         = TranslateTexelFormat(desc.format),
        .baseMipLevel   = desc.mipLevel,
        .levelCount     = 1,
        .baseArrayLayer = desc.arrayLayer,
        .layerCount     = 1
    });
    return MakeRPtr<VulkanTextureRtv>(this, desc, imageView);
}

RPtr<TextureSrv> VulkanTexture::CreateSrv(const TextureSrvDesc &_desc) const
{
    auto desc = _desc;
    if(desc.format == Format::Unknown)
    {
        desc.format = desc_.format;
    }

    const VkImageAspectFlags aspect =
        HasDepthAspect(desc_.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    auto imageView = CreateImageView(ViewKey{
        .isArray        = desc.isArray,
        .aspect         = aspect,
        .format         = TranslateTexelFormat(desc.format),
        .baseMipLevel   = desc.baseMipLevel,
        .levelCount     = desc.levelCount > 0 ? desc.levelCount : desc_.mipLevels,
        .baseArrayLayer = desc.baseArrayLayer,
        .layerCount     = desc.layerCount > 0 ? desc.layerCount : desc_.arraySize
    });
    return MakeRPtr<VulkanTextureSrv>(desc, imageView);
}

RPtr<TextureUav> VulkanTexture::CreateUav(const TextureUavDesc &_desc) const
{
    auto desc = _desc;
    if(desc.format == Format::Unknown)
    {
        desc.format = desc_.format;
    }

    auto imageView = CreateImageView(ViewKey{
        .isArray        = desc.isArray,
        .aspect         = VK_IMAGE_ASPECT_COLOR_BIT,
        .format         = TranslateTexelFormat(desc.format),
        .baseMipLevel   = desc.mipLevel,
        .levelCount     = 1,
        .baseArrayLayer = desc.baseArrayLayer,
        .layerCount     = desc.layerCount > 0 ? desc.layerCount : desc_.arraySize
    });
    return MakeRPtr<VulkanTextureUav>(desc, imageView);
}

RPtr<TextureDsv> VulkanTexture::CreateDsv(const TextureDsvDesc &_desc) const
{
    auto desc = _desc;
    if(desc.format == Format::Unknown)
    {
        desc.format = desc_.format;
    }

    const VkImageAspectFlags aspect =
        (HasDepthAspect(desc_.format) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
        (HasStencilAspect(desc_.format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
    auto imageView = CreateImageView(ViewKey
    {
        .aspect         = aspect,
        .format         = TranslateTexelFormat(desc.format),
        .baseMipLevel   = desc.mipLevel,
        .levelCount     = 1,
        .baseArrayLayer = desc.arrayLayer,
        .layerCount     = 1
    });
    return MakeRPtr<VulkanTextureDsv>(this, desc, imageView);
}

VkImage VulkanTexture::_internalGetNativeImage() const
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
    case TextureDimension::Tex1D:
        viewType = key.isArray ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        break;
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
    RTRC_VK_FAIL_MSG(
        vkCreateImageView(device_->_internalGetNativeDevice(), &createInfo, RTRC_VK_ALLOC, &imageView),
        "failed to create vulkan image view");
    RTRC_SCOPE_FAIL{ vkDestroyImageView(device_->_internalGetNativeDevice(), imageView, RTRC_VK_ALLOC); };
    
    views_[key] = imageView;
    return imageView;
}

RTRC_RHI_VK_END
