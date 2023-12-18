#pragma once

#include <Graphics/RenderGraph/Graph.h>
#include <Rtrc/Renderer/GPUScene/BlasBuilder.h>
#include <Rtrc/Resource/BindlessResourceManager.h>
#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>

RTRC_RENDERER_BEGIN

struct MeshRenderingCache
{
    const Mesh *mesh = nullptr;

    float    buildBlasSortKey = -1;
    RC<Blas> blas;

    // geometryBufferEntry[0] is vertex buffer
    // geometryBufferEntry[1] is index buffer iff hasIndexBuffer is true
    BindlessBufferEntry geometryBufferEntry;
    bool                hasIndexBuffer = false;
};

class MeshRenderingCacheManager : public Uncopyable
{
public:

    static constexpr int GLOBAL_BINDLESS_GEOMETRY_BUFFER_MAX_SIZE = 2048;

    explicit MeshRenderingCacheManager(Ref<Device> device);

    // Update records for all meshes in the scene
    void Update(const Scene &scene, const Camera &camera);

    RG::Pass *Build(RG::RenderGraph &renderGraph, int maxBuildCount, int maxPrimitiveCount);

    RC<BindingGroup> GetBindlessBufferBindingGroup() const;

    const MeshRenderingCache *FindMeshRenderingCache(const Mesh *mesh) const;

private:

    static float ComputeBlasSortKey(const Vector3f &eye, const MeshRenderer *meshRenderer);

    Ref<Device>        device_;
    Box<BindlessBufferManager> bindlessBuffers_;

    BlasBuilder blasBuilder_;

    std::vector<Box<MeshRenderingCache>> cachedMeshes_; // Sorted by raw mesh pointer
};

RTRC_RENDERER_END
