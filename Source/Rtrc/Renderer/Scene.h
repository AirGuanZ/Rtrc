#pragma once

#include <Rtrc/Renderer/Scene/Light.h>
#include <Rtrc/Renderer/Scene/StaticMesh.h>

RTRC_BEGIN

class Scene
{
public:

    void AddMesh(RC<StaticMesh> mesh);
    void RemoveMesh(const RC<StaticMesh> &mesh);

    void AddLight(RC<Light> light);
    void RemoveLight(const RC<Light> &light);

    Span<RC<StaticMesh>> GetAllStaticMeshes() const;

    Span<RC<Light>> GetAllLights() const;
    int GetDirectionalLightIndex() const; // There is at most one directional light

private:

    std::vector<RC<Light>> lights_;
    std::vector<RC<StaticMesh>> meshes_;
};

RTRC_END
