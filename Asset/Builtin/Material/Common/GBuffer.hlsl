#pragma once

namespace Builtin
{
    rtrc_define(Texture2D<float4>, _internalGBufferA)
    rtrc_define(Texture2D<float4>, _internalGBufferB)
    rtrc_define(Texture2D<float4>, _internalGBufferC)
    rtrc_define(Texture2D<float>,  _internalGBufferDepth)
    rtrc_sampler(_internalGBufferSampler, filter = point, address = clamp)

#define BUILTIN_INTERNAL_GBUFFERS \
    rtrc_ref(_internalGBufferA)   \
    rtrc_ref(_internalGBufferB)   \
    rtrc_ref(_internalGBufferC)   \
    rtrc_ref(_internalGBufferDepth)
    
#define BUILTIN_INTERNAL_GBUFFERS_FS    \
    rtrc_ref(_internalGBufferA,     FS) \
    rtrc_ref(_internalGBufferB,     FS) \
    rtrc_ref(_internalGBufferC,     FS) \
    rtrc_ref(_internalGBufferDepth, FS)

#define REFERENCE_BUILTIN_GBUFFERS(STAGES)  \
    rtrc_ref(_internalGBufferA,     STAGES) \
    rtrc_ref(_internalGBufferB,     STAGES) \
    rtrc_ref(_internalGBufferC,     STAGES) \
    rtrc_ref(_internalGBufferDepth, STAGES)

    struct GBufferPixelOutput
    {
        float4 gbufferA : SV_TARGET0;
        float4 gbufferB : SV_TARGET1;
        float4 gbufferC : SV_TARGET2;
    };

    struct GBufferPixelValue
    {
        float3 normal;
        float3 albedo;
        float  metallic;
        float  roughness;
        float  depth;
    };

    float3 LoadGBufferNormal(float2 uv)
    {
        return 2 * _internalGBufferA.SampleLevel(_internalGBufferSampler, uv, 0).rgb - 1;
    }

    float3 LoadGBufferAlbedo(float2 uv)
    {
        return _internalGBufferB.SampleLevel(_internalGBufferSampler, uv, 0).rgb;
    }

    float3 LoadGBufferMetallic(float2 uv)
    {
        return _internalGBufferB.SampleLevel(_internalGBufferSampler, uv, 0).a;
    }

    float LoadGBufferRoughness(float2 uv)
    {
        return _internalGBufferC.SampleLevel(_internalGBufferSampler, uv, 0).r;
    }

    float LoadGBufferDepth(float2 uv)
    {
        return _internalGBufferDepth.SampleLevel(_internalGBufferSampler, uv, 0);
    }

    GBufferPixelValue LoadGBufferPixel(float2 uv)
    {
        float4 a = _internalGBufferA.SampleLevel(_internalGBufferSampler, uv, 0);
        float4 b = _internalGBufferB.SampleLevel(_internalGBufferSampler, uv, 0);
        float4 c = _internalGBufferC.SampleLevel(_internalGBufferSampler, uv, 0);
        float depth = _internalGBufferDepth.SampleLevel(_internalGBufferSampler, uv, 0);

        GBufferPixelValue ret;
        ret.normal    = 2 * a.rgb - 1;
        ret.albedo    = b.rgb;
        ret.metallic  = b.a;
        ret.roughness = c.r;
        ret.depth     = depth;
        return ret;
    }

    GBufferPixelOutput EncodeGBufferPixel(float3 worldNormal, float3 albedo, float metallic, float roughness)
    {
        GBufferPixelOutput pixel;
        pixel.gbufferA = float4(0.5 * worldNormal + 0.5, 0);
        pixel.gbufferB = float4(albedo, metallic);
        pixel.gbufferC = float4(roughness, 0, 0, 0);
        return pixel;
    }
}
