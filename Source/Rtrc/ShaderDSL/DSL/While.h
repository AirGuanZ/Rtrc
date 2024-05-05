#pragma once

#include <Rtrc/ShaderDSL/DSL/eNumber.h>

RTRC_EDSL_BEGIN

class WhileBuilder
{
public:

    std::move_only_function<eNumber<bool>()> conditionFunc;

    template<typename CondFunc>
    WhileBuilder(CondFunc condFunc);

    template<typename BodyFunc>
    void operator+(BodyFunc bodyFunc);
};

#define RTRC_EDSL_WHILE(CONDITION) ::Rtrc::eDSL::WhileBuilder([&]{return ::Rtrc::eDSL::eNumber<bool>(CONDITION);})+[&]

RTRC_EDSL_END
