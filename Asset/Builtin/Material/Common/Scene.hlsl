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

struct TlasMaterial
{
    uint albedoTextureIndex;
    float albedoScale;
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
