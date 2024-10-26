#pragma once

#include "eConstantBuffer.h"

RTRC_EDSL_BEGIN

template <typename T>
const char* eConstantBuffer<T>::GetStaticTypeName()
{
    static const std::string ret = []
    {
        const char *element = T::GetStaticTypeName();
        return std::format("ConstantBuffer<{}>", element);
    }();
    return ret.data();
}

template <typename T>
eConstantBuffer<T> eConstantBuffer<T>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    eConstantBuffer ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T>
eConstantBuffer<T>::eConstantBuffer()
{
    assert(!IsStackVariableAllocationEnabled());
}

RTRC_EDSL_END
