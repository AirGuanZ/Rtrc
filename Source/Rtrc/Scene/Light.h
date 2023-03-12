#pragma once

#include <Rtrc/Graphics/Device/BindingGroupDSL.h>
#include <Rtrc/Scene/Hierarchy/SceneObject.h>
#include <Rtrc/Utility/SmartPointer/CopyOnWritePtr.h>

RTRC_BEGIN

class LightManager;

class Light : public SceneObject
{
public:

    enum class Type
    {
        Directional
    };

    class SharedRenderingData : public ReferenceCounted
    {
        friend class Light;

        Type     type_      = Type::Directional;
        Vector3f color_     = { 1, 1, 1 };
        float    intensity_ = 1;
        Vector3f direction_ = { 0, -1, 0 };

    public:

        Type            GetType() const { return type_; }
        const Vector3f &GetColor() const { return color_; }
        float           GetIntensity() const { return intensity_; }
        const Vector3f &GetDirection() const { assert(type_ == Type::Directional); return direction_; }
    };

    ~Light() override;

    Type            GetType() const { return data_->GetType(); }
    const Vector3f &GetColor() const { return data_->GetColor(); }
    float           GetIntensity() const { return data_->GetIntensity(); }
    const Vector3f &GetDirection() const { return data_->GetDirection(); }

    void SetType(Type type);
    void SetColor(const Vector3f &color);
    void SetIntensity(float intensity);
    void SetDirection(const Vector3f &direction);

    const ReferenceCountedPtr<SharedRenderingData> &GetRenderingData() const { return data_; }
    SharedRenderingData                            *GetMutableRenderingData() { return data_.Unshare(); }

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

inline void Light::SetDirection(const Vector3f &direction)
{
    assert(GetType() == Type::Directional);
    data_.Unshare()->direction_ = direction;
}

inline Light::Light()
    : manager_(nullptr)
{
    data_.Reset(new SharedRenderingData);
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
