#pragma once

struct ReservoirData
{
    uint lightIndex;
    float2 lightUV;
};

struct Reservoir
{
    void Reset()
    {
        data.lightIndex = 0;
        data.lightUV = 0;
        w = 0;
        M = 0;
    }

    void SetEncodedState(float4 state)
    {
        data.lightIndex = asuint(state.x);
        uint lightU16 = asuint(state.y) >> 16;
        uint lightV16 = asuint(state.y) & 0xffff;
        data.lightUV.x = f16tof32(lightU16);
        data.ligthUV.y = f16tof32(lightV16);
        w = state.z;
        M = state.w;
    }

    float4 GetEncodedState(float4 state)
    {
        float4 ret;
        ret.x = asfloat(data.lightIndex);
        ret.y = (f32tof16(data.lightUV.x) << 16) | f32tof16(data.lightUV.y);
        ret.z = w;
        ret.w = M;
        return ret;
    }

    void Update(ReservoirData newData, float weight, float rand01)
    {
        w += weight;
        ++M;
        if(rand01 < weight / w)
            data = newData;
    }

    ReservoirData data;
    float w;
    float M;
};
