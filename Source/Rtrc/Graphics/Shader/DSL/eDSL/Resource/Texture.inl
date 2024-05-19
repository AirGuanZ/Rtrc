#pragma once

#include "Texture.h"

RTRC_EDSL_BEGIN

template <typename T, bool IsRW>
const char* TemplateTexture2D<T, IsRW>::GetStaticTypeName()
{
    static const std::string ret = []
    {
        const char *basic = GetBasicTypeName();
        const char *element = T::GetStaticTypeName();
        return fmt::format("{}<{}>", basic, element);
    }();
    return ret.data();
}

template <typename T, bool IsRW>
TemplateTexture2D<T, IsRW> TemplateTexture2D<T, IsRW>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    TemplateTexture2D ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T, bool IsRW>
std::conditional_t<IsRW, T, const T> TemplateTexture2D<T, IsRW>::operator[](const uint2& coord) const
{
    std::string name = fmt::format("{}[{}]", this->eVariable_GetFullName(), CompileAsIdentifier(coord));
    return CreateTemporaryVariableForExpression<T>(std::move(name));
}

template <typename T, bool IsRW>
constexpr const char* TemplateTexture2D<T, IsRW>::GetBasicTypeName()
{
    return IsRW ? "RWTexture2D" : "Texture2D";
}

RTRC_EDSL_END
