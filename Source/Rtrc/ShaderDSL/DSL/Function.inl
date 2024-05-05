#pragma once

#include <Rtrc/Core/String.h>
#include <Rtrc/ShaderDSL/DSL/eVariable.h>
#include <Rtrc/ShaderDSL/DSL/Function.h>
#include <Rtrc/ShaderDSL/DSL/RecordContext.h>

RTRC_EDSL_BEGIN

template <typename Ret, typename... Args>
Ret Function<Ret, Args...>::operator()(const std::remove_reference_t<Args>&... args) const
{
    std::vector<std::string> argNames;
    (argNames.push_back(args.Compile()), ...);
    const std::string argStr = Join(", ", argNames.begin(), argNames.end());

    if constexpr(std::is_same_v<Ret, void>)
    {
        GetCurrentRecordContext().AppendLine("{}({});", functionName_, argStr);
        return (void)0;
    }
    else
    {
        Ret ret;
        GetCurrentRecordContext().AppendLine("{} = {}({});", eDSL::CompileAsIdentifier(ret), functionName_, argStr);
        return ret;
    }
}

template <typename Ret, typename... Args>
Function<Ret, Args...>::Function(std::string name)
    : functionName_(std::move(name))
{
    
}

template <typename Ret, typename... Args>
Function<Ret, std::remove_reference_t<Args>...> FunctionBuilder::BuildFunction(
    std::function<Ret(Args...)> bodyFunc)
{
    RecordContext &context = GetCurrentRecordContext();

    // Head

    const std::string functionName = context.AllocateFunction();
    const char *retTypeName = "void";
    if constexpr(!std::is_same_v<Ret, void>)
    {
        retTypeName = Ret::GetStaticTypeName();
    }

    std::vector<const char *> argTypeNames;
    (argTypeNames.push_back(std::remove_reference_t<Args>::GetStaticTypeName()), ...);

    std::vector<bool> argIsRef;
    (argIsRef.push_back(std::is_reference_v<Args>), ...);

    std::string argStr;
    for(unsigned i = 0; i < argTypeNames.size(); ++i)
    {
        const char *qualifier = argIsRef[i] ? "inout" : "in";
        if(i == 0)
        {
            argStr.append(fmt::format("{} {} arg{}", qualifier, argTypeNames[i], i));
        }
        else
        {
            argStr.append(fmt::format(", {} {} arg{}", qualifier, argTypeNames[i], i));
        }
    }

    context.AppendLine("{} {}({})", retTypeName, functionName, argStr);

    context.BeginScope();
    RTRC_SCOPE_EXIT { context.EndScope(); };

    // Args

    DisableStackVariableAllocation();
    std::tuple<std::remove_reference_t<Args>...> args;
    EnableStackVariableAllocation();

    {
        uint32_t argIndex = 0;
        std::apply([&](auto&...arg) { ((arg.eVariableName = fmt::format("arg{}", argIndex++)), ...); }, args);
    }

    // Body

    if constexpr(std::is_same_v<Ret, void>)
    {
        std::apply(std::move(bodyFunc), args);
    }
    else
    {
        Ret ret = std::apply(std::move(bodyFunc), args);
        context.AppendLine("return {};", eDSL::CompileAsIdentifier(ret));
    }

    return Function<Ret, std::remove_reference_t<Args>...>(functionName);
}

RTRC_EDSL_END
