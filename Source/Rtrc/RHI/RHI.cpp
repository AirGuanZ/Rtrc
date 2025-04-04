#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Core/String.h>

RTRC_RHI_BEGIN

const FormatDesc &GetFormatDesc(Format format)
{
    static const FormatDesc DESCS[] =
    {
        {
            .name                         = "Unknown",
            .texelSize                    = 0,
            .canBeAccessedAsFloatInShader = false,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "B8G8R8A8_UNorm",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = true,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R8G8B8A8_UNorm",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = true,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R32_Float",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R32G32_Float",
            .texelSize                    = 8,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R32G32B32A32_Float",
            .texelSize                    = 16,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R32G32B32_Float",
            .texelSize                    = 12,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R32G32B32A32_UInt",
            .texelSize                    = 16,
            .canBeAccessedAsFloatInShader = false,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = true,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "A2R10G10B10_UNorm",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = true,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R16_UInt",
            .texelSize                    = 2,
            .canBeAccessedAsFloatInShader = false,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = true,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R32_UInt",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = false,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = true,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R8_UNorm",
            .texelSize                    = 1,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = true,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R16_UNorm",
            .texelSize                    = 2,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = true,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R16G16_UNorm",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = true,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "R16G16_Float",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = false,
            .hasStencilAspect             = false
        },
        {
            .name                         = "D24S8",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = true,
            .hasStencilAspect             = true
        },
        {
            .name                         = "D32",
            .texelSize                    = 4,
            .canBeAccessedAsFloatInShader = true,
            .canBeAccessedAsIntInShader   = false,
            .canBeAccessedAsUIntInShader  = false,
            .needSNormAsTypedUAV          = false,
            .needUNormAsTypedUAV          = false,
            .hasDepthAspect               = true,
            .hasStencilAspect             = false
        },
    };
    return DESCS[std::to_underlying(format)];
}

std::string GetShaderStageFlagsName(ShaderStageFlags flags)
{
#define ADD_ATOMIC_STAGE(STAGE, NAME)      \
    if(flags.Contains(ShaderStage::STAGE)) \
    {                                      \
        names.push_back(NAME);             \
    }

    std::vector<std::string> names;
    {
        ADD_ATOMIC_STAGE(VS, "VS")
        ADD_ATOMIC_STAGE(FS, "FS")

        ADD_ATOMIC_STAGE(TS, "TS")
        ADD_ATOMIC_STAGE(MS, "MS")

        ADD_ATOMIC_STAGE(RT_CHS,         "RT_CHS")
        ADD_ATOMIC_STAGE(RT_IS,          "RT_IS")
        ADD_ATOMIC_STAGE(RT_AHS,         "RT_AHS")
        ADD_ATOMIC_STAGE(RT_RGS,         "RT_RGS")
        ADD_ATOMIC_STAGE(RT_MS,          "RT_MS")
        ADD_ATOMIC_STAGE(CallableShader, "Callable")

        ADD_ATOMIC_STAGE(CS, "CS")
    }
    return Join('|', names.begin(), names.end());

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
    return (access & ResourceAccessWriteOnlyMask) == access;
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

void QueueSyncQuery::Set(QueueOPtr queue)
{
    assert(queue && !queue_);
    queue_ = queue;
    sessionID_ = queue_->GetCurrentSessionID();
}

bool QueueSyncQuery::IsSynchronized() const
{
    assert(queue_);
    return sessionID_ <= queue_->GetSynchronizedSessionID();
}

RTRC_RHI_END
