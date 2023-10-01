#pragma once

#include <Graphics/Device/Device.h>

RTRC_BEGIN

rtrc_struct(AtmosphereProperties)
{
    // Rayleigh

    float3 scatterRayleigh = 1e-6f * Vector3f(5.802f, 13.558f, 33.1f);
    float  hDensityRayleigh = 1e3f * 8;

    // Mie

    float scatterMie   = 1e-6f * 3.996f;
    float asymmetryMie = 0.8f;
    float absorbMie    = 1e-6f * 4.4f;
    float hDensityMie  = 1e3f * 1.2f;

    // Ozone

    float3 absorbOzone       = 1e-6f * Vector3f(0.65f, 1.881f, 0.085f);
    float  ozoneCenterHeight = 1e3f * 25;
    float  ozoneThickness    = 1e3f * 30;

    // Geometry

    float planetRadius     = 1e3f * 6360;
    float atmosphereRadius = 1e3f * 6460;

    float3 terrainAlbedo = { 0.3f, 0.3f, 0.3f };

    auto operator<=>(const AtmosphereProperties &) const = default;
};

class Sky : public Uncopyable
{
public:

    RTRC_SET_GET(AtmosphereProperties, Atmosphere, atmosphere_)

    RTRC_SET_GET(Vector3f, SunDirection, sunDirection_)
    RTRC_SET_GET(Vector3f, SunColor,     sunColor_)
    RTRC_SET_GET(float,    SunIntensity, sunIntensity_)

private:

    AtmosphereProperties atmosphere_;

    Vector3f sunDirection_ = Vector3f(0, -1, 0);
    Vector3f sunColor_     = Vector3f(1, 1, 1);
    float    sunIntensity_ = 10;
};

RTRC_END
