rtrc_shader("PrepareThreadGroupCountForIndirectDispatch_1D")
{
    rtrc_keyword(ThreadGroupSize, s32, s64, s128, s256, Others)

    rtrc_comp(CSMain)

    rtrc_group(Pass)
    {
        rtrc_define(StructuredBuffer<uint>, ThreadCountBuffer)
        rtrc_define(RWStructuredBuffer<uint>, ThreadGroupCountBuffer)
    #if ThreadGroupSize == ThreadGroupSize_Others
        rtrc_uniform(uint, threadGroupSize)
    #endif
    };

    [numthreads(1, 1, 1)]
    void CSMain()
    {
    #if ThreadGroupSize == ThreadGroupSize_s32
        const uint gsize = 32;
    #elif ThreadGroupSize == ThreadGroupSize_s64
        const uint gsize = 64;
    #elif ThreadGroupSize == ThreadGroupSize_s128
        const uint gsize = 128;
    #elif ThreadGroupSize == ThreadGroupSize_s256
        const uint gsize = 256;
    #else
        const uint gsize = Pass.threadGroupSize;
    #endif
        const uint tcount = ThreadCountBuffer[0];
        const uint gcount = (tcount + gsize - 1) / gsize;
        ThreadGroupCountBuffer[0] = gcount;
        ThreadGroupCountBuffer[1] = 1;
        ThreadGroupCountBuffer[2] = 1;
    }
}
