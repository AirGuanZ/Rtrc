#pragma once

#include "Light.hlsl"
#include "Reservoir.hlsl"
#include "Rtrc/Shader/Common/Color.hlsl"
#include "Rtrc/Shader/Common/Random.hlsl"
#include "Rtrc/Shader/Common/Scene.hlsl"

rtrc_group(Pass, CS)
{
    rtrc_define(ConstantBuffer<CameraData>, Camera)

    rtrc_define(Texture2D<float>, Depth)
    rtrc_define(Texture2D<float3>, Normal)

    rtrc_define(StructuredBuffer<LightShadingData>, LightShadingDataBuffer)
    
    rtrc_define(Texture2D<uint4>, InputReservoirs)
    rtrc_define(RWTexture2D<uint4>, OutputReservoirs)

    rtrc_define(RWTexture2D<uint>, PcgState)

    rtrc_uniform(uint2, resolution)
    rtrc_uniform(uint, N)
    rtrc_uniform(uint, maxM)
    rtrc_uniform(float, radius)
};

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;
    
    const float deviceZ = Depth[tid];
    if(deviceZ >= 1)
        return;
    const float viewZ = CameraUtils::DeviceZToViewZ(Camera, deviceZ);
    
    const float2 uv = (tid + 0.5) / Pass.resolution;
    const float3 worldRay = CameraUtils::GetWorldRay(Camera, uv);
    const float3 worldPos = viewZ * worldRay + Camera.position;
    const float3 normal = 2.0 * Normal[tid] - 1.0;
    
    Reservoir r;
    r.SetEncodedState(InputReservoirs[tid]);

    Pcg::Sampler pcgSampler;
    pcgSampler.SetState(PcgState[tid]);

    float finalPBar;
    if(r.W > 1e-3)
        finalPBar = r.wsum / r.M / r.W;
    else
        finalPBar = 1;

    for(uint i = 0; i < Pass.N; ++i)
    {
        const float2 offset = Pass.radius * pcgSampler.NextUniformOnUnitDisk();
        const int2 sid = clamp(int2(floor(tid + 0.5 + offset)), 0, int2(Pass.resolution) - 1);
        
        const float sDeviceZ = Depth[sid];
        if(sDeviceZ >= 1)
            continue;
        const float sViewZ = CameraUtils::DeviceZToViewZ(Camera, sDeviceZ);
        const float absErr = abs(viewZ - sViewZ);
        const float relErr = absErr / max(viewZ, 0.01);
        if(absErr >= 1 || relErr >= 0.03)
            continue;

        const float3 sNormal = 2.0 * Normal[sid] - 1.0;
        if(dot(sNormal, normal) < 0.9)
            continue;
            
        const uint4 sRState = InputReservoirs[sid];
        Reservoir sR;
        sR.SetEncodedState(sRState);
        if(!sR.M || sR.wsum <= 1e-3 || sR.W <= 1e-3)
            continue;

        const LightShadingData light = LightShadingDataBuffer[sR.data.lightIndex];
        const float3 reshade = ShadeNoVisibility(worldPos, normal, light, sR.data.lightUV);
        const float reshadePBar = RelativeLuminance(reshade);

        if(r.Update(sR.data, reshadePBar * sR.W * sR.M, pcgSampler.NextFloat()))
            finalPBar = reshadePBar;
        r.M += sR.M;
    }
    
    if(r.M > Pass.maxM)
    {
        r.wsum *= float(Pass.maxM) / r.M;
        r.M = Pass.maxM;
    }

    r.W = 1.0 / max(finalPBar, 1e-5) * (1.0 / r.M) * r.wsum;

    OutputReservoirs[tid] = r.GetEncodedState();
    PcgState[tid] = pcgSampler.GetState();
}
