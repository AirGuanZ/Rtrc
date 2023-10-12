#pragma once

#include "Generated/Reflection.hlsl"
#include "Rtrc/Shader/Common/Random.hlsl"
#include "Rtrc/Renderer/Atmosphere/Shader/Atmosphere.hlsl"

typedef Rtrc::Renderer::ReSTIRDetail::LightShadingData LightShadingData;

static const float PI = 3.1415926;

#define LIGHT_SHADING_DATA_TYPE_DIRECTION 0
#define LIGHT_SHADING_DATA_TYPE_POINT     1
#define LIGHT_SHADING_DATA_TYPE_SPOT      2

float3 ShadeLightNoVisibility(float3 position, float3 normal, LightShadingData light, float2 uv, out float3 positionOnLight)
{
    const float3 us = Distribution::UniformOnUnitSphere(uv);
    float throughput; float3 direction;
    if(light.type == LIGHT_SHADING_DATA_TYPE_DIRECTION)
    {
        direction = -normalize(light.direction + light.softness * us);
        throughput = 1; // Directional light is not physically sampled
        positionOnLight = -direction;
    }
    else if(light.type == LIGHT_SHADING_DATA_TYPE_POINT)
    {
        const float radius = light.softness;
        const float3 p = light.position + radius * us;
        direction = normalize(p - position);
        const float s = dot(-direction, us);
        if(s > 1e-3)
        {
            const float dist = max(length(p - position), 1e-3);
            throughput = saturate(light.distFadeScale * dist + light.distFadeBias); // Custom distance attenuation

            const float area = max(4 * PI * radius * radius, 1e-4);
            const float pdfArea = 1 / area;
            const float pdf = pdfArea * dist * dist / s;

            const float radiance = 1 / area;
            throughput *= radiance / pdf;
        }
        else
            throughput = 0;
        positionOnLight = p;
    }
    else
    {
        const float radius = light.softness;
        const float3 p = light.position + radius * us;
        direction = normalize(p - position);
        const float s = dot(-direction, us);
        if(s > 1e-3)
        {
            const float dotc = dot(-direction, light.direction);
            throughput = saturate(light.coneFadeScale * dotc + light.coneFadeBias);
            const float dist = max(length(p - position), 1e-3);
            throughput *= saturate(light.distFadeScale * dist + light.distFadeBias);

            const float area = max(4 * PI * radius * radius, 1e-4);
            const float pdfArea = 1 / area;
            const float pdf = pdfArea * dist * dist / s;

            const float radiance = 1 / area;
            throughput *= radiance / pdf;
        }
        else
            throughput = 0;
        positionOnLight = p;
    }
    const float bsdfFactor = max(0, dot(direction, normal));
    return throughput * bsdfFactor * light.color * light.intensity;
}

rtrc_sampler(ReSTIR_SkySampler, filter = linear, address_u = repeat, address_v = clamp)

float3 ShadeSkyNoVisibility(float3 position, float3 normal, Texture2D<float3> sky, float2 lightUV, out float3 direction)
{
    const float phi = 2 * PI * lightUV.x;
    const float theta = PI * (lightUV.y - 0.5);
    direction = Distribution::UniformOnUnitSphere(lightUV);
    const float2 skyUV = Atmosphere::ComputeSkyLutTexCoord(direction);
    const float3 skyRadiance = sky.SampleLevel(ReSTIR_SkySampler, skyUV, 0);
    const float bsdfFactor = max(0, dot(direction, normal));
    const float pdf = 1 / (4 * PI);
    return bsdfFactor * skyRadiance / pdf;
}
