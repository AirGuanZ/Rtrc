#pragma once

// See Rtrc/Renderer/Common.h

struct CameraConstantBuffer
{
    float3   worldPosition;
    float3   worldFront;
    float3   worldLeft;
    float3   worldUp;
    float4x4 worldToCameraMatrix;
    float4x4 cameraToWorldMatrix;
    float4x4 cameraToClipMatrix;
    float4x4 clipToCameraMatrix;
    float4x4 worldToClipMatrix;
    float4x4 clipToWorldMatrix;
    float3   cameraRays[4];
    float3   worldRays[4];
};

struct PointLightShadingData
{
    float3 position;
    float  distFadeBias;
    float3 color;
    float  distFadeScale;
};

struct DirectionalLightShadingData
{
    float3 direction;
    float3 color;
};

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
    
    float DeviceZToViewZ(CameraConstantBuffer camera, float deviceZ)
    {
        return CameraUtils::DeviceZToViewZ(camera.cameraToClipMatrix, deviceZ);
    }
    
    float3 GetWorldRay(float3 cameraWorldRays[4], float2 uv)
    {
        float3 rayAB = lerp(cameraWorldRays[0], cameraWorldRays[1], uv.x);
        float3 rayCD = lerp(cameraWorldRays[2], cameraWorldRays[3], uv.x);
        float3 ray = lerp(rayAB, rayCD, uv.y);
        return ray;
    }
    
    float3 GetWorldRay(CameraConstantBuffer camera, float2 uv)
    {
        return CameraUtils::GetWorldRay(camera.worldRays, uv);
    }

} // namespace namespace CameraUtils
