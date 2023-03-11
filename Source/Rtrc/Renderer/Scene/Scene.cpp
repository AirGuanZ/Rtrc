#include <Rtrc/Renderer/Scene/Scene.h>

RTRC_BEGIN

SceneProxy::~SceneProxy()
{
    for(auto *light : lights_)
    {
        (void)light->DecreaseCounter();
    }
}

Scene::Scene()
{
    rootNode_.UpdateWorldMatrixRecursively(true);
}

Box<StaticMeshRenderer> Scene::CreateStaticMeshRenderer()
{
    auto ret = staticRendererManager_.CreateStaticMeshRenderer();
    ret->SetParent(&rootNode_);
    return ret;
}

Box<Light> Scene::CreateLight()
{
    return lightManager_.CreateLight();
}

void Scene::PrepareRendering()
{
    rootNode_.UpdateWorldMatrixRecursively(false);
}

Box<SceneProxy> Scene::CreateSceneProxy() const
{
    auto ret = MakeBox<SceneProxy>();

    staticRendererManager_.ForEachRenderer([&](const StaticMeshRenderer *renderer)
    {
        if(const StaticMeshRendererProxy *proxy = renderer->CreateProxyRaw(ret->arena_))
        {
            ret->staticMeshRendererProxies_.push_back(proxy);
        }
    });
    std::ranges::copy(ret->staticMeshRendererProxies_, std::back_inserter(ret->renderers_));

    lightManager_.ForEachLight([&](const Light *light)
    {
        const Light::SharedRenderingData *renderingData = light->GetRenderingData().Get();
        renderingData->IncreaseCounter();
        ret->lights_.push_back(renderingData);
    });

    return ret;
}

RTRC_END
