#pragma once

#include "Common/Fullscreen.hlsl"
#include "Common/GBuffer.hlsl"
#include "Common/Scene.hlsl"

rtrc_group(Pass, FS)
{
    REFERENCE_BUILTIN_GBUFFERS(FS)

    rtrc_define(ConstantBuffer<CameraConstantBuffer>, camera)
    
    rtrc_define(StructuredBuffer<PointLightShadingData>, PointLightBuffer)
    rtrc_uniform(uint, pointLightCount)
    
    rtrc_define(StructuredBuffer<DirectionalLightShadingData>, DirectionalLightBuffer)
    rtrc_uniform(uint, directionalLightCount)
};

float4 FSMainDefault(FullscreenPrimitive::VsToFsWithWorldRay input)
{
    Builtin::GBufferPixelValue gbuffer = Builtin::LoadGBufferPixel(input.uv);
    return float4(0.5 + 0.5 * gbuffer.normal, 1);
}

float4 FSMainSky(FullscreenPrimitive::VsToFsWithWorldRay input)
{
    return float4(0, 1, 1, 0);
}
