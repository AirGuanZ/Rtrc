#pragma once

#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Renderer/Scene/Renderer/Renderer.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_BEGIN

class Scene;
class StaticMeshRendererManager;

rtrc_struct(StaticMeshCBuffer)
{
    rtrc_var(float4x4, localToWorld);
    rtrc_var(float4x4, worldToLocal);
    rtrc_var(float4x4, localToCamera);
    rtrc_var(float4x4, localToClip);
};

class StaticMeshRendererProxy : public RendererProxy
{
public:

    ReferenceCountedPtr<const Mesh::SharedRenderingData>             meshRenderingData;
    ReferenceCountedPtr<const MaterialInstance::SharedRenderingData> materialRenderingData;
};

class StaticMeshRenderer : public Renderer
{
public:

    StaticMeshRenderer() = default;
    ~StaticMeshRenderer() override;

    void SetMesh(RC<Mesh> mesh) { mesh_.swap(mesh); }
    const RC<Mesh> &GetMesh() const { return mesh_; }

    void SetMaterial(RC<MaterialInstance> material) { matInst_.swap(material); }
    const RC<MaterialInstance> &GetMaterial() const { return matInst_; }

    RendererProxy *CreateProxy(LinearAllocator &proxyAllocator) const override;

private:

    friend class StaticMeshRendererManager;

    StaticMeshRendererManager               *manager_ = nullptr;
    std::list<StaticMeshRenderer*>::iterator iterator_;

    RC<Mesh>             mesh_;
    RC<MaterialInstance> matInst_;
};

class StaticMeshRendererManager : public Uncopyable
{
public:

    Box<StaticMeshRenderer> CreateStaticMeshRenderer();

    template<typename F>
    void ForEachRenderer(const F &func) const;

    void _internalRelease(StaticMeshRenderer *renderer);

private:

    // TODO: use intrusive list
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
