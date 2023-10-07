#pragma once

#define GBUFFER_MODE GBUFFER_MODE_ALL

#include "Generated/Reflection.hlsl"
#include "Rtrc/Renderer/Atmosphere/Shader/Atmosphere.hlsl"
#include "Rtrc/Shader/Common/Color.hlsl"
#include "Rtrc/Shader/Common/GBufferRead.hlsl"
#include "Rtrc/Shader/Common/Scene.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)
    rtrc_define(ConstantBuffer<CameraData>, Camera)
    rtrc_define(Texture2D<float3>, SkyLut)
    rtrc_define(Texture2D<float3>, DirectIllumination)
    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2, outputResolution)
};

rtrc_sampler(SkyLutSampler, filter = linear, address_u = repeat, address_v = clamp)

float3 PostprocessColor(float3 color, float2 uv)
{
    color = Tonemap(color);
    color = pow(color, 1.0 / 2.2);
    color = Dither01(color, uv);
    return color;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputResolution))
        return;
    const float2 uv = (tid + 0.5) / Pass.outputResolution;
    const GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.depth >= 1)
    {
        const float3 worldRay = CameraUtils::GetWorldRay(Camera, uv);
        const float3 dir = normalize(worldRay);
        const float2 skyLutUV = Atmosphere::ComputeSkyLutTexCoord(dir);
        const float3 color = SkyLut.SampleLevel(SkyLutSampler, skyLutUV, 0);
        Output[tid] = float4(PostprocessColor(color, uv), 1);
        return;
    }
    const float3 directIllum = DirectIllumination[tid];
    const float3 color = gpixel.albedo * directIllum;
    Output[tid] = float4(PostprocessColor(color, uv), 1);
}
