#pragma once

#include "./Common.hlsl"

rtrc_group(Pass)
{
    rtrc_define(RWStructuredBuffer<HashTableKey>, HashTableKeyBuffer)
    rtrc_define(RWStructuredBuffer<HashTableValue>, HashTableValueBuffer)
    rtrc_uniform(uint, hashTableSize)
};

[numthreads(64, 1, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    if(tid >= hashTableSize)
        return;

    HashTableKey key;
    item.key = 0xffffffff;
    item.check = 0xffffffff;
    HashTableKeyBuffer[tid] = item;
    HashTableValueBuffer[tid] = (HashTableValue)0;
}
