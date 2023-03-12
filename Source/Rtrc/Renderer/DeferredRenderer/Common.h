#pragma once

#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>
#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>

RTRC_BEGIN

namespace DeferredRendererCommon
{

    enum class StencilMaskBit : uint8_t
    {
        Nothing = 0,
        Regular = 1 << 0
    };

    struct FrameContext
    {
        TransientConstantBufferAllocator *transientConstantBufferAllocator = nullptr;

        const SceneProxy *scene = nullptr;
        const Camera     *camera = nullptr;
    };

    // See Asset/Builtin/Material/Common/Scene.hlsl
    rtrc_struct(DirectionalLightConstantBuffer)
    {
        rtrc_var(float3, direction) = Vector3f(0, -1, 0);
        rtrc_var(float3, color) = Vector3f(1, 1, 1);
        rtrc_var(float, intensity) = 1;
    };

    // See Asset/Builtin/Material/Common/Scene.hlsl
    rtrc_struct(CameraConstantBuffer)
    {
        static CameraConstantBuffer FromCamera(const Camera & camera)
        {
            CameraConstantBuffer ret;
            ret.worldToCameraMatrix = camera.GetWorldToCameraMatrix();
            ret.cameraToWorldMatrix = camera.GetCameraToWorldMatrix();
            ret.cameraToClipMatrix = camera.GetCameraToClipMatrix();
            ret.clipToCameraMatrix = camera.GetClipToCameraMatrix();
            ret.worldToClipMatrix = camera.GetWorldToClipMatrix();
            ret.clipToWorldMatrix = camera.GetClipToWorldMatrix();
            ret.worldRays[0] = camera.GetWorldRays()[0];
            ret.worldRays[1] = camera.GetWorldRays()[1];
            ret.worldRays[2] = camera.GetWorldRays()[2];
            ret.worldRays[3] = camera.GetWorldRays()[3];
            ret.cameraRays[0] = camera.GetCameraRays()[0];
            ret.cameraRays[1] = camera.GetCameraRays()[1];
            ret.cameraRays[2] = camera.GetCameraRays()[2];
            ret.cameraRays[3] = camera.GetCameraRays()[3];
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

    rtrc_group(GBuffer_Pass)
    {
        rtrc_uniform(CameraConstantBuffer, Camera);
    };

    rtrc_group(DeferredLighting_Pass, FS)
    {
        rtrc_define(Texture2D, gbufferA);
        rtrc_define(Texture2D, gbufferB);
        rtrc_define(Texture2D, gbufferC);
        rtrc_define(Texture2D, gbufferDepth);
        rtrc_define(Texture2D, skyLut);

        rtrc_uniform(float4, gbufferSize);
        rtrc_uniform(CameraConstantBuffer, camera);
        rtrc_uniform(DirectionalLightConstantBuffer, directionalLight);
    };

} // namespace DeferredRendererCommon

RTRC_END
