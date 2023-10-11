#pragma once

#include "Rtrc/Shader/Common/Scene.hlsl"
#include "Rtrc/Shader/Common/Random.hlsl"

struct ReservoirData
{
    uint lightIndex;
    float2 lightUV;
};

// Our reservoirs handle samples in primary sample space
struct Reservoir
{
    void Reset()
    {
        data.lightIndex = 0;
        data.lightUV = 0;
        wsum = 0;
        M = 0;
        W = 0;
    }

    void SetEncodedState(uint4 state)
    {
        data.lightIndex = state.x >> 16;
        data.lightUV.x = f16tof32(state.y >> 16);
        data.lightUV.y = f16tof32(state.y & 0xffff);
        M = state.x & 0xffff;
        wsum = asfloat(state.z);
        W = asfloat(state.w);
    }

    uint4 GetEncodedState()
    {
        uint4 ret;
        ret.x = (data.lightIndex << 16) | M;
        ret.y = (f32tof16(data.lightUV.x) << 16) | f32tof16(data.lightUV.y);
        ret.z = asuint(wsum);
        ret.w = asuint(W);
        return ret;
    }

    bool Update(ReservoirData newData, float weight, float rand01)
    {
        wsum += weight;
        if(rand01 < weight / wsum)
        {
            data = newData;
            return true;
        }
        return false;
    }

    ReservoirData data;
    float wsum;
    uint M;
    float W;
};
