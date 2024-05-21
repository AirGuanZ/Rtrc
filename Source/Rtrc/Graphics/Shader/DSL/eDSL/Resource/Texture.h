#pragma once

#include <Rtrc/Core/TypeList.h>

#include "../Value/eVector4.h"

RTRC_EDSL_BEGIN

template<typename T, bool IsRW>
class TemplateTexture1D : public eVariable<TemplateTexture1D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        float, float2, float3, float3,
        u32, uint2, uint3, uint4,
        i32, int2, int3, int4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture1D CreateFromName(std::string name);

    TemplateTexture1D() { PopConstructParentVariable(); }

    TemplateTexture1D &operator=(const TemplateTexture1D &other)
    {
        static_cast<eVariable<TemplateTexture1D> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const uint2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture2D : public eVariable<TemplateTexture2D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
            float, float2, float3, float3,
            u32, uint2, uint3, uint4,
            i32, int2, int3, int4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture2D CreateFromName(std::string name);

    TemplateTexture2D() { PopConstructParentVariable(); }

    TemplateTexture2D &operator=(const TemplateTexture2D &other)
    {
        static_cast<eVariable<TemplateTexture2D> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const uint2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture3D : public eVariable<TemplateTexture3D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        float, float2, float3, float3,
        u32, uint2, uint3, uint4,
        i32, int2, int3, int4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture3D CreateFromName(std::string name);

    TemplateTexture3D() { PopConstructParentVariable(); }

    TemplateTexture3D &operator=(const TemplateTexture3D &other)
    {
        static_cast<eVariable<TemplateTexture3D> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const uint2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture1DArray : public eVariable<TemplateTexture1D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        float, float2, float3, float3,
        u32, uint2, uint3, uint4,
        i32, int2, int3, int4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture1DArray CreateFromName(std::string name);

    TemplateTexture1DArray() { PopConstructParentVariable(); }

    TemplateTexture1DArray &operator=(const TemplateTexture1DArray &other)
    {
        static_cast<eVariable<TemplateTexture1DArray> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const uint2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture2DArray : public eVariable<TemplateTexture2D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
            float, float2, float3, float3,
            u32, uint2, uint3, uint4,
            i32, int2, int3, int4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture2DArray CreateFromName(std::string name);

    TemplateTexture2DArray() { PopConstructParentVariable(); }

    TemplateTexture2DArray &operator=(const TemplateTexture2DArray &other)
    {
        static_cast<eVariable<TemplateTexture2DArray> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const uint2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture3DArray : public eVariable<TemplateTexture3D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        float, float2, float3, float3,
        u32, uint2, uint3, uint4,
        i32, int2, int3, int4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture3DArray CreateFromName(std::string name);

    TemplateTexture3DArray() { PopConstructParentVariable(); }

    TemplateTexture3DArray &operator=(const TemplateTexture3DArray &other)
    {
        static_cast<eVariable<TemplateTexture3DArray> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const uint2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T> using Texture1D = TemplateTexture1D<T, false>;
template<typename T> using Texture2D = TemplateTexture2D<T, false>;
template<typename T> using Texture3D = TemplateTexture3D<T, false>;

template<typename T> using RWTexture1D = TemplateTexture1D<T, true>;
template<typename T> using RWTexture2D = TemplateTexture2D<T, true>;
template<typename T> using RWTexture3D = TemplateTexture3D<T, true>;

template<typename T> using Texture1DArray = TemplateTexture1DArray<T, false>;
template<typename T> using Texture2DArray = TemplateTexture2DArray<T, false>;
template<typename T> using Texture3DArray = TemplateTexture3DArray<T, false>;

template<typename T> using RWTexture1DArray = TemplateTexture1DArray<T, true>;
template<typename T> using RWTexture2DArray = TemplateTexture2DArray<T, true>;
template<typename T> using RWTexture3DArray = TemplateTexture3DArray<T, true>;

#define RTRC_EDSL_DEFINE_TEXTURE(TYPE, NAME) RTRC_EDSL_DEFINE_TEXTURE_IMPL(TYPE, NAME)
#define RTRC_EDSL_DEFINE_TEXTURE_IMPL(TYPE, NAME) TYPE NAME = TYPE::CreateFromName(#NAME)

RTRC_EDSL_END
