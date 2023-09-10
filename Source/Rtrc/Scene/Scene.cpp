#include <Rtrc/Renderer/GPUScene/RenderScene.h>
#include <Rtrc/Scene/Scene.h>

RTRC_BEGIN

Scene::Scene()
{
    root_.UpdateWorldMatrixRecursively(true);
}

Scene::~Scene()
{
    onDestroy_.Emit();
}

void Scene::PrepareRender()
{
    root_.UpdateWorldMatrixRecursively(false);
}

RTRC_END
