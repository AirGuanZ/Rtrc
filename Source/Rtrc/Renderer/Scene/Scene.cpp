#include <Rtrc/Renderer/Scene/Scene.h>

RTRC_BEGIN

SceneProxy::~SceneProxy()
{
    for(auto *light : lights_)
    {
        (void)light->DecreaseCounter();
    }
}

Box<SceneProxy> Scene::CreateSceneProxy() const
{
    auto ret = MakeBox<SceneProxy>();
    staticRendererManager_.ForEachRenderer([&](const StaticMeshRenderer *renderer)
    {
        if(const RendererProxy *proxy = renderer->CreateProxy(ret->arena_))
        {
            ret->rendererProxies_.push_back(proxy);
        }
    });
    lightManager_.ForEachLight([&](const Light *light)
    {
        const Light::SharedRenderingData *renderingData = light->GetRenderingData().Get();
        renderingData->IncreaseCounter();
        ret->lights_.push_back(renderingData);
    });
    return ret;
}

RTRC_END
