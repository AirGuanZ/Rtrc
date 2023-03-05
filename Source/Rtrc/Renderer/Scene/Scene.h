#pragma once

#include <Rtrc/Renderer/Scene/Light.h>
#include <Rtrc/Renderer/Scene/Renderer/StaticMeshRenderer.h>

RTRC_BEGIN

class SceneProxy
{
public:

    ~SceneProxy();

    Span<const Light::SharedRenderingData *> GetLights() const { return lights_; }
    Span<const RendererProxy *> GetRenderers() const { return renderers_; }

private:

    friend class Scene;

    LinearAllocator                                 arena_;
    std::vector<const Light::SharedRenderingData *> lights_;
    std::vector<const RendererProxy *>              renderers_;
};

class Scene
{
public:
    
    Box<StaticMeshRenderer> CreateStaticMeshRenderer();

    Box<Light> CreateLight();

    Box<SceneProxy> CreateSceneProxy() const;

private:
    
    LightManager              lightManager_;
    StaticMeshRendererManager staticRendererManager_;
};

inline Box<StaticMeshRenderer> Scene::CreateStaticMeshRenderer()
{
    return staticRendererManager_.CreateStaticMeshRenderer();
}

inline Box<Light> Scene::CreateLight()
{
    return lightManager_.CreateLight();
}

RTRC_END
