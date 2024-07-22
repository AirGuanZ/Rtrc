#include <mimalloc.h>

#define VMA_IMPLEMENTATION
#include <volk.h>
#include <vk_mem_alloc.h>
#undef VMA_IMPLEMENTATION

#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Core/Unreachable.h>

RTRC_RHI_VK_BEGIN

namespace VkCommonDetail
{

    void *VKAPI_PTR VkAlloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope)
    {
        return mi_aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
    }

    void *VKAPI_PTR VkRealloc(void *pUserData, void *orig, size_t size, size_t alignment, VkSystemAllocationScope scope)
    {
        return mi_realloc_aligned(orig, size, alignment);
    }

    void VKAPI_PTR VkFree(void *pUserData, void *ptr)
    {
        mi_free(ptr);
    }
    
} // namespace VkCommonDetail

VkAllocationCallbacks RtrcGlobalVulkanAllocationCallbacks = {
    .pfnAllocation         = VkCommonDetail::VkAlloc,
    .pfnReallocation       = VkCommonDetail::VkRealloc,
    .pfnFree               = VkCommonDetail::VkFree,
    .pfnInternalAllocation = nullptr,
    .pfnInternalFree       = nullptr
};

VkImageType TranslateTextureDimension(TextureDimension dim)
{
    switch(dim)
    {
    case TextureDimension::Tex1D: return VK_IMAGE_TYPE_1D;
    case TextureDimension::Tex2D: return VK_IMAGE_TYPE_2D;
    case TextureDimension::Tex3D: return VK_IMAGE_TYPE_3D;
    }
    Unreachable();
}

VkFormat TranslateTexelFormat(Format format)
{
    switch(format)
    {
    case Format::Unknown:            return VK_FORMAT_UNDEFINED;
    case Format::B8G8R8A8_UNorm:     return VK_FORMAT_B8G8R8A8_UNORM;
    case Format::R8G8B8A8_UNorm:     return VK_FORMAT_R8G8B8A8_UNORM;
    case Format::R32_Float:          return VK_FORMAT_R32_SFLOAT;
    case Format::R32G32_Float:       return VK_FORMAT_R32G32_SFLOAT;
    case Format::R32G32B32A32_Float: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Format::R32G32B32_Float:    return VK_FORMAT_R32G32B32_SFLOAT;
    case Format::R32G32B32A32_UInt:  return VK_FORMAT_R32G32B32A32_UINT;
    case Format::A2R10G10B10_UNorm:  return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    case Format::R16_UInt:           return VK_FORMAT_R16_UINT;
    case Format::R32_UInt:           return VK_FORMAT_R32_UINT;
    case Format::R8_UNorm:           return VK_FORMAT_R8_UNORM;
    case Format::R16_UNorm:          return VK_FORMAT_R16_UNORM;
    case Format::R16G16_UNorm:       return VK_FORMAT_R16G16_UNORM;
    case Format::R16G16_Float:       return VK_FORMAT_R16G16_SFLOAT;
    case Format::D24S8:              return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format::D32:                return VK_FORMAT_D32_SFLOAT;
    }
    Unreachable();
}

VkShaderStageFlagBits TranslateShaderType(ShaderType type)
{
    using enum ShaderType;
    switch(type)
    {
    case VertexShader:     return VK_SHADER_STAGE_VERTEX_BIT;
    case FragmentShader:   return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ComputeShader:    return VK_SHADER_STAGE_COMPUTE_BIT;
    case TaskShader:       return VK_SHADER_STAGE_TASK_BIT_EXT;
    case MeshShader:       return VK_SHADER_STAGE_MESH_BIT_EXT;
    case RayTracingShader:
    case WorkGraphShader:  Unreachable();
    }
    Unreachable();
}

