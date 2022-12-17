#pragma once

#include <Rtrc/Renderer/Scene/Light.h>
#include <Rtrc/Renderer/Scene/StaticMesh.h>

RTRC_BEGIN

class Scene
{
public:

    void AddMesh(RC<const StaticMesh> mesh);
    void RemoveMesh(const RC<const StaticMesh> &mesh);

    void AddLight(RC<const Light> light);
    void RemoveLight(const RC<const Light> &light);

    const std::set<RC<const StaticMesh>> &GetAllStaticMeshes() const;
    const std::set<RC<const Light>> &GetAllLights() const;
    const RC<const Light> &GetDirectionalLight() const;

private:

    RC<const Light> directionalLight_;
    std::set<RC<const Light>> lightSet_;
    std::set<RC<const StaticMesh>> meshSet_;
};

inline void Scene::AddMesh(RC<const StaticMesh> mesh)
{
    meshSet_.insert(std::move(mesh));
}

inline void Scene::RemoveMesh(const RC<const StaticMesh> &mesh)
{
    meshSet_.erase(mesh);
}

inline void Scene::AddLight(RC<const Light> light)
{
    if(light->GetType() == Light::Type::Directional)
    {
        if(directionalLight_)
        {
            throw Exception("Scene can contain at most one directional light");
        }
        directionalLight_ = light;
    }
    lightSet_.insert(std::move(light));
}

inline void Scene::RemoveLight(const RC<const Light> &light)
{
    if(directionalLight_ == light)
    {
        directionalLight_.reset();
    }
    lightSet_.erase(light);
}

inline const std::set<RC<const StaticMesh>> &Scene::GetAllStaticMeshes() const
{
    return meshSet_;
}

inline const std::set<RC<const Light>> &Scene::GetAllLights() const
{
    return lightSet_;
}

inline const RC<const Light> &Scene::GetDirectionalLight() const
{
    return directionalLight_;
}

RTRC_END
