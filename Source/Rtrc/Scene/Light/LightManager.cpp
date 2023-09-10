#include <Rtrc/Scene/Light/LightManager.h>

RTRC_BEGIN

Box<Light> LightManager::CreateLight()
{
    auto light = Box<Light>(new Light);
    lights_.push_front(light.get());
    light->manager_ = this;
    light->iterator_ = lights_.begin();
    return light;
}

void LightManager::_internalRelease(Light *light)
{
    assert(light && light->manager_ == this);
    lights_.erase(light->iterator_);
}

RTRC_END
