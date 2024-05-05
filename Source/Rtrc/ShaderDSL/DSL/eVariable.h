#pragma once

#include <Rtrc/ShaderDSL/Common.h>

RTRC_EDSL_BEGIN

struct eVariableInitializeAsMemberTagType { };

inline constexpr eVariableInitializeAsMemberTagType eVariableInitializeAsMemberTag;

struct eVariableCommonBase
{
    eVariableCommonBase *eVariableParent;
    std::string eVariableName;
    std::string eVariable_GetFullName() const;
};

void PushMemberVariableName(std::string name);
std::string PopMemberVariableName();

void PushParentVariable(eVariableCommonBase *var);
eVariableCommonBase *PopParentVariable();

void DisableStackVariableAllocation();
void EnableStackVariableAllocation();

template<TemplateStringParameter Str>
struct MemberVariableNameInitializer
{
    MemberVariableNameInitializer() { PushMemberVariableName(static_cast<const char *>(Str)); }
    MemberVariableNameInitializer(const MemberVariableNameInitializer &) : MemberVariableNameInitializer() { }
};

template<typename T>
struct eVariable : eVariableCommonBase
{
    eVariable();
    eVariable(const eVariable &) : eVariable() { }
    eVariable &operator=(const eVariable &) { return *this; }
    bool eVariable_HasParent() const { return eVariableParent != nullptr; }
};

template<typename T> requires std::is_base_of_v<eVariableCommonBase, T>
std::string CompileAsIdentifier(const T &var);

RTRC_EDSL_END
