#pragma once

#include "Builtin/Material/Common/Scene.hlsl"

rtrc_define(Texture2D<float4>, g_GBufferA)
rtrc_define(Texture2D<float4>, g_GBufferB)
rtrc_define(Texture2D<float4>, g_GBufferC)
rtrc_define(Texture2D<float>, g_GBufferDepth)

rtrc_sampler(_internalGBufferSampler, filter = point, address = clamp)

float3 LoadGBufferNormal(float2 uv);
float3 LoadGBufferAlbedo(float2 uv);
float LoadGBufferMetallic(float2 uv);
float LoadGBufferRoughness(float2 uv);
float LoadGBufferDepth(float2 uv);
