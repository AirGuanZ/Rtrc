#pragma once

#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

rtrc_struct(PhysicalAtmosphereProperties)
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

    auto operator<=>(const PhysicalAtmosphereProperties &) const = default;
};

RTRC_END
