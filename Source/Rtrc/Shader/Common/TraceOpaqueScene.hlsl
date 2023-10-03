#ifndef RTRC_COMMON_TRACE_OPAQUE_SCENE_HLSL
#define RTRC_COMMON_TRACE_OPAQUE_SCENE_HLSL

#if !ENABLE_GLOBAL_BINDLESS_GEOMETRY_BUFFERS
#error "Bindless geometry buffer must be enabled when including TraceOpaqueScene.hlsl"
#endif

#include "./Bindless.hlsl"
#include "./Scene.hlsl"

rtrc_define(RaytracingAccelerationStructure, OpaqueScene_Tlas)
rtrc_define(StructuredBuffer<TlasInstance>,  OpaqueScene_TlasInstances)

#if ENABLE_GLOBAL_BINDLESS_TEXTURES
rtrc_alias(Texture2D<float3>, OpaqueScene_AlbedoTextureArray, BindlessTextures)
#endif

rtrc_alias(StructuredBuffer<BuiltinVertexStruct_Default>, OpaqueScene_VertexBufferArray,  BindlessGeometryBuffers);
rtrc_alias(StructuredBuffer<uint>,                        OpaqueScene_Index32BufferArray, BindlessGeometryBuffers);
rtrc_alias(StructuredBuffer<uint16_t>,                    OpaqueScene_Index16BufferArray, BindlessGeometryBuffers);

rtrc_sampler(OpaqueScene_AlbedoSampler, filter = linear, address_u = clamp, address_v = clamp)

struct OpaqueSceneIntersection
{
    float3 position;
    float2 uv;
    LocalFrame shadingFrame;
    LocalFrame geometryFrame;
#if ENABLE_GLOBAL_BINDLESS_TEXTURES
    float3 albedo;
#endif
};

#define OpaqueScene_InterpolateWithBaryCoord(P0, P1, P2, BARY) ((P0) + (BARY).x * ((P1) - (P0)) + (BARY).y * ((P2) - (P0)))

OpaqueSceneIntersection GetOpaqueSceneIntersection(int instanceIndex, int primitiveIndex, float2 bary)
{
    TlasInstance instance = OpaqueScene_TlasInstances[instanceIndex];
    
    int index0 = 3 * primitiveIndex + 0;
    int index1 = 3 * primitiveIndex + 1;
    int index2 = 3 * primitiveIndex + 2;
    if(instance.HasIndexBuffer())
    {
        if(instance.IsUInt16Index())
        {
            StructuredBuffer<uint16_t> indexBuffer = OpaqueScene_Index16BufferArray[instance.GetGeometryBufferIndex() + 1];
            index0 = indexBuffer[index0];
            index1 = indexBuffer[index1];
            index2 = indexBuffer[index2];
        }
        else
        {
            StructuredBuffer<uint> indexBuffer = OpaqueScene_Index32BufferArray[instance.GetGeometryBufferIndex() + 1];
            index0 = indexBuffer[index0];
            index1 = indexBuffer[index1];
            index2 = indexBuffer[index2];
        }
    }

    StructuredBuffer<BuiltinVertexStruct_Default> vertexBuffer = OpaqueScene_VertexBufferArray[instance.GetGeometryBufferIndex()];
    BuiltinVertexStruct_Default vertex0 = vertexBuffer[index0];
    BuiltinVertexStruct_Default vertex1 = vertexBuffer[index1];
    BuiltinVertexStruct_Default vertex2 = vertexBuffer[index2];

    float3 geometryNormal = normalize(cross(vertex2.position - vertex1.position, vertex1.position - vertex0.position));
    float3 normal = normalize(OpaqueScene_InterpolateWithBaryCoord(vertex0.normal, vertex1.normal, vertex2.normal, bary));
    float3 tangent = normalize(OpaqueScene_InterpolateWithBaryCoord(vertex0.tangent, vertex1.tangent, vertex2.tangent, bary));

    LocalFrame geometryFrame;
    geometryFrame.z = geometryNormal;
    geometryFrame.y = normalize(cross(geometryNormal, tangent));
    geometryFrame.x = cross(geometryFrame.y, geometryFrame.z);

    LocalFrame shadingFrame;
    shadingFrame.z = normal;
    shadingFrame.y = normalize(cross(normal, tangent));
    shadingFrame.x = cross(shadingFrame.y, shadingFrame.z);

    float2 uv = OpaqueScene_InterpolateWithBaryCoord(vertex0.uv, vertex1.uv, vertex2.uv, bary);

    OpaqueSceneIntersection ret;
    ret.position      = OpaqueScene_InterpolateWithBaryCoord(vertex0.position, vertex1.position, vertex2.position, bary);
    ret.uv            = uv;
    ret.shadingFrame  = shadingFrame;
    ret.geometryFrame = geometryFrame;

#if ENABLE_GLOBAL_BINDLESS_TEXTURES
    uint albedoTextureIndex = instance.GetAlbedoTextureIndex();
    Texture2D<float3> albedoTexture = OpaqueScene_AlbedoTextureArray[albedoTextureIndex];
    float3 rawAlbedo = albedoTexture.SampleLevel(OpaqueScene_AlbedoSampler, uv, 0);
    ret.albedo = (float)instance.GetAlbedoScale() * rawAlbedo;
#endif

    return ret;
}

bool FindClosestIntersectionWithOpaqueScene(RayDesc ray, out OpaqueSceneIntersection intersection)
{
    RayQuery<RAY_FLAG_NONE> rayQuery;
    rayQuery.TraceRayInline(OpaqueScene_Tlas, RAY_FLAG_NONE, 0xff, ray);
    rayQuery.Proceed();

    if(rayQuery.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
        return false;
    
    int instanceIndex = rayQuery.CommittedInstanceIndex();
    int primitiveIndex = rayQuery.CommittedPrimitiveIndex();
    float2 baryCoord = rayQuery.CommittedTriangleBarycentrics();
    intersection = GetOpaqueSceneIntersection(instanceIndex, primitiveIndex, baryCoord);
    return true;
}

#endif // #ifndef RTRC_COMMON_TRACE_OPAQUE_SCENE_HLSL
