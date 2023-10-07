#pragma once

#define GBUFFER_MODE GBUFFER_MODE_NORMAL_DEPTH

#include "Light.hlsl"
#include "Reservoir.hlsl"
#include "Rtrc/Shader/Common/Color.hlsl"
#include "Rtrc/Shader/Common/GBufferRead.hlsl"
#include "Rtrc/Shader/Common/Random.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)
    
    rtrc_define(ConstantBuffer<CameraData>, Camera)
    rtrc_define(RaytracingAccelerationStructure, Scene)

    rtrc_define(StructuredBuffer<LightShadingData>, LightShadingDataBuffer)
    rtrc_uniform(uint, lightCount)

    rtrc_define(RWTexture2D<uint>, PcgStateTextureRW)
    
    rtrc_define(RWTexture2D<uint4>, OutputTextureRW)
    rtrc_uniform(uint2, outputResolution)

    rtrc_uniform(uint, M) // Number of candidates
};

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputResolution))
        return;
        
    float2 uv = (tid + 0.5) / Pass.outputResolution;
    GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.depth >= 1)
    {
        Reservoir r;
        r.Reset();
        OutputTextureRW[tid] = r.GetEncodedState();
        return;
    }
    
    float3 worldRay = CameraUtils::GetWorldRay(Camera, uv);
    float viewZ = CameraUtils::DeviceZToViewZ(Camera, gpixel.depth);
    float3 worldPos = viewZ * worldRay + Camera.position;
    
    Pcg::Sampler pcgSampler;
    pcgSampler.SetState(PcgStateTextureRW[tid]);

    Reservoir r;
    r.Reset();
    
    RayDesc ray;
    ray.Origin = worldPos + 4e-3 * gpixel.normal;
    ray.TMin = 0;
    for(uint i = 0; i < Pass.M; ++i)
    {
        const uint lightIndex = pcgSampler.NextUInt() % Pass.lightCount;
        const LightShadingData light = LightShadingDataBuffer[lightIndex];
        const float2 lightUV = pcgSampler.NextFloat2();
        const float p = 1.0 / Pass.lightCount;
        float3 positionOnLight;
        const float3 L = ShadeNoVisibility(worldPos, gpixel.normal, light, lightUV, positionOnLight);
        const float pbar = RelativeLuminance(L);
        const float w = pbar / p;
        if(w > 0)
        {
            ReservoirData data;
            data.lightIndex = lightIndex;
            data.lightUV = lightUV;
            if(r.Update(data, w, pbar, pcgSampler.NextFloat()))
            {
                if(light.type == LIGHT_SHADING_DATA_TYPE_DIRECTION)
                {
                    ray.Direction = positionOnLight;
                    ray.TMax= 1e5;
                }
                else
                {
                    ray.Direction = positionOnLight - ray.Origin;
                    ray.TMax = 1;
                }
            }
        }
    }
    r.M = Pass.M;

    if(r.wsum > 0)
    {
        RayQuery<RAY_FLAG_NONE> rayQuery;
        rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xff, ray);
        rayQuery.Proceed();
        if(rayQuery.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
            r.wsum = 0;
    }

    OutputTextureRW[tid] = r.GetEncodedState();
    PcgStateTextureRW[tid] = pcgSampler.GetState();
}
