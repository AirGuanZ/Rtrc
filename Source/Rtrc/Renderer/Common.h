#pragma once

#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Resource/ResourceManager.h>

RTRC_RENDERER_BEGIN

rtrc_refl_struct(CameraConstantBuffer, shader)
{
    static CameraConstantBuffer FromCamera(const CameraRenderData &camera)
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

enum class StencilBit : uint8_t
{
    None    = 0,
    Regular = 1 << 0,
};

rtrc_refl_struct(PointLightShadingData, shader)
{
    float3 position;
    float  distFadeBias = 0; // distFade = distFadeBias + distFadeScale * dist
    float3 color;
    float  distFadeScale = 0;
};
static_assert(sizeof(PointLightShadingData) == 8 * sizeof(float));

rtrc_refl_struct(DirectionalLightShadingData, shader)
{
    float3 direction; float pad0 = 0;
    float3 color;     float pad1 = 0;
};
static_assert(sizeof(DirectionalLightShadingData) == 8 * sizeof(float));

PointLightShadingData       ExtractPointLightShadingData      (const Light::SharedRenderingData *light);
DirectionalLightShadingData ExtractDirectionalLightShadingData(const Light::SharedRenderingData *light);

struct GBuffers
{
    RG::TextureResource *normal         = nullptr;
    RG::TextureResource *albedoMetallic = nullptr;
    RG::TextureResource *roughness      = nullptr;
    RG::TextureResource *depthStencil   = nullptr;

    RG::TextureResource *currDepth = nullptr;
    RG::TextureResource *prevDepth = nullptr;
};

class RenderAlgorithm : public Uncopyable
{
public:

    RenderAlgorithm(ObserverPtr<Device> device, ObserverPtr<ResourceManager> resources)
        : device_(device), resources_(resources)
    {
        
    }

protected:

    ObserverPtr<Device>          device_;
    ObserverPtr<ResourceManager> resources_;
};

RTRC_RENDERER_END
