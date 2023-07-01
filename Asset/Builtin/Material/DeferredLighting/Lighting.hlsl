#pragma once

#include "../Common/Fullscreen.hlsl"
#include "../Common/Scene.hlsl"
#include "../Common/Color.hlsl"

#define GBUFFER_MODE GBUFFER_MODE_ALL
#include "../Common/GBufferRead.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)

    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)
    
    rtrc_define(StructuredBuffer<PointLightShadingData>, PointLightBuffer)
    rtrc_uniform(uint, pointLightCount)
    
    rtrc_define(StructuredBuffer<DirectionalLightShadingData>, DirectionalLightBuffer)
    rtrc_uniform(uint, directionalLightCount)

    rtrc_define(Texture2D<float>, MainLightShadowMask)
#if MAIN_LIGHT_MODE == MAIN_LIGHT_MODE_POINT
    rtrc_uniform(PointLightShadingData, mainPointLightShadingData)
#elif MAIN_LIGHT_MODE == MAIN_LIGHT_MODE_DIRECTIONAL
    rtrc_uniform(DirectionalLightShadingData, mainDirectionalLightShadingData)
#elif MAIN_LIGHT_MODE != MAIN_LIGHT_MODE_NONE
    #error "Unsupported main light mode"
#endif

    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2, resolution)
    rtrc_uniform(float2, rcpResolution)
};

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

float3 GetWorldRay(float2 uv)
{
    float3 rayAB = lerp(Camera.worldRays[0], Camera.worldRays[1], uv.x);
    float3 rayCD = lerp(Camera.worldRays[2], Camera.worldRays[3], uv.x);
    float3 ray = lerp(rayAB, rayCD, uv.y);
    return ray;
}

[numthreads(8, 1, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;
    
    float2 uv = (tid + 0.5) * Pass.rcpResolution;
    GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.depth >= 1)
        return;
    
    float3 worldRay = GetWorldRay(uv);
    float viewSpaceZ = CameraUtils::DeviceZToViewZ(Camera.cameraToClipMatrix, gpixel.depth);
    float3 worldPos = viewSpaceZ * worldRay + Camera.worldPosition;

    // Main light color

    float3 mainLightResult = 0;
#if MAIN_LIGHT_MODE == MAIN_LIGHT_MODE_POINT
    mainLightResult = ComputePointLighting(gpixel, worldPos, Pass.mainPointLightShadingData);
#elif MAIN_LIGHT_MODE == MAIN_LIGHT_MODE_DIRECTIONAL
    mainLightResult = ComputeDirectionalLight(gpixel, Pass.mainDirectionalLightShadingData);
#endif

    // Main light shadow

    float shadow = 1;
#if MAIN_LIGHT_MODE != MAIN_LIGHT_MODE_NONE
    if(any(mainLightResult > 0))
        shadow = MainLightShadowMask.SampleLevel(ShadowMaskSampler, uv, 0);
#endif

    // Other lights

    float3 color = shadow * mainLightResult;
    for(int i = 0; i < Pass.pointLightCount; ++i)
    {
        PointLightShadingData light = PointLightBuffer[i];
        color += ComputePointLighting(gpixel, worldPos, light);
    }

    for(int j = 0; j < Pass.directionalLightCount; ++j)
    {
        DirectionalLightShadingData light = DirectionalLightBuffer[j];
        color += ComputeDirectionalLight(gpixel, light);
    }

    // Output
    
    color = Tonemap(color);
    color = pow(color, 1.0 / 2.2);
    color = Dither01(color, uv);
    Output[tid] = float4(color, 1);
}