VkShaderStageFlagBits TranslateShaderStage(ShaderStage type)
{
    using enum ShaderStage;
    switch(type)
    {
    case VertexShader:          return VK_SHADER_STAGE_VERTEX_BIT;
    case FragmentShader:        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ComputeShader:         return VK_SHADER_STAGE_COMPUTE_BIT;
    case RT_RayGenShader:       return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    case RT_ClosestHitShader:   return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    case RT_MissShader:         return VK_SHADER_STAGE_MISS_BIT_KHR;
    case RT_IntersectionShader: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
    case RT_AnyHitShader:       return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
    case CallableShader:        return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
    case TaskShader:            return VK_SHADER_STAGE_TASK_BIT_EXT;
    case MeshShader:            return VK_SHADER_STAGE_MESH_BIT_EXT;
    default:
        throw Exception(fmt::format("Single shader stage expected. Actual: {0:x}", std::to_underlying(type)));
    }
}

VkShaderStageFlags TranslateShaderStageFlag(ShaderStageFlags flags)
{
    VkShaderStageFlags result = 0;
    result |= flags.Contains(ShaderStage::VertexShader)          ? VK_SHADER_STAGE_VERTEX_BIT           : 0;
    result |= flags.Contains(ShaderStage::FragmentShader)        ? VK_SHADER_STAGE_FRAGMENT_BIT         : 0;
    result |= flags.Contains(ShaderStage::ComputeShader)         ? VK_SHADER_STAGE_COMPUTE_BIT          : 0;
    result |= flags.Contains(ShaderStage::RT_RayGenShader)       ? VK_SHADER_STAGE_RAYGEN_BIT_KHR       : 0;
    result |= flags.Contains(ShaderStage::RT_ClosestHitShader)   ? VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR  : 0;
    result |= flags.Contains(ShaderStage::RT_MissShader)         ? VK_SHADER_STAGE_MISS_BIT_KHR         : 0;
    result |= flags.Contains(ShaderStage::RT_IntersectionShader) ? VK_SHADER_STAGE_INTERSECTION_BIT_KHR : 0;
    result |= flags.Contains(ShaderStage::RT_AnyHitShader)       ? VK_SHADER_STAGE_ANY_HIT_BIT_KHR      : 0;
    result |= flags.Contains(ShaderStage::CallableShader)        ? VK_SHADER_STAGE_CALLABLE_BIT_KHR     : 0;
    result |= flags.Contains(ShaderStage::TaskShader)            ? VK_SHADER_STAGE_TASK_BIT_EXT         : 0;
    result |= flags.Contains(ShaderStage::MeshShader)            ? VK_SHADER_STAGE_MESH_BIT_EXT         : 0;
    return result;
}

VkPrimitiveTopology TranslatePrimitiveTopology(PrimitiveTopology topology)
{
    switch(topology)
    {
    case PrimitiveTopology::TriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    case PrimitiveTopology::LineList:     return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    }
    Unreachable();
}

VkPolygonMode TranslateFillMode(FillMode mode)
{
    switch(mode)
    {
    case FillMode::Fill:  return VK_POLYGON_MODE_FILL;
    case FillMode::Line:  return VK_POLYGON_MODE_LINE;
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
    case BindingType::Texture:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case BindingType::RWTexture:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case BindingType::Buffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case BindingType::RWBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    case BindingType::StructuredBuffer:
    case BindingType::RWStructuredBuffer:
    case BindingType::ByteAddressBuffer:
    case BindingType::RWByteAddressBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case BindingType::ConstantBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case BindingType::Sampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case BindingType::AccelerationStructure:
        return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    }
    Unreachable();
}

