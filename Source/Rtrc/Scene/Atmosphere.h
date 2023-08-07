#pragma once

#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

rtrc_refl_struct(AtmosphereProperties, shader)
{
    // Rayleigh

    float3 scatterRayleigh;
    float  hDensityRayleigh;

    // Mie

    float scatterMie;
    float asymmetryMie;
    float absorbMie;
    float hDensityMie;

    // Ozone

    float3 absorbOzone;
    float  ozoneCenterHeight;
    float  ozoneThickness;

    // Geometry

    float planetRadius;
    float atmosphereRadius;

    float3 terrainAlbedo;
    
    AtmosphereProperties()
    {
        scatterRayleigh  = 1e-6f * Vector3f(5.802f, 13.558f, 33.1f);
        hDensityRayleigh = 1e3f * 8;

        scatterMie   = 1e-6f * 3.996f;
        asymmetryMie = 0.8f;
        absorbMie    = 1e-6f * 4.4f;
        hDensityMie  = 1e3f * 1.2f;

        absorbOzone       = 1e-6f * Vector3f(0.65f, 1.881f, 0.085f);
        ozoneCenterHeight = 1e3f * 25;
        ozoneThickness    = 1e3f * 30;

        planetRadius     = 1e3f * 6360;
        atmosphereRadius = 1e3f * 6460;

        terrainAlbedo = { 0.3f, 0.3f, 0.3f };
    }

    auto operator<=>(const AtmosphereProperties &) const = default;

    size_t Hash() const
    {
        return Rtrc::Hash(
            scatterRayleigh, hDensityRayleigh,
            scatterMie, asymmetryMie, absorbMie, hDensityMie,
            absorbOzone, ozoneCenterHeight, ozoneThickness,
            planetRadius, atmosphereRadius,
            terrainAlbedo);
    }
};

RTRC_END
