#pragma once

#define GBUFFER_MODE GBUFFER_MODE_NORMAL_DEPTH

#include "Light.hlsl"
#include "Reservoir.hlsl"
#include "Rtrc/Shader/Common/GBufferRead.hlsl"

rtrc_group(Pass)
{
    REF_GBUFFERS(CS)

    rtrc_define(ConstantBuffer<CameraData>, Camera)
    rtrc_define(Texture2D<float3>, Sky)
    rtrc_define(RaytracingAccelerationStructure, Scene)
    
    rtrc_define(StructuredBuffer<LightShadingData>, LightShadingDataBuffer)
    rtrc_define(Texture2D<uint4>, Reservoirs)

    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2, outputResolution)
    rtrc_uniform(uint, lightCount)
};

float3 Resolve(uint2 tid)
{
    const float2 uv = (tid + 0.5) / Pass.outputResolution;
    const float3 worldRay = CameraUtils::GetWorldRay(Camera, uv);
    const GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.depth >= 1)
        return 0;
    
    Reservoir r;
    r.SetEncodedState(Reservoirs[tid]);
    if(r.wsum <= 0)
    {
        Output[tid] = float4(0, 0, 0, 0);
        return 0;
    }
    
    const float viewZ = CameraUtils::DeviceZToViewZ(Camera, gpixel.depth);
    const float3 worldPos = viewZ * worldRay + Camera.position;

    float3 f, lightSampleGeometry; bool isLightSampleGeometryDirection;
    if(r.data.lightIndex < Pass.lightCount)
    {
        const LightShadingData light = LightShadingDataBuffer[r.data.lightIndex];
        isLightSampleGeometryDirection = light.type == LIGHT_SHADING_DATA_TYPE_DIRECTION;
        f = ShadeLightNoVisibility(worldPos, gpixel.normal, light, r.data.lightUV, lightSampleGeometry);
    }
    else
    {
        isLightSampleGeometryDirection = true;
        f = ShadeSkyNoVisibility(worldPos, gpixel.normal, Sky, r.data.lightUV, lightSampleGeometry);
    }

    if(all(f <= 0))
        return 0;

    RayDesc ray;
    ray.Origin = worldPos + 4e-3 * gpixel.normal;
    ray.TMin = 0;
    if(isLightSampleGeometryDirection)
    {
        ray.Direction = lightSampleGeometry;
        ray.TMax = 1e5;
    }
    else
    {
        ray.Direction = lightSampleGeometry - ray.Origin;
        ray.TMax = 1;
    }
    
    RayQuery<RAY_FLAG_NONE> rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xff, ray);
    rayQuery.Proceed();
    if(rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        f = 0;
    
    f *= r.W;
    return f;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputResolution))
        return;
    const float3 result = Resolve(tid);
    Output[tid] = float4(result, 1);
}
