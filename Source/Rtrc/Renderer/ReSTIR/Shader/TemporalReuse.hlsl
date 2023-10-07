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

    rtrc_define(Texture2D<float>, PrevDepth)
    rtrc_define(Texture2D<float>, CurrDepth)

    rtrc_define(ConstantBuffer<CameraData>, PrevCamera)
    rtrc_define(ConstantBuffer<CameraData>, CurrCamera)

    rtrc_define(StructuredBuffer<LightShadingData>, LightShadingDataBuffer)

    rtrc_define(Texture2D<uint4>, HistoryReservoirs)
    rtrc_define(RWTexture2D<uint4>, NewReservoirs)

    rtrc_define(RWTexture2D<uint>, PcgState)

    rtrc_uniform(uint, maxM)
    rtrc_uniform(uint2, resolution)
};

rtrc_sampler(HistorySampler, filter = point, address = clamp)

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;
        
    // Decode gbuffer

    const float2 uv = (tid + 0.5) / Pass.resolution;
    const float3 worldRay = CameraUtils::GetWorldRay(CurrCamera, uv);
    const float deviceZ = CurrDepth[tid];
    if(deviceZ >= 1)
        return;
    const float viewZ = CameraUtils::DeviceZToViewZ(CurrCamera, deviceZ);
    const float3 worldPos = viewZ * worldRay + CurrCamera.position;

    // Reproject

    const float4 prevClip = mul(PrevCamera.worldToClip, float4(worldPos, 1));
    const float2 prevScr = prevClip.xy / prevClip.w;
    const float2 prevUV = 0.5 + float2(0.5, -0.5) * prevScr;
    const float prevDeviceZ = PrevDepth.SampleLevel(HistorySampler, prevUV, 0);
    if(prevClip.w <= 0 || any(abs(prevScr) > 1) || prevDeviceZ >= 1)
        return;
    
    const float expectedPrevDeviceZ = prevClip.z / prevClip.w;
    const float expectedPrevViewZ = CameraUtils::DeviceZToViewZ(PrevCamera, expectedPrevDeviceZ);
    const float actualPrevViewZ = CameraUtils::DeviceZToViewZ(PrevCamera, prevDeviceZ);
    const float absErr = abs((expectedPrevViewZ - actualPrevViewZ));
    const float relErr = absErr / max(expectedPrevViewZ, 0.01);
    if(absErr >= 5 || relErr >= 0.03)
        return;

    // Load prev/curr states

    const uint2 prevCoord = min(uint2(prevUV * Pass.resolution), Pass.resolution - 1);
    const uint4 prevEncodedState = HistoryReservoirs[prevCoord];
    Reservoir prevR;
    prevR.SetEncodedState(prevEncodedState);
    if(!prevR.M || prevR.wsum <= 1e-3 || prevR.pbar <= 1e-3)
        return;
    
    const uint4 currEncodedState = NewReservoirs[tid];
    Reservoir currR;
    currR.SetEncodedState(currEncodedState);

    // Combine reservoirs

    Pcg::Sampler pcgSampler;
    pcgSampler.SetState(PcgState[tid]);

    const LightShadingData prevLightData = LightShadingDataBuffer[prevR.data.lightIndex];
    const float3 normal = LoadGBufferNormal(uv);
    const float3 reshade = ShadeNoVisibility(worldPos, normal, prevLightData, prevR.data.lightUV);
    const float reshadePBar = RelativeLuminance(reshade);

    currR.Update(prevR.data, reshadePBar * prevR.W() * prevR.M, reshadePBar, pcgSampler.NextFloat());
    currR.M += prevR.M;

    if(currR.M > Pass.maxM)
    {
        currR.wsum *= float(Pass.maxM) / currR.M;
        currR.M = Pass.maxM;
    }

    NewReservoirs[tid] = currR.GetEncodedState();
    PcgState[tid] = pcgSampler.GetState();
}
