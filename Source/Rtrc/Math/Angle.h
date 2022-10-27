#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

constexpr float PI = 3.1415926535f;

inline float Deg2Rad(float deg)
{
    return deg * (PI / 180.0f);
}

inline float Rad2Deg(float rad)
{
    return rad * (180.0f / PI);
}

RTRC_END
