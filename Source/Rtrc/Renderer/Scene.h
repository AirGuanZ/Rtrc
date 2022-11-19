#pragma once

#include <Rtrc/Renderer/Scene/StaticMesh.h>

RTRC_BEGIN

class Scene
{
public:

    void AddMesh(RC<StaticMesh> mesh);
    void RemoveMesh(const RC<StaticMesh> &mesh);

    Span<RC<StaticMesh>> GetAllStaticMeshes() const;

private:

    std::vector<RC<StaticMesh>> meshes_;
};

RTRC_END