VkImageUsageFlags TranslateTextureUsageFlag(TextureUsageFlags flags)
{
    VkImageUsageFlags result = 0;
    if(flags.Contains(TextureUsage::TransferDst))
    {
        result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if(flags.Contains(TextureUsage::TransferSrc))
    {
        result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if(flags.Contains(TextureUsage::ShaderResource))
    {
        result |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if(flags.Contains(TextureUsage::UnorderedAccess))
    {
        result |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if(flags.Contains(TextureUsage::RenderTarget))
    {
        result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if(flags.Contains(TextureUsage::DepthStencil))
    {
        result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if(flags.Contains(TextureUsage::ClearDst))
    {
        result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    return result;
}

VkBufferUsageFlags TranslateBufferUsageFlag(BufferUsageFlag flag)
{
    VkBufferUsageFlags result = 0;
#define ADD_CASE(FLAG, VAL) if(flag.Contains(BufferUsage::FLAG)) { result |= (VAL); }
    ADD_CASE(TransferDst,                     VK_BUFFER_USAGE_TRANSFER_DST_BIT)
    ADD_CASE(TransferSrc,                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
    ADD_CASE(ShaderBuffer,                    VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
    ADD_CASE(ShaderRWBuffer,                  VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    ADD_CASE(ShaderStructuredBuffer,          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    ADD_CASE(ShaderRWStructuredBuffer,        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    ADD_CASE(ShaderByteAddressBuffer,         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    ADD_CASE(ShaderRWByteAddressBuffer,       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    ADD_CASE(ShaderConstantBuffer,            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    ADD_CASE(IndexBuffer,                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    ADD_CASE(VertexBuffer,                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
    ADD_CASE(IndirectBuffer,                  VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)
    ADD_CASE(DeviceAddress,                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    ADD_CASE(AccelerationStructureBuildInput, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    ADD_CASE(AccelerationStructure,           VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                              VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
    ADD_CASE(ShaderBindingTable,              VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR)
#undef ADD_CASE
    if(flag.Contains(BufferUsage::BackingMemory))
    {
        throw Exception("Vulkan backend: work graph is not supported");
    }
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

VkViewport TranslateViewport(const Viewport &viewport)
{
    return VkViewport
    {
        .x        = viewport.topLeftCorner.x,
        .y        = viewport.size.y - viewport.topLeftCorner.y,
        .width    = viewport.size.x,
        .height   = -viewport.size.y,
        .minDepth = viewport.minDepth,
        .maxDepth = viewport.maxDepth
    };
}

VkRect2D TranslateScissor(const Scissor &scissor)
{
    return VkRect2D{
        .offset = {
            static_cast<int32_t>(scissor.topLeftCorner.x),
            static_cast<int32_t>(scissor.topLeftCorner.y)
        },
        .extent = {
            static_cast<uint32_t>(scissor.size.x),
            static_cast<uint32_t>(scissor.size.y)
        }
    };
}

VmaAllocationCreateFlags TranslateBufferHostAccessType(BufferHostAccessType type)
{
    switch(type)
    {
    case BufferHostAccessType::None:     return 0;
    case BufferHostAccessType::Upload:   return VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    case BufferHostAccessType::Readback: return VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }
    Unreachable();
}

VkFilter TranslateSamplerFilterMode(FilterMode mode)
{
    switch(mode)
    {
    case FilterMode::Point:  return VK_FILTER_NEAREST;
    case FilterMode::Linear: return VK_FILTER_LINEAR;
    }
    Unreachable();
}

VkSamplerMipmapMode TranslateSamplerMipmapMode(FilterMode mode)
{
    switch(mode)
    {
    case FilterMode::Point:  return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case FilterMode::Linear: return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    Unreachable();
}

VkSamplerAddressMode TranslateSamplerAddressMode(AddressMode mode)
{
    switch(mode)
    {
    case AddressMode::Repeat: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case AddressMode::Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case AddressMode::Clamp:  return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case AddressMode::Border: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    }
    Unreachable();
}

VkPipelineStageFlags2 TranslatePipelineStageFlag(PipelineStageFlag flag)
{
    static constexpr VkPipelineStageFlags2 bitToFlag[] =
    {
        VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
        VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
        VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT,
        VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COPY_BIT,
        VK_PIPELINE_STAGE_2_CLEAR_BIT,
        VK_PIPELINE_STAGE_2_RESOLVE_BIT,
        VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR,
        VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT
    };
    VkPipelineStageFlags2 result = 0;
    for(size_t i = 0; i < GetArraySize(bitToFlag); ++i)
    {
        if(flag.Contains(1 << i))
        {
            result |= bitToFlag[i];
        }
    }
    return result;
}

VkAccessFlags2 TranslateAccessFlag(ResourceAccessFlag flag)
{
    assert(!flag.Contains(ResourceAccess::DummyReadAndWrite));
    static constexpr VkAccessFlags2 bitToFlag[] =
    {
        VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,                                                              // VertexBufferRead
        VK_ACCESS_2_INDEX_READ_BIT,                                                                         // IndexBufferRead
        VK_ACCESS_2_UNIFORM_READ_BIT,                                                                       // ConstantBufferRead
        VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT,                                                              // RenderTargetRead
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,                                                             // RenderTargetWrite
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,                                                      // DepthStencilRead
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,                                                     // DepthStencilWrite
        VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,                                                                // TextureRead
        VK_ACCESS_2_SHADER_STORAGE_READ_BIT,                                                                // RWTextureRead
        VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,                                                               // RWTextureWrite
        VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,                                                                // BufferRead
        VK_ACCESS_2_SHADER_STORAGE_READ_BIT,                                                                // StructuredBufferRead
        VK_ACCESS_2_SHADER_STORAGE_READ_BIT,                                                                // RWBufferRead
        VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,                                                               // RWBufferWrite
        VK_ACCESS_2_SHADER_STORAGE_READ_BIT,                                                                // RWStructuredBufferRead
        VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,                                                               // RWStructuredBufferWrite
        VK_ACCESS_2_TRANSFER_READ_BIT,                                                                      // CopyRead
        VK_ACCESS_2_TRANSFER_WRITE_BIT,                                                                     // CopyWrite
        VK_ACCESS_2_TRANSFER_READ_BIT,                                                                      // ResolveRead
        VK_ACCESS_2_TRANSFER_WRITE_BIT,                                                                     // ResolveWrite
        VK_ACCESS_2_TRANSFER_WRITE_BIT,                                                                     // ClearColorWrite
        VK_ACCESS_2_TRANSFER_WRITE_BIT,                                                                     // ClearDepthStencilWrite
        VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR,                                                    // ReadAS
        VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,                                                   // WriteAS
        VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, // BuildASScratch
        VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR,                                                      // ReadSBT
        VK_ACCESS_2_SHADER_READ_BIT,                                                                        // ReadForBuildAS
        VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT,                                                              // IndirectCommandRead
        VK_ACCESS_2_NONE,                                                                                   // Backing memory
        VK_ACCESS_2_NONE,                                                                                   // DummyReadAndWrite
        VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT                                          // All
    };
    if(flag.Contains(ResourceAccess::BackingMemory))
    {
        throw Exception("Vulkan backend: work graph is not supported");
    }
    VkAccessFlags2 result = 0;
    for(size_t i = 0; i < GetArraySize(bitToFlag); ++i)
    {
        if(flag.Contains(1 << i))
        {
            result |= bitToFlag[i];
        }
    }
    return result;
}

VkImageLayout TranslateImageLayout(TextureLayout layout)
{
    using enum TextureLayout;
    switch(layout)
    {
    case Undefined:                                    return VK_IMAGE_LAYOUT_UNDEFINED;
    case ColorAttachment:                              return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case DepthStencilAttachment:                       return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case DepthStencilReadOnlyAttachment:               return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    case ShaderTexture:                                return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case ShaderRWTexture:                              return VK_IMAGE_LAYOUT_GENERAL;
    case CopySrc:                                      return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case CopyDst:                                      return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ResolveSrc:                                   return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case ResolveDst:                                   return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case ClearDst:                                     return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    case Present:                                      return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    Unreachable();
}

VkImageSubresourceRange TranslateImageSubresources(Format format, const TexSubrscs &subresources)
{
    return VkImageSubresourceRange{
        .aspectMask     = GetAllAspects(format),
        .baseMipLevel   = subresources.mipLevel,
        .levelCount     = subresources.levelCount,
        .baseArrayLayer = subresources.arrayLayer,
        .layerCount     = subresources.layerCount
    };
}

VkFormat TranslateInputAttributeType(VertexAttributeType type)
{
    using enum VertexAttributeType;
    switch(type)
    {
    case Int:        return VK_FORMAT_R32_SINT;
    case Int2:       return VK_FORMAT_R32G32_SINT;
    case Int3:       return VK_FORMAT_R32G32B32_SINT;
    case Int4:       return VK_FORMAT_R32G32B32A32_SINT;
    case UInt:       return VK_FORMAT_R32_UINT;
    case UInt2:      return VK_FORMAT_R32G32_UINT;
    case UInt3:      return VK_FORMAT_R32G32B32_UINT;
    case UInt4:      return VK_FORMAT_R32G32B32A32_UINT;
    case Float:      return VK_FORMAT_R32_SFLOAT;
    case Float2:     return VK_FORMAT_R32G32_SFLOAT;
    case Float3:     return VK_FORMAT_R32G32B32_SFLOAT;
    case Float4:     return VK_FORMAT_R32G32B32A32_SFLOAT;
    case UChar4Norm: return VK_FORMAT_R8G8B8A8_UNORM;
    }
    Unreachable();
}

VkIndexType TranslateIndexFormat(IndexFormat format)
{
    switch(format)
    {
    case IndexFormat::UInt16: return VK_INDEX_TYPE_UINT16;
    case IndexFormat::UInt32: return VK_INDEX_TYPE_UINT32;
    }
    Unreachable();
}

VkBuildAccelerationStructureFlagsKHR TranslateAccelerationStructureBuildFlags(
    RayTracingAccelerationStructureBuildFlags flags)
{
    VkBuildAccelerationStructureFlagsKHR ret = 0;
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::AllowUpdate))
    {
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
    }
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::AllowCompaction))
    {
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    }
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::PreferFastBuild))
    {
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
    }
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::PreferFastTrace))
    {
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    }
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::PreferLowMemory))
    {
        ret |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
    }
    return ret;
}

VkGeometryTypeKHR TranslateGeometryType(RayTracingGeometryType type)
{
    if(type == RayTracingGeometryType::Triangles)
    {
        return VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    }
    assert(type == RayTracingGeometryType::Precodural);
    return VK_GEOMETRY_TYPE_AABBS_KHR;
}

VkFormat TranslateGeometryVertexFormat(RayTracingVertexFormat format)
{
    switch(format)
    {
    case RayTracingVertexFormat::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
    }
    Unreachable();
}

VkIndexType TranslateRayTracingIndexType(RayTracingIndexFormat format)
{
    switch(format)
    {
    case RayTracingIndexFormat::None:   return VK_INDEX_TYPE_NONE_KHR;
    case RayTracingIndexFormat::UInt16: return VK_INDEX_TYPE_UINT16;
    case RayTracingIndexFormat::UInt32: return VK_INDEX_TYPE_UINT32;
    }
    Unreachable();
}

bool HasColorAspect(Format format)
{
    return !HasDepthStencilAspect(format);
}

bool HasDepthStencilAspect(Format format)
{
    return HasDepthAspect(format) || HasStencilAspect(format);
}

VkImageAspectFlags GetAllAspects(Format format)
{
    if(HasColorAspect(format))
    {
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
    VkImageAspectFlags ret = 0;
    if(HasDepthAspect(format))
    {
        ret |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if(HasStencilAspect(format))
    {
        ret |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    return ret;
}

void SetObjectName(VulkanDevice* device, VkObjectType type, VkCommonHandle object, const char* name)
{
    device->_internalSetObjectName(type, reinterpret_cast<uint64_t>(object), name);
}

RTRC_RHI_VK_END
