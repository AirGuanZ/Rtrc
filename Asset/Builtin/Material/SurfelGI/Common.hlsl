#pragma once

#define PTGI_CC_ActiveSurfel            0
#define PTGI_CC_SurfelThreadGroupX      1
#define PTGI_CC_SurfelThreadGroupY      2
#define PTGI_CC_SurfelThreadGroupZ      3
#define PTGI_CC_PendingElementAllocator 4
#define PTGI_CC_CellRangeAllocator      5

struct SurfelData
{
    float3 position;
    float3 normal;
    float3 irradiance;
};

struct SurfelClipmapPendingElement
{
    uint cellIndex;
    uint surfelIndex  : 24;
    uint offsetInCell : 8;
};

#define SURFEL_CLIPMAP_LEVEL_COUNT 8

struct SurfelClipmapParameters
{
    float3 centerPosition;
    int resolutionMinusOne;
    float halfExtents[SURFEL_CLIPMAP_LEVEL_COUNT];
    float rcpGridSizes[SURFEL_CLIPMAP_LEVEL_COUNT];
};

rtrc_define(ConstantBuffer<SurfelClipmapParameters>, SurfelClipmap)

// returns -1 when no clipmap contains relPos
int ComputeSurfelClipmapLevel(float3 relPos)
{
    float3 absRelPos = abs(relPos);
    float maxAbsRelPos = max(absRelPos.x, max(absRelPos.y, absRelPos.z));
    int ret = -1;
    for(int i = 0; i < SURFEL_CLIPMAP_LEVEL_COUNT; ++i)
    {
        if(maxAbsRelPos <= SurfelClipmap.levelHalfExtents[i])
            ret = i;
    }
    return ret;
}

int3 ComputeSurfelClipmapGridCoord(float3 relPos, int level)
{
    float3 p = relPos + SurfelClipmap.halfExtents[level];
    int3 pi = int3(p * SurfelClipmap.rcpGridSizes[level]);
    return clamp(pi, 0, SurfelClipmap.resolutionMinusOne);
}
