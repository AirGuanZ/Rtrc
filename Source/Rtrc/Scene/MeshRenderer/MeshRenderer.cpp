#include <Rtrc/Scene/MeshRenderer/MeshRendererManager.h>

RTRC_BEGIN

MeshRenderer::~MeshRenderer()
{
    assert(manager_);
    manager_->_internalRelease(this);
}

void MeshRenderer::SetMesh(RC<Mesh> mesh)
{
    mesh_ = std::move(mesh);
    SetLocalBound(mesh_->GetBoundingBox());
}

RTRC_END
