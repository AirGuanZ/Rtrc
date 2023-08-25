#ifndef RTRC_COMMON_FULLSCREEN_HLSL
#define RTRC_COMMON_FULLSCREEN_HLSL

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
    FullscreenPrimitive::VsToFs output;
    output.position = float4(input.position, 1, 1);
    output.uv       = input.uv;
    return output;
}

FullscreenPrimitive::VsToFsWithWorldRay FullscreenVsToFsWithWorldRay(FullscreenPrimitive::VsInputWithWorldRay input)
{
    FullscreenPrimitive::VsToFsWithWorldRay output;
    output.position = float4(input.position, 1, 1);
    output.uv       = input.uv;
    output.ray      = input.ray;
    return output;
}

#endif // #ifndef RTRC_COMMON_FULLSCREEN_HLSL
