#pragma once

#include "Common.hlsl"

rtrc_group(Pass)
{
    rtrc_define(StructuredBuffer<HashTableKey>, HashTableKeyBuffer)
    rtrc_define(StructuredBuffer<HashTableValue>, HashTableValueBuffer)
    rtrc_uniform(uint, hashTableSize)

    rtrc_define(RaytracingAccelerationStructure, Scene)
};

[numthreads(64, 1, 1)]
void CSMain(uint tid : SV_DispatchThreadID)
{
    if(tid >= Pass.hashTableSize)
        return;
    
    const HashTableKey key = HashTableKeyBuffer[tid];
    if(!IsValidHashTableKey(key))
        return;
    
    const HashTableValue value = HashTableValueBuffer[tid];
    if(!IsValidHashTableValue(value))
        return;
    
    

    HashTableValueBuffer[tid] = value;
}
