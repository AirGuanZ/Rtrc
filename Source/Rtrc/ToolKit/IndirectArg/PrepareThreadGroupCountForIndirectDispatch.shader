rtrc_shader("Builtin/PrepareThreadGroupCountForIndirectDispatch_1D")
{
    rtrc_comp(CSMain)

    rtrc_group(Pass)
    {
        rtrc_define(StructuredBuffer<uint>, ThreadCountBuffer)
        rtrc_define(RWStructuredBuffer<uint>, ThreadGroupCountBuffer)
        rtrc_uniform(uint, threadGroupSize)
    };

    [numthreads(1, 1, 1)]
    void CSMain()
    {
        const uint tcount = ThreadCountBuffer[0];
        const uint gcount = (tcount + Pass.threadGroupSize - 1) / Pass.threadGroupSize;
        ThreadGroupCountBuffer[0] = gcount;
        ThreadGroupCountBuffer[1] = 1;
        ThreadGroupCountBuffer[2] = 1;
    }
}
