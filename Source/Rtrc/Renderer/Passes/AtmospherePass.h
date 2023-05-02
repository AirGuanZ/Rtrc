#pragma once

#include <random>

#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_RENDERER_BEGIN

class PhysicalAtmospherePass : public Uncopyable
{
public:

    struct RenderGraphOutput
    {
        RG::Pass *passS = nullptr; // Building sky lut
        RG::TextureResource *skyLut = nullptr;
    };

    struct SunParameters
    {
        Vector3f direction;
        Vector3f color;
    };

    struct CachedData
    {
        RC<StatefulTexture> skyLut;
    };

    PhysicalAtmospherePass(ObserverPtr<Device> device, ObserverPtr<const BuiltinResourceManager> builtinResources);

    const PhysicalAtmosphereProperties &GetProperties() const;
    void SetProperties(const PhysicalAtmosphereProperties &properties);

    void SetTransmittanceLutResolution(const Vector2u &res);
    void SetMultiScatteringLutResolution(const Vector2u &res);

    void SetSkyLutResolution(const Vector2u &res);
    void SetRayMarchingStepCount(int stepCount);

    RenderGraphOutput Render(
        RG::RenderGraph    &graph,
        const Vector3f     &sunDirection,
        const Vector3f     &sunColor,
        float               eyeAltitude,
        float               dt,
        CachedData &perSceneData);

private:

    enum AtmosphereMaterialPass
    {
        AMP_GenerateTransmittanceLut,
        AMP_GenerateMultiScatteringLut,
        AMP_GenerateSkyLut
    };

    static constexpr int MS_DIR_SAMPLE_COUNT = 2048;

    RG::TextureResource *GetTransmittanceLut(RG::RenderGraph &graph);
    RG::TextureResource *GetMultiScatteringLut(RG::RenderGraph &graph, RG::TextureResource *T);

    RG::TextureResource *InitializeSkyLut(
        RG::RenderGraph     &graph,
        RC<StatefulTexture> &tex,
        bool                 clearWhenCreated) const;

    std::default_random_engine rng_;

    ObserverPtr<Device>                       device_;
    ObserverPtr<const BuiltinResourceManager> builtinResources_;

    RC<BindingGroupLayout> skyPassBindingGroupLayout_;
    RC<BindingGroupLayout> transmittancePassBindingGroupLayout_;
    RC<BindingGroupLayout> multiScatteringPassBindingGroupLayout_;

    PhysicalAtmosphereProperties properties_;

    RC<Shader> transmittanceLutShader_;
    RC<Shader> multiScatteringLutShader_;
    RC<Shader> skyLutShader_;

    Vector2u transmittanceLutRes_;
    Vector2u multiScatteringLutRes_;

    Vector2u skyLutRes_;
    int rayMarchingStepCount_;

    RC<StatefulTexture> transmittanceLut_;
    RC<StatefulTexture> multiScatteringLut_;
};

RTRC_RENDERER_END
