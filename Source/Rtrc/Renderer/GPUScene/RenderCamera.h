#pragma once

#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/GPUScene/RenderScene.h>
#include <Rtrc/Renderer/ReSTIR/ReSTIR.h>
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

    RenderCamera(Ref<Device> device, const RenderScene &scene, const Camera &camera);

    const RenderScene &GetScene() const { return *scene_; }

    const Camera        &GetCamera()        const { return *camera_; }
    const CameraData    &GetCameraData()    const { return currRenderCamera_; }
    const RC<SubBuffer> &GetCameraCBuffer() const { return currCameraCBuffer_; }

    const CameraData    &GetPreviousCameraData()    const { return prevRenderCamera_; }
    const RC<SubBuffer> &GetPreviousCameraCBuffer() const { return prevCameraCBuffer_; }

    Span<MeshRendererRecord*> GetMeshRendererRecords()             const { return objects_; }
    const RC<Buffer>         &GetMeshRendererPerObjectDataBuffer() const { return perObjectDataBuffer_; }

          GBuffers &GetGBuffers()       { return gbuffers_; }
    const GBuffers &GetGBuffers() const { return gbuffers_; }

    RenderAtmosphere::PerCameraData &GetAtmosphereData() { return atmosphereData_; }
    ReSTIR::PerCameraData           &GetReSTIRData() { return restirData_; }
    
    void CreateGBuffers(Ref<RG::RenderGraph> renderGraph, const Vector2u &framebufferSize);
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
    
    Ref<Device> device_;
    
    const RenderScene *scene_;
    const Camera      *camera_;

    CameraData currRenderCamera_;
    CameraData prevRenderCamera_;

    RC<SubBuffer> currCameraCBuffer_;
    RC<SubBuffer> prevCameraCBuffer_;

    std::vector<MeshRendererRecord *> objects_;
    RC<Buffer>                        perObjectDataBuffer_;
    UploadBufferPool<>                perObjectDataBufferPool_;

    GBuffers gbuffers_;

    RenderAtmosphere::PerCameraData atmosphereData_;
    ReSTIR::PerCameraData           restirData_;
    
    RC<StatefulTexture> prevDepth_;
    RC<StatefulTexture> currDepth_;

    RC<StatefulTexture> prevNormal_;
    RC<StatefulTexture> currNormal_;
};

class RenderCameraManager : public Uncopyable
{
public:

    RenderCameraManager(Ref<Device> device, Ref<RenderSceneManager> scenes);

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
    
    Ref<Device>             device_;
    Ref<RenderSceneManager> scenes_;

    std::map<const Camera *, CameraRecord> cameras_;
};

template<typename T>
void RenderCamera::FillGBuffersIntoBindingGroup(T &data)
{
    Renderer::FillBindingGroupGBuffers(data, gbuffers_);
}

RTRC_RENDERER_END
