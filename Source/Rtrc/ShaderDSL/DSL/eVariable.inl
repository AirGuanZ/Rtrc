#pragma once

#include <stack>

#include <Rtrc/ShaderDSL/DSL/eVariable.h>

RTRC_EDSL_BEGIN

namespace Detail
{

    inline thread_local std::stack<std::string> gMemberVariableNameStack;
    inline thread_local std::stack<eVariableCommonBase *> gVariableInitializationStack;
    inline thread_local bool gEnableStackVariableAllocation = true;

} // namespace Detail

inline std::string eVariableCommonBase::eVariable_GetFullName() const
{
    if(eVariableParent)
    {
        return eVariableParent->eVariable_GetFullName() + "." + eVariableName;
    }
    return eVariableName;
}

inline void PushMemberVariableName(std::string name)
{
    Detail::gMemberVariableNameStack.push(std::move(name));
}

inline std::string PopMemberVariableName()
{
    std::string ret = Detail::gMemberVariableNameStack.top();
    Detail::gMemberVariableNameStack.pop();
    return ret;
}

inline void PushParentVariable(eVariableCommonBase *var)
{
    Detail::gVariableInitializationStack.push(var);
}

inline eVariableCommonBase *PopParentVariable()
{
    auto ret = Detail::gVariableInitializationStack.top();
    Detail::gVariableInitializationStack.pop();
    return ret;
}

inline void DisableStackVariableAllocation()
{
    assert(Detail::gEnableStackVariableAllocation);
    Detail::gEnableStackVariableAllocation = false;
}

inline void EnableStackVariableAllocation()
{
    assert(!Detail::gEnableStackVariableAllocation);
    Detail::gEnableStackVariableAllocation = true;
}

template<typename T>
eVariable<T>::eVariable()
{
    if(!Detail::gVariableInitializationStack.empty())
    {
        eVariableParent = Detail::gVariableInitializationStack.top();
        eVariableName = PopMemberVariableName();
    }
    else
    {
        eVariableParent = nullptr;
        if(Detail::gEnableStackVariableAllocation)
        {
            eVariableName = GetCurrentRecordContext().AllocateVariable();
            const char *type = T::GetStaticTypeName();
            GetCurrentRecordContext().AppendLine("{} {};", type, eVariableName);
        }
    }
    eDSL::PushParentVariable(this);
}

template<typename T> requires std::is_base_of_v<eVariableCommonBase, T>
std::string CompileAsIdentifier(const T &var)
{
    if constexpr(requires{ var.Compile(); })
    {
        return var.Compile();
    }
    else
    {
        return var.eVariable_GetFullName();
    }
}

RTRC_EDSL_END
