#pragma once

#include <Rtrc/Graphics/Shader/DSL/BindingGroup/BindingGroupHelpers.h>
#include <Rtrc/Graphics/Shader/DSL/eDSL.h>

RTRC_BEGIN

namespace NativeTypeMappingDetail
{

    template<typename T>
    struct Impl;

    template<RtrcGroupStruct T>
    struct Impl<T>
    {
        using Type = BindingGroupDetail::RebindSketchBuilder<T, BindingGroupDetail::BindingGroupBuilder_eDSL>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 1, false>>
    {
        using Type = eDSL::eTexture1D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 2, false>>
    {
        using Type = eDSL::eTexture2D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 3, false>>
    {
        using Type = eDSL::eTexture3D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 1, true>>
    {
        using Type = eDSL::eTexture1DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 2, true>>
    {
        using Type = eDSL::eTexture2DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 3, true>>
    {
        using Type = eDSL::eTexture3DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 1, false>>
    {
        using Type = eDSL::eRWTexture1D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 2, false>>
    {
        using Type = eDSL::eRWTexture2D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 3, false>>
    {
        using Type = eDSL::eRWTexture3D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 1, true>>
    {
        using Type = eDSL::eRWTexture1DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 2, true>>
    {
        using Type = eDSL::eRWTexture2DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 3, true>>
    {
        using Type = eDSL::eRWTexture3DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Buffer<T>>
    {
        using Type = eDSL::eBuffer<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWBuffer<T>>
    {
        using Type = eDSL::eRWBuffer<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_StructuredBuffer<T>>
    {
        using Type = eDSL::eStructuredBuffer<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWStructuredBuffer<T>>
    {
        using Type = eDSL::eRWStructuredBuffer<T>;
    };

    template<>
    struct Impl<BindingGroupDetail::MemberProxy_ByteAddressBuffer>
    {
        using Type = eDSL::eByteAddressBuffer;
    };

    template<>
    struct Impl<BindingGroupDetail::MemberProxy_RWByteAddressBuffer>
    {
        using Type = eDSL::eRWByteAddressBuffer;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_ConstantBuffer<T>>
    {
        using Element = typename Impl<T>::Type;
        using Type = eDSL::eConstantBuffer<Element>;
    };

    template<>
    struct Impl<BindingGroupDetail::MemberProxy_SamplerState>
    {
        using Type = eDSL::eSamplerState;
    };

    template<>
    struct Impl<BindingGroupDetail::MemberProxy_RaytracingAccelerationStructure>
    {
        using Type = eDSL::eRaytracingAccelerationStructure;
    };

} // namespace NativeTypeMappingDetail

namespace BindingGroupDetail
{

    struct BindingGroupBuilder_eDSL
    {
        struct Base
        {
            struct _rtrcDSLGroupTypeFlag {};
        };

        template<typename T>
        using Member = eDSL::NativeTypeToDSLType<T>;
    };

} // namespace BindingGroupDetail

RTRC_END
