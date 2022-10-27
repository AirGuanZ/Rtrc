#pragma once

#include <Rtrc/Renderer/StaticMesh.h>

RTRC_BEGIN

class Scene
{
public:

    void AddMesh(RC<StaticMesh> mesh);
    void RemoveMesh(const RC<StaticMesh> &mesh);

    Span<RC<StaticMesh>> GetAllMeshes() const;

private:

    std::vector<RC<StaticMesh>> meshes_;
};

RTRC_END
