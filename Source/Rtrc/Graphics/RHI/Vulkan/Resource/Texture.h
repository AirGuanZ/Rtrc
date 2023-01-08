#pragma once

#include <map>

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanTexture, Texture)
{
public:

#ifdef RTRC_STATIC_RHI
    RTRC_RHI_TEXTURE_COMMON
#endif

    VK_SET_OBJECT_NAME(device_, image_, VK_OBJECT_TYPE_IMAGE)

    VulkanTexture(
        const TextureDesc          &desc,
        VulkanDevice                 *device,
        VkImage                       image,
        const VulkanMemoryAllocation &alloc,
        ResourceOwnership             ownership);

    ~VulkanTexture() override;

    const TextureDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    Ptr<TextureRtv> CreateRtv(const TextureRtvDesc &desc) const RTRC_RHI_OVERRIDE;
    Ptr<TextureSrv> CreateSrv(const TextureSrvDesc &desc) const RTRC_RHI_OVERRIDE;
    Ptr<TextureUav> CreateUav(const TextureUavDesc &desc) const RTRC_RHI_OVERRIDE;
    Ptr<TextureDsv> CreateDsv(const TextureDsvDesc &desc) const RTRC_RHI_OVERRIDE;

    VkImage _internalGetNativeImage() const;

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
    mutable tbb::spin_rw_mutex             viewsMutex_;

};

RTRC_RHI_VK_END
