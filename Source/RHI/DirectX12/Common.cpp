#include <system_error>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <comdef.h>
#include <mimalloc.h>

#include <Core/Unreachable.h>
#include <RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

namespace DirectX12Detail
{

    void *AllocateCallback(size_t size, size_t alignment, void *)
    {
        return mi_aligned_alloc(alignment, size);
    }

    void FreeCallback(void *ptr, void *)
    {
        mi_free(ptr);
    }

} // namespace DirectX12Detail

extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 610; }
extern "C" { __declspec(dllexport) extern const char8_t *D3D12SDKPath = u8".\\D3D12\\"; }

std::string HResultToString(HRESULT hr)
{
    return _com_error(hr).ErrorMessage();
}

D3D12MA::ALLOCATION_CALLBACKS RtrcGlobalDirectX12AllocationCallbacks =
{
    .pAllocate    = DirectX12Detail::AllocateCallback,
    .pFree        = DirectX12Detail::FreeCallback,
    .pPrivateData = nullptr
};

std::wstring Utf8ToWin32W(std::string_view s)
{
    auto throwError = []
    {
        const DWORD err = GetLastError();
        throw Exception(fmt::format(
            "Fail to convert utf8 string to wstring. Error = {}",
            std::system_category().message(static_cast<int>(err))));
    };
    if(s.empty())
    {
        return {};
    }
    int len = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        s.data(), static_cast<int>(s.length()),
        nullptr, 0);
    if(!len)
    {
        throwError();
    }
    std::wstring ret;
    ret.resize(len);
    len = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS,
        s.data(), static_cast<int>(s.length()),
        ret.data(), len);
    if(!len)
    {
        throwError();
    }
    return ret;
}

DXGI_FORMAT TranslateFormat(Format format)
{
    using enum Format;
    switch(format)
    {
    case Unknown:
        return DXGI_FORMAT_UNKNOWN;
    case B8G8R8A8_UNorm:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case R8G8B8A8_UNorm:
        return DXGI_FORMAT_R8G8B8A8_UNORM;
    case R32_Float:
        return DXGI_FORMAT_R32_FLOAT;
    case R32G32_Float:
        return DXGI_FORMAT_R32G32_FLOAT;
    case R32G32B32A32_Float:
        return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case R32G32B32_Float:
        return DXGI_FORMAT_R32G32B32_FLOAT;
    case R32G32B32A32_UInt:
        return DXGI_FORMAT_R32G32B32A32_UINT;
    case A2R10G10B10_UNorm:
        return DXGI_FORMAT_R10G10B10A2_UNORM;
    case R16_UInt:
        return DXGI_FORMAT_R16_UINT;
    case R32_UInt:
        return DXGI_FORMAT_R32_UINT;
    case R8_UNorm:
        return DXGI_FORMAT_R8_UNORM;
    case R16G16_Float:
        return DXGI_FORMAT_R16G16_FLOAT;
    case D24S8:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;
    case D32:
        return DXGI_FORMAT_D32_FLOAT;
    }
    Unreachable();
}

D3D12_COMMAND_LIST_TYPE TranslateCommandListType(QueueType type)
{
    if(type == QueueType::Graphics)
    {
        return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }
    if(type == QueueType::Compute)
    {
        return D3D12_COMMAND_LIST_TYPE_COMPUTE;
    }
    assert(type == QueueType::Transfer);
    return D3D12_COMMAND_LIST_TYPE_COPY;
}

D3D12_TEXTURE_ADDRESS_MODE TranslateAddressMode(AddressMode mode)
{
    switch(mode)
    {
    case AddressMode::Repeat: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    case AddressMode::Mirror: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    case AddressMode::Clamp:  return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    case AddressMode::Border: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    }
    Unreachable();
}

D3D12_COMPARISON_FUNC TranslateCompareOp(CompareOp op)
{
    using enum CompareOp;
    switch(op)
    {
    case Less:         return D3D12_COMPARISON_FUNC_LESS;
    case LessEqual:    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
    case Equal:        return D3D12_COMPARISON_FUNC_EQUAL;
    case NotEqual:     return D3D12_COMPARISON_FUNC_NOT_EQUAL;
    case GreaterEqual: return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
    case Greater:      return D3D12_COMPARISON_FUNC_GREATER;
    case Always:       return D3D12_COMPARISON_FUNC_ALWAYS;
    case Never:        return D3D12_COMPARISON_FUNC_NEVER;
    }
    Unreachable();
}

