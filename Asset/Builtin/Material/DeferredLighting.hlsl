#pragma once

#include "Common/Fullscreen.hlsl"
#include "Common/GBuffer.hlsl"
#include "Common/Scene.hlsl"
#include "Common/Color.hlsl"

rtrc_group(Pass, FS)
{
    REFERENCE_BUILTIN_GBUFFERS(FS)

    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)
    
    rtrc_define(StructuredBuffer<PointLightShadingData>, PointLightBuffer)
    rtrc_uniform(uint, pointLightCount)
    
    rtrc_define(StructuredBuffer<DirectionalLightShadingData>, DirectionalLightBuffer)
    rtrc_uniform(uint, directionalLightCount)
};

float3 ComputePointLighting(PointLightShadingData light, float3 worldPos, float3 worldNormal)
{
    float3 surfaceToLight = light.position - worldPos;
    float3 L = normalize(surfaceToLight);
    float diffuseFactor = max(0.0, dot(L, worldNormal));
    float3 diffuse = diffuseFactor * light.color;
    return diffuse;
}

float4 FSMainDefault(FullscreenPrimitive::VsToFsWithWorldRay input) : SV_TARGET
{
    Builtin::GBufferPixelValue gbuffer = Builtin::LoadGBufferPixel(input.uv);
    float3 worldRay = input.ray;
    float viewSpaceZ = CameraUtils::DeviceZToViewZ(Camera.cameraToClipMatrix, gbuffer.depth);
    float3 worldPos = viewSpaceZ * worldRay;
    
    float3 color = 0;
    for(int i = 0; i < Pass.pointLightCount; ++i)
    {
        PointLightShadingData light = PointLightBuffer[i];
        color += ComputePointLighting(light, worldPos, gbuffer.normal);
    }
    
    color = Tonemap(color);
    color = pow(color, 1.0 / 2.2);
    return float4(color, 1);
}

float4 FSMainSky(FullscreenPrimitive::VsToFsWithWorldRay input) : SV_TARGET
{
    return float4(input.uv.x, 0, 0, 0) + float4(0, 1, 1, 0);
}
