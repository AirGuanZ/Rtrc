#pragma once

#include "Common.hlsl"

rtrc_group(Pass)
{
    rtrc_define(RWTexture2D<float4>, OutputTextureRW)
    rtrc_uniform(uint2, outputResolution)
};

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(all(tid < Pass.outputResolution))
    {
        Reservoir r;
        r.Reset();
        OutputTextureRW[tid] = r.GetEncodedState();
    }
}
