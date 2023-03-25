#pragma once

#include <random>

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Scene/Camera/Camera.h>

RTRC_RENDERER_BEGIN

namespace AtmosphereDetail
{
    
    rtrc_struct(AtmosphereProperties)
    {
        // Rayleigh

        rtrc_var(float3, scatterRayleigh)  = 1e-6f * Vector3f(5.802f, 13.558f, 33.1f);
        rtrc_var(float,  hDensityRayleigh) = 1e3f * 8;

        // Mie

        rtrc_var(float, scatterMie)    = 1e-6f * 3.996f;
        rtrc_var(float, assymmetryMie) = 0.8f;
        rtrc_var(float, absorbMie)     = 1e-6f * 4.4f;
        rtrc_var(float, hDensityMie)   = 1e3f * 1.2f;

        // Ozone

        rtrc_var(float3, absorbOzone)       = 1e-6f * Vector3f(0.65f, 1.881f, 0.085f);
        rtrc_var(float,  ozoneCenterHeight) = 1e3f * 25;
        rtrc_var(float,  ozoneThickness)    = 1e3f * 30;

        // Geometry

        rtrc_var(float, planetRadius)      = 1e3f * 6360;
        rtrc_var(float, atmosphereRadius)  = 1e3f * 6460;

        rtrc_var(float3, terrainAlbedo) = { 0.3f, 0.3f, 0.3f };

        auto operator<=>(const AtmosphereProperties &) const = default;
    };

    struct AtmosphereFrameParameters
    {
        AtmosphereProperties atmosphere;
        Vector3f             eyePosition;
        Vector3f             sunDirection;
        Vector3f             sunColor;
        float                sunIntensity;
        float                dt;
    };

    class TransmittanceLut
    {
    public:

        TransmittanceLut(
            const BuiltinResourceManager &builtinResources,
            const AtmosphereProperties   &properties,
            const Vector2i               &resolution);

        const TextureSrv &GetLut() const { return srv_; }

    private:

        TextureSrv srv_;
    };

    class MultiScatterLut
    {
    public:

        static constexpr int DIR_SAMPLE_COUNT = 2048;
        static constexpr int RAY_MARCH_STEP_COUNT = 64;

        MultiScatterLut(
            const BuiltinResourceManager &builtinResources,
            const TransmittanceLut       &transmittanceLut,
            const AtmosphereProperties   &properties,
            const Vector2i               &resolution);

        const TextureSrv &GetLut() const { return srv_; }

    private:

        TextureSrv srv_;
    };

    class SkyLut
    {
    public:

        struct RenderGraphInterface
        {
            RG::Pass            *inPass  = nullptr;
            RG::Pass            *outPass = nullptr;
            RG::TextureResource *skyLut  = nullptr;
        };

        explicit SkyLut(const BuiltinResourceManager &builtinResources);

        void SetRayMarchingStepCount(int stepCount);
        void SetOutputResolution(const Vector2i &res);

        // 'parameters' must be valid until renderGraph is executed
        RenderGraphInterface AddToRenderGraph(
            const AtmosphereFrameParameters *parameters,
            RG::RenderGraph                 *renderGraph,
            const TransmittanceLut          &transmittanceLut,
            const MultiScatterLut           &multiScatterLut);
        
    private:

        struct PrepareLutRGInterface
        {
            RG::Pass            *clearPass;
            RG::TextureResource *lut;
        };

        PrepareLutRGInterface PrepareLut(RG::RenderGraph *renderGraph, RC<StatefulTexture> &lut);

        ObserverPtr<Device> device_;
        RC<Shader>          shader_;

        std::default_random_engine randomEngine_;
        int                        stepCount_;
        Vector2i                   lutRes_;

        RC<StatefulTexture> prevLut_;
        RC<StatefulTexture> currLut_;
    };

    class AtmosphereRenderer : public Uncopyable
    {
    public:
        
        struct RenderGraphInterface
        {
            RG::Pass            *passIn  = nullptr;
            RG::Pass            *passOut = nullptr;
            RG::TextureResource *skyLut  = nullptr;
        };

        explicit AtmosphereRenderer(const BuiltinResourceManager &builtinResources);

        void SetSunDirection(const Vector3f &normalizedSunToScene);
        void SetSunIntensity(float intensity);
        void SetSunColor(const Vector3f &color);
        void SetProperties(const AtmosphereProperties &properties);

        void SetYOffset(float offset);

        void SetTransmittanceLutResolution(const Vector2i &res);
        void SetMultiScatterLutResolution(const Vector2i &res);
        void SetSkyLutResolution(const Vector2i &res);

        RenderGraphInterface AddToRenderGraph(RG::RenderGraph &graph, const Camera &camera, float dt);

    private:

        ObserverPtr<Device>           device_;
        const BuiltinResourceManager &builtinResources_;

        AtmosphereFrameParameters frameParameters_;
        float                     yOffset_;

        Vector2i transLutRes_;
        Vector2i msLutRes_;
        
        Box<TransmittanceLut> transLut_;
        Box<MultiScatterLut>  msLut_;
        Box<SkyLut>           skyLut_;
    };

} // namespace AtmosphereDetail

using AtmosphereRenderer = AtmosphereDetail::AtmosphereRenderer;

RTRC_RENDERER_END
