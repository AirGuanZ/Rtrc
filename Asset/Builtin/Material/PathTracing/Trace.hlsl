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
#include "../Common/TraceOpaqueScene.hlsl"

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)
    
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)

    rtrc_define(Texture2D<float3>, SkyLut)

    rtrc_ref(OpaqueScene_Tlas)
    rtrc_ref(OpaqueScene_TlasInstances)
    
    rtrc_define(RWTexture2D<uint>, RngState)
    
    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2,              outputResolution)

    rtrc_uniform(uint, maxDepth)
};

rtrc_sampler(SkyLutSampler, filter = linear, address_u = repeat, address_v = clamp)

static float PI = 3.14159265;

float3 SampleDiffuseDirection(float3 normal, inout uint rngState)
{
    float3 localDir = Pcg::NextUniformOnUnitHemisphere(rngState);
    LocalFrame frame;
    frame.InitializeFromNormalizedZ(normal);
    return frame.LocalToGlobal(localDir);
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
    float3 worldPos = viewZ * worldRay + Camera.worldPosition;

    uint rng = RngState[tid];

    float3 result = 0;
    const int N = 4;
    for(int s = 0; s < N; ++s)
    {
        float3 o = worldPos + 0.01 * gpixel.normal;
        float3 d = SampleDiffuseDirection(gpixel.normal, rng);
        float3 beta = 1;
        for(uint i = 0; i < Pass.maxDepth; ++i)
        {
            RayDesc ray;
            ray.Origin    = o;
            ray.Direction = d;
            ray.TMin      = 0;
            ray.TMax      = 9999;

            // Find closest intersection

            OpaqueSceneIntersection intersection;
            if(!FindClosestIntersectionWithOpaqueScene(ray, intersection))
            {
                float2 skyLutUV = Atmosphere::ComputeSkyLutTexCoord(d);
                float3 color = SkyLut.SampleLevel(SkyLutSampler, skyLutUV, 0);
                result += beta * color;
                break;
            }

            // Sample new direction

            float3 newDir = SampleDiffuseDirection(intersection.geometryFrame.z, rng);
            float pdf = 1 / (2 * PI);

            // Generate new ray

            o = intersection.position + 0.01 * intersection.geometryFrame.z;
            d = newDir;

            // Update beta

            float cosFactor = abs(dot(newDir, intersection.geometryFrame.z));
            beta *= saturate(intersection.albedo) / PI * cosFactor / pdf;
        }
    }
    result *= 1.0 / N;

    Output[tid] = float4(result, 1);
    RngState[tid] = rng;
}
