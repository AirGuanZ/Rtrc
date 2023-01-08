#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>

#include <Rtrc/Graphics/RHI/RHIDeclaration.h>

#define RTRC_RHI_VK_BEGIN RTRC_RHI_BEGIN namespace Vk {
#define RTRC_RHI_VK_END   } RTRC_RHI_END

RTRC_RHI_VK_BEGIN

struct VkResultChecker
{
    VkResult result;
    template<typename F>
    void operator+(F &&f)
    {
        if(result != VK_SUCCESS)
        {
            std::invoke(std::forward<F>(f), result);
        }
    }
};

#define VK_CHECK(RESULT) ::Rtrc::RHI::Vk::VkResultChecker{(RESULT)}+[&](VkResult errorCode)->void
#define VK_FAIL_MSG(RESULT, MSG) do { VK_CHECK(RESULT) { throw ::Rtrc::Exception(MSG); }; } while(false)

extern VkAllocationCallbacks RtrcGlobalVulkanAllocationCallbacks;
#define VK_ALLOC (&Rtrc::RHI::Vk::RtrcGlobalVulkanAllocationCallbacks)

#define RTRC_VULKAN_API_VERSION VK_API_VERSION_1_3

VkImageType              TranslateTextureDimension      (TextureDimension dim);
VkFormat                 TranslateTexelFormat           (Format format);
VkShaderStageFlagBits    TranslateShaderType            (ShaderStage type);
VkShaderStageFlags       TranslateShaderStageFlag       (EnumFlags<ShaderStage> flag);
VkPrimitiveTopology      TranslatePrimitiveTopology     (PrimitiveTopology topology);
VkPolygonMode            TranslateFillMode              (FillMode mode);
VkCullModeFlags          TranslateCullMode              (CullMode mode);
VkFrontFace              TranslateFrontFaceMode         (FrontFaceMode mode);
VkSampleCountFlagBits    TranslateSampleCount           (int count);
VkCompareOp              TranslateCompareOp             (CompareOp op);
VkStencilOp              TranslateStencilOp             (StencilOp op);
VkBlendFactor            TranslateBlendFactor           (BlendFactor factor);
VkBlendOp                TranslateBlendOp               (BlendOp op);
VkDescriptorType         TranslateBindingType           (BindingType type);
VkImageUsageFlags        TranslateTextureUsageFlag      (TextureUsageFlag flag);
VkPipelineStageFlags2    TranslatePipelineStageFlag     (PipelineStageFlag flag);
VkBufferUsageFlags       TranslateBufferUsageFlag       (BufferUsageFlag flag);
VkAttachmentLoadOp       TranslateLoadOp                (AttachmentLoadOp op);
VkAttachmentStoreOp      TranslateStoreOp               (AttachmentStoreOp op);
VkClearColorValue        TranslateClearColorValue       (const ColorClearValue &value);
VkClearDepthStencilValue TranslateClearDepthStencilValue(const DepthStencilClearValue &value);
VkClearValue             TranslateClearValue            (const ClearValue &value);
VkViewport               TranslateViewport              (const Viewport &viewport);
VkRect2D                 TranslateScissor               (const Scissor &scissor);
VmaAllocationCreateFlags TranslateBufferHostAccessType  (BufferHostAccessType type);
VkFilter                 TranslateSamplerFilterMode     (FilterMode mode);
VkSamplerMipmapMode      TranslateSamplerMipmapMode     (FilterMode mode);
VkSamplerAddressMode     TranslateSamplerAddressMode    (AddressMode mode);
VkAccessFlags2           TranslateAccessFlag            (ResourceAccessFlag flag);
VkImageLayout            TranslateImageLayout           (TextureLayout layout);
VkImageSubresourceRange  TranslateImageSubresources     (Format format, const TextureSubresources &subresources);
VkFormat                 TranslateInputAttributeType    (VertexAttributeType type);

bool HasColorAspect(Format format);
bool HasDepthStencilAspect(Format format);
VkImageAspectFlags GetAllAspects(Format format);

struct VulkanMemoryAllocation
{
    VmaAllocator  allocator;
    VmaAllocation allocation;
};

enum class ResourceOwnership
{
    Allocation,
    Resource,
    None
};

class VulkanDevice;

#if VK_USE_64_BIT_PTR_DEFINES
using VkCommonHandle = void *;
#else
using VkCommonHandle = uint64_t;
#endif

void SetObjectName(VulkanDevice *device, VkObjectType type, VkCommonHandle object, const char *name);

#define VK_SET_OBJECT_NAME(DEVICE, OBJECT, TYPE)                                            \
    void SetName(std::string name) override                                                 \
    {                                                                                       \
        RHIObject::SetName(std::move(name));                                                \
        ::Rtrc::RHI::Vk::SetObjectName(DEVICE, TYPE, OBJECT, RHIObject::GetName().c_str()); \
    }

RTRC_RHI_VK_END
