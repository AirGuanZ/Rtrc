#ifndef RTRC_COMMON_RANDOM_HLSL
#define RTRC_COMMON_RANDOM_HLSL

#include "./Frame.hlsl"

namespace Distribution
{

    static const float PI = 3.1415926;

    float3 UniformOnUnitSphere(float2 uv)
    {
        float z = 1.0f - (uv.x + uv.x);
        float phi = 2 * PI * uv.y;
        float r = sqrt(max(0.0, 1.0 - z * z));
        float x = r * cos(phi);
        float y = r * sin(phi);
        return float3(x, y, z);
    }

    float3 UniformOnUnitHemisphere(float2 uv)
    {
        float z = uv.x;
        float phi = 2 * PI * uv.y;
        float r = sqrt(max(0.0, 1 - z * z));
        float x = r * cos(phi);
        float y = r * sin(phi);
        return float3(x, y, z);
    }

    float3 ZWeightedOnHemisphere(float2 uv, out float pdfSa)
    {
        float u1 = 2 * uv.x - 1;
        float u2 = 2 * uv.y - 1;
        float2 sam;
        if(u1 || u2)
        {
            float theta, r;
            if(abs(u1) > abs(u2))
            {
                r = u1;
                theta = 0.25 * PI * (u2 / u1);
            }
            else
            {
                r = u2;
                theta = 0.5 * PI - 0.25 * PI * (u1 / u2);
            }
            sam = r * float2(cos(theta), sin(theta));
        }
        else
            sam = 0;
        float z = sqrt(max(0.0, 1 - dot(sam, sam)));
        pdfSa = z * (1 / PI);
        return float3(sam.x, sam.y, z);
    }

    float3 ZWeightedOnHemisphere(float2 uv)
    {
        float pdf;
        return ZWeightedOnHemisphere(uv, pdf);
    }

} // namespace Distribution

namespace Pcg
{

    uint Next(inout uint state)
    {
        state = state * 747796405u + 2891336453u;
        uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
        return (word >> 22u) ^ word;
    }

    float NextFloat(inout uint state)
    {
        uint word = Pcg::Next(state);
        return word * (1 / 4294967296.0);
    }

    float2 NextFloat2(inout uint state)
    {
        float2 ret;
        ret.x = NextFloat(state);
        ret.y = NextFloat(state);
        return ret;
    }

    float3 NextUniformOnUnitSphere(inout uint state)
    {
        float u = NextFloat(state);
        float v = NextFloat(state);
        return Distribution::UniformOnUnitSphere(float2(u, v));
    }
    
    float3 NextUniformOnUnitHemisphere(inout uint state)
    {
        float u = NextFloat(state);
        float v = NextFloat(state);
        return Distribution::UniformOnUnitHemisphere(float2(u, v));
    }

    struct Sampler
    {
        uint state_;

        void SetState(uint state) { state_ = state; }
        uint GetState() { return state_; }

        uint NextUInt() { return Pcg::Next(state_); }
        float NextFloat() { return Pcg::NextFloat(state_); }
        float2 NextFloat2() { return Pcg::NextFloat2(state_); }

        float3 NextUniformOnUnitSphere() { return Pcg::NextUniformOnUnitSphere(state_); }
        float3 NextUniformOnUnitHemisphere() { return Pcg::NextUniformOnUnitHemisphere(state_); }

        float3 NextUniformOnUnitHemisphere(float3 normal)
        {
            float3 localDir = NextUniformOnUnitHemisphere();
            LocalFrame frame;
            frame.InitializeFromNormalizedZ(normal);
            return frame.LocalToGlobal(localDir);
        }
    };

} // namespace Pcg

#endif // #ifndef RTRC_COMMON_RANDOM_HLSL
