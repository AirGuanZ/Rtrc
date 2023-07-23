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
    
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)

    rtrc_define(Texture2D<float3>, SkyLut)

    rtrc_define(RaytracingAccelerationStructure, Tlas)
    rtrc_define(StructuredBuffer<TlasInstance>,  Instances)
    
    rtrc_define(RWTexture2D<uint>, RngState)
    
    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2,              outputResolution)

    rtrc_uniform(uint, maxDepth)
};

rtrc_sampler(SkyLutSampler, filter = linear, address_u = repeat, address_v = clamp)
rtrc_sampler(AlbedoSampler, filter = linear, address_u = clamp, address_v = clamp)

rtrc_alias(Texture2D<float3>,                             AlbedoTextureArray, BindlessTextures);
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

#define InterpolateWithBaryCoord(P0, P1, P2, BARY) ((P0) + (BARY).x * ((P1) - (P0)) + (BARY).y * ((P2) - (P0)))

struct Intersection
{
    float3 position;
    float2 uv;
    LocalFrame shadingFrame;
    LocalFrame geometryFrame;
    float3 albedo;
};

Intersection GetIntersection(int instanceIndex, int primitiveIndex, float2 bary)
{
    TlasInstance instance = Instances[instanceIndex];

    int index0 = 3 * primitiveIndex + 0;
    int index1 = 3 * primitiveIndex + 1;
    int index2 = 3 * primitiveIndex + 2;
    if(instance.HasIndexBuffer())
    {
        if(instance.IsUInt16Index())
        {
            StructuredBuffer<uint16_t> indexBuffer = Index16BufferArray[instance.GetGeometryBufferIndex() + 1];
            index0 = indexBuffer[index0];
            index1 = indexBuffer[index1];
            index2 = indexBuffer[index2];
        }
        else
        {
            StructuredBuffer<uint> indexBuffer = Index32BufferArray[instance.GetGeometryBufferIndex() + 1];
            index0 = indexBuffer[index0];
            index1 = indexBuffer[index1];
            index2 = indexBuffer[index2];
        }
    }

    StructuredBuffer<BuiltinVertexStruct_Default> vertexBuffer = VertexBufferArray[instance.GetGeometryBufferIndex()];
    BuiltinVertexStruct_Default vertex0 = vertexBuffer[index0];
    BuiltinVertexStruct_Default vertex1 = vertexBuffer[index1];
    BuiltinVertexStruct_Default vertex2 = vertexBuffer[index2];

    float3 geometryNormal = normalize(cross(vertex2.position - vertex1.position, vertex1.position - vertex0.position));
    float3 normal = normalize(InterpolateWithBaryCoord(vertex0.normal, vertex1.normal, vertex2.normal, bary));
    float3 tangent = normalize(InterpolateWithBaryCoord(vertex0.tangent, vertex1.tangent, vertex2.tangent, bary));

    LocalFrame geometryFrame;
    geometryFrame.z = geometryNormal;
    geometryFrame.y = normalize(cross(geometryNormal, tangent));
    geometryFrame.x = cross(geometryFrame.y, geometryFrame.z);

    LocalFrame shadingFrame;
    shadingFrame.z = normal;
    shadingFrame.y = normalize(cross(normal, tangent));
    shadingFrame.x = cross(shadingFrame.y, shadingFrame.z);

    uint albedoTextureIndex = instance.GetAlbedoTextureIndex();
    Texture2D<float3> albedoTexture = AlbedoTextureArray[albedoTextureIndex];
    
    float2 uv = InterpolateWithBaryCoord(vertex0.uv, vertex1.uv, vertex2.uv, bary);
    float3 rawAlbedo = albedoTexture.SampleLevel(AlbedoSampler, uv, 0);
    float3 scaledAlbedo = (float)instance.GetAlbedoScale() * rawAlbedo;

    Intersection ret;
    ret.position      = InterpolateWithBaryCoord(vertex0.position, vertex1.position, vertex2.position, bary);
    ret.uv            = uv;
    ret.shadingFrame  = shadingFrame;
    ret.geometryFrame = geometryFrame;
    ret.albedo        = scaledAlbedo;
    return ret;
}

bool FindClosestIntersection(RayDesc ray, out Intersection intersection)
{
    RayQuery<RAY_FLAG_NONE> rayQuery;
    rayQuery.TraceRayInline(Tlas, RAY_FLAG_NONE, 0xff, ray);
    rayQuery.Proceed();

    if(rayQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        return false;
    
    int instanceIndex = rayQuery.CommittedInstanceIndex();
    int primitiveIndex = rayQuery.CommittedPrimitiveIndex();
    float2 baryCoord = rayQuery.CommittedTriangleBarycentrics();
    intersection = GetIntersection(instanceIndex, primitiveIndex, baryCoord);
    return true;
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

            Intersection intersection;
            if(!FindClosestIntersection(ray, intersection))
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
