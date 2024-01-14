#include <Rtrc/ToolKit/Atmosphere/Atmosphere.h>

#include <Rtrc/ToolKit/Atmosphere/Shader/Atmosphere.shader.outh>

RTRC_BEGIN

void GenerateT(
    Ref<ResourceManager>        resources,
    const AtmosphereProperties &atmosphere,
    RG::TextureResource        *T)
{
    StaticShaderInfo<"Atmosphere/GenerateTransmittanceLut">::Pass passData;
    passData.outputResolution                 = T->GetSize();
    passData.TransmittanceTextureRW           = T;
    passData.TransmittanceTextureRW.writeOnly = true;
    passData.atmosphere                       = atmosphere;
    T->GetGraph()->CreateComputePassWithThreadCount(
        "GenerateTransmittanceLut",
        resources->GetStaticShader<"Atmosphere/GenerateTransmittanceLut">(),
        T->GetSize(),
        passData);
}

void GenerateM(
    Ref<ResourceManager>        resources,
    const AtmosphereProperties &atmosphere,
    RG::TextureResource        *T,
    RG::TextureResource        *M,
    uint32_t                    distSamples)
{
    StaticShaderInfo<"Atmosphere/GenerateMultiScatterLut">::Pass passData;
    passData.MultiScatterTextureRW           = M;
    passData.MultiScatterTextureRW.writeOnly = true;
    passData.RawDirSamples                   = resources->GetPoissonDiskSamples2048();
    passData.TransmittanceLutTexture         = T;
    passData.outputResolution                = M->GetSize();
    passData.dirSampleCount                  = 2048;
    passData.rayMarchStepCount               = distSamples;
    passData.atmosphere                      = atmosphere;

    M->GetGraph()->CreateComputePassWithThreadCount(
        "GenerateMultiScatteringLut",
        resources->GetStaticShader<"Atmosphere/GenerateMultiScatterLut">(),
        M->GetSize(),
        passData);
}

void GenerateS(
    Ref<ResourceManager>        resources,
    const AtmosphereProperties &atmosphere,
    RG::TextureResource        *T,
    RG::TextureResource        *M,
    RG::TextureResource        *S,
    const Vector3f&             sunDir,
    const Vector3f&             sunColor,
    float                       eyeAltitude,
    float                       lerpFactor,
    float                       randomNumber01,
    int                         distSamples)
{
    StaticShaderInfo<"Atmosphere/GenerateSkyLut">::Pass skyPassData;
    skyPassData.SkyLutTextureRW           = S;
    skyPassData.SkyLutTextureRW.writeOnly = true;
    skyPassData.TransmittanceLutTexture   = T;
    skyPassData.MultiScatterLutTexture    = M;
    skyPassData.atmosphere                = atmosphere;
    skyPassData.outputResolution          = S->GetSize();
    skyPassData.rayMarchStepCount         = distSamples;
    skyPassData.frameRandom01             = randomNumber01;
    skyPassData.eyePosY                   = eyeAltitude;
    skyPassData.lerpFactor                = lerpFactor;
    skyPassData.sunDirection              = sunDir;
    skyPassData.sunIntensity              = sunColor;

    S->GetGraph()->CreateComputePassWithThreadCount(
        "GenerateSkyLut",
        resources->GetStaticShader<"Atmosphere/GenerateSkyLut">(),
        S->GetSize(),
        skyPassData);
}

RTRC_END
