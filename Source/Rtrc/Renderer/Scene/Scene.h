#pragma once

#include <Rtrc/Renderer/Scene/Light.h>
#include <Rtrc/Renderer/Scene/Renderer/StaticMeshRenderer.h>

RTRC_BEGIN

struct SceneVersion
{
    enum Type
    {
        UpdateStaticMeshTransform,
        AddOrRemoveStaticMesh,
        TypeCount
    };
    uint32_t versions[TypeCount] = {};
};

class SceneProxy
{
public:

    ~SceneProxy();

    Span<const Light::SharedRenderingData *> GetLights()              const { return lights_; }
    Span<const RendererProxy *>              GetRenderers()           const { return renderers_; }
    Span<const StaticMeshRendererProxy*>     GetStaticMeshRenderers() const { return staticMeshRendererProxies_; }

private:

    friend class Scene;

    enum VersionType
    {
        VersionType_UpdateStaticMeshTransform,
        VersionType_AddOrRemoveStaticMesh,
        VersionType_Count
    };

    LinearAllocator                                 arena_;
    std::vector<const Light::SharedRenderingData *> lights_;
    std::vector<const RendererProxy *>              renderers_;
    std::vector<const StaticMeshRendererProxy *>    staticMeshRendererProxies_;

    //SceneVersion version_;
};

class Scene : public Uncopyable
{
public:

    Scene();
    
    Box<StaticMeshRenderer> CreateStaticMeshRenderer();

    Box<Light> CreateLight();

    void PrepareRendering();

    Box<SceneProxy> CreateSceneProxy() const;

private:

    //SceneVersion version_;
    SceneObject rootNode_;

    LightManager              lightManager_;
    StaticMeshRendererManager staticRendererManager_;
};

RTRC_END