D3D12_SHADER_VISIBILITY TranslateShaderVisibility(ShaderStageFlags vis)
{
    if(vis == ShaderStage::VertexShader)
    {
        return D3D12_SHADER_VISIBILITY_VERTEX;
    }
    if(vis == ShaderStage::FragmentShader)
    {
        return D3D12_SHADER_VISIBILITY_PIXEL;
    }
    return D3D12_SHADER_VISIBILITY_ALL;
}

D3D12_VIEWPORT TranslateViewport(const Viewport &viewport)
{
    return D3D12_VIEWPORT
    {
        .TopLeftX = viewport.topLeftCorner.x,
        .TopLeftY = viewport.topLeftCorner.y,
        .Width    = viewport.size.x,
        .Height   = viewport.size.y,
        .MinDepth = viewport.minDepth,
        .MaxDepth = viewport.maxDepth
    };
}

D3D12_RECT TranslateScissor(const Scissor &scissor)
{
    return D3D12_RECT
    {
        .left   = scissor.topLeftCorner.x,
        .top    = scissor.topLeftCorner.y,
        .right  = scissor.topLeftCorner.x + scissor.size.x,
        .bottom = scissor.topLeftCorner.y + scissor.size.y
    };
}

D3D12_BLEND TranslateBlendFactor(BlendFactor factor)
{
    switch(factor)
    {
    case BlendFactor::Zero:             return D3D12_BLEND_ZERO;
    case BlendFactor::One:              return D3D12_BLEND_ONE;
    case BlendFactor::SrcAlpha:         return D3D12_BLEND_SRC_ALPHA;
    case BlendFactor::OneMinusSrcAlpha: return D3D12_BLEND_INV_SRC_ALPHA;
    case BlendFactor::DstAlpha:         return D3D12_BLEND_DEST_ALPHA;
    case BlendFactor::OneMinusDstAlpha: return D3D12_BLEND_INV_DEST_ALPHA;
    }
    Unreachable();
}

D3D12_BLEND_OP TranslateBlendOp(BlendOp op)
{
    switch(op)
    {
    case BlendOp::Add:        return D3D12_BLEND_OP_ADD;
    case BlendOp::Sub:        return D3D12_BLEND_OP_SUBTRACT;
    case BlendOp::SubReverse: return D3D12_BLEND_OP_REV_SUBTRACT;
    case BlendOp::Min:        return D3D12_BLEND_OP_MIN;
    case BlendOp::Max:        return D3D12_BLEND_OP_MAX;
    }
    Unreachable();
}

D3D12_FILL_MODE TranslateFillMode(FillMode mode)
{
    switch(mode)
    {
    case FillMode::Fill: return D3D12_FILL_MODE_SOLID;
    case FillMode::Line: return D3D12_FILL_MODE_WIREFRAME;
    }
    Unreachable();
}

D3D12_CULL_MODE TranslateCullMode(CullMode mode)
{
    switch(mode)
    {
    case CullMode::DontCull:  return D3D12_CULL_MODE_NONE;
    case CullMode::CullFront: return D3D12_CULL_MODE_FRONT;
    case CullMode::CullBack:  return D3D12_CULL_MODE_BACK;
    }
    Unreachable();
}

