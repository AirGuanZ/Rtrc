#include <Rtrc/Scene/Renderer/StaticMeshRenderObject.h>

RTRC_BEGIN

StaticMeshRenderObject::~StaticMeshRenderObject()
{
    assert(manager_);
    manager_->_internalRelease(this);
}

void StaticMeshRenderObject::SetMesh(RC<Mesh> mesh)
{
    mesh_.swap(mesh);
    if(mesh_)
    {
        assert(mesh_->GetBoundingBox().IsValid());
        SetLocalBound(mesh_->GetBoundingBox());
    }
    else
    {
        SetLocalBound(AABB3f());
    }
}

StaticMeshRenderProxy *StaticMeshRenderObject::CreateProxyRaw(LinearAllocator &proxyAllocator) const
{
    if(!mesh_ || !matInst_)
    {
        return nullptr;
    }
    auto proxy = proxyAllocator.Create<StaticMeshRenderProxy>();
    proxy->type = RenderProxy::Type::StaticMesh;
    proxy->localToWorld = GetLocalToWorld();
    proxy->worldToLocal = GetWorldToLocal();
    proxy->localBound = GetLocalBound();
    proxy->worldBound = GetWorldBound();
    proxy->rayTracingFlags = rayTracingFlags_;
    proxy->meshRenderingData = mesh_->GetRenderingData();
    proxy->materialRenderingData = matInst_->GetRenderingData();
    return proxy;
}

Box<StaticMeshRenderObject> StaticMeshRendererManager::CreateStaticMeshRenderer()
{
    auto ret = MakeBox<StaticMeshRenderObject>();
    renderers_.push_front(ret.get());
    ret->manager_ = this;
    ret->iterator_ = renderers_.begin();
    return ret;
}

void StaticMeshRendererManager::_internalRelease(StaticMeshRenderObject *renderer)
{
    assert(renderer && renderer->manager_ == this);
    renderers_.erase(renderer->iterator_);
}

RTRC_END
