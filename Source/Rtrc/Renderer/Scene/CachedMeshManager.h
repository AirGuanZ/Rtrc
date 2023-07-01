#pragma once

#include <Rtrc/Renderer/Scene/BlasBuilder.h>
#include <Rtrc/Renderer/RenderCommand.h>

RTRC_RENDERER_BEGIN

class CachedMeshManager : public Uncopyable
{
public:

    struct Config
    {
        bool rayTracing = false;
    };

    struct CachedMesh
    {
        UniqueId                         meshId = {};
        const Mesh::SharedRenderingData *meshRenderingData = nullptr;

        float               buildBlasSortKey = -1;
        RC<Blas>            blas;
        BindlessBufferEntry geometryBufferEntry;
    };

    CachedMeshManager(const Config &config, ObserverPtr<Device> device);

    void Update(const RenderCommand_RenderStandaloneFrame &frame);

    void BuildBlasForMeshes(RG::RenderGraph &renderGraph, int maxBuildCount, int maxPrimitiveCount);

    void ClearFrameData();

          CachedMesh *FindCachedMesh(UniqueId meshId);
    const CachedMesh *FindCachedMesh(UniqueId meshId) const;

    // Returns nullptr when no blas is built
    RG::Pass *GetRGBuildBlasPass() const { return buildBlasPass_; }
    
private:

    static float ComputeBuildBlasSortKey(const Vector3f &eye, const StaticMeshRenderProxy *renderer);

    Config config_;

    ObserverPtr<Device>        device_;
    Box<BindlessBufferManager> bindlessBufferManager_;

    BlasBuilder blasBuilder_;

    std::list<Box<CachedMesh>> cachedMeshes_;
    std::vector<CachedMesh *>  linearCachedMeshes_;

    RG::Pass *buildBlasPass_ = nullptr;
};

RTRC_RENDERER_END
