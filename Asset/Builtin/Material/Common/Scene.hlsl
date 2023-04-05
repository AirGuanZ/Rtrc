#pragma once

// See Rtrc/Renderer/Common.h

struct CameraConstantBuffer
{
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
    float3 color;
    float  range;
};

struct DirectionalLightShadingData
{
    float3 direction;
    float3 color;
};

namespace CameraUtils
{

    float DeviceZToViewZ(float4x4 cameraToClip, float deviceZ)
    {
        // Let P(row, col) = perspective projection matrix
        // deviceZ = P(2, 2) + P(2, 3) / viewZ
        return (deviceZ - cameraToClip._22) / cameraToClip._23;
    }

} // namespace namespace CameraUtils
