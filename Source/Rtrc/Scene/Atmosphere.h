#pragma once

#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

rtrc_struct(AtmosphereProperties)
{
    // Rayleigh

    rtrc_var(float3, scatterRayleigh);
    rtrc_var(float,  hDensityRayleigh);

    // Mie

    rtrc_var(float, scatterMie);
    rtrc_var(float, assymmetryMie);
    rtrc_var(float, absorbMie);
    rtrc_var(float, hDensityMie);

    // Ozone

    rtrc_var(float3, absorbOzone);
    rtrc_var(float,  ozoneCenterHeight);
    rtrc_var(float,  ozoneThickness);

    // Geometry

    rtrc_var(float, planetRadius);
    rtrc_var(float, atmosphereRadius);

    rtrc_var(float3, terrainAlbedo);

    AtmosphereProperties()
    {
        scatterRayleigh  = 1e-6f * Vector3f(5.802f, 13.558f, 33.1f);
        hDensityRayleigh = 1e3f * 8;

        scatterMie    = 1e-6f * 3.996f;
        assymmetryMie = 0.8f;
        absorbMie     = 1e-6f * 4.4f;
        hDensityMie   = 1e3f * 1.2f;

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
            scatterMie, assymmetryMie, absorbMie, hDensityMie,
            absorbOzone, ozoneCenterHeight, ozoneThickness,
            planetRadius, atmosphereRadius,
            terrainAlbedo);
    }
};

RTRC_END
