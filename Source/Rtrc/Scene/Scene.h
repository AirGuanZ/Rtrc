#pragma once

#include <Core/SignalSlot.h>
#include <Rtrc/Scene/Light/LightManager.h>
#include <Rtrc/Scene/MeshRenderer/MeshRendererManager.h>
#include <Rtrc/Scene/Sky/Sky.h>

RTRC_BEGIN

enum class SceneFlagBit : uint32_t
{
    EnableRayTracing
};

class Scene : public Uncopyable
{
public:

    Scene();
    ~Scene();

    Box<Light>        CreateLight();
    Box<MeshRenderer> CreateMeshRenderer();

    const Sky &GetSky() const;
          Sky &GetSky();

    int GetLightCount() const;

    template<typename F>
    void ForEachLight(const F &func);
    template<typename F>
    void ForEachLight(const F &func) const;

    template<typename F>
    void ForEachMeshRenderer(const F &func);
    template<typename F>
    void ForEachMeshRenderer(const F &func) const;

    void PrepareRender();

    template<typename...Args>
    Connection OnDestroy(Args&&...args) const { return onDestroy_.Connect(std::forward<Args>(args)...); }

private:

    SceneObject root_;

    Sky                 sky_;
    LightManager        lights_;
    MeshRendererManager meshRenderers_;

    mutable Signal<SignalThreadPolicy::SpinLock> onDestroy_;
};

inline Box<Light> Scene::CreateLight()
{
    auto ret = lights_.CreateLight();
    ret->SetParent(&root_);
    return ret;
}

inline Box<MeshRenderer> Scene::CreateMeshRenderer()
{
    auto ret = meshRenderers_.CreateMeshRenderer();
    ret->SetParent(&root_);
    return ret;
}

inline const Sky &Scene::GetSky() const
{
    return sky_;
}

inline Sky &Scene::GetSky()
{
    return sky_;
}

inline int Scene::GetLightCount() const
{
    return lights_.GetLightCount();
}

template<typename F>
void Scene::ForEachLight(const F &func)
{
    lights_.ForEachLight(func);
}

template<typename F>
void Scene::ForEachLight(const F &func) const
{
    lights_.ForEachLight(func);
}

template<typename F>
void Scene::ForEachMeshRenderer(const F &func)
{
    meshRenderers_.ForEachMeshRenderer(func);
}

template<typename F>
void Scene::ForEachMeshRenderer(const F &func) const
{
    meshRenderers_.ForEachMeshRenderer(func);
}

RTRC_END
