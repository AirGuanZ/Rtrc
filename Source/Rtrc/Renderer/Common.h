#pragma once

#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Graphics/Device/Device.h>

RTRC_RENDERER_BEGIN

rtrc_struct(CameraConstantBuffer)
{
    static CameraConstantBuffer FromCamera(const RenderCamera &camera)
    {
        CameraConstantBuffer ret;
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

    rtrc_var(float4x4, worldToCameraMatrix);
    rtrc_var(float4x4, cameraToWorldMatrix);
    rtrc_var(float4x4, cameraToClipMatrix);
    rtrc_var(float4x4, clipToCameraMatrix);
    rtrc_var(float4x4, worldToClipMatrix);
    rtrc_var(float4x4, clipToWorldMatrix);
    rtrc_var(float3[4], cameraRays);
    rtrc_var(float3[4], worldRays);
};

rtrc_struct(PhysicalAtmosphereProperties)
{
    // Rayleigh

    rtrc_var(float3, scatterRayleigh)  = 1e-6f * Vector3f(5.802f, 13.558f, 33.1f);
    rtrc_var(float,  hDensityRayleigh) = 1e3f * 8;

    // Mie

    rtrc_var(float, scatterMie)    = 1e-6f * 3.996f;
    rtrc_var(float, assymmetryMie) = 0.8f;
    rtrc_var(float, absorbMie)     = 1e-6f * 4.4f;
    rtrc_var(float, hDensityMie)   = 1e3f * 1.2f;

    // Ozone

    rtrc_var(float3, absorbOzone)       = 1e-6f * Vector3f(0.65f, 1.881f, 0.085f);
    rtrc_var(float,  ozoneCenterHeight) = 1e3f * 25;
    rtrc_var(float,  ozoneThickness)    = 1e3f * 30;

    // Geometry

    rtrc_var(float, planetRadius)      = 1e3f * 6360;
    rtrc_var(float, atmosphereRadius)  = 1e3f * 6460;

    rtrc_var(float3, terrainAlbedo) = { 0.3f, 0.3f, 0.3f };

    auto operator<=>(const PhysicalAtmosphereProperties &) const = default;
};

enum class StencilBit : uint8_t
{
    None    = 0,
    Regular = 1 << 0,
};

struct PointLightShadingData
{
    Vector3f position;
    Vector3f color;
    float    range;
};

struct DirectionalLightShadingData
{
    Vector3f direction;
    Vector3f color;
};

RTRC_RENDERER_END
