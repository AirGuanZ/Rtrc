#pragma once

#include "../Value/eNumber.h"

RTRC_EDSL_BEGIN

struct SamplerState : eVariable<SamplerState>
{
    static const char *GetStaticTypeName() { return "SamplerState"; }

    SamplerState() { PopConstructParentVariable(); }

    SamplerState &operator=(const SamplerState &other)
    {
        static_cast<eVariable &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }
};

RTRC_EDSL_END
