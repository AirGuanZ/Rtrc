#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/Core/Unreachable.h>

RTRC_RHI_BEGIN

const char *GetFormatName(Format format)
{
#define ADD_CASE(NAME) case Format::NAME: return #NAME;
    switch(format)
    {
    ADD_CASE(Unknown)
    ADD_CASE(B8G8R8A8_UNorm)
    ADD_CASE(R8G8B8A8_UNorm)
    ADD_CASE(R32_Float)
    ADD_CASE(R32G32_Float)
    ADD_CASE(R32G32B32A32_Float)
    ADD_CASE(R32G32B32_Float)
    ADD_CASE(R32G32B32A32_UInt)
    ADD_CASE(A2R10G10B10_UNorm)
    ADD_CASE(R16_UInt)
    ADD_CASE(R32_UInt)
    ADD_CASE(R8_UNorm)
    ADD_CASE(R16_UNorm)
    ADD_CASE(R16G16_UNorm)
    ADD_CASE(R16G16_Float)
    ADD_CASE(D24S8)
    ADD_CASE(D32)
    }
    throw Exception("Unknown format: " + std::to_string(static_cast<int>(format)));
#undef ADD_CASE
}

size_t GetTexelSize(Format format)
{
    switch(format)
    {
    case Format::Unknown:
        return 0;
    case Format::R32G32B32A32_Float:
    case Format::R32G32B32A32_UInt:
        return 16;
    case Format::R32G32B32_Float:
        return 12;
    case Format::R32G32_Float:
        return 8;
    case Format::B8G8R8A8_UNorm:
    case Format::R8G8B8A8_UNorm:
    case Format::R32_Float:
    case Format::R32_UInt:
    case Format::R16G16_Float:
    case Format::R16G16_UNorm:
        return 4;
    case Format::A2R10G10B10_UNorm:
    case Format::R16_UInt:
    case Format::R16_UNorm:
        return 2;
    case Format::R8_UNorm:
        return 1;
    case Format::D24S8:
    case Format::D32:
        throw Exception(fmt::format("Texel size of {} is unknown", GetFormatName(format)));
    }
    throw Exception("Unknown format: " + std::to_string(static_cast<int>(format)));
}

bool CanBeAccessedAsFloatInShader(Format format)
{
    if(format == Format::R16_UInt || format == Format::R32_UInt)
        return false;
    return true;
}

bool CanBeAccessedAsIntInShader(Format format)
{
    return format == Format::R16_UInt || format == Format::R32_UInt;
}

bool CanBeAccessedAsUIntInShader(Format format)
{
    return format == Format::R16_UInt || format == Format::R32_UInt;
}

bool NeedUNormAsTypedUAV(Format format)
{
    return format == Format::B8G8R8A8_UNorm ||
           format == Format::R8G8B8A8_UNorm ||
           format == Format::A2R10G10B10_UNorm ||
           format == Format::R8_UNorm ||
           format == Format::R16_UNorm ||
           format == Format::R16G16_UNorm;
}

bool NeedSNormAsTypedUAV(Format format)
{
    return false;
}

bool HasDepthAspect(Format format)
{
    return format == Format::D24S8 || format == Format::D32;
}

bool HasStencilAspect(Format format)
{
    return format == Format::D24S8;
}

std::string GetShaderStageFlagsName(ShaderStageFlags flags)
{
#define ADD_COMPOSED_STAGES(STAGE)         \
    if(flags.Contains(ShaderStage::STAGE)) \
    {                                      \
        names.push_back(#STAGE);           \
    }                                      \
    else
#define ADD_ATOMIC_STAGE(STAGE) ADD_COMPOSED_STAGES(STAGE) { }

    std::vector<std::string> names;
    ADD_COMPOSED_STAGES(All)
    {
        ADD_COMPOSED_STAGES(AllGraphics)
        {
            ADD_ATOMIC_STAGE(VS)
            ADD_ATOMIC_STAGE(FS)
        }

        ADD_COMPOSED_STAGES(AllRT)
        {
            ADD_COMPOSED_STAGES(AllRTHit)
            {
                ADD_ATOMIC_STAGE(RT_CHS)
                ADD_ATOMIC_STAGE(RT_IS)
                ADD_ATOMIC_STAGE(RT_AHS)
            }

            ADD_COMPOSED_STAGES(AllRTCommon)
            {
                ADD_ATOMIC_STAGE(RT_RGS)
                ADD_ATOMIC_STAGE(RT_MS)
            }

            ADD_ATOMIC_STAGE(CallableShader)
        }

        ADD_ATOMIC_STAGE(CS)
    }
    return Join('|', names.begin(), names.end());

#undef ADD_COMPOSED_STAGES
#undef ADD_ATOMIC_STAGE
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
      | ReadForBuildAS
      | IndirectCommandRead;
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

bool IsUAVOnly(ResourceAccessFlag access)
{
    return !RemoveUAVAccesses(access);
}

bool HasUAVAccess(ResourceAccessFlag access)
{
    return RemoveUAVAccesses(access) != access;
}

ResourceAccessFlag RemoveUAVAccesses(ResourceAccessFlag access)
{
    using enum ResourceAccess;
    constexpr ResourceAccessFlag UAV_MASK =
        RWBufferRead | RWBufferWrite |
        RWStructuredBufferRead | RWStructuredBufferWrite |
        RWTextureRead | RWTextureWrite;
    return access & ~UAV_MASK;
}

RTRC_RHI_END
