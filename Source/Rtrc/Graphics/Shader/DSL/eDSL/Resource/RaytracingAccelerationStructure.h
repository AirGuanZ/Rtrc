#pragma once

#include <cassert>

#include "../eVariable.h"

RTRC_EDSL_BEGIN

struct RaytracingAccelerationStructure : eVariable<RaytracingAccelerationStructure>
{
    static const char *GetStaticTypeName() { return "RaytracingAccelerationStructure"; }

    static RaytracingAccelerationStructure CreateFromName(std::string name)
    {
        DisableStackVariableAllocation();
        RaytracingAccelerationStructure ret;
        ret.eVariableName = std::move(name);
        EnableStackVariableAllocation();
        return ret;
    }

private:

    RaytracingAccelerationStructure()
    {
        assert(!IsStackVariableAllocationEnabled());
    }
};

RTRC_EDSL_END
