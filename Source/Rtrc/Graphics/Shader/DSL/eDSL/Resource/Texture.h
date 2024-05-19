#pragma once

#include <Rtrc/Core/TypeList.h>

#include "../Value/eVector4.h"

RTRC_EDSL_BEGIN

template<typename T, bool IsRW>
class TemplateTexture2D : public eVariable<TemplateTexture2D<T, IsRW>>
{
public:

    static_assert(
        TypeList<
            f32, float2, float3, float3,
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

template<typename T>
using Texture2D = TemplateTexture2D<T, false>;
template<typename T>
using RWTexture2D = TemplateTexture2D<T, true>;

#define RTRC_EDSL_DEFINE_TEXTURE(TYPE, NAME) RTRC_EDSL_DEFINE_TEXTURE_IMPL(TYPE, NAME)
#define RTRC_EDSL_DEFINE_TEXTURE_IMPL(TYPE, NAME) TYPE NAME = TYPE::CreateFromName(#NAME)

RTRC_EDSL_END
