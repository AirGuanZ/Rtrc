#pragma once

#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/ToolKit/Resource/ResourceManager.h>

RTRC_BEGIN

rtrc_struct(AtmosphereProperties)
{
    // Rayleigh

    float3 scatterRayleigh = 1e-6f * Vector3f(5.802f, 13.558f, 33.1f);
    float  hDensityRayleigh = 1e3f * 8;

    // Mie

    float scatterMie = 1e-6f * 3.996f;
    float asymmetryMie = 0.8f;
    float absorbMie = 1e-6f * 4.4f;
    float hDensityMie = 1e3f * 1.2f;

    // Ozone

    float3 absorbOzone = 1e-6f * Vector3f(0.65f, 1.881f, 0.085f);
    float  ozoneCenterHeight = 1e3f * 25;
    float  ozoneThickness = 1e3f * 30;

    // Geometry

    float planetRadius = 1e3f * 6360;
    float atmosphereRadius = 1e3f * 6460;

    float3 terrainAlbedo = { 0.3f, 0.3f, 0.3f };

    auto operator<=>(const AtmosphereProperties &) const = default;
};

void GenerateT(
    Ref<ResourceManager>        resources,
    const AtmosphereProperties &atmosphere,
    RG::TextureResource        *T);

void GenerateM(
    Ref<ResourceManager>        resources,
    const AtmosphereProperties &atmosphere,
    RG::TextureResource        *T,
    RG::TextureResource        *M,
    uint32_t                    distSamples = 24);

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
    int                         distSamples = 24);

RTRC_END
