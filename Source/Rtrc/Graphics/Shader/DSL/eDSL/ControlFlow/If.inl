#pragma once

#include <Rtrc/Core/ScopeGuard.h>

#include "../RecordContext.h"
#include "If.h"

RTRC_EDSL_BEGIN

inline ElseBuilder::ElseBuilder(const eNumber<bool>& condition, std::move_only_function<void()> inThenBodyFunc)
    : condition(condition), thenBodyFunc(std::move(inThenBodyFunc))
{
    
}

inline ElseBuilder::~ElseBuilder()
{
    auto &context = GetCurrentRecordContext();
    context.AppendLine("if({})", condition.Compile());
    {
        context.BeginScope();
        RTRC_SCOPE_EXIT{ context.EndScope(); };
        thenBodyFunc();
    }
    if(elseBodyFunc)
    {
        context.AppendLine("else");
        context.BeginScope();
        RTRC_SCOPE_EXIT{ context.EndScope(); };
        elseBodyFunc();
    }
}

template <typename ElseBodyFunc>
void ElseBuilder::operator-(ElseBodyFunc inElseBodyFunc)
{
    elseBodyFunc = std::move(inElseBodyFunc);
}

inline IfBuilder::IfBuilder(const eNumber<bool>& condition)
    : condition(condition)
{
    
}

template <typename ThenBodyFunc>
ElseBuilder IfBuilder::operator+(ThenBodyFunc thenBodyFunc)
{
    return ElseBuilder(condition, std::move(thenBodyFunc));
}

RTRC_EDSL_END
