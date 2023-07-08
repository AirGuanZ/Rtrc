#pragma once

#include <Rtrc/Renderer/Scene/RenderScene.h>
#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_RENDERER_BEGIN

// Per-camera rendering data
class RenderCamera
{
public:

    struct StaticMeshRecord : RenderScene::StaticMeshRecord
    {
        RC<BindingGroup> perObjectBindingGroup;
    };

    RenderCamera(ObserverPtr<Device> device, const RenderScene &scene, UniqueId cameraId);
    
    const CameraRenderData &GetCameraRenderData() const { return renderCamera_; }
    const RC<SubBuffer> &GetCameraCBuffer() const { return cameraCBuffer_; }

    const RenderScene &GetCachedScene() const { return scene_; }

    Span<StaticMeshRecord *> GetStaticMeshes() const { return objects_; }
    const RC<Buffer>        &GetStaticMeshPerObjectData() const { return perObjectDataBuffer_; }
    
    RenderAtmosphere::PerCameraData &GetAtmosphereData() { return atmosphereData_; }

    void Update(
        const CameraRenderData           &camera,
        TransientConstantBufferAllocator &transientConstantBufferAllocator,
        LinearAllocator                  &linearAllocator);

private:
    
    const RenderScene  &scene_;
    CameraRenderData    renderCamera_;

    RC<SubBuffer> cameraCBuffer_;

    std::vector<StaticMeshRecord *> objects_;
    RC<Buffer>                      perObjectDataBuffer_;
    UploadBufferPool<>              perObjectDataBufferPool_;
    
    RenderAtmosphere::PerCameraData atmosphereData_;
};

RTRC_RENDERER_END
