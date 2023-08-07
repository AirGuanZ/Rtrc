#pragma once

#include <Rtrc/Renderer/KajiyaGI/KajiyaGI.h>
#include <Rtrc/Renderer/PathTracer/PathTracer.h>
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
        PerObjectData perObjectData;
    };

    RenderCamera(ObserverPtr<Device> device, const RenderScene &scene, UniqueId cameraId);

    const RenderScene &GetScene() const { return scene_; }

    const CameraRenderData &GetCameraRenderData() const { return currRenderCamera_; }
    const RC<SubBuffer>    &GetCameraCBuffer()    const { return currCameraCBuffer_; }

    const CameraRenderData &GetPrevCameraRenderData() const { return prevRenderCamera_; }
    const RC<SubBuffer>    &GetPrevCameraCBuffer()    const { return prevCameraCBuffer_; }

    Span<StaticMeshRecord *> GetStaticMeshes()            const { return objects_; }
    const RC<Buffer>        &GetStaticMeshPerObjectData() const { return perObjectDataBuffer_; }
    
    RenderAtmosphere::PerCameraData &GetAtmosphereData()  { return atmosphereData_; }
    PathTracer::PerCameraData       &GetPathTracingData() { return pathTracingData_; }
    KajiyaGI::PerCameraData         &GetKajiyaGIData()    { return kajiyaGIData_; }

    void Update(
        const CameraRenderData           &camera,
        TransientConstantBufferAllocator &transientConstantBufferAllocator,
        LinearAllocator                  &linearAllocator);

    void UpdateDepth(RG::RenderGraph &renderGraph, GBuffers &gbuffers);

private:

    ObserverPtr<Device> device_;
    
    const RenderScene  &scene_;
    CameraRenderData    currRenderCamera_;
    CameraRenderData    prevRenderCamera_;

    RC<SubBuffer> currCameraCBuffer_;
    RC<SubBuffer> prevCameraCBuffer_;

    std::vector<StaticMeshRecord *> objects_;
    RC<Buffer>                      perObjectDataBuffer_;
    UploadBufferPool<>              perObjectDataBufferPool_;
    
    RenderAtmosphere::PerCameraData atmosphereData_;
    PathTracer::PerCameraData       pathTracingData_;
    KajiyaGI::PerCameraData         kajiyaGIData_;

    RC<StatefulTexture> prevDepth_;
    RC<StatefulTexture> currDepth_;
};

RTRC_RENDERER_END
