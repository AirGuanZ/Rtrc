#pragma once

#define GBUFFER_MODE_NORMAL_DEPTH
#include "../Common/Frame.hlsl"
#include "../Common/GBufferRead.hlsl"
#include "../Common/Random.hlsl"
#include "../Common/Scene.hlsl"

rtrc_group(TracePassGroup)
{
    REF_GBUFFERS(CS)
    
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)

    rtrc_define(RaytracingAccelerationStructure,                   Tlas)
    rtrc_define(StructuredBuffer<TlasMaterial>,                    Materials)
    rtrc_bindless(StructuredBuffer<Primitive>[MAX_INSTANCE_COUNT], Geometries)
    
    rtrc_define(RWTexture2D<uint>, RngState)
    
    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2,              outputResolution)

    rtrc_uniform(uint, maxDepth)
};

float2 SampleDiffuseDirection(float3 normal, inout uint rngState)
{
    float3 localDir = Pcg::NextUniformOnUnitHemisphere(rngState);
    LocalFrame frame;
    frame.InitializeFromNormalizedZ(normal);
    return frame.LocalToGlobal(localDir);
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputSize))
        return;
    
    float2 uv = (tid + 0.5) / Pass.outputSize;
    GBufferPixel gpixel = LoadGBufferPixel(uv);
    if(gpixel.z >= 1)
        return;
    
    float3 worldRay = CameraUtils::GetWorldRay(Camera, uv);
    float viewZ = CameraUtils::DeviceZToViewZ(Camera, gpixel.depth);
    float3 worldPos = viewZ * worldRay + Camera.worldPosition;

    uint rng = RngState[tid];

    float3 o = worldPos + 0.01 * gpixel.normal;
    float3 d = SampleDiffuseDirection(gpixel.normal, rng);

    for(uint i = 0; i < Pass.maxDepth; ++i)
    {
        
    }

    RngState[tid] = rng;
}
