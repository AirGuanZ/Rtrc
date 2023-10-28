#pragma once

#define GBUFFER_MODE GBUFFER_MODE_NORMAL

#include "Light.hlsl"
#include "Reservoir.hlsl"
#include "Rtrc/Shader/Common/Color.hlsl"
#include "Rtrc/Shader/Common/GBufferRead.hlsl"
#include "Rtrc/Shader/Common/Random.hlsl"
#include "Rtrc/Shader/Common/Scene.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)

    rtrc_define(Texture2D<float3>, Sky)

    rtrc_define(Texture2D<float>, PrevDepth)
    rtrc_define(Texture2D<float>, CurrDepth)

    rtrc_define(Texture2D<float3>, PrevNormal)
    rtrc_define(Texture2D<float3>, CurrNormal)

    rtrc_define(ConstantBuffer<CameraData>, PrevCamera)
    rtrc_define(ConstantBuffer<CameraData>, CurrCamera)

    rtrc_define(StructuredBuffer<LightShadingData>, LightShadingDataBuffer)

    rtrc_define(Texture2D<uint4>, NewReservoirs)
    rtrc_define(Texture2D<uint4>, PrevReservoirs)
    rtrc_define(RWTexture2D<uint4>, CurrReservoirs)

    rtrc_define(RWTexture2D<uint>, PcgState)

    rtrc_uniform(uint, maxM)
    rtrc_uniform(uint, lightCount)
    rtrc_uniform(uint2, resolution)
};

uint4 Reuse(uint2 tid, float deviceZ, float2 uv)
{
    const float viewZ = CameraUtils::DeviceZToViewZ(CurrCamera, deviceZ);

    const float3 worldRay = CameraUtils::GetWorldRay(CurrCamera, uv);
    const float3 worldPos = viewZ * worldRay + CurrCamera.position;
    
    const uint4 currEncodedState = NewReservoirs[tid];
    Reservoir currR;
    currR.SetEncodedState(currEncodedState);

    // Reproject

    const float4 prevClip = mul(PrevCamera.worldToClip, float4(worldPos, 1));
    const float2 prevScr = prevClip.xy / prevClip.w;
    if(prevClip.w <= 0 || any(abs(prevScr) > 1))
        return currEncodedState;
    
    const float2 prevUV = 0.5 + float2(0.5, -0.5) * prevScr;
    const uint2 prevCoord = min(uint2(prevUV * Pass.resolution), Pass.resolution - 1);
    const float prevDeviceZ = PrevDepth[prevCoord];
    if(prevDeviceZ >= 1)
        return currEncodedState;
    
    const float expectedPrevDeviceZ = prevClip.z / prevClip.w;
    const float expectedPrevViewZ = CameraUtils::DeviceZToViewZ(PrevCamera, expectedPrevDeviceZ);
    const float actualPrevViewZ = CameraUtils::DeviceZToViewZ(PrevCamera, prevDeviceZ);
    const float absErr = abs(expectedPrevViewZ - actualPrevViewZ);
    const float relErr = absErr / max(expectedPrevViewZ, 0.01);
    if(absErr >= 0.2 || relErr >= 0.02)
        return currEncodedState;
    
    const float3 prevNormal = 2 * PrevNormal[prevCoord] - 1;
    const float3 currNormal = 2 * CurrNormal[tid] - 1;
    if(dot(prevNormal, currNormal) < 0.95)
        return currEncodedState;

    // Load prev/curr states

    const uint4 prevEncodedState = PrevReservoirs[prevCoord];
    Reservoir prevR;
    prevR.SetEncodedState(prevEncodedState);
    if(!prevR.M || prevR.wsum <= 1e-3 || prevR.W <= 1e-3)
        return currEncodedState;

    // Combine reservoirs

    Pcg::Sampler pcgSampler;
    pcgSampler.SetState(PcgState[tid]);
    
    const float3 normal = LoadGBufferNormal(uv);

    float3 reshade, lightSampleGeometry;
    if(prevR.data.lightIndex < Pass.lightCount)
    {
        const LightShadingData prevLightData = LightShadingDataBuffer[prevR.data.lightIndex];
        reshade = ShadeLightNoVisibility(worldPos, normal, prevLightData, prevR.data.lightUV, lightSampleGeometry);
    }
    else
        reshade = ShadeSkyNoVisibility(worldPos, normal, Sky, prevR.data.lightUV, lightSampleGeometry);
    const float reshadePBar = RelativeLuminance(reshade);
    
    float finalPBar;
    if(currR.W > 1e-3)
        finalPBar = currR.wsum / currR.M / currR.W;
    else
        finalPBar = 1;

    if(currR.Update(prevR.data, reshadePBar * prevR.W * prevR.M, pcgSampler.NextFloat()))
        finalPBar = reshadePBar;

    currR.M += prevR.M;

    if(currR.M > Pass.maxM)
    {
        currR.wsum *= float(Pass.maxM) / currR.M;
        currR.M = Pass.maxM;
    }

    currR.W = 1.0 / max(finalPBar, 1e-5) * (1.0 / currR.M) * currR.wsum;

    PcgState[tid] = pcgSampler.GetState();
    return currR.GetEncodedState();
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;

    const float2 uv = (tid + 0.5) / Pass.resolution;
    const float deviceZ = CurrDepth[tid];
    if(deviceZ >= 1)
        return;
    
    const uint4 newState = Reuse(tid, deviceZ, uv);
    CurrReservoirs[tid] = newState;
}
