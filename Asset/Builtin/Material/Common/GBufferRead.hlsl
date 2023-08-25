#ifndef RTRC_COMMON_GBUFFER_READ_HLSL
#define RTRC_COMMON_GBUFFER_READ_HLSL

#define GBUFFER_MODE_ALL          1
#define GBUFFER_MODE_NORMAL_DEPTH 2
#define GBUFFER_MODE_DEPTH        3

#if GBUFFER_MODE == GBUFFER_MODE_ALL
#define GBUFFER_ENABLE_NORMAL    1
#define GBUFFER_ENABLE_ALBEDO    1
#define GBUFFER_ENABLE_METALLIC  1
#define GBUFFER_ENABLE_ROUGHNESS 1
#define GBUFFER_ENABLE_DEPTH     1
#elif GBUFFER_MODE == GBUFFER_MODE_NORMAL_DEPTH
#define GBUFFER_ENABLE_NORMAL 1
#define GBUFFER_ENABLE_DEPTH  1
#elif GBUFFER_MODE == GBUFFER_MODE_DEPTH
#define GBUFFER_ENABLE_DEPTH 1
#else
#error "GBUFFER_MODE must be properly defined before including GBuffer.hlsl"
#endif

#if GBUFFER_ENABLE_NORMAL
rtrc_define(Texture2D<float3>, _internalGBuffer_Normal)
#define REF_GBUFFER_NORMAL(STAGES) rtrc_ref(_internalGBuffer_Normal, STAGES)
#else
#define REF_GBUFFER_NORMAL(STAGES)
#endif

#if GBUFFER_ENABLE_ALBEDO || GBUFFER_ENABLE_METALLIC
rtrc_define(Texture2D<float4>, _internalGBuffer_AlbedoMetallic)
#define REF_GBUFFER_ALBEDOMETALLIC(STAGES) rtrc_ref(_internalGBuffer_AlbedoMetallic, STAGES)
#else
#define REF_GBUFFER_ALBEDOMETALLIC(STAGES)
#endif

#if GBUFFER_ENABLE_ROUGHNESS
rtrc_define(Texture2D<float>, _internalGBuffer_Roughness)
#define REF_GBUFFER_ROUGHNESS(STAGES) rtrc_ref(_internalGBuffer_Roughness, STAGES)
#else
#define REF_GBUFFER_ROUGHNESS(STAGES)
#endif

#if GBUFFER_ENABLE_DEPTH
rtrc_define(Texture2D<float>, _internalGBuffer_Depth)
#define REF_GBUFFER_DEPTH(STAGES) rtrc_ref(_internalGBuffer_Depth, STAGES)
#else
#define REF_GBUFFER_DEPTH(STAGES)
#endif

#define REF_GBUFFERS(STAGES)           \
    REF_GBUFFER_NORMAL(STAGES)         \
    REF_GBUFFER_ALBEDOMETALLIC(STAGES) \
    REF_GBUFFER_ROUGHNESS(STAGES)      \
    REF_GBUFFER_DEPTH(STAGES)

rtrc_sampler(_internalGBufferSampler, filter = point, address = clamp)

#if GBUFFER_ENABLE_NORMAL
float3 LoadGBufferNormal(float2 uv)
{
    float3 encoded = _internalGBuffer_Normal.SampleLevel(_internalGBufferSampler, uv, 0);
    return 2.0 * encoded - 1.0;
}
float3 LoadGBufferNormal(uint2 coord)
{
    float3 encoded = _internalGBuffer_Normal[coord];
    return 2.0 * encoded - 1.0;
}
#endif

#if GBUFFER_ENABLE_ALBEDO
float3 LoadGBufferAlbedo(float2 uv)
{
    return _internalGBuffer_AlbedoMetallic.SampleLevel(_internalGBufferSampler, uv, 0).rgb;
}
#endif

#if GBUFFER_ENABLE_METALLIC
float LoadGBufferMetallic(float2 uv)
{
    return _internalGBuffer_AlbedoMetallic.SampleLevel(_internalGBufferSampler, uv, 0).a;
}
#endif

#if GBUFFER_ENABLE_ROUGHNESS
float LoadGBufferRoughness(float2 uv)
{
    return _internalGBuffer_Roughness.SampleLevel(_internalGBufferSampler, uv, 0);
}
#endif

#if GBUFFER_ENABLE_DEPTH
float LoadGBufferDepth(float2 uv)
{
    return _internalGBuffer_Depth.SampleLevel(_internalGBufferSampler, uv, 0);
}
float LoadGBufferDepth(uint2 pixel)
{
    return _internalGBuffer_Depth[pixel];
}
#endif

struct GBufferPixel
{
#if GBUFFER_ENABLE_NORMAL
    float3 normal;
#endif
#if GBUFFER_ENABLE_ALBEDO
    float3 albedo;
#endif
#if GBUFFER_ENABLE_METALLIC
    float metallic;
#endif
#if GBUFFER_ENABLE_ROUGHNESS
    float roughness;
#endif
#if GBUFFER_ENABLE_DEPTH
    float depth;
#endif
};

GBufferPixel LoadGBufferPixel(float2 uv)
{
    GBufferPixel ret;
#if GBUFFER_ENABLE_NORMAL
    ret.normal = LoadGBufferNormal(uv);
#endif
#if GBUFFER_ENABLE_ALBEDO || GBUFFER_ENABLE_METALLIC
    float4 albedoMetallic = _internalGBuffer_AlbedoMetallic.SampleLevel(_internalGBufferSampler, uv, 0);
#endif
#if GBUFFER_ENABLE_ALBEDO
    ret.albedo = albedoMetallic.rgb;
#endif
#if GBUFFER_ENABLE_METALLIC
    ret.metallic = albedoMetallic.a;
#endif
#if GBUFFER_ENABLE_ROUGHNESS
    ret.roughness = LoadGBufferRoughness(uv);
#endif
#if GBUFFER_ENABLE_DEPTH
    ret.depth = LoadGBufferDepth(uv);
#endif
    return ret;
}

uint2 GetGBufferResolution()
{
    uint2 ret;
#if GBUFFER_ENABLE_NORMAL
    _internalGBuffer_Normal.GetDimensions(ret.x, ret.y);
#elif GBUFFER_ENABLE_ALBEDO || GBUFFER_ENABLE_METALLIC
    _internalGBuffer_AlbedoMetallic.GetDimensions(ret.x, ret.y);
#elif GBUFFER_ENABLE_ROUGHNESS
    _internalGBuffer_Roughness.GetDimensions(ret.x, ret.y);
#elif GBUFFER_ENABLE_DEPTH
    _internalGBuffer_Depth.GetDimensions(ret.x, ret.y);
#else
    #error "Cannot call GetGBufferResolution when no gbuffer is enabled"
#endif
    return ret;
}

#endif // #ifndef RTRC_COMMON_GBUFFER_READ_HLSL
