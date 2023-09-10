#pragma once

#include "../Common/Scene.hlsl"

rtrc_group(Pass, CS)
{
    rtrc_define(Texture2D<float>, CurrDepth)
    rtrc_define(Texture2D<float>, PrevDepth)

    rtrc_define(ConstantBuffer<CameraData>, CurrCamera)
    rtrc_define(ConstantBuffer<CameraData>, PrevCamera)

    rtrc_define(Texture2D<float3>,   History)
    rtrc_define(Texture2D<float3>,   TraceResult)
    rtrc_define(RWTexture2D<float4>, Resolved)

    rtrc_uniform(uint2, resolution)
};

rtrc_sampler(DepthSampler, filter = linear, address_u = clamp, address_v = clamp)
rtrc_sampler(HistorySampler, filter = linear, address_u = clamp, address_v = clamp)

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;

    float2 uv = (tid + 0.5) / Pass.resolution;
    float3 worldRay = CameraUtils::GetWorldRay(CurrCamera, uv);

    float deviceZ = CurrDepth[tid];
    if(deviceZ >= 1)
        return;

    float viewZ = CameraUtils::DeviceZToViewZ(CurrCamera, deviceZ);
    float3 worldPos = viewZ * worldRay + CurrCamera.position;

    float4 prevClip = mul(PrevCamera.worldToClip, float4(worldPos, 1));
    float2 prevScr = prevClip.xy / prevClip.w;
    float2 prevUV = 0.5 + float2(0.5, -0.5) * prevScr;
    float prevDeviceZ = PrevDepth.SampleLevel(DepthSampler, prevUV, 0);

    float3 newValue = TraceResult[tid];
    if(any(saturate(prevUV) != prevUV) || prevDeviceZ >= 1)
    {
        Resolved[tid] = float4(newValue, 1);
        return;
    }

    float prevViewZ = CameraUtils::DeviceZToViewZ(PrevCamera, prevDeviceZ);
    float3 prevWorldRay = CameraUtils::GetWorldRay(PrevCamera, prevUV);
    float3 prevWorldPos = PrevCamera.position + prevWorldRay * prevViewZ;

    float alpha = 0.03;
    alpha *= exp(min(3 * distance(worldPos, prevWorldPos), 10.0));
    alpha = saturate(alpha);
    
    float3 history = History.SampleLevel(HistorySampler, prevUV, 0);
    float3 result = lerp(history, newValue, alpha);
    Resolved[tid] = float4(result, 1);
}
