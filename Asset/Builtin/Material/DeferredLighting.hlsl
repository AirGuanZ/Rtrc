#pragma once

#include "Atmosphere.hlsl"
#include "Common/Fullscreen.hlsl"
#include "Common/GBuffer.hlsl"
#include "Common/Scene.hlsl"
#include "Common/Color.hlsl"

#define LIGHTING_MODE_REGULAR 0
#define LIGHTING_MODE_SKY     1

rtrc_group(Pass, FS)
{
    REFERENCE_BUILTIN_GBUFFERS(FS)

    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)
    
    rtrc_define(StructuredBuffer<PointLightShadingData>, PointLightBuffer)
    rtrc_uniform(uint, pointLightCount)
    
    rtrc_define(StructuredBuffer<DirectionalLightShadingData>, DirectionalLightBuffer)
    rtrc_uniform(uint, directionalLightCount)

    rtrc_define(Texture2D<float3>, SkyLut)
};

rtrc_sampler(SkyLutSampler, filter = linear, address_u = repeat, address_v = clamp)

float3 ComputePointLighting(Builtin::GBufferPixelValue gpixel, float3 worldPos, PointLightShadingData light)
{
    float3 surfaceToLight = light.position - worldPos;
    float distance = length(surfaceToLight);
    float3 L = surfaceToLight / max(distance, 1e-5);
    float diffuseFactor = max(0.0, dot(L, gpixel.normal));
    float distanceFactor = saturate(1 - distance / light.range);
    distanceFactor *= distanceFactor;
    return gpixel.albedo * diffuseFactor * distanceFactor * light.color;
}

#if LIGHTING_MODE == LIGHTING_MODE_REGULAR

float4 FSMain(FullscreenPrimitive::VsToFsWithWorldRay input) : SV_TARGET
{
    Builtin::GBufferPixelValue gpixel = Builtin::LoadGBufferPixel(input.uv);
    float3 worldRay = input.ray;
    float viewSpaceZ = CameraUtils::DeviceZToViewZ(Camera.cameraToClipMatrix, gpixel.depth);
    float3 worldPos = viewSpaceZ * worldRay + Camera.worldPosition;
    
    float3 color = 0;
    for(int i = 0; i < Pass.pointLightCount; ++i)
    {
        PointLightShadingData light = PointLightBuffer[i];
        color += ComputePointLighting(gpixel, worldPos, light);
    }
    
    color = Tonemap(color);
    color = pow(color, 1.0 / 2.2);
    return float4(color, 1);
}

#elif LIGHTING_MODE == LIGHTING_MODE_SKY

float4 FSMain(FullscreenPrimitive::VsToFsWithWorldRay input) : SV_TARGET
{
    float3 dir = normalize(input.ray);
    float2 uv = Atmosphere::ComputeSkyLutTexCoord(dir);
    float3 color = SkyLut.SampleLevel(SkyLutSampler, uv, 0);
    color = Tonemap(color);
    color = pow(color, 1.0 / 2.2);
    color = Dither01(color, input.uv);
    return float4(color, 1);
}

#endif // #if LIGHTING_MODE == ...
