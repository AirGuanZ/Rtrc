#pragma once

rtrc_group(Pass)
{
    rtrc_define(RWTexture2D<float4>, Color)
    rtrc_define(RWTexture2D<float2>, Moments)
    rtrc_uniform(uint2, resolution)
};

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;
    Color[tid] = float4(0, 0, 0, 0);
    Moments[tid] = float2(0, 0);
}
