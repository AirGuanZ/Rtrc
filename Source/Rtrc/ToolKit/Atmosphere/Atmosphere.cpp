#include <Rtrc/ToolKit/Atmosphere/Atmosphere.h>

namespace Atmosphere
{
    using Properties = Rtrc::AtmosphereProperties;
}

#include <Rtrc/ToolKit/Atmosphere/Shader/Atmosphere.shader.outh>

RTRC_BEGIN

void GenerateT(
    const AtmosphereProperties &atmosphere,
    RG::TextureResource        *T)
{
    using Shader = RtrcShader::Builtin::Atmosphere::GenerateTransmittanceLut;

    Shader::Pass passData;
    passData.outputResolution                 = T->GetSize();
    passData.TransmittanceTextureRW           = T;
    passData.TransmittanceTextureRW.writeOnly = true;
    passData.atmosphere                       = atmosphere;
    T->GetGraph()->CreateComputePassWithThreadCount(
        Shader::Name,
        T->GetGraph()->GetDevice()->GetShader<Shader::Name>(),
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
    using Shader = RtrcShader::Builtin::Atmosphere::GenerateMultiScatterLut;

    Shader::Pass passData;
    passData.MultiScatterTextureRW           = M;
    passData.MultiScatterTextureRW.writeOnly = true;
    passData.RawDirSamples                   = resources->GetPoissonDiskSamples2048();
    passData.TransmittanceLutTexture         = T;
    passData.outputResolution                = M->GetSize();
    passData.dirSampleCount                  = 2048;
    passData.rayMarchStepCount               = distSamples;
    passData.atmosphere                      = atmosphere;

    M->GetGraph()->CreateComputePassWithThreadCount(
        Shader::Name,
        T->GetGraph()->GetDevice()->GetShader<Shader::Name>(),
        M->GetSize(),
        passData);
}

void GenerateS(
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
    using Shader = RtrcShader::Builtin::Atmosphere::GenerateSkyLut;

    Shader::Pass skyPassData;
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
        Shader::Name,
        T->GetGraph()->GetDevice()->GetShader<Shader::Name>(),
        S->GetSize(),
        skyPassData);
}

RTRC_END
