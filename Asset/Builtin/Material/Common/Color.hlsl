#pragma once

float3 Tonemap(float3 input)
{
    float POSTCOLOR_A = 2.51;
    float POSTCOLOR_B = 0.03;
    float POSTCOLOR_C = 2.43;
    float POSTCOLOR_D = 0.59;
    float POSTCOLOR_E = 0.14;
    return (input * (POSTCOLOR_A * input + POSTCOLOR_B))
         / (input * (POSTCOLOR_C * input + POSTCOLOR_D) + POSTCOLOR_E);
}

float3 Dither01(float3 input, float2 seed)
{
    float rand = frac(sin(dot(seed, float2(12.9898, 78.233) * 2.0)) * 43758.5453);
    input = 255 * saturate(input);
    input = rand.xxx < (input - floor(input)) ? ceil(input) : floor(input);
    input *= 1.0 / 255;
    return input;
}