#pragma once

#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>
#include <Rtrc/Graphics/Device/Device.h>

RTRC_RENDERER_BEGIN

rtrc_struct(CameraConstantBuffer)
{
    static CameraConstantBuffer FromCamera(const RenderCamera &camera)
    {
        CameraConstantBuffer ret;
        ret.worldPosition       = camera.position;
        ret.worldFront          = camera.forward;
        ret.worldLeft           = camera.left;
        ret.worldUp             = camera.up;
        ret.worldToCameraMatrix = camera.worldToCamera;
        ret.cameraToWorldMatrix = camera.cameraToWorld;
        ret.cameraToClipMatrix  = camera.cameraToClip;
        ret.clipToCameraMatrix  = camera.clipToCamera;
        ret.worldToClipMatrix   = camera.worldToClip;
        ret.clipToWorldMatrix   = camera.clipToWorld;
        ret.worldRays[0]        = camera.worldRays[0];
        ret.worldRays[1]        = camera.worldRays[1];
        ret.worldRays[2]        = camera.worldRays[2];
        ret.worldRays[3]        = camera.worldRays[3];
        ret.cameraRays[0]       = camera.cameraRays[0];
        ret.cameraRays[1]       = camera.cameraRays[1];
        ret.cameraRays[2]       = camera.cameraRays[2];
        ret.cameraRays[3]       = camera.cameraRays[3];
        return ret;
    }

    rtrc_var(float3,    worldPosition);
    rtrc_var(float3,    worldFront);
    rtrc_var(float3,    worldLeft);
    rtrc_var(float3,    worldUp);
    rtrc_var(float4x4,  worldToCameraMatrix);
    rtrc_var(float4x4,  cameraToWorldMatrix);
    rtrc_var(float4x4,  cameraToClipMatrix);
    rtrc_var(float4x4,  clipToCameraMatrix);
    rtrc_var(float4x4,  worldToClipMatrix);
    rtrc_var(float4x4,  clipToWorldMatrix);
    rtrc_var(float3[4], cameraRays);
    rtrc_var(float3[4], worldRays);
};

enum class StencilBit : uint8_t
{
    None    = 0,
    Regular = 1 << 0,
};

struct PointLightShadingData
{
    Vector3f position;
    float    distFadeBias; // distFade = distFadeBias + distFadeScale * dist
    Vector3f color;
    float    distFadeScale;
};

struct DirectionalLightShadingData
{
    Vector3f direction;
    Vector3f color;
};

RTRC_RENDERER_END
