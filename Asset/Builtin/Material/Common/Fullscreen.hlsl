#pragma once

namespace FullscreenPrimitive
{

    struct VsInput
    {
        float2 position : POSITION;
        float2 uv       : UV;
    };

    struct VsInputWithWorldRay
    {
        float2 position : POSITION;
        float2 uv       : UV;
        float3 ray      : WORLD_RAY;
    };

    struct VsToFs
    {
        float4 position : SV_POSITION;
        float2 uv       : UV;
    };

    struct VsToFsWithWorldRay
    {
        float4 position : SV_POSITION;
        float2 uv       : UV;
        float3 ray      : WORLD_RAY;
    };

} // namespace FullscreenPrimitive

FullscreenPrimitive::VsToFs FullscreenVsToFs(FullscreenPrimitive::VsInput input)
{
    FullscreenInput::VsToFs output;
    output.position = float4(input.position, 1, 1);
    output.uv       = input.uv;
    return output;
}

FullscreenPrimitive::VsToFsWithWorldRay FullscreenVsToFsWithWorldRay(FullscreenPrimitive::VsInputWithWorldRay input)
{
    FullscreenInput::VsToFsWithWorldRay output;
    output.position = float4(input.position, 1, 1);
    output.uv       = input.uv;
    output.ray      = input.ray;
    return output;
}