#pragma once

#include "Generated/Reflection.hlsl"

namespace Atmosphere
{

    typedef Rtrc::AtmosphereProperties Properties;

    static float PI = 3.14159265;

    float3 GetSigmaS(Properties properties, float h)
    {
        float3 rayleigh = properties.scatterRayleigh * exp(-h / properties.hDensityRayleigh);
        float mie = properties.scatterMie * exp(-h / properties.hDensityMie);
        return rayleigh + mie;
    }

    float3 GetSigmaT(Properties properties, float h)
    {
        float3 rayleigh = properties.scatterRayleigh * exp(-h / properties.hDensityRayleigh);
        float mie = (properties.scatterMie + properties.absorbMie) * exp(-h / properties.hDensityMie);
        float3 ozone = properties.absorbOzone * max(0.0f, 1 - 0.5 * abs(h - properties.ozoneCenterHeight) / properties.ozoneThickness);
        return rayleigh + mie + ozone;
        return 0;
    }

    void GetSigmaST(Properties properties, float h, out float3 sigmaS, out float3 sigmaT)
    {    
        float3 rayleigh = properties.scatterRayleigh * exp(-h / properties.hDensityRayleigh);
        float mieDensity = exp(-h / properties.hDensityMie);
        float mieS = properties.scatterMie * mieDensity;
        float mieT = (properties.scatterMie + properties.absorbMie) * mieDensity;
        float3 ozone = properties.absorbOzone * max(0.0f, 1 - 0.5 * abs(h - properties.ozoneCenterHeight) / properties.ozoneThickness);
        sigmaS = rayleigh + mieS;
        sigmaT = rayleigh + mieT + ozone;
    }

    float3 EvaluatePhaseFunction(Properties properties, float h, float u)
    {
        float3 sRayleigh = properties.scatterRayleigh * exp(-h / properties.hDensityRayleigh);
        float sMie = properties.scatterMie * exp(-h / properties.hDensityMie);
        float3 s = sRayleigh + sMie;
        float g = properties.asymmetryMie, g2 = g * g, u2 = u * u;
        float pRayleigh = 3 / (16 * PI) * (1 + u2);
        float m = 1 + g2 - 2 * g * u;
        float pMie = 3 / (8 * PI) * (1 - g2) * (1 + u2) / ((2 + g2) * m * sqrt(m));
        float3 result;
        result.x = s.x > 0 ? (pRayleigh * sRayleigh.x + pMie * sMie) / s.x : 0;
        result.y = s.y > 0 ? (pRayleigh * sRayleigh.y + pMie * sMie) / s.y : 0;
        result.z = s.z > 0 ? (pRayleigh * sRayleigh.z + pMie * sMie) / s.z : 0;
        return result;
    }

    float3 SampleTransmittanceLut(Texture2D<float3> lut, SamplerState s, Properties atmosphere, float h, float theta)
    {
        float u = h / (atmosphere.atmosphereRadius - atmosphere.planetRadius);
        float v = 0.5 + 0.5 * sin(theta);
        return lut.SampleLevel(s, float2(u, v), 0);
    }
    
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
        nearT = max(0, (-B - delta) * inv2A);
        farT = (-B + delta) * inv2A;
        return nearT <= farT;
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

    // dir must be normalized
    float2 ComputeSkyLutTexCoord(float3 dir)
    {
        float phi = any(dir != 0) ? atan2(dir.z, dir.x) : 0.0;
        float u = phi / (2 * PI);
        float theta = asin(dir.y);
        float v = 0.5 + 0.5 * sign(theta) * sqrt(abs(theta) / (PI / 2));
        return float2(u, v);
    }

} // namespace Atmosphere
