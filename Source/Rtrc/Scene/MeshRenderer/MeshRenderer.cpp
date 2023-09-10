#include <Rtrc/Scene/MeshRenderer/MeshRendererManager.h>

RTRC_BEGIN

MeshRenderer::~MeshRenderer()
{
    assert(manager_);
    manager_->_internalRelease(this);
}

RTRC_END
