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
        f32, f32x2, f32x3, f32x4,
        u32, u32x2, u32x3, u32x4,
        i32, i32x2, i32x3, i32x4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture1D CreateFromName(std::string name);

    TemplateTexture1D() { PopConstructParentVariable(); }
    TemplateTexture1D(const TemplateTexture1D &other)
        : eVariable<TemplateTexture1D>(other)
    {
        PopConstructParentVariable();
    }

    TemplateTexture1D &operator=(const TemplateTexture1D &other)
    {
        static_cast<eVariable<TemplateTexture1D> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    TemporaryValueWrapper<T> operator[](const u32x2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture2D : public eVariable<TemplateTexture2D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        f32, f32x2, f32x3, f32x4,
        u32, u32x2, u32x3, u32x4,
        i32, i32x2, i32x3, i32x4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture2D CreateFromName(std::string name);

    TemplateTexture2D() { PopConstructParentVariable(); }
    TemplateTexture2D(const TemplateTexture2D &other)
        : eVariable<TemplateTexture2D>(other)
    {
        PopConstructParentVariable();
    }

    TemplateTexture2D &operator=(const TemplateTexture2D &other)
    {
        static_cast<eVariable<TemplateTexture2D> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    TemporaryValueWrapper<T> operator[](const u32x2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture3D : public eVariable<TemplateTexture3D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        f32, f32x2, f32x3, f32x4,
        u32, u32x2, u32x3, u32x4,
        i32, i32x2, i32x3, i32x4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture3D CreateFromName(std::string name);

    TemplateTexture3D() { PopConstructParentVariable(); }
    TemplateTexture3D(const TemplateTexture3D &other)
        : eVariable<TemplateTexture3D>(other)
    {
        PopConstructParentVariable();
    }

    TemplateTexture3D &operator=(const TemplateTexture3D &other)
    {
        static_cast<eVariable<TemplateTexture3D> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    TemporaryValueWrapper<T> operator[](const u32x2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture1DArray : public eVariable<TemplateTexture1D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        f32, f32x2, f32x3, f32x4,
        u32, u32x2, u32x3, u32x4,
        i32, i32x2, i32x3, i32x4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture1DArray CreateFromName(std::string name);

    TemplateTexture1DArray() { PopConstructParentVariable(); }
    TemplateTexture1DArray(const TemplateTexture1DArray &other)
        : eVariable<TemplateTexture1DArray>(other)
    {
        PopConstructParentVariable();
    }

    TemplateTexture1DArray &operator=(const TemplateTexture1DArray &other)
    {
        static_cast<eVariable<TemplateTexture1DArray> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const u32x2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture2DArray : public eVariable<TemplateTexture2D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        f32, f32x2, f32x3, f32x4,
        u32, u32x2, u32x3, u32x4,
        i32, i32x2, i32x3, i32x4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture2DArray CreateFromName(std::string name);

    TemplateTexture2DArray() { PopConstructParentVariable(); }
    TemplateTexture2DArray(const TemplateTexture2DArray &other)
        : eVariable<TemplateTexture2DArray>(other)
    {
        PopConstructParentVariable();
    }

    TemplateTexture2DArray &operator=(const TemplateTexture2DArray &other)
    {
        static_cast<eVariable<TemplateTexture2DArray> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const u32x2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T, bool IsRW>
class TemplateTexture3DArray : public eVariable<TemplateTexture3D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
        f32, f32x2, f32x3, f32x4,
        u32, u32x2, u32x3, u32x4,
        i32, i32x2, i32x3, i32x4>::Contains<T>);

    static const char *GetStaticTypeName();

    static TemplateTexture3DArray CreateFromName(std::string name);

    TemplateTexture3DArray() { PopConstructParentVariable(); }
    TemplateTexture3DArray(const TemplateTexture3DArray &other)
        : eVariable<TemplateTexture3DArray>(other)
    {
        PopConstructParentVariable();
    }

    TemplateTexture3DArray &operator=(const TemplateTexture3DArray &other)
    {
        static_cast<eVariable<TemplateTexture3DArray> &>(*this) = other;
        PopCopyParentVariable();
        return *this;
    }

    std::conditional_t<IsRW, T, const T> operator[](const u32x2 &coord) const;

private:

    static constexpr const char *GetBasicTypeName();
};

template<typename T> using eTexture1D = TemplateTexture1D<T, false>;
template<typename T> using eTexture2D = TemplateTexture2D<T, false>;
template<typename T> using eTexture3D = TemplateTexture3D<T, false>;

template<typename T> using eRWTexture1D = TemplateTexture1D<T, true>;
template<typename T> using eRWTexture2D = TemplateTexture2D<T, true>;
template<typename T> using eRWTexture3D = TemplateTexture3D<T, true>;

template<typename T> using eTexture1DArray = TemplateTexture1DArray<T, false>;
template<typename T> using eTexture2DArray = TemplateTexture2DArray<T, false>;
template<typename T> using eTexture3DArray = TemplateTexture3DArray<T, false>;

template<typename T> using eRWTexture1DArray = TemplateTexture1DArray<T, true>;
template<typename T> using eRWTexture2DArray = TemplateTexture2DArray<T, true>;
template<typename T> using eRWTexture3DArray = TemplateTexture3DArray<T, true>;

RTRC_EDSL_END
