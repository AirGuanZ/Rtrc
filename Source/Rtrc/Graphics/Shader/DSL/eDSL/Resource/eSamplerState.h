#pragma once

#include "../Value/eNumber.h"

RTRC_EDSL_BEGIN

struct eSamplerState : eVariable<eSamplerState>
{
    static const char *GetStaticTypeName() { return "SamplerState"; }

    eSamplerState() { PopConstructParentVariable(); }
    eSamplerState(const eSamplerState &other) : eVariable(other) { PopConstructParentVariable(); }

    eSamplerState &operator=(const eSamplerState &other)
    {
        static_cast<eVariable &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }
};

RTRC_EDSL_END
