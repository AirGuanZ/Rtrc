#pragma once

#include <vector>

#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Utils/Struct.h>

RTRC_BEGIN

enum class StructMemberType
{
    Float,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    UInt,
    UInt2,
    UInt3,
    UInt4,
    Struct,
    Array
};

struct ArrayInfo;
struct StructInfo;

struct TypeInfo
{
    StructMemberType  type;
    const StructInfo *structInfo;
    const ArrayInfo  *arrayInfo;
};

struct ArrayInfo
{
    int             arraySize;
    const TypeInfo *elementType;
};

struct MemberInfo
{
    std::string     name;
    const TypeInfo *type;
    size_t          offset;
};

struct StructInfo
{
    std::string             name;
    std::vector<MemberInfo> members;
};

#define RTRC_STRUCT_BEGIN(NAME)  \
    RTRC_META_STRUCT_BEGIN(NAME) \
    struct _rtrcStructTypeTag { };

#define RTRC_STRUCT_END() RTRC_META_STRUCT_END()

#define RTRC_STRUCT_MEMBER(TYPE, NAME)                  \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                   \
    {                                                   \
        f.template operator()(&_rtrcSelf::NAME, #NAME); \
    }                                                   \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                  \
    using _rtrcType##NAME = TYPE;                       \
    _rtrcType##NAME NAME;

#define $struct_begin RTRC_STRUCT_BEGIN
#define $struct_end   RTRC_STRUCT_END
#define $variable     RTRC_STRUCT_MEMBER

template<typename T>
concept RtrcStructType = requires { typename T::_rtrcStructTypeTag; };

template<RtrcStructType T>
const StructInfo *GetRtrcStructInfo();

template<typename T>
const TypeInfo *GetTypeInfo();

template<typename Element, int N>
const ArrayInfo *GetArrayInfo()
{
    static const ArrayInfo ret = {
        .arraySize = N,
        .elementType = GetTypeInfo<Element>()
    };
    return &ret;
}

template<StructMemberType ClosedType>
const TypeInfo *GetClosedTypeInfo()
{
    static const TypeInfo ret = {
        .type = ClosedType,
        .structInfo = nullptr,
        .arrayInfo = nullptr
    };
    return &ret;
}

template<RtrcStructType StructType>
const TypeInfo *GetStructTypeInfo()
{
    static const TypeInfo ret = {
        .type = StructMemberType::Struct,
        .structInfo = GetRtrcStructInfo<StructType>(),
        .arrayInfo = nullptr
    };
    return &ret;
}

template<typename T>
struct GetArrayTypeInfoAux { };

template<typename Element, int N>
struct GetArrayTypeInfoAux<Element[N]>
{
    static const TypeInfo *GetInfo()
    {
        static const TypeInfo ret = {
            .type = StructMemberType::Array,
            .structInfo = nullptr,
            .arrayInfo = GetArrayInfo<Element, N>()
        };
        return &ret;
    }
};

template<typename T>
const TypeInfo *GetArrayTypeInfo()
{
    return GetArrayTypeInfoAux<T>::GetInfo();
}

template<typename T>
const TypeInfo *GetTypeInfo()
{
#define ADD_CASE(TYPE, VAL)                                \
    if constexpr(std::is_same_v<T, TYPE>)                  \
    {                                                      \
        return GetClosedTypeInfo<StructMemberType::VAL>(); \
    }
    ADD_CASE(int32_t,  Int)    else
    ADD_CASE(uint32_t, UInt)   else
    ADD_CASE(float,    Float)  else
    ADD_CASE(Vector2f, Float2) else
    ADD_CASE(Vector3f, Float3) else
    ADD_CASE(Vector3f, Float4) else
    ADD_CASE(Vector2i, Int2)   else
    ADD_CASE(Vector3i, Int3)   else
    ADD_CASE(Vector4i, Int4)   else
    ADD_CASE(Vector2u, UInt2)  else
    ADD_CASE(Vector3u, UInt3)  else
    ADD_CASE(Vector4u, UInt4)
    else if constexpr(std::is_array_v<T>)
    {
        return GetArrayTypeInfo<T>();
    }
    else
    {
        return GetStructTypeInfo<T>();
    }
#undef ADD_CASE
}

template<RtrcStructType T>
const StructInfo *GetRtrcStructInfo()
{
    static const StructInfo ret = []
    {
        StructInfo info;
        info.name = T::_rtrcSelfName;
        T::ForEachMember([&]<typename Member>(Member T:: *ptr, const char *name)
        {
            info.members.push_back(MemberInfo{
                .name       = name,
                .type       = GetTypeInfo<Member>(),
                .offset     = GetMemberOffset(ptr),
            });
        });
        return info;
    }();
    return &ret;
}

RTRC_END
