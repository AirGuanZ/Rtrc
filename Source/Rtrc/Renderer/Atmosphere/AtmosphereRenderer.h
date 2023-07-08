#pragma once

#include <random>

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

// Per-scene rendering data
class RenderAtmosphere : public RenderAlgorithm
{
public:

    struct PerCameraData
    {
        RC<StatefulTexture> persistentS;
        RG::TextureResource *S = nullptr;
        std::default_random_engine rng{ 42 };
    };

    using RenderAlgorithm::RenderAlgorithm;

    void SetLutTResolution(const Vector2u &res);
    void SetLutMResolution(const Vector2u &res);
    void SetLutSResolution(const Vector2u &res);
    void SetRayMarchingStepCount(int stepCount);

    void Update(RG::RenderGraph &renderGraph, const AtmosphereProperties &properties);
    void ClearFrameData();

    void Render(
        RG::RenderGraph &renderGraph,
        const Vector3f  &sunDir,
        const Vector3f  &sunColor,
        float            eyeAltitude,
        float            dt,
        PerCameraData   &perCameraData) const;
    void ClearPerCameraFrameData(PerCameraData &perCameraData) const;

private:
    
    enum BuiltinMaterialPass
    {
        Pass_GenerateT,
        Pass_GenerateM,
        Pass_GenerateS
    };

    RG::TextureResource *GenerateT(RG::RenderGraph &renderGraph, const AtmosphereProperties &properties);
    RG::TextureResource *GenerateM(RG::RenderGraph &renderGraph, const AtmosphereProperties &properties);
    
    Vector2u resT_ = { 256, 256 };
    Vector2u resM_ = { 256, 256 };
    Vector2u resS_ = { 256, 128 };
    int rayMarchingStepCount_ = 24;

    AtmosphereProperties properties_;
    RC<StatefulTexture> persistentT_;
    RC<StatefulTexture> persistentM_;
    RG::TextureResource *T_ = nullptr;
    RG::TextureResource *M_ = nullptr;

    static constexpr int MULTI_SCATTER_DIR_SAMPLE_COUNT = 2048;
    RC<Buffer> multiScatterDirSamples_;
};

RTRC_RENDERER_END
