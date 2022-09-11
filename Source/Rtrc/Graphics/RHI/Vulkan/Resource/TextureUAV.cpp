#include <Rtrc/Graphics/RHI/Vulkan/Resource/TextureUAV.h>

RTRC_RHI_VK_BEGIN

VulkanTextureUAV::VulkanTextureUAV(const TextureUAVDesc &desc, VkImageView imageView)
    : desc_(desc), imageView_(imageView)
{
    
}

const TextureUAVDesc &VulkanTextureUAV::GetDesc() const
{
    return desc_;
}

VkImageView VulkanTextureUAV::GetNativeImageView() const
{
    return imageView_;
}

RTRC_RHI_VK_END
