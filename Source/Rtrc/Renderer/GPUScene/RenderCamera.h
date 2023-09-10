#pragma once

#include <Rtrc/Renderer/CriusGI/CriusGI.h>
#include <Rtrc/Renderer/PathTracer/PathTracer.h>
#include <Rtrc/Renderer/GPUScene/RenderScene.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>
#include <Core/Memory/Arena.h>

RTRC_RENDERER_BEGIN

// Per-camera rendering data
class RenderCamera
{
public:

    struct MeshRendererRecord
    {
        const MeshRenderer           *meshRenderer  = nullptr;
        const MeshRenderingCache     *meshCache     = nullptr;
        const MaterialRenderingCache *materialCache = nullptr;

        PerObjectData perObjectData;
        uint32_t indexInPerObjectDataBuffer = 0;
    };

    RenderCamera(ObserverPtr<Device> device, const RenderScene &scene, const Camera &camera);

    const RenderScene &GetScene() const { return scene_; }

    const Camera        &GetCamera()        const { return camera_; }
    const CameraData    &GetCameraData()    const { return currRenderCamera_; }
    const RC<SubBuffer> &GetCameraCBuffer() const { return currCameraCBuffer_; }

    const CameraData    &GetPreviousCameraData()    const { return prevRenderCamera_; }
    const RC<SubBuffer> &GetPreviousCameraCBuffer() const { return prevCameraCBuffer_; }

    Span<MeshRendererRecord*> GetMeshRendererRecords()             const { return objects_; }
    const RC<Buffer>         &GetMeshRendererPerObjectDataBuffer() const { return perObjectDataBuffer_; }

    RenderAtmosphere::PerCameraData &GetAtmosphereData()  { return atmosphereData_; }
    PathTracer::PerCameraData       &GetPathTracingData() { return pathTracingData_; }
    CriusGI::PerCameraData          &GetCriusGIGIData()   { return criusGIGIData_; }

    void Update(
        const MeshRenderingCacheManager     &cachedMeshes,
        const MaterialRenderingCacheManager &cachedMaterials,
        LinearAllocator                     &linearAllocator);

    void UpdateDepthTexture(RG::RenderGraph &renderGraph, GBuffers &gbuffers);

private:

    ObserverPtr<Device> device_;
    
    const RenderScene &scene_;

    const Camera &camera_;

    CameraData currRenderCamera_;
    CameraData prevRenderCamera_;

    RC<SubBuffer> currCameraCBuffer_;
    RC<SubBuffer> prevCameraCBuffer_;

    std::vector<MeshRendererRecord *> objects_;
    RC<Buffer>                        perObjectDataBuffer_;
    UploadBufferPool<>                perObjectDataBufferPool_;
    
    RenderAtmosphere::PerCameraData atmosphereData_;
    PathTracer::PerCameraData       pathTracingData_;
    CriusGI::PerCameraData          criusGIGIData_;

    RC<StatefulTexture> prevDepth_;
    RC<StatefulTexture> currDepth_;
};

class RenderCameraManager : public Uncopyable
{
public:

    RenderCameraManager(ObserverPtr<Device> device, ObserverPtr<RenderSceneManager> scenes);

    ~RenderCameraManager();

    RenderCamera &GetRenderCamera(const Scene &scene, const Camera &camera);

private:

    struct CameraRecord
    {
        Box<RenderCamera> camera;
        const Scene *scene;
        Connection cameraConnection;
        Connection sceneConnection;
    };
    
    ObserverPtr<Device>             device_;
    ObserverPtr<RenderSceneManager> scenes_;

    std::map<const Camera *, CameraRecord> cameras_;
};

RTRC_RENDERER_END
