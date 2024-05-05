#pragma once

#include <Rtrc/ShaderDSL/DSL/eNumber.h>

RTRC_EDSL_BEGIN

struct ElseBuilder
{
    eNumber<bool> condition;
    std::move_only_function<void()> thenBodyFunc;
    std::move_only_function<void()> elseBodyFunc;

    ElseBuilder(const eNumber<bool> &condition, std::move_only_function<void()> inThenBodyFunc);
    ~ElseBuilder();

    template<typename ElseBodyFunc>
    void operator-(ElseBodyFunc inElseBodyFunc);
};

struct IfBuilder
{
    eNumber<bool> condition;

    explicit IfBuilder(const eNumber<bool> &condition);

    template<typename ThenBodyFunc>
    ElseBuilder operator+(ThenBodyFunc thenBodyFunc);
};

#define RTRC_EDSL_IF(CONDITION) ::Rtrc::eDSL::IfBuilder(CONDITION)+[&]
#define RTRC_EDSL_ELSE -[&]

RTRC_EDSL_END
