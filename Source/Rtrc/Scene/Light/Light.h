#pragma once

#include <list>

#include <Core/EnumFlags.h>
#include <Rtrc/Scene/SceneObject.h>

RTRC_BEGIN

class LightManager;

enum class LightFlagBit : uint32_t
{
    None            = 0,
    RayTracedShadow = 1 << 0,
};
RTRC_DEFINE_ENUM_FLAGS(LightFlagBit)

class Light : public SceneObject
{
public:

    using Flags = EnumFlagsLightFlagBit;

    enum class Type
    {
        Directional,
        Point,
        Spot,
    };

    ~Light() override;

    RTRC_SET_GET(Type,     Type,      type_)
    RTRC_SET_GET(Vector3f, Color,     color_)
    RTRC_SET_GET(float,    Intensity, intensity_)
    RTRC_SET_GET(float,    Softness,  softness_)
    RTRC_SET_GET(Flags,    Flags,     flags_)

    RTRC_SET_GET(Vector3f, Position,   position_)  // For point/spot light
    RTRC_SET_GET(Vector3f, Direction,  direction_) // For spot/directional light

    RTRC_SET_GET(float, DistFadeBegin, distFadeBegin_)
    RTRC_SET_GET(float, DistFadeEnd,   distFadeEnd_)
    RTRC_SET_GET(float, ConeFadeBegin, coneFadeBegin) // In angle, \in [0, 90)
    RTRC_SET_GET(float, ConeFadeEnd,   coneFadeEnd_)

private:

    friend class LightManager;

    Light() = default;

    LightManager               *manager_ = nullptr;
    std::list<Light*>::iterator iterator_;

    Type     type_           = Type::Directional;
    Vector3f color_          = { 1, 1, 1 };
    float    intensity_      = 1;
    float    softness_       = 0;
    Flags    flags_          = LightFlagBit::None;

    Vector3f position_;
    Vector3f direction_ = { 0, -1, 0 };
    float    distFadeBegin_ = 2;
    float    distFadeEnd_   = 3;
    float    coneFadeBegin  = 30;
    float    coneFadeEnd_   = 40;
};

RTRC_END
