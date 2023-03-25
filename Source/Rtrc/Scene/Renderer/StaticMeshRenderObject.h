#pragma once

#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Scene/Renderer/RenderObject.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_BEGIN

class Scene;
class StaticMeshRendererManager;

// ========================= Ray tracing flag =========================

enum class StaticMeshRendererRayTracingFlagBit : uint32_t
{
    None         = 0,
    InOpaqueTlas = 1 << 0
};
RTRC_DEFINE_ENUM_FLAGS(StaticMeshRendererRayTracingFlagBit)
using StaticMeshRendererRayTracingFlags = EnumFlagsStaticMeshRendererRayTracingFlagBit;

// ========================= Per-object constant buffer struct =========================

rtrc_struct(StaticMeshCBuffer)
{
    rtrc_var(float4x4, localToWorld);
    rtrc_var(float4x4, worldToLocal);
    rtrc_var(float4x4, localToCamera);
    rtrc_var(float4x4, localToClip);
};

// ========================= RenderObject proxy =========================

class StaticMeshRenderProxy : public RenderProxy
{
public:

    using RayTracingFlagBit = StaticMeshRendererRayTracingFlagBit;
    using RayTracingFlags   = StaticMeshRendererRayTracingFlags;

    RayTracingFlags                                            rayTracingFlags;
    ReferenceCountedPtr<Mesh::SharedRenderingData>             meshRenderingData;
    ReferenceCountedPtr<MaterialInstance::SharedRenderingData> materialRenderingData;
};

// ========================= RenderObject =========================

class StaticMeshRenderObject : public RenderObject
{
public:

    using RayTracingFlagBit = StaticMeshRendererRayTracingFlagBit;
    using RayTracingFlags   = StaticMeshRendererRayTracingFlags;

    StaticMeshRenderObject() = default;
    ~StaticMeshRenderObject() override;

    void SetRayTracingFlags(RayTracingFlags flags) { rayTracingFlags_ = flags; }
    RayTracingFlags GetRayTracingFlags() const { return rayTracingFlags_; }

    void SetMesh(RC<Mesh> mesh) { mesh_.swap(mesh); }
    const RC<Mesh> &GetMesh() const { return mesh_; }

    void SetMaterial(RC<MaterialInstance> material) { matInst_.swap(material); }
    const RC<MaterialInstance> &GetMaterial() const { return matInst_; }

    RenderProxy *CreateProxy(LinearAllocator &proxyAllocator) const override { return CreateProxyRaw(proxyAllocator); }

    StaticMeshRenderProxy *CreateProxyRaw(LinearAllocator &proxyAllocator) const;

private:

    friend class StaticMeshRendererManager;

    StaticMeshRendererManager               *manager_ = nullptr;
    std::list<StaticMeshRenderObject*>::iterator iterator_;

    RayTracingFlags      rayTracingFlags_;
    RC<Mesh>             mesh_;
    RC<MaterialInstance> matInst_;
};

// ========================= RenderObject manager =========================

class StaticMeshRendererManager : public Uncopyable
{
public:

    Box<StaticMeshRenderObject> CreateStaticMeshRenderer();

    template<typename F>
    void ForEachRenderer(const F &func) const;

    void _internalRelease(StaticMeshRenderObject *renderer);

private:

    // IMPROVE: use intrusive list
    std::list<StaticMeshRenderObject*> renderers_;
};

template<typename F>
void StaticMeshRendererManager::ForEachRenderer(const F &func) const
{
    for(StaticMeshRenderObject *renderer : renderers_)
    {
        func(renderer);
    }
}

RTRC_END
