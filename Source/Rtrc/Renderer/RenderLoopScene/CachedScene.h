#pragma once

#include <Rtrc/Renderer/RenderLoopScene/CachedMaterialManager.h>
#include <Rtrc/Renderer/RenderLoopScene/CachedMeshManager.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_RENDERER_BEGIN

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
        const CachedMeshManager                   &meshManager,
        const CachedMaterialManager               &materialManager,
        RG::RenderGraph                           &renderGraph);
    
private:

    void AcquireTlasMaterialDataUploadBuffer(size_t byteSize, RC<Buffer> &uploadBuffer);

    ObserverPtr<Device> device_;

    LinearAllocator                objectAllocator_;
    std::vector<StaticMeshRecord*> objects_;
    std::vector<StaticMeshRecord*> tlasObjects_;

    Box<BindlessBufferManager> tlasGeometryBuffers_;
    RC<StatefulBuffer>         tlasMaterials_;

    UploadBufferPool materialDataUploadBufferPool_;
    UploadBufferPool instanceDataUploadBufferPool_;

    RC<StatefulBuffer> opaqueTlasScratchBuffer_;
    RC<StatefulBuffer> opaqueTlasBuffer_;
    RC<Tlas>           opaqueTlas_;
};

RTRC_RENDERER_END
