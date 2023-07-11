#pragma once

#include <Rtrc/Renderer/Atmosphere/AtmosphereRenderer.h>
#include <Rtrc/Renderer/Scene/RenderMaterials.h>
#include <Rtrc/Renderer/Scene/RenderMeshes.h>
#include <Rtrc/Renderer/Scene/RenderLights.h>
#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_RENDERER_BEGIN

class RenderCamera;

class RenderScene : public Uncopyable
{
public:

    struct Config
    {
        bool rayTracing = false;
    };

    struct StaticMeshRecord
    {
        const StaticMeshRenderProxy           *proxy          = nullptr;
        const RenderMeshes::RenderMesh        *renderMesh     = nullptr;
        const RenderMaterials::RenderMaterial *renderMaterial = nullptr;
    };

    struct TlasMaterial
    {
        uint32_t albedoTextureIndex;
        float    albedoScale;
    };
    
    RenderScene(
        const Config                       &config,
        ObserverPtr<Device>                 device,
        ObserverPtr<BuiltinResourceManager> builtinResources);

    void Update(
        const RenderCommand_RenderStandaloneFrame &frame,
        TransientConstantBufferAllocator          &transientConstantBufferAllocator,
        const RenderMeshes                        &renderMeshes,
        const RenderMaterials                     &renderMaterials,
        RG::RenderGraph                           &renderGraph,
        LinearAllocator                           &linearAllocator);

    void ClearFrameData();
    
    RenderCamera *GetRenderCamera(UniqueId cameraId) const;

    Span<StaticMeshRecord *> GetStaticMeshes() const { return objects_; }
    
    Span<const Light::SharedRenderingData*> GetLights()            const { return scene_->GetLights(); }
    const RenderLights                     &GetRenderLights()      const { return *renderLights_; }
    const RenderAtmosphere                 &GetRenderAtmosphere()  const { return *renderAtmosphere_; }

    RG::BufferResource *GetRGPointLightBuffer()       const { return renderLights_->GetRGPointLightBuffer(); }
    RG::BufferResource *GetRGDirectionalLightBuffer() const { return renderLights_->GetRGDirectionalLightBuffer(); }
    
    RG::Pass           *GetRGBuildTlasPass()      const { return buildTlasPass_;      }
    RG::BufferResource *GetRGTlasMaterialBuffer() const { return tlasMaterialBuffer_; }
    RG::TlasResource   *GetRGTlas()               const { return opaqueTlas_;         }
    
private:

    // Update objects_ & tlasObjects_
    void CollectObjects(
        const SceneProxy      &scene,
        const RenderMeshes    &meshManager,
        const RenderMaterials &materialManager,
        LinearAllocator       &linearAllocator);

    // Update tlas & materials
    void UpdateTlasScene(RG::RenderGraph &renderGraph);
    
    Config              config_;
    ObserverPtr<Device> device_;
    const SceneProxy   *scene_;
    
    std::vector<StaticMeshRecord*> objects_;
    std::vector<StaticMeshRecord*> tlasObjects_;

    // Tlas scene

    UploadBufferPool<>  materialDataUploadBufferPool_;
    UploadBufferPool<>  instanceDataUploadBufferPool_;
    RG::Pass           *buildTlasPass_      = nullptr;
    RG::BufferResource *tlasMaterialBuffer_ = nullptr;
    RG::TlasResource   *opaqueTlas_         = nullptr;
    RC<StatefulBuffer>  cachedOpaqueTlasBuffer_;
    RC<Tlas>            cachedOpaqueTlas_;

    // Lights

    Box<RenderLights> renderLights_;

    // Atmosphere

    Box<RenderAtmosphere> renderAtmosphere_;

    // Per-camera data

    std::vector<Box<RenderCamera>> renderCameras_;
};

RTRC_RENDERER_END
