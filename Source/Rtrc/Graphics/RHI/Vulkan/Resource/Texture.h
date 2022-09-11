#pragma once

#include <map>

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanTexture : public Texture
{
public:

    VK_SET_OBJECT_NAME(device_, image_, VK_OBJECT_TYPE_IMAGE)

    VulkanTexture(
        const TextureDesc          &desc,
        VulkanDevice                 *device,
        VkImage                       image,
        const VulkanMemoryAllocation &alloc,
        ResourceOwnership             ownership);

    ~VulkanTexture() override;

    const TextureDesc &GetDesc() const override;

    Ptr<TextureRTV> CreateRTV(const TextureRTVDesc &desc) const override;
    Ptr<TextureSRV> CreateSRV(const TextureSRVDesc &desc) const override;
    Ptr<TextureUAV> CreateUAV(const TextureUAVDesc &desc) const override;

    VkImage GetNativeImage() const;

private:

    struct ViewKey
    {
        bool               isArray;
        VkImageAspectFlags aspect;
        VkFormat           format;
        uint32_t           baseMipLevel;
        uint32_t           levelCount;
        uint32_t           baseArrayLayer;
        uint32_t           layerCount;

        auto operator<=>(const ViewKey &) const = default;
    };

    VkImageView CreateImageView(const ViewKey &key) const;

    TextureDesc                            desc_;
    VulkanDevice                          *device_;
    VkImage                                image_;
    VulkanMemoryAllocation                 alloc_;
    ResourceOwnership                      ownership_;
    mutable std::map<ViewKey, VkImageView> views_;
};

RTRC_RHI_VK_END
