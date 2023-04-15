#pragma once

#include "../Common/Scene.hlsl"

#define GBUFFER_MODE GBUFFER_MODE_NORMAL_DEPTH
#include "../Common/GBufferRead.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)

    rtrc_define(RaytracingAccelerationStructure, Scene)
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)

    rtrc_define(RWTexture2D<float>, OutputTextureRW)

    rtrc_uniform(uint2,  outputResolution)
    rtrc_uniform(float2, rcpOutputResolution)
    rtrc_uniform(uint,   lightType)       // 0: point; 1: directional
    rtrc_uniform(float3, lightGeometry) // position or normalized direction
};

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_DIRECTIONAL 1

float3 GetWorldRay(float2 uv)
{
    float3 rayAB = lerp(Camera.worldRays[0], Camera.worldRays[1], uv.x);
    float3 rayCD = lerp(Camera.worldRays[2], Camera.worldRays[3], uv.x);
    float3 ray = lerp(rayAB, rayCD, uv.y);
    return ray;
}

[numthreads(8, 8, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputResolution))
        return;

    GBufferPixel gpixel = LoadGBufferPixel(input.uv);
    if(gpixel.depth >= 1)
        return;

    float2 uv = (tid + 0.5) * Pass.rcpOutputResolution;
    float3 worldRay = GetWorldRay(uv);
    float viewZ = CameraUtils::DeviceZToViewZ(Camera.cameraToClipMatrix, gpixel.depth);
    float3 worldPosition = viewZ * worldRay + Camera.worldPosition;
    float3 worldNormal = gpixel.normal;

    float3 origin = worldPosition + 1e-3 * worldNormal;
    float3 direction = Pass.lightType == LIGHT_TYPE_POINT ? normalize(Pass.lightGeometry - worldPosition) : -Pass.lightGeometry;

    RayDesc ray;
    ray.Origin    = origin;
    ray.Durection = direction;
    ray.TMin      = 0;
    ray.TMax      = 1e5;

    RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH> rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xff, ray);
    rayQuery.Proceed();
    bool isInShadow = rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT;

    OutputTextureRW[tid] = isInShadow ? 0.0 : 1.0;
}
