#pragma once

#include "eTexture.h"

RTRC_EDSL_BEGIN

template <typename T, bool IsRW>
const char *TemplateTexture1D<T, IsRW>::GetStaticTypeName()
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
TemplateTexture1D<T, IsRW> TemplateTexture1D<T, IsRW>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    TemplateTexture1D ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T, bool IsRW>
TemporaryValueWrapper<T> TemplateTexture1D<T, IsRW>::operator[](const u32x2 &coord) const
{
    std::string name = fmt::format("{}[{}]", this->eVariable_GetFullName(), CompileAsIdentifier(coord));
    return CreateTemporaryVariableForExpression<T>(std::move(name));
}

template <typename T, bool IsRW>
constexpr const char *TemplateTexture1D<T, IsRW>::GetBasicTypeName()
{
    return IsRW ? "RWTexture1D" : "Texture1D";
}

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
TemporaryValueWrapper<T> TemplateTexture2D<T, IsRW>::operator[](const u32x2& coord) const
{
    std::string name = fmt::format("{}[{}]", this->eVariable_GetFullName(), CompileAsIdentifier(coord));
    return CreateTemporaryVariableForExpression<T>(std::move(name));
}

template <typename T, bool IsRW>
constexpr const char* TemplateTexture2D<T, IsRW>::GetBasicTypeName()
{
    return IsRW ? "RWTexture2D" : "Texture2D";
}

template <typename T, bool IsRW>
const char *TemplateTexture3D<T, IsRW>::GetStaticTypeName()
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
TemplateTexture3D<T, IsRW> TemplateTexture3D<T, IsRW>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    TemplateTexture3D ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T, bool IsRW>
TemporaryValueWrapper<T> TemplateTexture3D<T, IsRW>::operator[](const u32x2 &coord) const
{
    std::string name = fmt::format("{}[{}]", this->eVariable_GetFullName(), CompileAsIdentifier(coord));
    return CreateTemporaryVariableForExpression<T>(std::move(name));
}

template <typename T, bool IsRW>
constexpr const char *TemplateTexture3D<T, IsRW>::GetBasicTypeName()
{
    return IsRW ? "RWTexture3D" : "Texture3D";
}

template <typename T, bool IsRW>
const char *TemplateTexture1DArray<T, IsRW>::GetStaticTypeName()
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
TemplateTexture1DArray<T, IsRW> TemplateTexture1DArray<T, IsRW>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    TemplateTexture1DArray ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T, bool IsRW>
std::conditional_t<IsRW, T, const T> TemplateTexture1DArray<T, IsRW>::operator[](const u32x2 &coord) const
{
    std::string name = fmt::format("{}[{}]", this->eVariable_GetFullName(), CompileAsIdentifier(coord));
    return CreateTemporaryVariableForExpression<T>(std::move(name));
}

template <typename T, bool IsRW>
constexpr const char *TemplateTexture1DArray<T, IsRW>::GetBasicTypeName()
{
    return IsRW ? "RWTexture1DArray" : "Texture1DArray";
}

template <typename T, bool IsRW>
const char* TemplateTexture2DArray<T, IsRW>::GetStaticTypeName()
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
TemplateTexture2DArray<T, IsRW> TemplateTexture2DArray<T, IsRW>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    TemplateTexture2DArray ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T, bool IsRW>
std::conditional_t<IsRW, T, const T> TemplateTexture2DArray<T, IsRW>::operator[](const u32x2& coord) const
{
    std::string name = fmt::format("{}[{}]", this->eVariable_GetFullName(), CompileAsIdentifier(coord));
    return CreateTemporaryVariableForExpression<T>(std::move(name));
}

template <typename T, bool IsRW>
constexpr const char* TemplateTexture2DArray<T, IsRW>::GetBasicTypeName()
{
    return IsRW ? "RWTexture2DArray" : "Texture2DArray";
}

template <typename T, bool IsRW>
const char *TemplateTexture3DArray<T, IsRW>::GetStaticTypeName()
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
TemplateTexture3DArray<T, IsRW> TemplateTexture3DArray<T, IsRW>::CreateFromName(std::string name)
{
    DisableStackVariableAllocation();
    TemplateTexture3DArray ret;
    ret.eVariableName = std::move(name);
    EnableStackVariableAllocation();
    return ret;
}

template <typename T, bool IsRW>
std::conditional_t<IsRW, T, const T> TemplateTexture3DArray<T, IsRW>::operator[](const u32x2 &coord) const
{
    std::string name = fmt::format("{}[{}]", this->eVariable_GetFullName(), CompileAsIdentifier(coord));
    return CreateTemporaryVariableForExpression<T>(std::move(name));
}

template <typename T, bool IsRW>
constexpr const char *TemplateTexture3DArray<T, IsRW>::GetBasicTypeName()
{
    return IsRW ? "RWTexture3DArray" : "Texture3DArray";
}

RTRC_EDSL_END
