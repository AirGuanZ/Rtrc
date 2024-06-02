#pragma once

#include <Rtrc/Core/TemplateStringParameter.h>

#include "Common.h"

RTRC_EDSL_BEGIN

struct eVariableInitializeAsMemberTagType { };

inline constexpr eVariableInitializeAsMemberTagType eVariableInitializeAsMemberTag;

struct eVariableCommonBase
{
    eVariableCommonBase *eVariableParent = nullptr;
    std::string eVariableName;
    std::string eVariable_GetFullName() const;
};

void PushMemberVariableName(std::string name);
std::string PopMemberVariableName();

// Called when constructing a new eVariable.
// Members of the var will use the pushed parent pointer to record the parent-children relationship.
void PushConstructParentVariable(eVariableCommonBase *var);
// For each class inherited from eVariable, call this after the last eVariable-derived member is constructed.
eVariableCommonBase *PopConstructParentVariable();

// Similar to PushConstructParentVariable, but applies to assignment operator.
void PushCopyParentVariable(eVariableCommonBase *var);
// For each class inherited from eVariable,
// call this after the last eVariable-derived member is assigned in overloaded assignment.
eVariableCommonBase *PopCopyParentVariable();

// After calling this, automatic variable name allocation will be disabled.
// Newly constructed (top-most) variables will leave their names to be empty.
// Allocation is enabled by default. This function cannot be called when the allocation is already disabled.
void DisableStackVariableAllocation();
// Enable the automatic variable name allocation. This function cannot be called when the allocation is already enabled.
void EnableStackVariableAllocation();
bool IsStackVariableAllocationEnabled();

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

// When initializing variables via expressions that create temporary objects, RVO leads to unexpected naming issues.
// Consider the following example where 'CreateTemporaryVariableForExpression<T>' returns a temporary object of type T:
//
//     float4 foo() { return CreateTemporaryVariableForExpression<float4>("float4(1,2,3,4)"); }
//     ...
//     float4 a = foo();
//     a.x = 4;
//
// This results in:
//
//     float4(1,2,3,4).x = 4
//
// However, the expected behavior is:
//
//     float4 a = float4(1,2,3,4);
//     a.x = 4;
//
// To circumvent this issue, use 'TemporaryValueWrapper<T>' in place of 'T'.
template<typename T>
struct TemporaryValueWrapper : T
{
    template<typename U> requires requires { std::declval<T &>() = std::declval<std::remove_reference_t<U>>(); }
    TemporaryValueWrapper &operator=(U &&other)
    {
        static_cast<T &>(*this) = std::forward<U>(other);
        return *this;
    }
};

template<typename T> requires std::is_base_of_v<eVariableCommonBase, T>
TemporaryValueWrapper<T> CreateTemporaryVariableForExpression(std::string name);

RTRC_EDSL_END
