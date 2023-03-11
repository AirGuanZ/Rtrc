#include <Rtrc/Renderer/Scene/Renderer/StaticMeshRenderer.h>

RTRC_BEGIN

StaticMeshRenderer::~StaticMeshRenderer()
{
    assert(manager_);
    manager_->_internalRelease(this);
}

StaticMeshRendererProxy *StaticMeshRenderer::CreateProxyRaw(LinearAllocator &proxyAllocator) const
{
    if(!mesh_ || !matInst_)
    {
        return nullptr;
    }
    auto proxy = proxyAllocator.Create<StaticMeshRendererProxy>();
    proxy->type = RendererProxy::Type::StaticMesh;
    proxy->localToWorld = GetLocalToWorld();
    proxy->worldToLocal = GetWorldToLocal();
    proxy->localBound = GetLocalBound();
    proxy->worldBound = GetWorldBound();
    proxy->meshRenderingData = mesh_->GetRenderingData();
    proxy->materialRenderingData = matInst_->GetRenderingData();
    return proxy;
}

Box<StaticMeshRenderer> StaticMeshRendererManager::CreateStaticMeshRenderer()
{
    auto ret = MakeBox<StaticMeshRenderer>();
    renderers_.push_front(ret.get());
    ret->manager_ = this;
    ret->iterator_ = renderers_.begin();
    return ret;
}

void StaticMeshRendererManager::_internalRelease(StaticMeshRenderer *renderer)
{
    assert(renderer && renderer->manager_ == this);
    renderers_.erase(renderer->iterator_);
}

RTRC_END
