#pragma once

// See Source/Rtrc/Renderer/Camera.h
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

// See Source/Rtrc/Renderer/Scene/Light.h
struct DirectionalLightConstantBuffer
{
    float3 direction;
    float3 color;
    float  intensity;
};
