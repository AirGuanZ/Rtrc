#pragma once

#include "ConstantBuffer.h"

RTRC_EDSL_BEGIN

template <typename T>
const char* ConstantBuffer<T>::GetStaticTypeName()
{
    static const std::string ret = []
    {
        const char *element = T::GetStaticTypeName();
        return fmt::format("ConstantBuffer<{}>", element);
    }();
    return ret.data();
}

template <typename T>
ConstantBuffer<T> ConstantBuffer<T>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    ConstantBuffer ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T>
ConstantBuffer<T>::ConstantBuffer()
{
    assert(!IsStackVariableAllocationEnabled());
}

RTRC_EDSL_END
