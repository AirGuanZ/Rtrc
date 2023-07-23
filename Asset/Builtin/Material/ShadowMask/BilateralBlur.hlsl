#pragma once

#include "../Common/Scene.hlsl"

#define GBUFFER_MODE GBUFFER_MODE_DEPTH
#include "../Common/GBufferRead.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)

    rtrc_define(Texture2D<float>, ShadowMaskIn)
    rtrc_define(RWTexture2D<float>, ShadowMaskOut)
    
    rtrc_uniform(int2, resolution)
    rtrc_uniform(float4x4, cameraToClip)
};

#define BLUR_AXIS_GROUP_SIZE 64

#if IS_BLUR_DIRECTION_Y
#define GROUP_SIZE_X 1
#define GROUP_SIZE_Y BLUR_AXIS_GROUP_SIZE
#else
#define GROUP_SIZE_X BLUR_AXIS_GROUP_SIZE
#define GROUP_SIZE_Y 1
#endif

groupshared float SharedSourceData[BLUR_AXIS_GROUP_SIZE + 2 + 2];
groupshared float SharedSourceDepth[BLUR_AXIS_GROUP_SIZE + 2 + 2];

void LoadSourceData(int2 pixel, out float value, out float depth)
{
    uint2 clampedPixel = clamp(pixel, int2(0, 0), Pass.resolution);
    value = ShadowMaskIn[clampedPixel];
    
    float deviceZ = LoadGBufferDepth(clampedPixel);
    depth = deviceZ < 1 ? CameraUtils::DeviceZToViewZ(Pass.cameraToClip, deviceZ) : -1e10;
}

void AddSample(float refDepth, float sampleDepth, float sampleValue, float spatialWeight, inout float sumValue, inout float sumWeight)
{
    float depthError = abs(refDepth - sampleDepth) / refDepth;
    if(depthError > 0.1)
        return;
    
    float weight = spatialWeight / 1;//max(depthError, 0.02);
    sumValue += weight * sampleValue;
    sumWeight += weight;
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void CSMain(int2 tid : SV_DispatchThreadID, int2 ltid : SV_GroupThreadID)
{
    float value, depth;
    LoadSourceData(tid, value, depth);

#if IS_BLUR_DIRECTION_Y
    int lid = ltid.y;
#else
    int lid = ltid.x;
#endif
    SharedSourceData[2 + lid] = value;
    SharedSourceDepth[2 + lid] = depth;

    if(lid < 2)
    {
        float value2, depth2;
        int2 tid2 = tid;
#if IS_BLUR_DIRECTION_Y
        tid2.y -= 2;
#else
        tid2.x -= 2;
#endif
        LoadSourceData(tid2, value2, depth2);
        SharedSourceData[lid] = value2;
        SharedSourceDepth[lid] = depth2;
    }

    if(lid >= BLUR_AXIS_GROUP_SIZE - 2)
    {
        float value3, depth3;
        int2 tid3 = tid;
#if IS_BLUR_DIRECTION_Y
        tid3.y += 2;
#else
        tid3.x += 2;
#endif
        LoadSourceData(tid3, value3, depth3);
        SharedSourceData[4 + lid] = value3;
        SharedSourceDepth[4 + lid] = depth3;
    }

    GroupMemoryBarrierWithGroupSync();

    if(depth < 0)
        return;

    float v0 = SharedSourceData[2 + lid - 2];
    float v1 = SharedSourceData[2 + lid - 1];
    float v2 = SharedSourceData[2 + lid - 0];
    float v3 = SharedSourceData[2 + lid + 1];
    float v4 = SharedSourceData[2 + lid + 2];
    
    float d0 = SharedSourceDepth[2 + lid - 2];
    float d1 = SharedSourceDepth[2 + lid - 1];
    float d2 = SharedSourceDepth[2 + lid - 0];
    float d3 = SharedSourceDepth[2 + lid + 1];
    float d4 = SharedSourceDepth[2 + lid + 2];

    float sumValue = 0, sumWeight = 0;
    AddSample(d2, d0, v0, 1.0, sumValue, sumWeight);
    AddSample(d2, d1, v1, 2.0, sumValue, sumWeight);
    AddSample(d2, d2, v2, 3.0, sumValue, sumWeight);
    AddSample(d2, d3, v3, 2.0, sumValue, sumWeight);
    AddSample(d2, d4, v4, 1.0, sumValue, sumWeight);
    float result = sumValue / max(1e-10, sumWeight);

    if(all(tid < Pass.resolution))
        ShadowMaskOut[tid] = result;
}
