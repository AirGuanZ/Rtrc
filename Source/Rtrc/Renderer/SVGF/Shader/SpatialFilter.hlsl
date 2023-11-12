#pragma once

#include "Rtrc/Shader/Common/Color.hlsl"
#include "Rtrc/Shader/Common/Scene.hlsl"

#define GROUP_SIZE 128
#define RADIUS 2

rtrc_group(Pass)
{
    rtrc_define(ConstantBuffer<CameraData>, Camera)
    rtrc_define(Texture2D<float>, Depth)
    rtrc_define(Texture2D<float3>, Normal)
    rtrc_define(Texture2D<float4>, Color)
    rtrc_define(Texture2D<float>, Variance)
    rtrc_define(RWTexture2D<float4>, OutputColor)
    rtrc_define(RWTexture2D<float>, OutputVariance)
    rtrc_uniform(uint2, resolution)
};

float ComputePointPlaneDistance(float3 o, float3 d, float3 p)
{
    const float3 op = p - o;
    return dot(op, d);
}

float ComputeProjectionDistance(float3 o, float3 d, float3 p)
{
    const float3 op = p - o;
    const float3 projOP = op - dot(op, d);
    return length(projOP);
}

void AddSample(
    float        filterKernel,
    float3       centerPos,
    float3       centerNor,
    float3       centerColor,
    float        lumVariance,
    int2         sampleCoord,
    float        sampleDepth,
    float3       sampleNor,
    float3       sampleColor,
    float        sampleVariance,
    inout float3 sumWeightedColor,
    inout float  sumWeightedVariance,
    inout float  sumWeight)
{   
    const float2 sampleUV = (sampleCoord + 0.5) / Pass.resolution;
    const float3 samplePos = CameraUtils::GetWorldPosition(Camera, sampleUV, sampleDepth);

    const float sigmaN = 32;
    const float wN = pow(max(0, dot(centerNor, sampleNor)), sigmaN);

    const float centerLum = RelativeLuminance(centerColor);
    const float sampleLum = RelativeLuminance(sampleColor);

    const float sigmaL = 4;
    const float wL = exp(-abs(centerLum - sampleLum) / max(1e-5, sigmaL * sqrt(lumVariance)));

    const float sigmaZ = 1;
    const float dist = ComputePointPlaneDistance(centerPos, centerNor, samplePos);
    const float projDist = ComputeProjectionDistance(centerPos, centerNor, samplePos);
    const float wP = exp(-abs(dist) / (sigmaZ * abs(projDist) + 1e-5));

    const float w = wN * wL * wP;
    sumWeightedColor    += filterKernel * w * sampleColor;
    sumWeightedVariance += filterKernel * w * sampleVariance;
    sumWeight           += filterKernel * w;
}

float EvalFilterKernel(int x)
{
    x = abs(x);
    return x == 0 ? 3.0 / 8.0 :
           x == 1 ? 1.0 / 4.0 :
                    1.0 / 16.0;
}

groupshared float4 gNormalDepth  [GROUP_SIZE + 2 * RADIUS];
groupshared float4 gColorVariance[GROUP_SIZE + 2 * RADIUS];

#if IS_DIRECTION_Y
[numthreads(1, GROUP_SIZE, 1)]
#else
[numthreads(GROUP_SIZE, 1, 1)]
#endif
void CSMain(int2 tid : SV_DispatchThreadID, int2 lid : SV_GroupThreadID)
{
    float4 normalDepth = float4(0, 0, 0, 1), colorVariance = 0;
    float historyFramesT = 0;
    if(all(tid < Pass.resolution))
    {
        normalDepth.xyz   = 2 * Normal[tid] - 1;
        normalDepth.w     = Depth[tid];
        colorVariance.xyz = Color[tid].xyz;
        colorVariance.w   = Variance[tid];
        historyFramesT    = Color[tid].w;

        colorVariance.w = lerp(max(colorVariance.w, 3), colorVariance.w, historyFramesT);
    }
    const int lv = lid[IS_DIRECTION_Y];
    gNormalDepth[RADIUS + lv] = normalDepth;
    gColorVariance[RADIUS + lv] = colorVariance;

    bool loadOffsetedData = false;
    int2 offsetedTid = tid;
    int offsetedDst = 0;
    if(lv < RADIUS)
    {
        loadOffsetedData = true;
        offsetedTid[IS_DIRECTION_Y] = max(0, tid[IS_DIRECTION_Y] - RADIUS);
        offsetedDst = lv;
    }
    else if(lv >= GROUP_SIZE - RADIUS)
    {
        loadOffsetedData = true;
        offsetedTid[IS_DIRECTION_Y] = min(int(Pass.resolution[IS_DIRECTION_Y] - 1), tid[IS_DIRECTION_Y] + RADIUS);
        offsetedDst = 2 * RADIUS + lv;
    }

    if(loadOffsetedData)
    {
        const float3 offsetedNormal = 2 * Normal[offsetedTid] - 1;
        const float offsetedDepth = Depth[offsetedTid];
        gNormalDepth[offsetedDst] = float4(offsetedNormal, offsetedDepth);
        
        const float3 offsetedColor = Color[offsetedTid].xyz;
        const float offsetedVariance = Variance[offsetedTid];
        gColorVariance[offsetedDst] = float4(offsetedColor, offsetedVariance);
    }

    GroupMemoryBarrierWithGroupSync();

    if(normalDepth.w >= 1)
        return;
        
    const float2 uv = (tid + 0.5) / Pass.resolution;
    const float3 worldPos = CameraUtils::GetWorldPosition(Camera, uv, normalDepth.w);
    
    float3 sumColor = 0;
    float sumVariance = 0;
    float sumWeight = 0;
    for(int i = -RADIUS; i <= RADIUS; ++i)
    {
        const float4 nNormalDepth = gNormalDepth[RADIUS + lv + i];
        if(nNormalDepth.w >= 1)
            continue;
        const float4 nColorVariance = gColorVariance[RADIUS + lv + i];
        const float h = EvalFilterKernel(i);
        int2 nid = tid;
        nid[IS_DIRECTION_Y] += i;
        AddSample(
            h, worldPos, normalDepth.xyz, colorVariance.xyz, colorVariance.w,
            nid, nNormalDepth.w, nNormalDepth.xyz, nColorVariance.xyz, nColorVariance.w,
            sumColor, sumVariance, sumWeight);
    }

    const float3 filteredColor = sumColor / max(1e-5, sumWeight);
    const float filteredVariance = sumVariance / max(1e-5, sumWeight);
    OutputColor[tid] = float4(filteredColor, historyFramesT);
    OutputVariance[tid] = filteredVariance;
}
