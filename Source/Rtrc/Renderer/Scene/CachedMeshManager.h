#pragma once

#include <Rtrc/Renderer/Scene/BlasBuilder.h>
#include <Rtrc/Renderer/RenderCommand.h>

RTRC_RENDERER_BEGIN

class CachedMeshManager : public Uncopyable
{
public:

    struct CachedMesh
    {
        UniqueId                         meshId = {};
        const Mesh::SharedRenderingData *mesh = nullptr;

        float               buildBlasSortKey = -1;
        RC<Blas>            blas;
        BindlessBufferEntry geometryBufferEntry;
    };

    explicit CachedMeshManager(ObserverPtr<Device> device);

    void UpdateCachedMeshData(const RenderCommand_RenderStandaloneFrame &frame);

          CachedMesh *FindCachedMesh(UniqueId meshId);
    const CachedMesh *FindCachedMesh(UniqueId meshId) const;

    RG::Pass *BuildBlasForMeshes(RG::RenderGraph &renderGraph, int maxBuildCount, int maxPrimitiveCount);

private:

    static float ComputeBuildBlasSortKey(const Vector3f &eye, const StaticMeshRenderProxy *renderer);

    ObserverPtr<Device>        device_;
    Box<BindlessBufferManager> bindlessBufferManager_;

    BlasBuilder blasBuilder_;

    std::list<Box<CachedMesh>> cachedMeshes_;
    std::vector<CachedMesh *>  linearCachedMeshes_;
};

RTRC_RENDERER_END
