#pragma once

rtrc_group(Pass)
{
    rtrc_define(Texture2D<float>, Depth)
    rtrc_define(Texture2D<float>, Variance)
    rtrc_define(RWTexture2D<float>, OutputVariance)
    rtrc_uniform(uint2, resolution)
};

float Gaussian(float sigma, float x)
{
    return exp(-(x * x) / (2 * sigma * sigma));
}

[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(any(tid >= Pass.resolution))
        return;
    
    const float deviceZ = Depth[tid];
    if(deviceZ >= 1)
        return;
    
    // TODO: optimize with LDS
    float sumWeightedVariance = 0;
    float sumWeight = 0;
    for(int x = -1; x <= 1; ++x)
    {
        const float gx = Gaussian(0.5, x);
        for(int y = -1; y <= 1; ++y)
        {
            const int2 nid = clamp(int2(tid) + int2(x, y), int2(0, 0), int2(Pass.resolution - 1));
            const float nz = Depth[nid];
            if(nz >= 1)
                continue;
            
            const float gy = Gaussian(0.5, y);
            const float weight = gx * gy;

            const float nv = Variance[nid];
            sumWeightedVariance += weight * nv;
            sumWeight += weight;
        }
    }

    const float filteredVariance = sumWeightedVariance / sumWeight;
    OutputVariance[tid] = filteredVariance;
}
