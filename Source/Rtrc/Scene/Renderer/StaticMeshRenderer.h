#pragma once

#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Scene/Renderer/Renderer.h>
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

// ========================= Renderer proxy =========================

class StaticMeshRendererProxy : public RendererProxy
{
public:

    using RayTracingFlagBit = StaticMeshRendererRayTracingFlagBit;
    using RayTracingFlags   = StaticMeshRendererRayTracingFlags;

    ReferenceCountedPtr<const Mesh::SharedRenderingData>             meshRenderingData;
    ReferenceCountedPtr<const MaterialInstance::SharedRenderingData> materialRenderingData;
};

// ========================= Renderer =========================

class StaticMeshRenderer : public Renderer
{
public:

    using RayTracingFlagBit = StaticMeshRendererRayTracingFlagBit;
    using RayTracingFlags   = StaticMeshRendererRayTracingFlags;

    StaticMeshRenderer() = default;
    ~StaticMeshRenderer() override;

    void SetMesh(RC<Mesh> mesh) { mesh_.swap(mesh); }
    const RC<Mesh> &GetMesh() const { return mesh_; }

    void SetMaterial(RC<MaterialInstance> material) { matInst_.swap(material); }
    const RC<MaterialInstance> &GetMaterial() const { return matInst_; }

    RendererProxy *CreateProxy(LinearAllocator &proxyAllocator) const override { return CreateProxyRaw(proxyAllocator); }

    StaticMeshRendererProxy *CreateProxyRaw(LinearAllocator &proxyAllocator) const;

private:

    friend class StaticMeshRendererManager;

    StaticMeshRendererManager               *manager_ = nullptr;
    std::list<StaticMeshRenderer*>::iterator iterator_;

    RC<Mesh>             mesh_;
    RC<MaterialInstance> matInst_;
};

// ========================= Renderer manager =========================

class StaticMeshRendererManager : public Uncopyable
{
public:

    Box<StaticMeshRenderer> CreateStaticMeshRenderer();

    template<typename F>
    void ForEachRenderer(const F &func) const;

    void _internalRelease(StaticMeshRenderer *renderer);

private:

    // IMPROVE: use intrusive list
    std::list<StaticMeshRenderer*> renderers_;
};

template<typename F>
void StaticMeshRendererManager::ForEachRenderer(const F &func) const
{
    for(StaticMeshRenderer *renderer : renderers_)
    {
        func(renderer);
    }
}

RTRC_END
