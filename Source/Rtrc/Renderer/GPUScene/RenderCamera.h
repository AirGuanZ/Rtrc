#pragma once

#include <Rtrc/Renderer/Crius/Crius.h>
#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/GPUScene/RenderScene.h>
#include <Rtrc/Renderer/PathTracer/PathTracer.h>
#include <Rtrc/Renderer/ReSTIR/ReSTIR.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>
#include <Rtrc/Renderer/VXIR/VXIR.h>
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

          GBuffers &GetGBuffers()       { return gbuffers_; }
    const GBuffers &GetGBuffers() const { return gbuffers_; }

    RenderAtmosphere::PerCameraData &GetAtmosphereData() { return atmosphereData_; }
    PathTracer::PerCameraData       &GetPathTracingData() { return pathTracingData_; }
    ReSTIR::PerCameraData           &GetReSTIRData() { return restirData_; }
    VXIR::PerCameraData             &GetVXIRData() { return vxirData_; }
    Crius::PerCameraData            &GetCriusData() { return criusData_; }

    void CreateGBuffers(ObserverPtr<RG::RenderGraph> renderGraph, const Vector2u &framebufferSize);
    void ClearGBuffers();

    template<typename T>
    void FillGBuffersIntoBindingGroup(T &data);

    void CreateNormalTexture(RG::RenderGraph &renderGraph, const Vector2u &framebufferSize, GBuffers &gbuffers);

    void Update(
        const MeshRenderingCacheManager     &cachedMeshes,
        const MaterialRenderingCacheManager &cachedMaterials,
        LinearAllocator                     &linearAllocator);

    void ResolveDepthTexture(RG::RenderGraph &renderGraph);

private:
    
    ObserverPtr<Device> device_;
    
    const RenderScene &scene_;
    const Camera      &camera_;

    CameraData currRenderCamera_;
    CameraData prevRenderCamera_;

    RC<SubBuffer> currCameraCBuffer_;
    RC<SubBuffer> prevCameraCBuffer_;

    std::vector<MeshRendererRecord *> objects_;
    RC<Buffer>                        perObjectDataBuffer_;
    UploadBufferPool<>                perObjectDataBufferPool_;

    GBuffers gbuffers_;

    RenderAtmosphere::PerCameraData atmosphereData_;
    PathTracer::PerCameraData       pathTracingData_;
    ReSTIR::PerCameraData           restirData_;
    VXIR::PerCameraData             vxirData_;
    Crius::PerCameraData            criusData_;

    RC<StatefulTexture> prevDepth_;
    RC<StatefulTexture> currDepth_;

    RC<StatefulTexture> prevNormal_;
    RC<StatefulTexture> currNormal_;
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

template<typename T>
void RenderCamera::FillGBuffersIntoBindingGroup(T &data)
{
    Renderer::FillBindingGroupGBuffers(data, gbuffers_);
}

RTRC_RENDERER_END
