rtrc_shader("Builtin/InitializePcgState2D")
{
    rtrc_compute(CSMain)

    #include "Rtrc/ToolKit/Shader/Common/Random.hlsl"

    rtrc_group(Pass)
    {
        rtrc_define(RWTexture2D<uint>, Output)
        rtrc_uniform(uint2, resolution)
    };

    [numthreads(8, 8, 1)]
    void CSMain(uint2 tid : SV_DispatchThreadID)
    {
        if(any(tid >= Pass.resolution))
            return;
        uint state = 1 + tid.y * Pass.resolution.x + tid.x;
        Pcg::Next(state);
        Output[tid] = state;
    }

} // rtrc_shader("InitializePcgState2D")
