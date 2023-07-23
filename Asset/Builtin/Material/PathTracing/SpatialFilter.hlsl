#pragma once

#ifndef IS_FILTER_DIRECTION_Y
#error "macro 'IS_FILTER_DIRECTION_Y' must be defined before including spatial filtering header file"
#endif

#define GBUFFER_MODE GBUFFER_MODE_NORMAL_DEPTH
#include "../Common/Scene.hlsl"
#include "../Common/GBufferRead.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)

    rtrc_define(Texture2D<float4>, In)
    rtrc_define(RWTexture2D<float4>, Out)

    rtrc_uniform(int2, resolution)
    rtrc_uniform(float4x4, cameraToClip)
};

void LoadSourceData(int2 coord, out float3 value, out float depth, out float3 normal)
{
    coord = clamp(coord, int2(0, 0), Pass.resolution);
    value = In[coord].xyz;
    float deviceZ = LoadGBufferDepth(coord);
    depth = deviceZ < 1 ? CameraUtils::DeviceZToViewZ(Pass.cameraToClip, deviceZ) : -1e10;
    normal = LoadGBufferNormal(coord);
}

void AddSample(
    float        refDepth,
    float3       refNormal,
    float        sampleDepth,
    float3       sampleNormal,
    float3       sampleValue,
    float        spatialWeight,
    inout float3 accumValue,
    inout float  accumWeight)
{
    float depthError = abs(refDepth - sampleDepth) / refDepth;
    if(depthError > 0.1)
        return;
    if(dot(refNormal, sampleNormal) < 0.9)
        return;
    float weight = spatialWeight;
    accumValue += weight * sampleValue;
    accumWeight += weight;
}

#define FILTER_AXIS_GROUP_SIZE 64
#define FILTER_RADIUS 5

groupshared float4 SharedSourceValue[FILTER_AXIS_GROUP_SIZE + 2 * FILTER_RADIUS];
groupshared float4 SharedSourceDepthNormal[FILTER_AXIS_GROUP_SIZE + 2 * FILTER_RADIUS];

#if IS_FILTER_DIRECTION_Y
[numthreads(1, FILTER_AXIS_GROUP_SIZE, 1)]
#else
[numthreads(FILTER_AXIS_GROUP_SIZE, 1, 1)]
#endif
void CSMain(int2 tid : SV_DispatchThreadID, int2 ltid : SV_GroupThreadID)
{
    float3 value, normal; float depth;
    LoadSourceData(tid, value, depth, normal);

#if IS_FILTER_DIRECTION_Y
    int lid = ltid.y;
#else
    int lid = ltid.x;
#endif
    SharedSourceValue[FILTER_RADIUS + lid] = float4(value, 0);
    SharedSourceDepthNormal[FILTER_RADIUS + lid] = float4(depth, normal);

    if(lid < FILTER_RADIUS)
    {
        float3 value2, normal2; float depth2;
#if IS_FILTER_DIRECTION_Y
        int2 tid2 = int2(tid.x, tid.y - FILTER_RADIUS);
#else
        int2 tid2 = int2(tid.x - FILTER_RADIUS, tid.y);
#endif
        LoadSourceData(tid2, value2, depth2, normal2);
        SharedSourceValue[lid] = float4(value2, 0);
        SharedSourceDepthNormal[lid] = float4(depth2, normal2);
    }

    if(lid >= FILTER_AXIS_GROUP_SIZE - FILTER_RADIUS)
    {
        float3 value3, normal3; float depth3;
#if IS_FILTER_DIRECTION_Y
        int2 tid3 = int2(tid.x, tid.y + FILTER_RADIUS);
#else
        int2 tid3 = int2(tid.x + FILTER_RADIUS, tid.y);
#endif
        LoadSourceData(tid3, value3, depth3, normal3);
        SharedSourceValue[2 * FILTER_RADIUS + lid] = float4(value3, 0);
        SharedSourceDepthNormal[2 * FILTER_RADIUS + lid] = float4(depth3, normal3);
    }

    GroupMemoryBarrierWithGroupSync();

    if(depth < 0)
        return;

    float3 sumValue = 0;
    float sumWeight = 0;

    [unroll]
    for(int i = -FILTER_RADIUS; i <= FILTER_RADIUS; ++i)
    {
        float3 sampleValue = SharedSourceValue[FILTER_RADIUS + lid + i].xyz;
        float4 sampleDepthNormal = SharedSourceDepthNormal[FILTER_RADIUS + lid + i];
        float spatialWeight = 1.0 - abs(i / (FILTER_RADIUS + 1.0));
        AddSample(depth, normal, sampleDepthNormal.x, sampleDepthNormal.yzw, sampleValue, spatialWeight, sumValue, sumWeight);
    }

    float3 result = sumValue / max(1e-10, sumWeight);
    if(all(tid < Pass.resolution))
        Out[tid] = float4(result, 1);
}
