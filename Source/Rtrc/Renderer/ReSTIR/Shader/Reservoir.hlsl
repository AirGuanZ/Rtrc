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
        pbar = 0;
    }

    void SetEncodedState(uint4 state)
    {
        data.lightIndex = state.x;
        data.lightUV.x = f16tof32(state.y >> 16);
        data.lightUV.y = f16tof32(state.y & 0xffff);
        wsum = asfloat(state.z);
        M = state.w >> 16;
        pbar = f16tof32(state.w & 0xffff);
    }

    uint4 GetEncodedState()
    {
        uint4 ret;
        ret.x = data.lightIndex;
        ret.y = (f32tof16(data.lightUV.x) << 16) | f32tof16(data.lightUV.y);
        ret.z = asuint(wsum);
        ret.w = (M << 16) | f32tof16(pbar);
        return ret;
    }

    bool Update(ReservoirData newData, float weight, float newPBar, float rand01)
    {
        wsum += weight;
        if(rand01 < weight / wsum)
        {
            data = newData;
            pbar = newPBar;
            return true;
        }
        return false;
    }

    float W()
    {
        return (1.0 / pbar) * (1.0 / M) * wsum;
    }

    ReservoirData data;
    float wsum;
    uint M;
    float pbar;
};
