#pragma once

#include <list>

#include <Rtrc/Scene/SceneObject.h>
#include <Core/EnumFlags.h>

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
        Point
    };

    ~Light() override;

    RTRC_SET_GET(Type,     Type,           type_)
    RTRC_SET_GET(Vector3f, Color,          color_)
    RTRC_SET_GET(float,    Intensity,      intensity_)
    RTRC_SET_GET(float,    ShadowSoftness, shadowSoftness_)
    RTRC_SET_GET(Flags,    Flags,          flags_)

    RTRC_SET_GET(Vector3f, Position,          pointLightData_.position)
    RTRC_SET_GET(float,    DistanceFadeBegin, pointLightData_.distanceFadeBegin)
    RTRC_SET_GET(float,    DistanceFadeEnd,   pointLightData_.distanceFadeEnd)

    RTRC_SET_GET(Vector3f, Direction, directionalLightData_.direction)

private:

    friend class LightManager;

    struct PointLightData
    {
        Vector3f position;
        float distanceFadeBegin = 1;
        float distanceFadeEnd = 2;
    };

    struct DirectionalLightData
    {
        Vector3f direction;
    };

    Light() = default;

    LightManager               *manager_ = nullptr;
    std::list<Light*>::iterator iterator_;

    Type     type_           = Type::Directional;
    Vector3f color_          = { 1, 1, 1 };
    float    intensity_      = 1;
    float    shadowSoftness_ = 0;
    Flags    flags_          = LightFlagBit::None;

    union
    {
        DirectionalLightData directionalLightData_ = {};
        PointLightData       pointLightData_;
    };
};

RTRC_END
