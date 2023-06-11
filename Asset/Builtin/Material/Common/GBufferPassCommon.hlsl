#pragma once

#include "Scene.hlsl"

struct PerObjectData
{
    float4x4 localToWorld;
    float4x4 worldToLocal;
    float4x4 localToCamera;
    float4x4 localToClip;
};

rtrc_group(Pass)
{
    rtrc_define(ConstantBuffer<CameraConstantBuffer>, Camera)
    rtrc_define(StructuredBuffer<PerObjectData>, PerObjectDataBuffer)
};

rtrc_push_constant(PerObject, All)
{
    uint dataOffset;
};

#if ENABLE_INSTANCE
#define DefineInstanceID()            uint instanceID : SV_InstanceID
#define TransferInstanceID(IN, OUT)   OUT.instanceID = IN.instanceID
#define GetInstancedPerObjectData(IN) (PerObjectDataBuffer[rtrc_get_push_constant(PerObject, dataOffset) + IN.instanceID])
#else
#define DefineInstanceID()
#define TransferInstanceID(IN, OUT)
#define GetInstancedPerObjectData(IN) (PerObjectDataBuffer[rtrc_get_push_constant(PerObject, dataOffset)])
#define GetPerObjectData()            (PerObjectDataBuffer[rtrc_get_push_constant(PerObject, dataOffset)])
#endif

struct GBufferPixelOutput
{
    float4 gbufferNormal         : SV_TARGET0;
    float4 gbufferAlbedoMetallic : SV_TARGET1;
    float4 gbufferRoughness      : SV_TARGET2;
};

GBufferPixelOutput EncodeGBufferPixel(float3 worldNormal, float3 albedo, float metallic, float roughness)
{
    GBufferPixelOutput pixel;
    pixel.gbufferNormal         = float4(0.5 * worldNormal + 0.5, 0);
    pixel.gbufferAlbedoMetallic = float4(albedo, metallic);
    pixel.gbufferRoughness      = float4(roughness, 0, 0, 0);
    return pixel;
}
