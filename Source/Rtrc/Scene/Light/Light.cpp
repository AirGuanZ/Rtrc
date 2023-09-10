#include <Rtrc/Scene/Light/LightManager.h>

RTRC_BEGIN

Light::~Light()
{
    assert(manager_);
    manager_->_internalRelease(this);
}

RTRC_END
