#pragma once

#include "eNumber.h"

RTRC_EDSL_BEGIN

template<typename T>
struct eVector2;
template<typename T>
struct eVector3;
template<typename T>
struct eVector4;

template<typename Vec, typename TVec, int I0, int I1, int I2, int I3>
struct eVectorSwizzle
{
    Vec *_vec;

    operator TVec() const;
    eVectorSwizzle &operator=(const TVec &value);
};

RTRC_EDSL_END
