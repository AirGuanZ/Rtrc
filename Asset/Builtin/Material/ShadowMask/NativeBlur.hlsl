#pragma once

rtrc_group(Pass, CS)
{
    rtrc_define(Texture2D<float>, In)
    rtrc_define(RWTexture2D<float>, Out)
    rtrc_uniform(int2, resolution)
};

#define BLUR_AXIS_GROUP_SIZE 64

#if IS_BLUR_DIRECTION_Y
#define GROUP_SIZE_X 1
#define GROUP_SIZE_Y BLUR_AXIS_GROUP_SIZE
#else
#define GROUP_SIZE_X BLUR_AXIS_GROUP_SIZE
#define GROUP_SIZE_Y 1
#endif

#define BLUR_RADIUS 3

groupshared float SharedData[BLUR_AXIS_GROUP_SIZE + BLUR_RADIUS + BLUR_RADIUS];

float Load(int2 xy)
{
    xy = clamp(xy, int2(0, 0), Pass.resolution - int2(1, 1));
    return In[xy];
}

[numthreads(GROUP_SIZE_X, GROUP_SIZE_Y, 1)]
void CSMain(int2 tid : SV_DispatchThreadID, int2 ltid : SV_GroupThreadID)
{
#if IS_BLUR_DIRECTION_Y
    int lid = ltid.y;
#else
    int lid = ltid.x;
#endif

    SharedData[BLUR_RADIUS + lid] = Load(tid);

    if(lid < BLUR_RADIUS)
    {
        int2 tid2 = tid;
#if IS_BLUR_DIRECTION_Y
        tid2.y -= BLUR_RADIUS;
#else
        tid2.x -= BLUR_RADIUS;
#endif
        float value = Load(tid2);
        SharedData[lid] = value;
    }

    if(lid >= BLUR_AXIS_GROUP_SIZE - BLUR_RADIUS)
    {
        int2 tid3 = tid;
#if IS_BLUR_DIRECTION_Y
        tid3.y += BLUR_RADIUS;
#else
        tid3.x += BLUR_RADIUS;
#endif
        float value = Load(tid3);
        SharedData[2 * BLUR_RADIUS + lid] = value;
    }

    GroupMemoryBarrierWithGroupSync();

    if(any(tid >= Pass.resolution))
        return;

    float sum = 0;
    for(int i = -BLUR_RADIUS; i <= BLUR_RADIUS; ++i)
        sum += SharedData[BLUR_RADIUS + lid + i];
    float value = sum * (1.0 / (BLUR_RADIUS + BLUR_RADIUS + 1));

    Out[tid] = value;
}
