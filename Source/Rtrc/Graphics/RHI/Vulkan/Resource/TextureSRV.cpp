#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureSRV.h>

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

RTRC_RHI_VK_END
