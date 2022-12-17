#pragma once

#include <Rtrc/Renderer/Scene/SceneObject.h>
#include <Rtrc/Utility/Variant.h>

RTRC_BEGIN

// See Asset/Builtin/Material/Common/Scene.hlsl
rtrc_struct(DirectionalLightConstantBuffer)
{
    rtrc_var(Vector3f, direction) = Vector3f(0, -1, 0);
    rtrc_var(Vector3f, color)     = Vector3f(1, 1, 1);
    rtrc_var(float,    intensity) = 1;
};

class Light : public SceneObject
{
public:

    enum class Type
    {
        Directional
    };

    struct DirectionalData
    {
        Vector3f direction = { 0, -1, 0 };
    };

    Light();

    Type GetType() const;
    void SetType(Type type);

    const Vector3f &GetColor() const;
    void SetColor(const Vector3f &color);

    float GetIntensity() const;
    void SetIntensity(float intensity);

    const DirectionalData &GetDirectionalData() const;
          DirectionalData &GetDirectionalData();

private:

    Vector3f color_;
    float intensity_;
    Variant<DirectionalData> data_;
};

inline Light::Light()
    : color_(1, 1, 1), intensity_(1), data_{ DirectionalData{ } }
{
    
}

inline Light::Type Light::GetType() const
{
    return data_.Match(
        [](const DirectionalData &) { return Type::Directional; });
}

inline void Light::SetType(Type type)
{
    switch(type)
    {
    case Type::Directional:
        data_ = DirectionalData{};
        break;
    }
}

inline const Vector3f &Light::GetColor() const
{
    return color_;
}

inline void Light::SetColor(const Vector3f &color)
{
    color_ = color;
}

inline float Light::GetIntensity() const
{
    return intensity_;
}

inline void Light::SetIntensity(float intensity)
{
    intensity_ = intensity;
}

inline const Light::DirectionalData &Light::GetDirectionalData() const
{
    return data_.As<DirectionalData>();
}

inline Light::DirectionalData &Light::GetDirectionalData()
{
    return data_.As<DirectionalData>();
}

RTRC_END
