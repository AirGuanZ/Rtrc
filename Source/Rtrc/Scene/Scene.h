#pragma once

#include <Rtrc/Scene/Light.h>
#include <Rtrc/Scene/Renderer/StaticMeshRenderObject.h>

RTRC_BEGIN

class SceneProxy
{
public:

    ~SceneProxy();

    Span<const Light::SharedRenderingData *> GetLights()              const { return lights_; }
    Span<const StaticMeshRenderProxy*>     GetStaticMeshRenderers() const { return staticMeshRendererProxies_; }

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
    std::vector<const StaticMeshRenderProxy *>    staticMeshRendererProxies_;
};

class Scene : public Uncopyable, public WithUniqueObjectID
{
public:

    Scene();
    
    Box<StaticMeshRenderObject> CreateStaticMeshRenderer();

    Box<Light> CreateLight();

    void PrepareRendering();

    Box<SceneProxy> CreateSceneProxy() const;

private:
    
    SceneObject rootNode_;

    LightManager              lightManager_;
    StaticMeshRendererManager staticRendererManager_;
};

RTRC_END