D3D12_STENCIL_OP TranslateStencilOp(StencilOp op)
{
    using enum StencilOp;
    switch(op)
    {
    case Keep:          return D3D12_STENCIL_OP_KEEP;
    case Zero:          return D3D12_STENCIL_OP_ZERO;
    case Replace:       return D3D12_STENCIL_OP_REPLACE;
    case IncreaseClamp: return D3D12_STENCIL_OP_INCR_SAT;
    case DecreaseClamp: return D3D12_STENCIL_OP_DECR_SAT;
    case IncreaseWrap:  return D3D12_STENCIL_OP_INCR;
    case DecreaseWrap:  return D3D12_STENCIL_OP_DECR;
    case Invert:        return D3D12_STENCIL_OP_INVERT;
    }
    Unreachable();
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE TranslatePrimitiveTopologyType(PrimitiveTopology topology)
{
    switch(topology)
    {
    case PrimitiveTopology::TriangleList: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    case PrimitiveTopology::LineList:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    }
    Unreachable();
}

D3D12_PRIMITIVE_TOPOLOGY TranslatePrimitiveTopology(PrimitiveTopology topology)
{
    switch(topology)
    {
    case PrimitiveTopology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    case PrimitiveTopology::LineList:     return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
    }
    Unreachable();
}

DXGI_FORMAT TranslateVertexAttributeType(VertexAttributeType type)
{
    using enum VertexAttributeType;
    switch(type)
    {
    case UInt: return DXGI_FORMAT_R32_UINT;
    case UInt2: return DXGI_FORMAT_R32G32_UINT;
    case UInt3: return DXGI_FORMAT_R32G32B32_UINT;
    case UInt4: return DXGI_FORMAT_R32G32B32A32_UINT;
    case Int: return DXGI_FORMAT_R32_SINT;
    case Int2: return DXGI_FORMAT_R32G32_SINT;
    case Int3: return DXGI_FORMAT_R32G32B32_SINT;
    case Int4: return DXGI_FORMAT_R32G32B32A32_SINT;
    case Float: return DXGI_FORMAT_R32_FLOAT;
    case Float2: return DXGI_FORMAT_R32G32_FLOAT;
    case Float3: return DXGI_FORMAT_R32G32B32_FLOAT;
    case Float4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case UChar4Norm: return DXGI_FORMAT_R8G8B8A8_UNORM;
    }
    Unreachable();
}

D3D12_SAMPLER_DESC TranslateSamplerDesc(const SamplerDesc &desc)
{
    D3D12_SAMPLER_DESC ret = {};

    static constexpr D3D12_FILTER FILTER_LUT[16] = // [comp][min][mag][mip]
    {
        D3D12_FILTER_MIN_MAG_MIP_POINT,
        D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR,
        D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT,
        D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
        D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
    };
    ret.Filter = FILTER_LUT[
        (desc.enableComparision ? 1 : 0) * 8 +
        static_cast<int>(desc.minFilter) * 4 +
        static_cast<int>(desc.magFilter) * 2 +
        static_cast<int>(desc.mipFilter)];
    if(desc.enableAnisotropy)
    {
        ret.Filter = desc.enableComparision ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC;
    }

    ret.AddressU = TranslateAddressMode(desc.addressModeU);
    ret.AddressV = TranslateAddressMode(desc.addressModeV);
    ret.AddressW = TranslateAddressMode(desc.addressModeW);

    ret.MipLODBias     = desc.mipLODBias;
    ret.MaxAnisotropy  = desc.maxAnisotropy;
    ret.ComparisonFunc = desc.enableComparision ? TranslateCompareOp(desc.compareOp) : D3D12_COMPARISON_FUNC_NONE;
    ret.BorderColor[0] = desc.borderColor[0];
    ret.BorderColor[1] = desc.borderColor[1];
    ret.BorderColor[2] = desc.borderColor[2];
    ret.BorderColor[3] = desc.borderColor[3];
    ret.MinLOD         = desc.minLOD;
    ret.MaxLOD         = desc.maxLOD;
    
    return ret;
}

D3D12_STATIC_SAMPLER_DESC TranslateStaticSamplerDesc(const SamplerDesc &desc, D3D12_SHADER_VISIBILITY vis)
{
    const auto &sDesc = TranslateSamplerDesc(desc);
    auto ret = D3D12_STATIC_SAMPLER_DESC
    {
        .Filter           = sDesc.Filter,
        .AddressU         = sDesc.AddressU,
        .AddressV         = sDesc.AddressV,
        .AddressW         = sDesc.AddressW,
        .MipLODBias       = sDesc.MipLODBias,
        .MaxAnisotropy    = sDesc.MaxAnisotropy,
        .ComparisonFunc   = sDesc.ComparisonFunc,
        .MinLOD           = sDesc.MinLOD,
        .MaxLOD           = sDesc.MaxLOD,
        .ShaderRegister   = 0,
        .RegisterSpace    = 0,
        .ShaderVisibility = vis,
    };
    if(desc.borderColor[0] == 0 && desc.borderColor[1] == 0 && desc.borderColor[2] == 0 && desc.borderColor[3] == 0)
    {
        ret.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    }
    else if(desc.borderColor[0] == 1 && desc.borderColor[1] == 1 && desc.borderColor[2] == 1 && desc.borderColor[3] == 1)
    {
        ret.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
    }
    else
    {
        ret.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    }
    return ret;
}

D3D12_BARRIER_SYNC TranslateBarrierSync(PipelineStageFlag stages)
{
    constexpr D3D12_BARRIER_SYNC syncs[] =
    {
        D3D12_BARRIER_SYNC_INDEX_INPUT,                             // VertexInput
        D3D12_BARRIER_SYNC_VERTEX_SHADING,                          // IndexInput
        D3D12_BARRIER_SYNC_VERTEX_SHADING,                          // VertexShader
        D3D12_BARRIER_SYNC_PIXEL_SHADING,                           // FragmentShader
        D3D12_BARRIER_SYNC_COMPUTE_SHADING,                         // ComputeShader
        D3D12_BARRIER_SYNC_RAYTRACING,                              // RayTracingShader
        D3D12_BARRIER_SYNC_DEPTH_STENCIL,                           // DepthStencil
        D3D12_BARRIER_SYNC_RENDER_TARGET,                           // RenderTarget
        D3D12_BARRIER_SYNC_COPY,                                    // Copy
        D3D12_BARRIER_SYNC_RENDER_TARGET,                           // Clear
        D3D12_BARRIER_SYNC_RESOLVE,                                 // Resolve
        D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE, // BuildAS
        D3D12_BARRIER_SYNC_COPY_RAYTRACING_ACCELERATION_STRUCTURE,  // CopyAS
        D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,                        // IndirectCommand
        D3D12_BARRIER_SYNC_ALL                                      // All
    };
    D3D12_BARRIER_SYNC ret = D3D12_BARRIER_SYNC_NONE;
    for(size_t i = 0; i < GetArraySize(syncs); ++i)
    {
        if(stages.Contains(1 << i))
        {
            ret |= syncs[i];
        }
    }
    return ret;
}

D3D12_BARRIER_ACCESS TranslateBarrierAccess(ResourceAccessFlag accesses)
{
    constexpr D3D12_BARRIER_ACCESS d3dAccesses[] =
    {
        D3D12_BARRIER_ACCESS_VERTEX_BUFFER,                           // VertexBufferRead
        D3D12_BARRIER_ACCESS_INDEX_BUFFER,                            // IndexBufferRead
        D3D12_BARRIER_ACCESS_CONSTANT_BUFFER,                         // ConstantBufferRead
        D3D12_BARRIER_ACCESS_RENDER_TARGET,                           // RenderTargetRead
        D3D12_BARRIER_ACCESS_RENDER_TARGET,                           // RenderTargetWrite
        D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ,                      // DepthStencilRead
        D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,                     // DepthStencilWrite
        D3D12_BARRIER_ACCESS_SHADER_RESOURCE,                         // TextureRead
        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,                        // RWTextureRead
        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,                        // RWTextureWrite
        D3D12_BARRIER_ACCESS_SHADER_RESOURCE,                         // BufferRead
        D3D12_BARRIER_ACCESS_SHADER_RESOURCE,                         // StructuredBufferRead
        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,                        // RWBufferRead
        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,                        // RWBufferWrite
        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,                        // RWStructuredBufferRead
        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,                        // RWStructuredBufferWrite
        D3D12_BARRIER_ACCESS_COPY_SOURCE,                             // CopyRead
        D3D12_BARRIER_ACCESS_COPY_DEST,                               // CopyWrite
        D3D12_BARRIER_ACCESS_RESOLVE_SOURCE,                          // ResolveRead
        D3D12_BARRIER_ACCESS_RESOLVE_DEST,                            // ResolveWrite
        D3D12_BARRIER_ACCESS_RENDER_TARGET,                           // ClearWrite
        D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ,  // ReadAS
        D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE, // WriteAS
        D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,                        // BuildASScratch
        D3D12_BARRIER_ACCESS_SHADER_RESOURCE,                         // ReadSBT
        D3D12_BARRIER_ACCESS_SHADER_RESOURCE,                         // ReadForBuildAS
        D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT,                       // IndirectCommandRead
        D3D12_BARRIER_ACCESS_COMMON,                                  // All
    };
    if(accesses.Contains(ResourceAccess::All))
    {
        return D3D12_BARRIER_ACCESS_COMMON;
    }
    if(accesses == ResourceAccess::None)
    {
        return D3D12_BARRIER_ACCESS_NO_ACCESS;
    }
    D3D12_BARRIER_ACCESS ret = D3D12_BARRIER_ACCESS_COMMON;
    for(size_t i = 0; i < GetArraySize(d3dAccesses); ++i)
    {
        if(accesses.Contains(1 << i))
        {
            ret |= d3dAccesses[i];
        }
    }
    return ret;
}

D3D12_BARRIER_LAYOUT TranslateTextureLayout(TextureLayout layout)
{
    using enum TextureLayout;
    switch(layout)
    {
    case Undefined:                      return D3D12_BARRIER_LAYOUT_UNDEFINED;
    case ColorAttachment:                return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    case DepthStencilAttachment:         return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
    case DepthStencilReadOnlyAttachment: return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
    case ShaderTexture:                  return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
    case ShaderRWTexture:                return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    case CopySrc:                        return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
    case CopyDst:                        return D3D12_BARRIER_LAYOUT_COPY_DEST;
    case ResolveSrc:                     return D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE;
    case ResolveDst:                     return D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
    case ClearDst:                       return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    case Present:                        return D3D12_BARRIER_LAYOUT_PRESENT;
    }
    Unreachable();
}

DXGI_FORMAT TranslateRayTracingIndexType(RayTracingIndexFormat type)
{
    switch(type)
    {
    case RayTracingIndexFormat::None:   return DXGI_FORMAT_UNKNOWN;
    case RayTracingIndexFormat::UInt16: return DXGI_FORMAT_R16_UINT;
    case RayTracingIndexFormat::UInt32: return DXGI_FORMAT_R32_UINT;
    }
    Unreachable();
}

DXGI_FORMAT TranslateRayTracingVertexFormat(RayTracingVertexFormat format)
{
    switch(format)
    {
    case RayTracingVertexFormat::Float3: return DXGI_FORMAT_R32G32B32_FLOAT;
    }
    Unreachable();
}

D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS
    TranslateRayTracingAccelerationStructureBuildFlags(RayTracingAccelerationStructureBuildFlags flags)
{
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS ret = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
    if(flags.Contains(RayTracingAccelerationStructureBuildFlags::AllowUpdate))
    {
        ret |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
    }
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::AllowCompaction))
    {
        ret |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
    }
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::PreferFastBuild))
    {
        ret |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
    }
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::PreferFastTrace))
    {
        ret |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    }
    if(flags.Contains(RayTracingAccelerationStructureBuildFlagBit::PreferLowMemory))
    {
        ret |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
    }
    return ret;
}

