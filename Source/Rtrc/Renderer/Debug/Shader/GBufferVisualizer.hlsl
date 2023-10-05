#pragma once

#define GBUFFER_MODE GBUFFER_MODE_ALL
#include "Rtrc/Shader/Common/GBufferRead.hlsl"

#define VIS_MODE_NORMAL 1

rtrc_group(Pass, CS)
{
    REF_GBUFFERS(CS)
    rtrc_define(RWTexture2D<float4>, Output)
    rtrc_uniform(uint2, outputResolution)
};

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.outputResolution))
        return;
    
    float4 result;
#if VIS_MODE == VIS_MODE_NORMAL
    result.xyz = 0.5 + 0.5 * LoadGBufferNormal(tid);
    result.w = 1;
#else
    #error "VIS_MODE must be properly defined before including GBufferVisualizer.hlsl"
#endif
    Output[tid] = result;
}
