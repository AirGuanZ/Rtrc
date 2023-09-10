#pragma once

#define GBUFFER_MODE GBUFFER_MODE_NORMAL_DEPTH
#define ENABLE_GLOBAL_BINDLESS_TEXTURES 1
#define ENABLE_GLOBAL_BINDLESS_GEOMETRY_BUFFERS 1

#include "../Atmosphere/Atmosphere.hlsl"
#include "../Common/Bindless.hlsl"
#include "../Common/Frame.hlsl"
#include "../Common/GBufferRead.hlsl"
#include "../Common/Random.hlsl"
#include "../Common/Scene.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)

    rtrc_define(ConstantBuffer<CameraData>, Camera)
    rtrc_define(Texture2D<float3>,          SkyLut)
    
    rtrc_define(RaytracingAccelerationStructure, Tlas)
    rtrc_define(StructuredBuffer<TlasInstance>,  Instances)
    
    rtrc_define(RWTexture2D<uint>, RngState)
    
    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2,              outputResolution)
};

rtrc_alias(StructuredBuffer<BuiltinVertexStruct_Default>, VertexBufferArray,  BindlessGeometryBuffers);
rtrc_alias(StructuredBuffer<uint>,                        Index32BufferArray, BindlessGeometryBuffers);
rtrc_alias(StructuredBuffer<uint16_t>,                    Index16BufferArray, BindlessGeometryBuffers);

static float PI = 3.14159265;

float3 SampleDiffuseDirection(float3 normal, inout uint rngState)
{
    float3 localDir = Pcg::NextUniformOnUnitHemisphere(rngState);
    LocalFrame frame;
    frame.InitializeFromNormalizedZ(normal);
    return frame.LocalToGlobal(localDir);
}

float3 Trace(GBufferPixel gpixel, float3 worldPos, inout uint rng)
{
    float3 o = worldPos + 0.01 * gpixel.normal;
    float3 d = SampleDiffuseDirection(gpixel.normal, rng);

    
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

    Pcg::Sampler sam;
    sam.SetState(RngState[tid]);

    RngState[tid] = sam.GetState();
}
