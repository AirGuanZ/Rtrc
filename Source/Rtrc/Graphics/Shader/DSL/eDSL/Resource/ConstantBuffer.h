#pragma once

#include "../eVariable.h"

RTRC_EDSL_BEGIN

template<typename T>
struct ConstantBuffer : T
{
    static_assert(std::is_base_of_v<eVariableCommonBase, T>);

    static const char *GetStaticTypeName();

    static ConstantBuffer CreateFromName(std::string name);

private:

    ConstantBuffer();
};

RTRC_EDSL_END
