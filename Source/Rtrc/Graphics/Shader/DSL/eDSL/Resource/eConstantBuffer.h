#pragma once

#include "../eVariable.h"

RTRC_EDSL_BEGIN

template<typename T>
struct eConstantBuffer : T
{
    static_assert(std::is_base_of_v<eVariableCommonBase, T>);

    static const char *GetStaticTypeName();

    static eConstantBuffer CreateFromName(std::string name);

private:

    eConstantBuffer();
};

RTRC_EDSL_END
