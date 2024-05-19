#pragma once

#include <concepts>

#include <Rtrc/Core/Struct.h>

#include "Common.h"

template<typename T>
class Vector2;
template<typename T>
class Vector3;
template<typename T>
class Vector4;

RTRC_EDSL_BEGIN

template<typename T>
struct eNumber;
template<typename T>
struct eVector2;
template<typename T>
struct eVector3;
template<typename T>
struct eVector4;

namespace eDSLStructDetail
{

    struct DSLStructBuilder;

} // namespace eDSLStructDetail

namespace NativeTypeMappingDetail
{

    template<typename T>
    struct Impl;

    template<std::integral T>
    struct Impl<T>
    {
        using Type = eNumber<T>;
    };

    template<std::floating_point T>
    struct Impl<T>
    {
        using Type = eNumber<T>;
    };

    template<typename T>
    struct Impl<Vector2<T>>
    {
        using Type = eVector2<T>;
    };

    template<typename T>
    struct Impl<Vector3<T>>
    {
        using Type = eVector3<T>;
    };

    template<typename T>
    struct Impl<Vector4<T>>
    {
        using Type = eVector4<T>;
    };

    template<RtrcStruct T>
    struct Impl<T>
    {
        using Type = StructDetail::RebindSketchBuilder<T, eDSLStructDetail::DSLStructBuilder>;
    };

} // namespace NativeTypeMappingDetail

template<typename T>
using NativeTypeToDSLType = typename NativeTypeMappingDetail::Impl<T>::Type;

RTRC_EDSL_END
