#ifndef RTRC_COMMON_SCENE_HLSL
#define RTRC_COMMON_SCENE_HLSL

#include "Generated/Reflection.hlsl"

// See Rtrc/Renderer/Common.h

typedef Rtrc::CameraData    CameraData;
typedef Rtrc::PerObjectData PerObjectData;

struct TlasInstance
{
    uint16_t  _albedoTextureIndex;
    float16_t _albedoScale;
    uint16_t  _encodeGeometryBufferInfo;
    uint16_t  _pad0;

    uint GetAlbedoTextureIndex()
    {
        return _albedoTextureIndex;
    }

    float GetAlbedoScale()
    {
        return _albedoScale;
    }

    uint GetGeometryBufferIndex()
    {
        // 2023.07.16:
        // If we don't explicitly convert _encodeGeometryBufferInfo to uint before using bit operation,
        // dxc will generate invalid spirv for vulkan backend.
        return (uint)_encodeGeometryBufferInfo & 0x3fff;
    }

    bool HasIndexBuffer()
    {
        return ((uint)_encodeGeometryBufferInfo >> 15) != 0;
    }

    bool IsUInt16Index()
    {
        return ((uint)_encodeGeometryBufferInfo & 8000) != 0;
    }
};

struct BuiltinVertexStruct_Default
{
    float3 position;
    float3 normal;
    float2 uv;
    float3 tangent;
};

namespace CameraUtils
{

    float DeviceZToViewZ(float4x4 cameraToClip, float deviceZ)
    {
        // Let P(row, col) = perspective projection matrix
        // deviceZ = P(2, 2) + P(2, 3) / viewZ
        return cameraToClip._34 / (deviceZ - cameraToClip._33);
    }
    
    float DeviceZToViewZ(CameraData camera, float deviceZ)
    {
        return CameraUtils::DeviceZToViewZ(camera.cameraToClip, deviceZ);
    }
    
    float3 GetWorldRay(float3 cameraWorldRays[4], float2 uv)
    {
        float3 rayAB = lerp(cameraWorldRays[0], cameraWorldRays[1], uv.x);
        float3 rayCD = lerp(cameraWorldRays[2], cameraWorldRays[3], uv.x);
        float3 ray = lerp(rayAB, rayCD, uv.y);
        return ray;
    }
    
    float3 GetWorldRay(CameraData camera, float2 uv)
    {
        return CameraUtils::GetWorldRay(camera.worldRays, uv);
    }

    float3 GetWorldPosition(CameraData camera, float2 uv, float deviceZ)
    {
        const float viewZ = DeviceZToViewZ(camera, deviceZ);
        const float3 worldRay = GetWorldRay(camera, uv);
        return viewZ * worldRay + camera.position;
    }

} // namespace namespace CameraUtils

#endif // #ifndef RTRC_COMMON_SCENE_HLSL