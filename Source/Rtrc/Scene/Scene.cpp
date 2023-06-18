#include <Rtrc/Scene/Scene.h>

RTRC_BEGIN

std::atomic<int> proxyCount = 0;

SceneProxy::~SceneProxy()
{
    for(auto *light : lights_)
    {
        if(!light->DecreaseCounter())
        {
            delete light;
        }
    }
}

Scene::Scene()
    : sunDirection_(0, -1, 0)
    , sunColor_(1, 1, 1)
    , sunIntensity_(10)
{
    rootNode_.UpdateWorldMatrixRecursively(true);
}

Box<StaticMeshRenderObject> Scene::CreateStaticMeshRenderer()
{
    auto ret = staticRendererManager_.CreateStaticMeshRenderer();
    ret->SetParent(&rootNode_);
    return ret;
}

Box<Light> Scene::CreateLight()
{
    return lightManager_.CreateLight();
}

PhysicalAtmosphereProperties &Scene::GetAtmosphere()
{
    return atmosphere_;
}

const PhysicalAtmosphereProperties &Scene::GetAtmosphere() const
{
    return atmosphere_;
}

void Scene::PrepareRendering()
{
    rootNode_.UpdateWorldMatrixRecursively(false);
}

Box<SceneProxy> Scene::CreateSceneProxy() const
{
    auto ret = MakeBox<SceneProxy>();

    ret->atmosphere_   = atmosphere_;
    ret->sunDirection_ = Normalize(sunDirection_);
    ret->sunColor_     = sunColor_;
    ret->sunIntensity_ = sunIntensity_;

    staticRendererManager_.ForEachRenderer([&](const StaticMeshRenderObject *renderer)
    {
        if(const StaticMeshRenderProxy *proxy = renderer->CreateProxyRaw(ret->arena_))
        {
            ret->staticMeshRenderObjects_.push_back(proxy);
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
