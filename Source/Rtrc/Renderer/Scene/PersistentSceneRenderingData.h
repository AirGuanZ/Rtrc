#pragma once

#include <Rtrc/Renderer/Scene/CachedMaterialManager.h>
#include <Rtrc/Renderer/Scene/CachedMeshManager.h>
#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_RENDERER_BEGIN

class PersistentSceneCameraRenderingData;

class PersistentSceneRenderingData : public Uncopyable
{
public:

    struct Config
    {
        bool rayTracing = false;
    };

    struct StaticMeshRecord
    {
        const StaticMeshRenderProxy                 *proxy          = nullptr;
        const CachedMeshManager::CachedMesh         *cachedMesh     = nullptr;
        const CachedMaterialManager::CachedMaterial *cachedMaterial = nullptr;
    };

    struct TlasMaterial
    {
        uint32_t albedoTextureIndex;
        float    albedoScale;
    };

    struct RenderGraphInterface
    {
        RG::Pass *buildTlasPass               = nullptr; // Optional[RayTracing]
        RG::Pass *prepareTlasMaterialDataPass = nullptr; // Optional[RayTracing]

        RG::BufferResource *tlasMaterialDataBuffer = nullptr; // Optional[RayTracing]
        RG::BufferResource *tlasBuffer             = nullptr; // Optional[RayTracing]
    };

    PersistentSceneRenderingData(const Config &config, ObserverPtr<Device> device);

    RenderGraphInterface Update(
        const RenderCommand_RenderStandaloneFrame &frame,
        TransientConstantBufferAllocator          &transientConstantBufferAllocator,
        const CachedMeshManager                   &meshManager,
        const CachedMaterialManager               &materialManager,
        RG::RenderGraph                           &renderGraph,
        LinearAllocator                           &linearAllocator);

    PersistentSceneCameraRenderingData *GetSceneCameraRenderingData(UniqueId cameraId) const;

    Span<StaticMeshRecord *> GetStaticMeshes() const { return objects_; }

    Span<const Light::SharedRenderingData*> GetLights() const { return scene_->GetLights(); }

    const RC<Tlas> &GetOpaqueTlas() const { return opaqueTlas_; }
    
private:

    Config config_;
    
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

    std::vector<Box<PersistentSceneCameraRenderingData>> cachedCameras_;
};


RTRC_RENDERER_END
