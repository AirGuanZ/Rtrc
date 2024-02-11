rtrc_shader("InitRng")
{

rtrc_compute(CSMain)

#include "Rtrc/Toolkit/Shader/Common/Random.hlsl"

rtrc_group(Pass)
{
    rtrc_define(RWTexture2D<uint>, RngTexture)
    rtrc_uniform(uint2, Resolution)
};

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.Resolution))
        return;
    uint state = 1 + tid.y * Pass.Resolution.x + tid.x;
    Pcg::Next(state);
    RngTexture[tid] = state;
}

} // Shader "InitRng"
