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
        using Type = eDSL::Texture1D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 2, false>>
    {
        using Type = eDSL::Texture2D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 3, false>>
    {
        using Type = eDSL::Texture3D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 1, true>>
    {
        using Type = eDSL::Texture1DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 2, true>>
    {
        using Type = eDSL::Texture2DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Texture<T, 3, true>>
    {
        using Type = eDSL::Texture3DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 1, false>>
    {
        using Type = eDSL::RWTexture1D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 2, false>>
    {
        using Type = eDSL::RWTexture2D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 3, false>>
    {
        using Type = eDSL::RWTexture3D<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 1, true>>
    {
        using Type = eDSL::RWTexture1DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 2, true>>
    {
        using Type = eDSL::RWTexture2DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWTexture<T, 3, true>>
    {
        using Type = eDSL::RWTexture3DArray<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_Buffer<T>>
    {
        using Type = eDSL::Buffer<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWBuffer<T>>
    {
        using Type = eDSL::RWBuffer<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_StructuredBuffer<T>>
    {
        using Type = eDSL::StructuredBuffer<T>;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_RWStructuredBuffer<T>>
    {
        using Type = eDSL::RWStructuredBuffer<T>;
    };

    template<>
    struct Impl<BindingGroupDetail::MemberProxy_ByteAddressBuffer>
    {
        using Type = eDSL::ByteAddressBuffer;
    };

    template<>
    struct Impl<BindingGroupDetail::MemberProxy_RWByteAddressBuffer>
    {
        using Type = eDSL::RWByteAddressBuffer;
    };

    template<typename T>
    struct Impl<BindingGroupDetail::MemberProxy_ConstantBuffer<T>>
    {
        using Element = typename Impl<T>::Type;
        using Type = eDSL::ConstantBuffer<Element>;
    };

    template<>
    struct Impl<BindingGroupDetail::MemberProxy_SamplerState>
    {
        using Type = eDSL::SamplerState;
    };

    template<>
    struct Impl<BindingGroupDetail::MemberProxy_RaytracingAccelerationStructure>
    {
        using Type = eDSL::RaytracingAccelerationStructure;
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
