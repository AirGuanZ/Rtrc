#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Renderer/BuiltinResources.h>
#include <Rtrc/Renderer/Camera/Camera.h>

RTRC_BEGIN

namespace AtmosphereDetail
{
    
    rtrc_struct(AtmosphereProperties)
    {
        // Rayleigh

        rtrc_var(Vector3f, scatterRayleigh) = 1e-6f * Vector3f(5.802f, 13.558f, 33.1f);
        rtrc_var(float, hDensityRayleigh)   = 1e3f * 8;

        // Mie

        rtrc_var(float, scatterMie)    = 1e-6f * 3.996f;
        rtrc_var(float, assymmetryMie) = 0.8f;
        rtrc_var(float, absorbMie)     = 1e-6f * 4.4f;
        rtrc_var(float, hDensityMie)   = 1e3f * 1.2f;

        // Ozone

        rtrc_var(Vector3f, absorbOzone)    = 1e-6f * Vector3f(0.65f, 1.881f, 0.085f);
        rtrc_var(float, ozoneCenterHeight) = 1e3f * 25;
        rtrc_var(float, ozoneThickness)    = 1e3f * 30;

        // Geometry

        rtrc_var(float, planetRadius)      = 1e3f * 6360;
        rtrc_var(float, atmosphereRadius)  = 1e3f * 6460;
    };

    class TransmittanceLut
    {
    public:

        TransmittanceLut(
            const BuiltinResourceManager &builtinResources,
            const AtmosphereProperties   &properties,
            const Vector2i               &resolution);

        const TextureSRV &GetLut() const { return srv_; }

    private:

        TextureSRV srv_;
    };

    class MultiScatterLut
    {
    public:

        static constexpr int DIR_SAMPLE_COUNT = 64;
        static constexpr int RAY_MARCH_STEP_COUNT = 256;

        MultiScatterLut(
            const BuiltinResourceManager &builtinResources,
            const TransmittanceLut       &transmittanceLut,
            const AtmosphereProperties   &properties,
            const Vector2i               &resolution);

        const TextureSRV &GetLut() const { return srv_; }

    private:

        TextureSRV srv_;
    };

    class SkyLut
    {
    public:

        struct RenderGraphInterface
        {
            RG::Pass *inPass = nullptr;
            RG::Pass *outPass = nullptr;
            RG::TextureResource *skyLut = nullptr;
        };

        explicit SkyLut(const BuiltinResourceManager &builtinResources);

        void SetRayMarchingStepCount(int stepCount);
        void SetOutputResolution(const Vector2i &res);
        void SetCamera(const Vector3f &eyePos);
        void SetAtomsphere(const AtmosphereProperties *properties);
        void SetSun(const Vector3f &direction, const Vector3f &intensity);

        RenderGraphInterface AddToRenderGraph(
            RG::RenderGraph              *renderGraph,
            const TransmittanceLut       &transmittanceLut,
            const MultiScatterLut        &multiScatterLut) const;

    private:

        Device &device_;
        RC<Shader> shader_;

        int stepCount_ = 32;
        Vector2i lutRes_;
        Vector3f eyePos_;
        const AtmosphereProperties *properties_;
        Vector3f direction_;
        Vector3f intensity_;
    };

    class AtmosphereRenderer : public Uncopyable
    {
    public:

        using Properties = AtmosphereProperties;

        struct RenderGraphInterface
        {
            RG::Pass *inPass = nullptr;
            RG::Pass *outPass = nullptr;
            RG::TextureResource *skyLUT = nullptr;
            RG::TextureResource *aerialLUT = nullptr;
        };

        explicit AtmosphereRenderer(Device &device);

        RenderGraphInterface AddToRenderGraph(RG::RenderGraph &graph, const Camera &camera);

    private:

        float sunAngleX_;
        float sunAngleY_;
        float sunIntensity_;
        Vector3f sunColor_;

        Vector2i transLutRes_;
        Vector2i msLutRes_;
        Vector2i skyLutRes_;
        Vector2i aerialLutRes_;
    };

} // namespace AtmosphereDetail

using AtmosphereRenderer = AtmosphereDetail::AtmosphereRenderer;

RTRC_END
