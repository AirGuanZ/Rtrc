#pragma once

#include <Rtrc/Core/ScopeGuard.h>

#include "While.h"
#include "../RecordContext.h"

RTRC_EDSL_BEGIN

template <typename CondFunc>
WhileBuilder::WhileBuilder(CondFunc condFunc)
    : conditionFunc(std::move(condFunc))
{
    
}

template <typename BodyFunc>
void WhileBuilder::operator+(BodyFunc bodyFunc)
{
    auto &context = GetCurrentRecordContext();
    context.AppendLine("while(true)");
    context.BeginScope();
    RTRC_SCOPE_EXIT{ context.EndScope(); };

    {
        context.BeginScope();
        RTRC_SCOPE_EXIT{ context.EndScope(); };
        eNumber<bool> condVar = std::invoke(std::move(conditionFunc));
        context.AppendLine("if({}) break;", condVar.Compile());
    }

    if constexpr(std::is_same_v<std::invoke_result_t<BodyFunc>, void>)
    {
        std::invoke(std::move(bodyFunc));
    }
    else
    {
        auto ret = std::invoke(std::move(bodyFunc));
        context.AppendLine("return {};", eDSL::CompileAsIdentifier(ret));
    }
}

RTRC_EDSL_END
