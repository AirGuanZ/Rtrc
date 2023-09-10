#include <Rtrc/Scene/MeshRenderer/MeshRendererManager.h>

RTRC_BEGIN

Box<MeshRenderer> MeshRendererManager::CreateMeshRenderer()
{
    auto renderer = Box<MeshRenderer>(new MeshRenderer);
    renderers_.push_front(renderer.get());
    renderer->manager_ = this;
    renderer->iterator_ = renderers_.begin();
    return renderer;
}

void MeshRendererManager::_internalRelease(MeshRenderer *renderer)
{
    assert(renderer->manager_ == this);
    renderers_.erase(renderer->iterator_);
}

RTRC_END
