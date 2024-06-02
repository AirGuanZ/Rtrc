#pragma once

#include <Rtrc/Graphics/Shader/DSL/BindingGroup/ResourceProxy.h>

RTRC_EDSL_BEGIN

namespace ArgumentWrapper
{

    template<typename T>
    using BufferProxy = BindingGroupDetail::MemberProxy_Buffer<T>;
    template<typename T>
    using RWBufferProxy = BindingGroupDetail::MemberProxy_RWBuffer<T>;

    template<typename T>
    using StructuredBufferProxy = BindingGroupDetail::MemberProxy_StructuredBuffer<T>;
    template<typename T>
    using RWStructuredBufferProxy = BindingGroupDetail::MemberProxy_RWStructuredBuffer<T>;

    using ByteAddressBufferProxy = BindingGroupDetail::MemberProxy_ByteAddressBuffer;
    using RWByteAddressBufferProxy = BindingGroupDetail::MemberProxy_RWByteAddressBuffer;

    template<typename T>
    using ConstantBufferProxy = BindingGroupDetail::MemberProxy_ConstantBuffer<T>;

    using RaytracingAccelerationStructureProxy = BindingGroupDetail::MemberProxy_RaytracingAccelerationStructure;

    using SamplerStateProxy = BindingGroupDetail::MemberProxy_SamplerState;

    template<typename T>
    using Texture1DProxy = BindingGroupDetail::MemberProxy_Texture1D<T>;
    template<typename T>
    using Texture2DProxy = BindingGroupDetail::MemberProxy_Texture2D<T>;
    template<typename T>
    using Texture3DProxy = BindingGroupDetail::MemberProxy_Texture3D<T>;

    template<typename T>
    using Texture1DArrayProxy = BindingGroupDetail::MemberProxy_Texture1DArray<T>;
    template<typename T>
    using Texture2DArrayProxy = BindingGroupDetail::MemberProxy_Texture2DArray<T>;
    template<typename T>
    using Texture3DArrayProxy = BindingGroupDetail::MemberProxy_Texture3DArray<T>;

    template<typename T>
    using RWTexture1DProxy = BindingGroupDetail::MemberProxy_RWTexture1D<T>;
    template<typename T>
    using RWTexture2DProxy = BindingGroupDetail::MemberProxy_RWTexture2D<T>;
    template<typename T>
    using RWTexture3DProxy = BindingGroupDetail::MemberProxy_RWTexture3D<T>;

    template<typename T>
    using RWTexture1DArrayProxy = BindingGroupDetail::MemberProxy_RWTexture1DArray<T>;
    template<typename T>
    using RWTexture2DArrayProxy = BindingGroupDetail::MemberProxy_RWTexture2DArray<T>;
    template<typename T>
    using RWTexture3DArrayProxy = BindingGroupDetail::MemberProxy_RWTexture3DArray<T>;

    template<typename T>
    struct ResourceArgumentWrapper
    {
        using Proxy = T;

        Proxy proxy;

        ResourceArgumentWrapper() = default;

        template<typename U>
            requires requires { std::declval<Proxy &>() = std::declval<std::remove_reference_t<U>>(); }
        ResourceArgumentWrapper(U &&value)
        {
            proxy = std::forward<U>(value);
        }

        auto GetRtrcObject() const
        {
            return proxy.GetRtrcObject();
        }

        void DeclareRenderGraphResourceUsage(RGPass pass, RHI::PipelineStageFlag stages) const
        {
            if constexpr(requires{ proxy.DeclareRenderGraphResourceUsage(pass, stages); })
            {
                proxy.DeclareRenderGraphResourceUsage(pass, stages);
            }
        }
    };

} // namespace ArgumentWrapper

RTRC_EDSL_END
