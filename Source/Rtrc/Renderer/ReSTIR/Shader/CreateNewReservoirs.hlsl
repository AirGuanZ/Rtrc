#pragma once

#define GBUFFER_MODE GBUFFER_MODE_NORMAL_DEPTH

#include "Generated/Reflection.hlsl"
#include "Rtrc/Shader/Common/Scene.hlsl"
#include "Rtrc/Shader/Common/GBufferRead.hlsl"
#include "Rtrc/Shader/Common/Random.hlsl"
#include "Common.hlsl"

typedef LightShadingData = Rtrc::Renderer::ReSTIRDetail::LightShadingData;

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)
    rtrc_define(ConstantBuffer<CameraData>, Camera)

    rtrc_define(StructuredBuffer<LightShadingData>, LightShadingDataBuffer)
    rtrc_uniform(uint, lightCount)

    rtrc_define(RWTexture2D<uint>, PcgStateTextureRW)
    
    rtrc_define(RWTexture2D<float4>, OutputTextureRW)
    rtrc_uniform(uint2, outputResolution)

    rtrc_uniform(uint, M) // Number of candidates
};

float3 ShadeNoVisibility(float3 position, float3 normal, LightShadingData light, float2 ligthUV)
{
    // TODO
    return 1;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputResolution))
        return;
        
    float2 uv = (tid + 0.5) / Pass.outputResolution;
    GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.depth >= 1)
        return;
    
    float3 worldRay = CameraUtils::GetWorldRay(Camera, uv);
    float viewZ = CameraUtils::DeviceZToViewZ(Camera, gpixel.depth);
    float3 worldPos = viewZ * worldRay + Camera.position;
    
    Pcg::Sampler pcgSampler;
    pcgSampler.SetState(PcgStateTextureRW[tid]);

    Reservoir r;
    r.Reset();
    
    for(uint i = 0; i < Pass.M; ++i)
    {
        const uint lightIndex = pcgSampler.NextUInt() % Pass.lightCount;
        const LightShadingData light = LightShadingDataBuffer[lightIndex];
        const float2 lightUV = pcgSampler.NextFloat2();
        const float3 L = ShadeNoVisibility(worldPos, gpixel.normal, light, lightUV);
        const float w = RelativeLuminance(L);
        if(w > 0)
        {
            ReservoirData data;
            data.lightIndex = lightIndex;
            data.lightUV = lightUV;
            r.Update(data, w, pcgSampler.NextFloat());
        }
    }

    OutputTextureRW[tid] = r.GetEncodedState();
    PcgStateTextureRW[tid] = pcgSampler.GetState();
}
