#pragma once

namespace Atmosphere
{

    struct Properties
    {
        float3 scatterRayleigh;
        float  hDensityRayleigh;

        float scatterMie;
        float asymmetryMie;
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

        void GetSigmaST(float h, out float3 sigmaS, out float3 sigmaT)
        {    
            float3 rayleigh = scatterRayleigh * exp(-h / hDensityRayleigh);
            float mieDensity = exp(-h / hDensityMie);
            float mieS = scatterMie * mieDensity;
            float mieT = (scatterMie + absorbMie) * mieDensity;
            float3 ozone = absorbOzone * max(0.0f, 1 - 0.5 * abs(h - ozoneCenterHeight) / ozoneThickness);
            sigmaS = rayleigh + mieS;
            sigmaT = rayleigh + mieT + ozone;
        }

        float3 EvaluatePhaseFunction(float h, float u)
        {
            float3 sRayleigh = scatterRayleigh * exp(-h / hDensityRayleigh);
            float sMie = scatterMie * exp(-h / hDensityMie);
            float3 s = sRayleigh + sMie;
            float g = asymmetryMie, g2 = g * g, u2 = u * u;
            float pRayleigh = 3 / (16 * PI) * (1 + u2);
            float m = 1 + g2 - 2 * g * u;
            float pMie = 3 / (8 * PI) * (1 - g2) * (1 + u2) / ((2 + g2) * m * sqrt(m));
            float3 result;
            result.x = s.x > 0 ? (pRayleigh * sRayleigh.x + pMie * sMie) / s.x : 0;
            result.y = s.y > 0 ? (pRayleigh * sRayleigh.y + pMie * sMie) / s.y : 0;
            result.z = s.z > 0 ? (pRayleigh * sRayleigh.z + pMie * sMie) / s.z : 0;
            return result;
        }
    };

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
    
    bool TestRaySphere(float3 o, float3 d, float R)
    {
        float A = dot(d, d);
        float B = 2 * dot(o, d);
        float C = dot(o, o) - R * R;
        float delta = B * B - 4 * A * C;
        return (delta >= 0) && ((C <= 0) | (B <= 0));
    }

    // returns smaller non-negative t
    bool IntersectRayCircle(float2 o, float2 d, float R, out float nearT)
    {
        float A = dot(d, d);
        float B = 2 * dot(o, d);
        float C = dot(o, o) - R * R;
        float delta = B * B - 4 * A * C;
        if(delta < 0)
            return false;
        delta = sqrt(delta);
        nearT = (-B + (C <= 0 ? delta : -delta)) / (2 * A);
        return (C <= 0) || (B <= 0);
    }

    // returns both nearT and farT
    bool IntersectRayCircle2(float2 o, float2 d, float R, out float nearT, out float farT)
    {
        float A = dot(d, d);
        float B = 2 * dot(o, d);
        float C = dot(o, o) - R * R;
        float delta = B * B - 4 * A * C;
        if(delta < 0)
            return false;
        delta = sqrt(delta);
        float inv2A = 1.0 / (A + A);
        nearT = (-B - delta) * inv2A;
        farT = (-B + delta) * inv2A;
        return true;
    }

    // returns smaller non-negative t
    bool IntersectRaySphere(float3 o, float3 d, float R, out float nearT)
    {
        float A = dot(d, d);
        float B = 2 * dot(o, d);
        float C = dot(o, o) - R * R;
        float delta = B * B - 4 * A * C;
        if(delta < 0)
            return false;
        delta = sqrt(delta);
        nearT = (-B + (C <= 0 ? delta : -delta)) / (2 * A);
        return (C <= 0) || (B <= 0);
    }

} // namespace Atmosphere
