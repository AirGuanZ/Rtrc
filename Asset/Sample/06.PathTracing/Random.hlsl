#pragma once

uint PcgNext(inout uint state)
{
    state = state * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float PcgNextFloat(inout uint state)
{
    uint word = PcgNext(state);
    return word * (1 / 4294967296.0);
}

float3 UniformOnUnitSphere(float2 uv)
{
    float z = 1.0f - (uv.x + uv.x);
    float phi = 2 * 3.1415926 * uv.y;
    float r = sqrt(max(0.0, 1.0 - z * z));
    float x = r * cos(phi);
    float y = r * sin(phi);
    return float3(x, y, z);
}
