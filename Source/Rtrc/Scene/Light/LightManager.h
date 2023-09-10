#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Scene/Light/Light.h>

RTRC_BEGIN

class LightManager : public Uncopyable
{
public:

    // ============= Management =============
    
    Box<Light> CreateLight();

    template<typename F>
    void ForEachLight(const F &f);
    template<typename F>
    void ForEachLight(const F &f) const;

    void _internalRelease(Light *light);

private:

    std::list<Light *> lights_;
};

template<typename F>
void LightManager::ForEachLight(const F &f)
{
    for(auto light : lights_)
    {
        f(light);
    }
}

template<typename F>
void LightManager::ForEachLight(const F &f) const
{
    for(const Light* light: lights_)
    {
        f(light);
    }
}

RTRC_END
