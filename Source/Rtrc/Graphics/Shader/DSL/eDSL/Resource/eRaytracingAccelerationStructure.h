#pragma once

#include <cassert>

#include "../eVariable.h"

RTRC_EDSL_BEGIN

struct eRaytracingAccelerationStructure : eVariable<eRaytracingAccelerationStructure>
{
    static const char *GetStaticTypeName() { return "RaytracingAccelerationStructure"; }

    eRaytracingAccelerationStructure() { PopConstructParentVariable(); }

    eRaytracingAccelerationStructure(const eRaytracingAccelerationStructure &other)
        : eVariable(other)
    {
        PopConstructParentVariable();
    }

    eRaytracingAccelerationStructure &operator=(const eRaytracingAccelerationStructure &other)
    {
        static_cast<eVariable&>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    static eRaytracingAccelerationStructure CreateFromName(std::string name)
    {
        DisableStackVariableAllocation();
        eRaytracingAccelerationStructure ret;
        ret.eVariableName = std::move(name);
        EnableStackVariableAllocation();
        return ret;
    }
};

RTRC_EDSL_END