D3D12_RESOURCE_DESC TranslateBufferDesc(const BufferDesc &desc)
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    if(desc.usage & (BufferUsage::ShaderRWBuffer | BufferUsage::ShaderRWStructuredBuffer))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if(desc.usage.Contains(BufferUsage::AccelerationStructure))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        flags |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
    }

    const D3D12_RESOURCE_DESC resourceDesc =
    {
        .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment        = 0,
        .Width            = desc.size,
        .Height           = 1,
        .DepthOrArraySize = 1,
        .MipLevels        = 1,
        .Format           = DXGI_FORMAT_UNKNOWN,
        .SampleDesc       = { 1, 0 },
        .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags            = flags
    };
    return resourceDesc;
}

D3D12_RESOURCE_DESC TranslateTextureDesc(const TextureDesc &desc)
{
    D3D12_RESOURCE_DIMENSION dimension;
    if(desc.dim == TextureDimension::Tex2D)
    {
        dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    }
    else if(desc.dim == TextureDimension::Tex3D)
    {
        dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    }
    else
    {
        Unreachable();
    }
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    if(desc.usage.Contains(TextureUsage::RenderTarget))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if(desc.usage.Contains(TextureUsage::DepthStencil))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if(desc.usage.Contains(TextureUsage::UnorderAccess))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if(desc.concurrentAccessMode == QueueConcurrentAccessMode::Concurrent)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
    }
    if(!desc.usage.Contains(TextureUsage::ShaderResource) &&
       desc.usage.Contains(TextureUsage::DepthStencil))
    {
        flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }
    if(desc.usage.Contains(TextureUsage::ClearColor))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    const D3D12_RESOURCE_DESC resourceDesc =
    {
        .Dimension        = dimension,
        .Alignment        = 0,
        .Width            = desc.width,
        .Height           = desc.height,
        .DepthOrArraySize = static_cast<UINT16>(desc.dim == TextureDimension::Tex3D ? desc.depth : desc.arraySize),
        .MipLevels        = static_cast<UINT16>(desc.mipLevels),
        .Format           = TranslateFormat(desc.format),
        .SampleDesc       = DXGI_SAMPLE_DESC{ desc.sampleCount, 0 },
        .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags            = flags
    };
    return resourceDesc;
}

RTRC_RHI_D3D12_END
