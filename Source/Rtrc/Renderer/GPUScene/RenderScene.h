#pragma once

#include <Rtrc/Renderer/Atmosphere/AtmosphereRenderer.h>
#include <Rtrc/Renderer/GPUScene/MaterialRenderingCacheManager.h>
#include <Rtrc/Renderer/GPUScene/MeshRenderingCacheManager.h>
#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>

RTRC_RENDERER_BEGIN

class RenderScene : public Uncopyable
{
public:

    struct Config
    {
        bool opaqueTlas = false;
    };
    
    struct TlasInstance
    {
        uint16_t albedoTextureIndex        = 0;
        float16  albedoScale               = 1.0_f16;
        uint16_t encodedGeometryBufferInfo = 0; // [0, 15): vertex buffer index
                                                // [15, 16): has index buffer
        uint16_t pad0 = 0;
    };
    static_assert(sizeof(TlasInstance) == 8);

    struct MeshRendererRenderingCache
    {
        const MeshRenderer           *renderer = nullptr;
        const MeshRenderingCache     *meshCache = nullptr;
        const MaterialRenderingCache *materialCache = nullptr;
    };
    
    RenderScene(
        ObserverPtr<ResourceManager>               resources,
        ObserverPtr<MaterialRenderingCacheManager> cachedMaterials,
        ObserverPtr<MeshRenderingCacheManager>     cachedMeshes,
        ObserverPtr<BindlessTextureManager>        bindlessTextures,
        const Scene                               &scene);

    const Scene &GetScene() const { return scene_; }

    void FrameUpdate(const Config &config, RG::RenderGraph &renderGraph);
    void ClearFrameData();
    
    const RenderAtmosphere &GetRenderAtmosphere()  const { return *atmosphere_; }
    
    RG::Pass           *GetBuildTlasPass()  const { return buildTlasPass_;  }
    RG::TlasResource   *GetTlas()           const { return opaqueTlas_;     }
    RG::BufferResource *GetInstanceBuffer() const { return tlasInstanceBuffer_; }

    RC<BindingGroup> GetBindlessTextures()        const { return bindlessTextures_->GetBindingGroup(); }
    RC<BindingGroup> GetBindlessGeometryBuffers() const { return cachedMeshes_->GetBindlessBufferBindingGroup(); }
    
private:
    
    const Scene                               &scene_;
    ObserverPtr<Device>                        device_;
    ObserverPtr<ResourceManager>               resources_;
    ObserverPtr<MaterialRenderingCacheManager> cachedMaterials_;
    ObserverPtr<MeshRenderingCacheManager>     cachedMeshes_;
    ObserverPtr<BindlessTextureManager>        bindlessTextures_;
    
    // Tlas scene

    UploadBufferPool<> instanceStagingBufferPool_;
    UploadBufferPool<> rhiInstanceDataBufferPool_;

    RG::Pass           *buildTlasPass_      = nullptr;
    RG::BufferResource *tlasInstanceBuffer_ = nullptr;
    RG::TlasResource   *opaqueTlas_         = nullptr;

    RC<StatefulBuffer> cachedOpaqueTlasBuffer_;
    RC<Tlas>           cachedOpaqueTlas_;
    
    // Atmosphere

    Box<RenderAtmosphere> atmosphere_;
};

class RenderSceneManager : public Uncopyable
{
public:

    explicit RenderSceneManager(
        ObserverPtr<ResourceManager>               resources,
        ObserverPtr<MaterialRenderingCacheManager> cachedMaterials,
        ObserverPtr<MeshRenderingCacheManager>     cachedMeshes,
        ObserverPtr<BindlessTextureManager>        bindlessTextures);

    ~RenderSceneManager();

    RenderScene &GetRenderScene(const Scene &scene);

private:

    struct SceneRecord
    {
        Box<RenderScene> renderScene;
        Connection connection;
    };

    ObserverPtr<ResourceManager>               resources_;
    ObserverPtr<MaterialRenderingCacheManager> cachedMaterials_;
    ObserverPtr<MeshRenderingCacheManager>     cachedMeshes_;
    ObserverPtr<BindlessTextureManager>        bindlessTextures_;

    std::map<const Scene *, SceneRecord> renderScenes_;
};

RTRC_RENDERER_END
