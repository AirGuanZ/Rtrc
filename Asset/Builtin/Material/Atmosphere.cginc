#pragma once

namespace Atmosphere
{

    struct Properties
    {
        float3 scatterRayleigh;
        float  hDensityRayleigh;

        float scatterMie;
        float assymmetryMie;
        float absorbMie;
        float hDensityMie;

        float3 absorbOzone;
        float  ozoneCenterHeight;
        float  ozoneThickness;
        float  planetRadius;
        float  atmosphereRadius;

        float3 GetSigmaS(float h)
        {
            float3 rayleigh = scatterRayleigh * exp(-h / hDensityRayleigh);
            float mie = scatterMie * exp(-h / hDensityMie);
            return rayleigh + mie;
        }

        float3 GetSigmaT(float h)
        {
            float3 rayleigh = scatterRayleigh * exp(-h / hDensityRayleigh);
            float mie = (scatterMie + absorbMie) * exp(-h / hDensityMie);
            float3 ozone = absorbOzone * max(0.0f, 1 - 0.5 * abs(h - ozoneCenterHeight) / ozoneThickness);
            return rayleigh + mie + ozone;
        }
    };

    bool IntersectRayCircle(float2 o, float2 d, float R, out float nearestT)
    {
        float A = dot(d, d);
        float B = 2 * dot(o, d);
        float C = dot(o, o) - R * R;
        float delta = B * B - 4 * A * C;
        if(delta < 0)
            return false;
        delta = sqrt(delta);
        nearestT = (-B + (C <= 0 ? delta : -delta)) / (2 * A);
        return (C <= 0) | (B <= 0);
    }

    struct TransmittanceLut
    {
        Texture2D<float3> lut_;
        SamplerState samplerState_;
        Properties atmosphere_;

        void Initialize(Texture2D<float3> lut, SamplerState samplerState s, Properties atmosphere)
        {
            lut_ = lut;
            samplerState_ = s;
            atmosphere_ = atmosphere;
        }

        void Sample(float h, float theta)
        {
            float u = h / (atmosphere_.atmosphereRadius - atmosphere_.planetRadius);
            float v = 0.5 + 0.5 * sin(theta);
            return lut_.SampleLevel(samplerState_, float2(u, v), 0);
        }
    };

} // namespace Atmosphere
