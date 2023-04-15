#pragma once

#include "Atmosphere.hlsl"
#include "Common/Fullscreen.hlsl"
#include "Common/Scene.hlsl"
#include "Common/Color.hlsl"

#define GBUFFER_MODE GBUFFER_MODE_ALL
#include "Common/GBufferRead.hlsl"

#define LIGHTING_MODE_REGULAR 0
#define LIGHTING_MODE_SKY     1

rtrc_group(Pass, FS)
{
    REF_GBUFFERS(FS)

    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)
    
    rtrc_define(StructuredBuffer<PointLightShadingData>, PointLightBuffer)
    rtrc_uniform(uint, pointLightCount)
    
    rtrc_define(StructuredBuffer<DirectionalLightShadingData>, DirectionalLightBuffer)
    rtrc_uniform(uint, directionalLightCount)

    rtrc_define(Texture2D<float3>, SkyLut)

    rtrc_define(Texture2D<float>, ShadowMask)
    rtrc_uniform(uint, shadowMaskLightType) // 0: none; 1: pointlight; 2: directionalLight
                                            // If >= 1, should be applied to the first of the specified type
};

rtrc_sampler(SkyLutSampler, filter = linear, address_u = repeat, address_v = clamp)
rtrc_sampler(ShadowMaskSampler, filter = point, address_u = clamp, address_v = clamp)

float3 ComputePointLighting(GBufferPixel gpixel, float3 worldPos, PointLightShadingData light)
{
    float3 surfaceToLight = light.position - worldPos;
    float distance = length(surfaceToLight);
    float3 L = surfaceToLight / max(distance, 1e-5);
    float cosFactor = max(0.0, dot(L, gpixel.normal));
    float distFactor = saturate(distance * light.distFadeScale + light.distFadeBias);
    distFactor *= distFactor;
    return gpixel.albedo * cosFactor * distFactor * light.color;
}

float3 ComputeDirectionalLight(GBufferPixel gpixel, DirectionalLightShadingData light)
{
    float3 L = -light.direction;
    float cosFactor = max(0.0, dot(L, gpixel.normal));
    return gpixel.albedo * cosFactor * light.color;
}

#if LIGHTING_MODE == LIGHTING_MODE_REGULAR

float4 FSMain(FullscreenPrimitive::VsToFsWithWorldRay input) : SV_TARGET
{
    GBufferPixel gpixel = LoadGBufferPixel(input.uv);
    float3 worldRay = input.ray;
    float viewSpaceZ = CameraUtils::DeviceZToViewZ(Camera.cameraToClipMatrix, gpixel.depth);
    float3 worldPos = viewSpaceZ * worldRay + Camera.worldPosition;
    
    float3 color = 0;

    float3 mainLightResult = 0;
    if(Pass.shadowMaskLightType == 1)
    {
        PointLightShadingData light = PointLightBuffer[0];
        mainLightResult = ComputePointLighting(gpixel, worldPos, light);
    }
    else if(Pass.shadowMaskLightType == 2)
    {
        DirectionalLightShadingData light = DirectionalLightBuffer[0];
        mainLightResult = ComputeDirectionalLight(gpixel, light);
    }
    float shadow = 1;
    if(any(mainLightResult > 0))
        shadow = ShadowMask.SampleLevel(ShadowMaskSampler, input.uv, 0);
    color += shadow * mainLightResult;

    for(int i = Pass.shadowMaskLightType == 1 ? 1 : 0; i < Pass.pointLightCount; ++i)
    {
        PointLightShadingData light = PointLightBuffer[i];
        color += ComputePointLighting(gpixel, worldPos, light);
    }

    for(int j = Pass.shadowMaskLightType == 2 ? 1 : 0; j < Pass.directionalLightCount; ++j)
    {
        DirectionalLightShadingData light = DirectionalLightBuffer[j];
        color += ComputeDirectionalLight(gpixel, light);
    }
    
    color = Tonemap(color);
    color = pow(color, 1.0 / 2.2);
    color = Dither01(color, input.uv);
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
