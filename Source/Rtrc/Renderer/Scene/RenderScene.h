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
    
    struct TlasInstance
    {
        uint16_t albedoTextureIndex        = 0;
        float16  albedoScale               = 1.0_f16;
        uint16_t encodedGeometryBufferInfo = 0; // [0, 14): vertex buffer index
                                                // [14, 15): is index 16-bit
                                                // [15, 16): has index buffer
        uint16_t pad0                   = 0;
    };
    static_assert(sizeof(TlasInstance) == 8);
    
    RenderScene(
        const Config                       &config,
        ObserverPtr<Device>                 device,
        ObserverPtr<ResourceManager>        resources,
        ObserverPtr<BindlessTextureManager> bindlessTextures);

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
    
    RG::Pass           *GetRGBuildTlasPass()  const { return buildTlasPass_;  }
    RG::TlasResource   *GetRGTlas()           const { return opaqueTlas_;     }
    RG::BufferResource *GetRGInstanceBuffer() const { return instanceBuffer_; }

    const RenderMeshes    &GetRenderMeshes()    const { return *renderMeshes_;    }
    const RenderMaterials &GetRenderMaterials() const { return *renderMaterials_; }

    RC<BindingGroup>              GetGlobalTextureBindingGroup()       const { return bindlessTextures_->GetBindingGroup();       }
    const RC<BindingGroupLayout> &GetGlobalTextureBindingGroupLayout() const { return bindlessTextures_->GetBindingGroupLayout(); }

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

    ObserverPtr<BindlessTextureManager> bindlessTextures_;
    
    std::vector<StaticMeshRecord*> objects_;
    std::vector<StaticMeshRecord*> tlasObjects_;

    const RenderMeshes    *renderMeshes_;
    const RenderMaterials *renderMaterials_;

    // Tlas scene

    UploadBufferPool<> instanceStagingBufferPool_;
    UploadBufferPool<> rhiInstanceDataBufferPool_;

    RG::Pass           *buildTlasPass_  = nullptr;
    RG::BufferResource *instanceBuffer_ = nullptr;
    RG::TlasResource   *opaqueTlas_     = nullptr;

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
