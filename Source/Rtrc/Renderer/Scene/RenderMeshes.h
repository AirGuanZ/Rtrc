#pragma once

#include <Rtrc/Renderer/Scene/BlasBuilder.h>
#include <Rtrc/Renderer/RenderCommand.h>

RTRC_RENDERER_BEGIN

class RenderMeshes : public Uncopyable
{
public:

    static constexpr int GLOBAL_BINDLESS_GEOMETRY_BUFFER_MAX_SIZE = 2048;

    struct Config
    {
        bool rayTracing = false;
    };

    struct RenderMesh
    {
        UniqueId                         meshId = {};
        const Mesh::SharedRenderingData *meshRenderingData = nullptr;

        float               buildBlasSortKey = -1;
        RC<Blas>            blas;
        BindlessBufferEntry geometryBufferEntry;
        bool                hasIndexBuffer = false;
    };

    RenderMeshes(const Config &config, ObserverPtr<Device> device);

    void Update(const RenderCommand_RenderStandaloneFrame &frame);

    void BuildBlasForMeshes(RG::RenderGraph &renderGraph, int maxBuildCount, int maxPrimitiveCount);

    void ClearFrameData();

          RenderMesh *FindCachedMesh(UniqueId meshId);
    const RenderMesh *FindCachedMesh(UniqueId meshId) const;

    // Returns nullptr when no blas is built
    RG::Pass *GetRGBuildBlasPass() const;

          RC<BindingGroup>        GetGlobalGeometryBuffersBindingGroup()       const;
    const RC<BindingGroupLayout> &GetGlobalGeometryBuffersBindingGroupLayout() const;
    
private:

    static float ComputeBuildBlasSortKey(const Vector3f &eye, const StaticMeshRenderProxy *renderer);

    Config config_;

    ObserverPtr<Device>        device_;
    Box<BindlessBufferManager> bindlessBufferManager_;

    BlasBuilder blasBuilder_;

    std::list<Box<RenderMesh>> cachedMeshes_;
    std::vector<RenderMesh *>  linearCachedMeshes_;

    RG::Pass *buildBlasPass_ = nullptr;
};

inline RG::Pass *RenderMeshes::GetRGBuildBlasPass() const
{
    return buildBlasPass_;
}

inline RC<BindingGroup> RenderMeshes::GetGlobalGeometryBuffersBindingGroup() const
{
    return bindlessBufferManager_->GetBindingGroup();
}

inline const RC<BindingGroupLayout> &RenderMeshes::GetGlobalGeometryBuffersBindingGroupLayout() const
{
    return bindlessBufferManager_->GetBindingGroupLayout();
}

RTRC_RENDERER_END
