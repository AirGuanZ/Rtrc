#pragma once

#include <Rtrc/Renderer/Scene/CachedMaterialManager.h>
#include <Rtrc/Renderer/Scene/CachedMeshManager.h>
#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_RENDERER_BEGIN

class CachedScenePerCamera;

class CachedScene : public Uncopyable
{
public:

    struct StaticMeshRecord
    {
        const StaticMeshRenderProxy                 *proxy          = nullptr;
        const CachedMeshManager::CachedMesh         *cachedMesh     = nullptr;
        const CachedMaterialManager::CachedMaterial *cachedMaterial = nullptr;
    };

    struct TlasMaterial
    {
        uint32_t albedoTextureIndex;
    };

    struct RenderGraphInterface
    {
        RG::Pass *buildTlasPass               = nullptr;
        RG::Pass *prepareTlasMaterialDataPass = nullptr;

        RG::BufferResource *tlasMaterialDataBuffer = nullptr;
        RG::BufferResource *tlasBuffer             = nullptr;
    };

    explicit CachedScene(ObserverPtr<Device> device);

    RenderGraphInterface Update(
        const RenderCommand_RenderStandaloneFrame &frame,
        TransientConstantBufferAllocator          &transientConstantBufferAllocator,
        const CachedMeshManager                   &meshManager,
        const CachedMaterialManager               &materialManager,
        RG::RenderGraph                           &renderGraph,
        LinearAllocator                           &linearAllocator);

    const CachedScenePerCamera *GetCachedScenePerCamera(UniqueId cameraId) const;

    Span<StaticMeshRecord *> GetStaticMeshes() const { return objects_; }

    Span<const Light::SharedRenderingData*> GetLights() const { return scene_->GetLights(); }
    
private:
    
    ObserverPtr<Device> device_;

    const SceneProxy *scene_;
    
    std::vector<StaticMeshRecord*> objects_;
    std::vector<StaticMeshRecord*> tlasObjects_;

    Box<BindlessBufferManager> tlasGeometryBuffers_;
    RC<StatefulBuffer>         tlasMaterials_;

    UploadBufferPool<> materialDataUploadBufferPool_;
    UploadBufferPool<> instanceDataUploadBufferPool_;

    RC<StatefulBuffer> opaqueTlasScratchBuffer_;
    RC<StatefulBuffer> opaqueTlasBuffer_;
    RC<Tlas>           opaqueTlas_;

    std::vector<Box<CachedScenePerCamera>> cachedScenesPerCamera_;
};

class CachedScenePerCamera
{
public:

    struct StaticMeshRecord : CachedScene::StaticMeshRecord
    {
        RC<BindingGroup> perObjectBindingGroup;
        int              gbufferPassIndex = -1;
    };

    CachedScenePerCamera(const CachedScene &scene, UniqueId cameraId);
    
    const RenderCamera &GetCamera() const { return renderCamera_; }
    const RC<SubBuffer> &GetCameraCBuffer() const { return cameraCBuffer_; }
    Span<StaticMeshRecord *> GetStaticMeshes() const { return objects_; }
    Span<const Light::SharedRenderingData*> GetLights() const { return scene_.GetLights(); }

    void Update(
        const RenderCamera               &camera,
        TransientConstantBufferAllocator &transientConstantBufferAllocator,
        LinearAllocator                  &linearAllocator);

private:

    const CachedScene  &scene_;
    RenderCamera        renderCamera_;

    RC<SubBuffer>                   cameraCBuffer_;
    std::vector<StaticMeshRecord *> objects_;
};

RTRC_RENDERER_END