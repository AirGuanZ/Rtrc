#pragma once

#include "eVectorSwizzle.h"

RTRC_EDSL_BEGIN

namespace VectorSwizzleDetail
{

    constexpr std::string_view ComponentIndexToName(int index)
    {
        switch(index)
        {
        case 0: return "x";
        case 1: return "y";
        case 2: return "z";
        case 3: return "w";
        default: return "";
        }
    }

} // namespace VectorSwizzleDetail

template <typename Vec, typename TVec, int I0, int I1, int I2, int I3>
eVectorSwizzle<Vec, TVec, I0, I1, I2, I3>::operator TVec() const
{
    return CreateTemporaryVariableForExpression<TVec>(fmt::format(
        "{}.{}{}{}{}",
        _vec->Compile(),
        VectorSwizzleDetail::ComponentIndexToName(I0),
        VectorSwizzleDetail::ComponentIndexToName(I1),
        VectorSwizzleDetail::ComponentIndexToName(I2),
        VectorSwizzleDetail::ComponentIndexToName(I3)));
}

template <typename Vec, typename TVec, int I0, int I1, int I2, int I3>
eVectorSwizzle<Vec, TVec, I0, I1, I2, I3>& eVectorSwizzle<Vec, TVec, I0, I1, I2, I3>::operator=(const TVec& value)
{
    CreateTemporaryVariableForExpression<TVec>(fmt::format(
        "{}.{}{}{}{}",
        _vec->Compile(),
        VectorSwizzleDetail::ComponentIndexToName(I0),
        VectorSwizzleDetail::ComponentIndexToName(I1),
        VectorSwizzleDetail::ComponentIndexToName(I2),
        VectorSwizzleDetail::ComponentIndexToName(I3))) = value;
    return *this;
}

RTRC_EDSL_END
