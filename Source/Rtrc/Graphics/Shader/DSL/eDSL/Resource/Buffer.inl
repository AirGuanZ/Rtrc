#pragma once

#include "../RecordContext.h"
#include "Buffer.h"

RTRC_EDSL_BEGIN

template <typename T, TemplateBufferType Type>
const char* TemplateBuffer<T, Type>::GetStaticTypeName()
{
    if constexpr(std::is_same_v<T, void>)
    {
        return GetBasicTypeName();
    }
    else
    {
        static const std::string ret = []
        {
            const char *basic = GetBasicTypeName();
            const char *element = T::GetStaticTypeName();
            return fmt::format("{}<{}>", basic, element);
        }();
        return ret.data();
    }
}

template <typename T, TemplateBufferType Type>
TemplateBuffer<T, Type> TemplateBuffer<T, Type>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    TemplateBuffer ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T, TemplateBufferType Type>
T TemplateBuffer<T, Type>::operator[](const u32 &index) const
{
    static_assert(
        Type == TemplateBufferType::Buffer || Type == TemplateBufferType::StructuredBuffer ||
        Type == TemplateBufferType::RWBuffer || Type == TemplateBufferType::RWStructuredBuffer,
        "'operator[]' can only be called with (RW)(Structured)Buffer");

    std::string name = fmt::format("{}[{}]", this->eVariable_GetFullName(), CompileAsIdentifier(index));
    return CreateTemporaryVariableForExpression<T>(std::move(name));
}

template <typename T, TemplateBufferType Type>
template <typename S>
const S TemplateBuffer<T, Type>::Load(const u32 &offsetInBytes) const
{
    static_assert(
        Type == TemplateBufferType::ByteAddressBuffer || Type == TemplateBufferType::RWByteAddressBuffer,
        "'Load' can only be called with (RW)ByteAddressBuffer");
    const std::string idThis = this->eVariable_GetFullName();
    const std::string typeStr = S::GetStaticTypeName();
    const std::string idOffset = CompileAsIdentifier(offsetInBytes);
    std::string name = fmt::format("{}.Load<{}>({})", idThis, typeStr, idOffset);
    return CreateTemporaryVariableForExpression<S>(std::move(name));
}

template <typename T, TemplateBufferType Type>
template <typename S>
void TemplateBuffer<T, Type>::Store(const u32 &offsetInBytes, const S &value) const
{
    static_assert(Type == TemplateBufferType::RWByteAddressBuffer, "'Store' can only be called with RWByteAddressBuffer");
    const std::string idThis = this->eVariable_GetFullName();
    const std::string typeStr = S::GetStaticTypeName();
    const std::string idOffset = CompileAsIdentifier(offsetInBytes);
    const std::string idValue = CompileAsIdentifier(value);
    GetCurrentRecordContext().AppendLine("{}.Store<{}>({}, {});", idThis, typeStr, idOffset, idValue);
}

template <typename T, TemplateBufferType Type>
constexpr const char* TemplateBuffer<T, Type>::GetBasicTypeName()
{
    switch(Type)
    {
    case TemplateBufferType::Buffer:              return "Buffer";
    case TemplateBufferType::StructuredBuffer:    return "StructuredBuffer";
    case TemplateBufferType::ByteAddressBuffer:   return "ByteAddressBuffer";
    case TemplateBufferType::RWBuffer:            return "RWBuffer";
    case TemplateBufferType::RWStructuredBuffer:  return "RWStructuredBuffer";
    case TemplateBufferType::RWByteAddressBuffer: return "RWByteAddressBuffer";
    }
    return "UnknownBufferType";
}

RTRC_EDSL_END
