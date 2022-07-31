#pragma once

#include <map>

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanTexture : public Texture
{
public:

    virtual VkImage GetNativeImage() const = 0;
};

class VulkanTexture2D : public VulkanTexture
{
public:

    VK_SET_OBJECT_NAME(device_, image_, VK_OBJECT_TYPE_IMAGE)

    VulkanTexture2D(
        const Texture2DDesc          &desc,
        VulkanDevice                 *device,
        VkImage                       image,
        const VulkanMemoryAllocation &alloc,
        ResourceOwnership             ownership);

    ~VulkanTexture2D() override;

    TextureDimension GetDimension() const override;

    const Texture2DDesc &Get2DDesc() const override;

    Format GetFormat() const override;

    Ptr<Texture2DRTV> Create2DRTV(const Texture2DRTVDesc &desc) const override;

    Ptr<Texture2DSRV> Create2DSRV(const Texture2DSRVDesc &desc) const override;

    Ptr<Texture2DUAV> Create2DUAV(const Texture2DUAVDesc &desc) const override;

    VkImage GetNativeImage() const override;

private:

    struct ViewKey
    {
        VkImageAspectFlags aspect;
        VkFormat           format;
        uint32_t           baseMipLevel;
        uint32_t           levelCount;
        uint32_t           baseArrayLayer;
        uint32_t           layerCount;

        auto operator<=>(const ViewKey &) const = default;
    };

    VkImageView CreateImageView(const ViewKey &key) const;

    Texture2DDesc                          desc_;
    VulkanDevice                          *device_;
    VkImage                                image_;
    VulkanMemoryAllocation                 alloc_;
    ResourceOwnership                      ownership_;
    mutable std::map<ViewKey, VkImageView> views_;
};

RTRC_RHI_VK_END
