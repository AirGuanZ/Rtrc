#pragma once

#define GBUFFER_MODE_NORMAL_DEPTH
#include "../Common/GBufferRead.hlsl"
#include "../Common/Scene.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)
    rtrc_define(RaytracingAccelerationStructure, Scene)
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)
    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2, outputSize)
    rtrc_uniform(uint, maxDepth)
};

float3 GetWorldRay(float2 uv)
{
    float3 rayAB = lerp(Camera.worldRays[0], Camera.worldRays[1], uv.x);
    float3 rayCD = lerp(Camera.worldRays[2], Camera.worldRays[3], uv.x);
    float3 ray = lerp(rayAB, rayCD, uv.y);
    return ray;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputSize))
        return;
    
    float2 uv = (tid + 0.5) / Pass.outputSize;
    GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.z >= 1)
        return;
    
    float3 worldRay = GetWorldRay(uv);
    float viewZ = CameraUtils::DeviceZToViewZ(Camera.cameraToClipMatrix, gpixel.depth);
    float3 worldPos = viewZ * worldRay + Camera.worldPosition;

    for(uint i = 0; i < Pass.maxDepth; ++i)
    {
        
    }
}
