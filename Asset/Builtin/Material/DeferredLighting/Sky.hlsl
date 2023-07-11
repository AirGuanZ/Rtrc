#pragma once

#include "../Atmosphere/Atmosphere.hlsl"
#include "../Common/Fullscreen.hlsl"
#include "../Common/Scene.hlsl"
#include "../Common/Color.hlsl"

#define GBUFFER_MODE GBUFFER_MODE_DEPTH
#include "../Common/GBufferRead.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)
    rtrc_define(Texture2D<float3>, SkyLut)
    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2, resolution)
    rtrc_uniform(float2, rcpResolution)
};

rtrc_sampler(SkyLutSampler, filter = linear, address_u = repeat, address_v = clamp)

[numthreads(8, 1, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;
    
    float2 uv = (tid + 0.5) * Pass.rcpResolution;
    GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.depth < 1)
        return;

    float3 worldRay = CameraUtils::GetWorldRay(Camera, uv);
    float3 dir = normalize(worldRay);
    float2 skyLutUv = Atmosphere::ComputeSkyLutTexCoord(dir);
    float3 color = SkyLut.SampleLevel(SkyLutSampler, skyLutUv, 0);
    color = Tonemap(color);
    color = pow(color, 1.0 / 2.2);
    color = Dither01(color, uv);
    Output[tid] = float4(color, 1);
}
