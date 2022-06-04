#define VMA_IMPLEMENTATION
#include <mimalloc.h>
#include <Rtrc/RHI/Vulkan/Common.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_VK_BEGIN

namespace
{

    void *VkAlloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope)
    {
        return mi_aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
    }

    void *VkRealloc(void *pUserData, void *orig, size_t size, size_t alignment, VkSystemAllocationScope scope)
    {
        return mi_realloc_aligned(orig, size, alignment);
    }

    void VkFree(void *pUserData, void *ptr)
    {
        mi_free(ptr);
    }

} // namespace anonymous

VkAllocationCallbacks RtrcGlobalVulkanAllocationCallbacks = {
    .pfnAllocation         = VkAlloc,
    .pfnReallocation       = VkRealloc,
    .pfnFree               = VkFree,
    .pfnInternalAllocation = nullptr,
    .pfnInternalFree       = nullptr
};

VkFormat TranslateTexelFormat(Format format)
{
    switch(format)
    {
    case Format::Unknown:        return VK_FORMAT_UNDEFINED;
    case Format::B8G8R8A8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;
    }
    Unreachable();
}

VkShaderStageFlagBits TranslateShaderType(ShaderStage type)
{
    switch(type)
    {
    case ShaderStage::VertexShader:   return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::FragmentShader: return VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    Unreachable();
}

VkShaderStageFlags TranslateShaderStageFlag(EnumFlags<ShaderStage> flags)
{
    VkShaderStageFlags result = 0;
    result |= flags.contains(ShaderStage::VertexShader) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
    result |= flags.contains(ShaderStage::FragmentShader) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
    return result;
}

VkPrimitiveTopology TranslatePrimitiveTopology(PrimitiveTopology topology)
{
    switch(topology)
    {
    case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    }
    Unreachable();
}

VkPolygonMode TranslateFillMode(FillMode mode)
{
    switch(mode)
    {
    case FillMode::Fill:  return VK_POLYGON_MODE_FILL;
    case FillMode::Line:  return VK_POLYGON_MODE_LINE;
    case FillMode::Point: return VK_POLYGON_MODE_POINT;
    }
    Unreachable();
}

VkCullModeFlags TranslateCullMode(CullMode mode)
{
    switch(mode)
    {
    case CullMode::DontCull:  return VK_CULL_MODE_NONE;
    case CullMode::CullFront: return VK_CULL_MODE_FRONT_BIT;
    case CullMode::CullBack:  return VK_CULL_MODE_BACK_BIT;
    case CullMode::CullAll:   return VK_CULL_MODE_FRONT_AND_BACK;
    }
    Unreachable();
}

VkFrontFace TranslateFrontFaceMode(FrontFaceMode mode)
{
    switch(mode)
    {
    case FrontFaceMode::Clockwise:        return VK_FRONT_FACE_CLOCKWISE;
    case FrontFaceMode::CounterClockwise: return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
    Unreachable();
}

VkSampleCountFlagBits TranslateSampleCount(int count)
{
    switch(count)
    {
    case 1:  return VK_SAMPLE_COUNT_1_BIT;
    case 2:  return VK_SAMPLE_COUNT_2_BIT;
    case 4:  return VK_SAMPLE_COUNT_4_BIT;
    case 8:  return VK_SAMPLE_COUNT_8_BIT;
    case 16: return VK_SAMPLE_COUNT_16_BIT;
    case 32: return VK_SAMPLE_COUNT_32_BIT;
    case 64: return VK_SAMPLE_COUNT_64_BIT;
    default: throw Exception("unsupported multisample count: " + std::to_string(count));
    }
}

VkCompareOp TranslateCompareOp(CompareOp op)
{
    switch(op)
    {
    case CompareOp::Less:         return VK_COMPARE_OP_LESS;
    case CompareOp::LessEqual:    return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::Equal:        return VK_COMPARE_OP_EQUAL;
    case CompareOp::NotEqual:     return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp::Greater:      return VK_COMPARE_OP_GREATER;
    case CompareOp::Always:       return VK_COMPARE_OP_ALWAYS;
    case CompareOp::Never:        return VK_COMPARE_OP_NEVER;
    }
    Unreachable();
}

VkStencilOp TranslateStencilOp(StencilOp op)
{
    switch(op)
    {
    case StencilOp::Keep:          return VK_STENCIL_OP_KEEP;
    case StencilOp::Zero:          return VK_STENCIL_OP_ZERO;
    case StencilOp::Replace:       return VK_STENCIL_OP_REPLACE;
    case StencilOp::IncreaseClamp: return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case StencilOp::DecreaseClamp: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case StencilOp::IncreaseWrap:  return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case StencilOp::DecreaseWrap:  return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    case StencilOp::Invert:        return VK_STENCIL_OP_INVERT;
    }
    Unreachable();
}

VkBlendFactor TranslateBlendFactor(BlendFactor factor)
{
    switch(factor)
    {
    case BlendFactor::Zero:             return VK_BLEND_FACTOR_ZERO;
    case BlendFactor::One:              return VK_BLEND_FACTOR_ONE;
    case BlendFactor::SrcAlpha:         return VK_BLEND_FACTOR_SRC_ALPHA;
    case BlendFactor::DstAlpha:         return VK_BLEND_FACTOR_DST_ALPHA;
    case BlendFactor::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case BlendFactor::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    }
    Unreachable();
}

VkBlendOp TranslateBlendOp(BlendOp op)
{
    switch(op)
    {
    case BlendOp::Add:        return VK_BLEND_OP_ADD;
    case BlendOp::Sub:        return VK_BLEND_OP_SUBTRACT;
    case BlendOp::SubReverse: return VK_BLEND_OP_REVERSE_SUBTRACT;
    case BlendOp::Min:        return VK_BLEND_OP_MIN;
    case BlendOp::Max:        return VK_BLEND_OP_MAX;
    }
    Unreachable();
}

VkDescriptorType TranslateBindingType(BindingType type)
{
    switch(type)
    {
    case BindingType::Texture:            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case BindingType::RWTexture:          return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case BindingType::Buffer:             return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case BindingType::RWBuffer:           return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    case BindingType::StructuredBuffer:
    case BindingType::RWStructuredBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case BindingType::ConstantBuffer:     return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case BindingType::Sampler:            return VK_DESCRIPTOR_TYPE_SAMPLER;
    }
    Unreachable();
}

VkImageUsageFlags TranslateTextureUsageFlag(TextureUsageFlag flags)
{
    VkImageUsageFlags result = 0;
    if(flags.contains(TextureUsage::TransferDst))
    {
        result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if(flags.contains(TextureUsage::TransferSrc))
    {
        result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if(flags.contains(TextureUsage::ShaderResource))
    {
        result |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if(flags.contains(TextureUsage::UnorderAccess))
    {
        result |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if(flags.contains(TextureUsage::RenderTarget))
    {
        result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if(flags.contains(TextureUsage::DepthStencil))
    {
        result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    return result;
}

VkImageLayout TranslateTextureLayout(TextureLayout layout)
{
    switch(layout)
    {
    case TextureLayout::Undefined:                     return VK_IMAGE_LAYOUT_UNDEFINED;
    case TextureLayout::RenderTarget:                  return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case TextureLayout::ShaderResource:                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case TextureLayout::DepthStencilReadWrite:         return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case TextureLayout::DepthStencilReadOnly:          return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case TextureLayout::DepthReadWriteStencilReadOnly: return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
    case TextureLayout::DepthReadOnlyStencilReadWrite: return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
    case TextureLayout::DepthReadWrite:                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    case TextureLayout::DepthReadOnly:                 return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
    case TextureLayout::StencilReadWrite:              return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
    case TextureLayout::StencilReadOnly:               return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
    case TextureLayout::TransferDst:                   return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case TextureLayout::TransferSrc:                   return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case TextureLayout::Present:                       return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    Unreachable();
}

VkPipelineStageFlags2 TranslatePipelineStageFlag(PipelineStageFlag flag)
{
    VkPipelineStageFlags2 result = 0;
#define ADD_CASE(FLAG, VAL) if(flag.contains(PipelineStage::FLAG)) { result |= VAL; }
    ADD_CASE(All,                   VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT)
    ADD_CASE(None,                  VK_PIPELINE_STAGE_2_NONE)
    ADD_CASE(DrawIndirect,          VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT)
    ADD_CASE(VertexInput,           VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT)
    ADD_CASE(IndexInput,            VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT)
    ADD_CASE(VertexShader,          VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT)
    ADD_CASE(FragmentShader,        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT)
    ADD_CASE(EarlyFragmentTests,    VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT)
    ADD_CASE(LateFragmentTests,     VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT)
    ADD_CASE(ColorAttachmentOutput, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
    ADD_CASE(ComputeShader,         VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)
    ADD_CASE(Copy,                  VK_PIPELINE_STAGE_2_COPY_BIT)
    ADD_CASE(Resolve,               VK_PIPELINE_STAGE_2_RESOLVE_BIT)
    ADD_CASE(Blit,                  VK_PIPELINE_STAGE_2_BLIT_BIT)
    ADD_CASE(Clear,                 VK_PIPELINE_STAGE_2_CLEAR_BIT)
    ADD_CASE(Host,                  VK_PIPELINE_STAGE_2_HOST_BIT)
#undef ADD_CASE
    return result;
}

VkAccessFlags2 TranslateAccessTypeFlag(AccessTypeFlag flag)
{
    VkAccessFlags2 result = 0;
#define ADD_CASE(FLAG, VAL) if(flag.contains(AccessType::FLAG)) { result |= VAL; }
    ADD_CASE(None,                    VK_ACCESS_2_NONE)
    ADD_CASE(IndirectCommandRead,     VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT)
    ADD_CASE(IndexRead,               VK_ACCESS_2_INDEX_READ_BIT)
    ADD_CASE(VertexAttributeRead,     VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT)
    ADD_CASE(UniformBufferRead,       VK_ACCESS_2_UNIFORM_READ_BIT)
    ADD_CASE(ImageRead,               VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)
    ADD_CASE(BufferRead,              VK_ACCESS_2_SHADER_SAMPLED_READ_BIT)
    ADD_CASE(StructuredBufferRead,    VK_ACCESS_2_SHADER_STORAGE_READ_BIT)
    ADD_CASE(RWImageRead,             VK_ACCESS_2_SHADER_STORAGE_READ_BIT)
    ADD_CASE(RWBufferRead,            VK_ACCESS_2_SHADER_STORAGE_READ_BIT)
    ADD_CASE(RWStructuredBufferRead,  VK_ACCESS_2_SHADER_STORAGE_READ_BIT)
    ADD_CASE(RWImageWrite,            VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)
    ADD_CASE(RWBufferWrite,           VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)
    ADD_CASE(RWStructuredBufferWrite, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT)
    ADD_CASE(ColorAttachmentRead,     VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT)
    ADD_CASE(ColorAttachmentWrite,    VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
    ADD_CASE(DepthStencilRead,        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
    ADD_CASE(DepthStencilWrite,       VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
    ADD_CASE(TransferRead,            VK_ACCESS_2_TRANSFER_READ_BIT)
    ADD_CASE(TransferWrite,           VK_ACCESS_2_TRANSFER_WRITE_BIT)
    ADD_CASE(HostRead,                VK_ACCESS_2_HOST_READ_BIT)
    ADD_CASE(HostWrite,               VK_ACCESS_2_HOST_WRITE_BIT)
#undef ADD_CASE
    return result;
}

VkImageAspectFlags TranslateAspectTypeFlag(AspectTypeFlag flag)
{
    VkImageAspectFlags result = 0;
    if(flag.contains(AspectType::Color))
    {
        result |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if(flag.contains(AspectType::Depth))
    {
        result |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if(flag.contains(AspectType::Stencil))
    {
        result |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return result;
}

VkBufferUsageFlags TranslateBufferUsageFlag(BufferUsageFlag flag)
{
    VkBufferUsageFlags result = 0;
#define ADD_CASE(FLAG, VAL) if(flag.contains(BufferUsage::FLAG)) { result |= VAL; }
    ADD_CASE(TransferDst,              VK_BUFFER_USAGE_TRANSFER_DST_BIT)
    ADD_CASE(TransferSrc,              VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
    ADD_CASE(ShaderBuffer,             VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
    ADD_CASE(ShaderRWBuffer,           VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    ADD_CASE(ShaderStructuredBuffer,   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    ADD_CASE(ShaderRWStructuredBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    ADD_CASE(ShaderUniformBuffer,      VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    ADD_CASE(IndexBuffer,              VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    ADD_CASE(VertexBuffer,             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    ADD_CASE(IndirectBuffer,           VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
#undef ADD_CASE
    return result;
}

VkAttachmentLoadOp TranslateLoadOp(AttachmentLoadOp op)
{
    switch(op)
    {
    case AttachmentLoadOp::Load:     return VK_ATTACHMENT_LOAD_OP_LOAD;
    case AttachmentLoadOp::Clear:    return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case AttachmentLoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    Unreachable();
}

VkAttachmentStoreOp TranslateStoreOp(AttachmentStoreOp op)
{
    switch(op)
    {
    case AttachmentStoreOp::Store:    return VK_ATTACHMENT_STORE_OP_STORE;
    case AttachmentStoreOp::DontCare: return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    Unreachable();
}

VkClearColorValue TranslateClearColorValue(const ColorClearValue &value)
{
    VkClearColorValue result;
    result.float32[0] = value.r;
    result.float32[1] = value.g;
    result.float32[2] = value.b;
    result.float32[3] = value.a;
    return result;
}

VkClearDepthStencilValue TranslateClearDepthStencilValue(const DepthStencilClearValue &value)
{
    return VkClearDepthStencilValue{
        .depth = value.depth,
        .stencil = value.stencil
    };
}

VkClearValue TranslateClearValue(const ClearValue &value)
{
    VkClearValue result;
    value.Match(
        [&](const ColorClearValue &v)
        {
            result.color = TranslateClearColorValue(v);
        },
        [&](const DepthStencilClearValue &v)
        {
            result.depthStencil = TranslateClearDepthStencilValue(v);
        });
    return result;
}

VmaAllocationCreateFlags TranslateBufferHostAccessType(BufferHostAccessType type)
{
    switch(type)
    {
    case BufferHostAccessType::None:            return 0;
    case BufferHostAccessType::SequentialWrite: return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    case BufferHostAccessType::Random:          return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }
    Unreachable();
}

RTRC_RHI_VK_END
