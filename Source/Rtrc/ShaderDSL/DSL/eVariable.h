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

// Called when constructing a new eVariable.
// Members of the var will use the pushed parent pointer to record the parent-children relationship.
void PushParentVariable(eVariableCommonBase *var);
// For each class inherited from eVariable, call this after the last eVariable-derived member is constructed.
eVariableCommonBase *PopParentVariable();

// After calling this, automatic variable name allocation will be disabled.
// Newly constructed (top-most) variables will leave their names to be empty.
// Allocation is enabled by default. This function cannot be called when the allocation is already disabled.
void DisableStackVariableAllocation();
// Enable the automatic variable name allocation. This function cannot be called when the allocation is already enabled.
void EnableStackVariableAllocation();

// Helper class calling PushMemberVariableName(Str) when constructed.
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
    eVariable(const eVariable &other) : eVariable() { *this = other; }
    eVariable &operator=(const eVariable &other);
    bool eVariable_HasParent() const { return eVariableParent != nullptr; }
};

template<typename T> requires std::is_base_of_v<eVariableCommonBase, T>
std::string CompileAsIdentifier(const T &var);

template<typename T> requires std::is_base_of_v<eVariableCommonBase, T>
T CreateTemporaryVariableForExpression(std::string name);

RTRC_EDSL_END
