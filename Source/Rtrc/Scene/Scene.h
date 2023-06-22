#pragma once

#include <Rtrc/Scene/Atmosphere.h>
#include <Rtrc/Scene/Light.h>
#include <Rtrc/Scene/Renderer/StaticMeshRenderObject.h>

RTRC_BEGIN

class SceneProxy : public Uncopyable
{
public:

    ~SceneProxy();

    const PhysicalAtmosphereProperties &GetAtmosphere() const { return atmosphere_; }
    const Vector3f &GetSunDirection() const { return sunDirection_; }
    const Vector3f &GetSunColor() const { return sunColor_; }
    float GetSunIntensity() const { return sunIntensity_; }

    Span<const Light::SharedRenderingData *> GetLights() const { return lights_; }
    Span<const StaticMeshRenderProxy*> GetStaticMeshRenderObjects() const { return staticMeshRenderObjects_; }

    UniqueId GetOriginalSceneID() const { return originanSceneID_; }

private:

    friend class Scene;

    enum VersionType
    {
        VersionType_UpdateStaticMeshTransform,
        VersionType_AddOrRemoveStaticMesh,
        VersionType_Count
    };

    LinearAllocator arena_;

    PhysicalAtmosphereProperties atmosphere_;
    Vector3f                     sunDirection_;
    Vector3f                     sunColor_;
    float                        sunIntensity_;
    
    std::vector<const Light::SharedRenderingData *> lights_;
    std::vector<const StaticMeshRenderProxy *>      staticMeshRenderObjects_;

    UniqueId originanSceneID_;
};

class Scene : public Uncopyable, public WithUniqueObjectID
{
public:

    Scene();
    
    Box<StaticMeshRenderObject> CreateStaticMeshRenderer();

    Box<Light> CreateLight();

          PhysicalAtmosphereProperties &GetAtmosphere();
    const PhysicalAtmosphereProperties &GetAtmosphere() const;

    void SetAtmosphereSunDirection(const Vector3f &direction) { sunDirection_ = Normalize(direction); }
    void SetAtmosphereSunColor(const Vector3f &color) { sunColor_ = color; }
    void SetAtmosphereSunIntensity(float intensity) { sunIntensity_ = intensity; }

    const Vector3f GetAtmosphereSunDirection() const { return sunDirection_; }
    const Vector3f GetAtmosphereSunColor() const { return sunColor_; }
    float GetAtmosphereSunIntensity() const { return sunIntensity_; }

    void PrepareRendering();

    Box<SceneProxy> CreateSceneProxy() const;

private:
    
    SceneObject rootNode_;

    PhysicalAtmosphereProperties atmosphere_;
    Vector3f                     sunDirection_;
    Vector3f                     sunColor_;
    float                        sunIntensity_;

    LightManager              lightManager_;
    StaticMeshRendererManager staticRendererManager_;
};

RTRC_END
