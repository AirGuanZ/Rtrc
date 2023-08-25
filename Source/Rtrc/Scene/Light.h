#pragma once

#include <list>

#include <Rtrc/Scene/Hierarchy/SceneObject.h>
#include <Rtrc/Core/SmartPointer/CopyOnWritePtr.h>

RTRC_BEGIN

class LightManager;

namespace LightDetail
{

    enum class FlagBit
    {
        None                  = 0,
        EnableRayTracedShadow = 1 << 0,
    };
    RTRC_DEFINE_ENUM_FLAGS(FlagBit)
    using Flags = EnumFlagsFlagBit;

} // namespace LightDetail

class Light : public SceneObject
{
public:

    using Flags = LightDetail::Flags;

    enum class Type
    {
        Directional,
        Point
    };

    class SharedRenderingData : public ReferenceCounted
    {
        friend class Light;

        Type     type_      = Type::Directional;
        Vector3f color_     = { 1, 1, 1 };
        float    intensity_ = 1;

        Vector3f position_         = { 0, 0, 0 };  // Valid for point light
        Vector3f direction_        = { 0, -1, 0 }; // Valid for directional light

        float    distanceFadeBegin = 1; // Valid for point light
        float    distanceFadeEnd   = 2;
        
        float shadowSoftness_ = 0.0f;

        Flags flags_    = Flags::None;

    public:

        Type            GetType()              const { return type_; }
        const Vector3f &GetColor()             const { return color_; }
        float           GetIntensity()         const { return intensity_; }
        const Vector3f &GetPosition()          const { assert(type_ == Type::Point); return position_; }
        const Vector3f &GetDirection()         const { assert(type_ == Type::Directional); return direction_; }
        float           GetDistanceFadeBegin() const { assert(type_ == Type::Point); return distanceFadeBegin; }
        float           GetDistanceFadeEnd()   const { assert(type_ == Type::Point); return distanceFadeEnd; }
        Flags           GetFlags()             const { return flags_; }
        float           GetShadowSoftness()    const { return shadowSoftness_; }
    };

    ~Light() override;

    Type            GetType()              const { return data_->GetType(); }
    const Vector3f &GetColor()             const { return data_->GetColor(); }
    float           GetIntensity()         const { return data_->GetIntensity(); }
    const Vector3f &GetPosition()          const { return data_->GetPosition(); }
    const Vector3f &GetDirection()         const { return data_->GetDirection(); }
    float           GetDistanceFadeBegin() const { return data_->GetDistanceFadeBegin(); }
    float           GetDistanceFadeEnd()   const { return data_->GetDistanceFadeEnd(); }
    bool            GetFlags()             const { return data_->GetFlags(); }
    float           GetShadowSoftness()    const { return data_->GetShadowSoftness(); }

    void SetType(Type type);
    void SetColor(const Vector3f &color);
    void SetIntensity(float intensity);
    void SetPosition(const Vector3f &position);
    void SetDirection(const Vector3f &direction);
    void SetDistanceFadeBegin(float dist);
    void SetDistanceFadeEnd(float dist);
    void SetFlags(Flags flags);
    void SetShadowSoftness(float softness);

    ReferenceCountedPtr<SharedRenderingData> GetRenderingData() const { return data_; }
    SharedRenderingData                     *GetMutableRenderingData() { return data_.Unshare(); }

private:

    friend class LightManager;

    Light();

    LightManager                *manager_;
    std::list<Light *>::iterator iterator_;

    CopyOnWritePtr<SharedRenderingData> data_;
};

class LightManager : public Uncopyable
{
public:

    Box<Light> CreateLight();

    template<typename F>
    void ForEachLight(const F &func) const;

    void _internalRelease(Light *light);

private:

    std::list<Light *> lights_;
};

inline Light::~Light()
{
    assert(manager_);
    manager_->_internalRelease(this);
}

inline void Light::SetType(Type type)
{
    data_.Unshare()->type_ = type;
}

inline void Light::SetColor(const Vector3f &color)
{
    data_.Unshare()->color_ = color;
}

inline void Light::SetIntensity(float intensity)
{
    data_.Unshare()->intensity_ = intensity;
}

inline void Light::SetPosition(const Vector3f &position)
{
    assert(GetType() == Type::Point);
    data_.Unshare()->position_ = position;
}

inline void Light::SetDirection(const Vector3f &direction)
{
    assert(GetType() == Type::Directional);
    data_.Unshare()->direction_ = Normalize(direction);
}

inline void Light::SetDistanceFadeBegin(float dist)
{
    assert(GetType() == Type::Point);
    data_.Unshare()->distanceFadeBegin = dist;
}

inline void Light::SetDistanceFadeEnd(float dist)
{
    assert(GetType() == Type::Point);
    data_.Unshare()->distanceFadeEnd = dist;
}

inline void Light::SetFlags(Flags flags)
{
    data_.Unshare()->flags_ = flags;
}

inline void Light::SetShadowSoftness(float softness)
{
    data_.Unshare()->shadowSoftness_ = softness;
}

inline Light::Light()
    : manager_(nullptr)
{
    data_ = MakeReferenceCountedPtr<SharedRenderingData>();
}

inline Box<Light> LightManager::CreateLight()
{
    auto light = Box<Light>(new Light);
    lights_.push_front(light.get());
    light->manager_ = this;
    light->iterator_ = lights_.begin();
    return light;
}

template<typename F>
void LightManager::ForEachLight(const F &func) const
{
    for(Light *light : lights_)
    {
        func(light);
    }
}

inline void LightManager::_internalRelease(Light *light)
{
    assert(light && light->manager_ == this);
    lights_.erase(light->iterator_);
}

RTRC_END
