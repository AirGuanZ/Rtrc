#include <Rtrc/Graphics/RHI/RHI.h>
#include <Rtrc/Utility/Unreachable.h>

RTRC_RHI_BEGIN

const char *GetFormatName(Format format)
{
#define ADD_CASE(NAME) case Format::NAME: return #NAME;
    switch(format)
    {
    ADD_CASE(Unknown)
    ADD_CASE(B8G8R8A8_UNorm)
    ADD_CASE(R8G8B8A8_UNorm)
    ADD_CASE(R32G32_Float)
    ADD_CASE(R32G32B32A32_Float)
    ADD_CASE(A2R10G10B10_UNorm)
    ADD_CASE(A2B10G10R10_UNorm)
    ADD_CASE(R11G11B10_UFloat)
    ADD_CASE(R32_UInt)
    ADD_CASE(R8_UNorm)
    ADD_CASE(D24S8)
    ADD_CASE(D32S8)
    ADD_CASE(D32)
    }
    throw Exception("Unknown format: " + std::to_string(static_cast<int>(format)));
#undef ADD_CASE
}

size_t GetTexelSize(Format format)
{
    switch(format)
    {
    case Format::Unknown:            return 0;
    case Format::B8G8R8A8_UNorm:
    case Format::R8G8B8A8_UNorm:     return 4;
    case Format::R32G32_Float:       return 8;
    case Format::R32G32B32A32_Float: return 16;
    case Format::A2R10G10B10_UNorm:
    case Format::A2B10G10R10_UNorm:
    case Format::R11G11B10_UFloat:
    case Format::R32_UInt:           return 4;
    case Format::R8_UNorm:           return 1;
    case Format::D24S8:
    case Format::D32S8:
    case Format::D32:
        throw Exception(fmt::format("Texel size of {} is unknown", GetFormatName(format)));
    }
    throw Exception("Unknown format: " + std::to_string(static_cast<int>(format)));
}

bool IsReadOnly(ResourceAccessFlag access)
{
    using enum ResourceAccess;
    constexpr ResourceAccessFlag READONLY_MASK =
        VertexBufferRead
      | IndexBufferRead
      | ConstantBufferRead
      | RenderTargetRead
      | DepthStencilRead
      | TextureRead
      | RWTextureRead
      | BufferRead
      | RWBufferRead
      | RWStructuredBufferRead
      | CopyRead
      | ResolveRead
      | ReadAS
      | ReadSBT
      | ReadForBuildAS;
    return (access & READONLY_MASK) == access;
}

bool IsWriteOnly(ResourceAccessFlag access)
{
    using enum ResourceAccess;
    constexpr ResourceAccessFlag WRITEONLY_MASK =
        RenderTargetWrite
      | DepthStencilWrite
      | RWTextureWrite
      | RWBufferWrite
      | RWStructuredBufferWrite
      | CopyWrite
      | ResolveWrite
      | ClearWrite
      | WriteAS;
    return (access & WRITEONLY_MASK) == access;
}

RTRC_RHI_END
