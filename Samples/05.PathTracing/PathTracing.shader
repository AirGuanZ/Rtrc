rtrc_shader("PathTracing")
{

rtrc_compute(CSMain)

#include "Random.hlsl"

#define MAX_INSTANCE_COUNT 128
#define GAMMA_CORRECTION 1

struct SurfaceMaterial
{
    float3 color;
};

struct Primitive
{
    float3 a;
    float3 b;
    float3 c;
    float3 normal;
};

struct Intersection
{
    float3 position;
    float3 normal;
    float3 color;
};

rtrc_group(Pass)
{
    rtrc_define(RWTexture2D<float4>, AccumulateTexture)
    rtrc_define(RWTexture2D<float4>, OutputTexture)
    rtrc_define(RWTexture2D<uint>,   RngTexture)

    rtrc_define(RaytracingAccelerationStructure,                   Scene)
    rtrc_define(StructuredBuffer<SurfaceMaterial>,                 Materials)
    rtrc_bindless(StructuredBuffer<Primitive>[MAX_INSTANCE_COUNT], Geometries)

    rtrc_uniform(float3, EyePosition)
    rtrc_uniform(float3, FrustumA)
    rtrc_uniform(float3, FrustumB)
    rtrc_uniform(float3, FrustumC)
    rtrc_uniform(float3, FrustumD)
    rtrc_uniform(uint2,  Resolution)
};

bool FindClosestIntersection(RayDesc ray, out Intersection intersection)
{
    RayQuery<RAY_FLAG_NONE> rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xff, ray);
    rayQuery.Proceed();

    if(rayQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        return false;

    int instanceIndex = rayQuery.CommittedInstanceIndex();
    int primitiveIndex = rayQuery.CommittedPrimitiveIndex();
    intersection.position = ray.Origin + ray.Direction * rayQuery.CommittedRayT();
    intersection.normal = Geometries[NonUniformResourceIndex(instanceIndex)][primitiveIndex].normal;
    intersection.color = Materials[instanceIndex].color;
    if(dot(ray.Direction, intersection.normal) > 0)
        intersection.normal = -intersection.normal;
    return true;
}

bool HasIntersection(RayDesc ray)
{
    RayQuery<RAY_FLAG_NONE> rayQuery;
    rayQuery.TraceRayInline(Scene, RAY_FLAG_NONE, 0xff, ray);
    rayQuery.Proceed();
    return rayQuery.CommittedStatus() != COMMITTED_NOTHING;
}

float3 GetSkyColor(float3 toSky)
{
    return lerp(0, 1, saturate(0.5 + 0.5 * toSky.y));
}

float3 ComputePixelColor(uint2 tid, inout uint rngState)
{
    float2 uv;
    uv.x = (tid.x + PcgNextFloat(rngState)) / Pass.Resolution.x;
    uv.y = (tid.y + PcgNextFloat(rngState)) / Pass.Resolution.y;

    float3 frustumAB = lerp(Pass.FrustumA, Pass.FrustumB, uv.x);
    float3 frustumCD = lerp(Pass.FrustumC, Pass.FrustumD, uv.x);

    RayDesc ray;
    ray.Origin    = Pass.EyePosition;
    ray.Direction = normalize(lerp(frustumAB, frustumCD, uv.y));
    ray.TMin      = 0;
    ray.TMax      = 9999;

    float3 beta = 1;
    for(int i = 0; i < 4; ++i)
    {
        Intersection intersection;
        if(!FindClosestIntersection(ray, intersection))
            return beta * GetSkyColor(ray.Direction);
        float u1 = PcgNextFloat(rngState);
        float u2 = PcgNextFloat(rngState);
        ray.Origin = intersection.position + 0.01 * intersection.normal;
        ray.Direction = normalize(intersection.normal + 0.999 * UniformOnUnitSphere(float2(u1, u2)));
        beta *= intersection.color;
    }

    return 0;
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.Resolution))
        return;

    uint rngState = RngTexture[tid];
    float3 color = ComputePixelColor(tid, rngState);
    RngTexture[tid] = rngState;

    float4 accumulatedValue = AccumulateTexture[tid];
    accumulatedValue += float4(color, 1);
    AccumulateTexture[tid] = accumulatedValue;

    float3 outputColor = accumulatedValue.rgb / accumulatedValue.a;
#if GAMMA_CORRECTION
    outputColor = pow(outputColor, 1 / 2.2);
#endif
    OutputTexture[tid] = float4(outputColor, 1);
}

} // Shader "PathTracing"
