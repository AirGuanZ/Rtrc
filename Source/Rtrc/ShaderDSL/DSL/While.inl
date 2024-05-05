#pragma once

#include <Rtrc/Core/ScopeGuard.h>
#include <Rtrc/ShaderDSL/DSL/While.h>
#include <Rtrc/ShaderDSL/DSL/RecordContext.h>

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

    std::invoke(std::move(bodyFunc));
}

RTRC_EDSL_END
