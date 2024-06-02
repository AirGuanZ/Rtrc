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
    if constexpr(std::is_same_v<std::invoke_result_t<ElseBodyFunc>, void>)
    {
        elseBodyFunc = std::move(inElseBodyFunc);
    }
    else
    {
        auto func = [inElseBodyFunc = std::move(inElseBodyFunc)]
        {
            auto ret = inElseBodyFunc();
            GetCurrentRecordContext().AppendLine("return {};", eDSL::CompileAsIdentifier(ret));
        };
        elseBodyFunc = std::move(func);
    }
}

inline IfBuilder::IfBuilder(const eNumber<bool>& condition)
    : condition(condition)
{
    
}

template <typename ThenBodyFunc>
ElseBuilder IfBuilder::operator+(ThenBodyFunc thenBodyFunc)
{
    if constexpr(std::is_same_v<std::invoke_result_t<ThenBodyFunc>, void>)
    {
        return ElseBuilder(condition, std::move(thenBodyFunc));
    }
    else
    {
        auto func = [thenBodyFunc = std::move(thenBodyFunc)]
        {
            auto ret = thenBodyFunc();
            GetCurrentRecordContext().AppendLine("return {};", eDSL::CompileAsIdentifier(ret));
        };
        return ElseBuilder(condition, std::move(func));
    }
}

RTRC_EDSL_END
