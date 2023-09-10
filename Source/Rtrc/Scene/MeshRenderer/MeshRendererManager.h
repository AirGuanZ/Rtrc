#pragma once

#include <Rtrc/Scene/MeshRenderer/MeshRenderer.h>

RTRC_BEGIN

class MeshRendererManager : public Uncopyable
{
public:

    Box<MeshRenderer> CreateMeshRenderer();

    template<typename F>
    void ForEachMeshRenderer(const F &func);
    template<typename F>
    void ForEachMeshRenderer(const F &func) const;

    void _internalRelease(MeshRenderer *renderer);

private:

    // IMPROVE: use intrusive list
    std::list<MeshRenderer *> renderers_;
};

template<typename F>
void MeshRendererManager::ForEachMeshRenderer(const F &func)
{
    for(auto renderer : renderers_)
    {
        func(renderer);
    }
}

template<typename F>
void MeshRendererManager::ForEachMeshRenderer(const F &func) const
{
    for(const MeshRenderer* renderer : renderers_)
    {
        func(renderer);
    }
}

RTRC_END
