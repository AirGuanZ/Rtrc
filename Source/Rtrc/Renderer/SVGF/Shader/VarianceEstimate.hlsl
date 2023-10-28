#pragma once

rtrc_group(Pass)
{
    rtrc_define(Texture2D<float2>, Moments)
    rtrc_define(RWTexture2D<float>, Variance)
    rtrc_uniform(uint2, resolution)
};

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;

    // TODO: use spatial color distribution to estimate variance when history info of moments is limited
    const float2 moments = Moments[tid];
    const float variance = abs(moments.x * moments.x - moments.y);
    Variance[tid] = variance;
}
