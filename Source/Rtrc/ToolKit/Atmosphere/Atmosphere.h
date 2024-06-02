#pragma once

#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/ToolKit/Resource/ResourceManager.h>

RTRC_BEGIN

rtrc_struct(AtmosphereProperties)
{
    rtrc_struct_name(AtmosphereProperties)()
    {
        scatterRayleigh  = 1e-6f * float3(5.802f, 13.558f, 33.1f);
        hDensityRayleigh = 1e3f * 8;

        scatterMie   = 1e-6f * 3.996f;
        asymmetryMie = 0.8f;
        absorbMie    = 1e-6f * 4.4f;
        hDensityMie  = 1e3f * 1.2f;

        absorbOzone       = 1e-6f * float3(0.65f, 1.881f, 0.085f);
        ozoneCenterHeight = 1e3f * 25;
        ozoneThickness    = 1e3f * 30;

        planetRadius     = 1e3f * 6360;
        atmosphereRadius = 1e3f * 6460;

        terrainAlbedo = { 0.3f, 0.3f, 0.3f };
    }

    // Rayleigh

    rtrc_var(float3, scatterRayleigh);
    rtrc_var(float, hDensityRayleigh);

    // Mie

    rtrc_var(float, scatterMie);
    rtrc_var(float, asymmetryMie);
    rtrc_var(float, absorbMie);
    rtrc_var(float, hDensityMie);

    // Ozone

    rtrc_var(float3, absorbOzone);
    rtrc_var(float, ozoneCenterHeight);
    rtrc_var(float, ozoneThickness);

    // Geometry

    rtrc_var(float, planetRadius);
    rtrc_var(float, atmosphereRadius);

    rtrc_var(float3, terrainAlbedo);

    auto operator<=>(const rtrc_struct_name(AtmosphereProperties) &) const = default;
};

void GenerateT(
    const AtmosphereProperties &atmosphere,
    RGTexture                   T);

void GenerateM(
    Ref<ResourceManager>        resources,
    const AtmosphereProperties &atmosphere,
    RGTexture                   T,
    RGTexture                   M,
    uint32_t                    distSamples = 24);

void GenerateS(
    const AtmosphereProperties &atmosphere,
    RGTexture                   T,
    RGTexture                   M,
    RGTexture                   S,
    const Vector3f&             sunDir,
    const Vector3f&             sunColor,
    float                       eyeAltitude,
    float                       lerpFactor,
    float                       randomNumber01,
    int                         distSamples = 24);

RTRC_END
