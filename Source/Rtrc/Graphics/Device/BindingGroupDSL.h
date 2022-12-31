#pragma once

#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Buffer/DynamicBuffer.h>
#include <Rtrc/Graphics/Device/Texture/Texture.h>
#include <Rtrc/Math/Vector2.h>
#include <Rtrc/Math/Vector3.h>
#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Utility/Struct.h>
#include <Rtrc/Utility/MacroOverloading.h>

RTRC_BEGIN

class DynamicBuffer;
class Sampler;

namespace BindingGroupDSL
{

    template<typename T>
    struct MemberProxy_Texture2D
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Texture2D;
        TextureSRV _rtrcObj;

        auto &operator=(TextureSRV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const RC<Texture> &tex)
        {
            _rtrcObj = tex->CreateSRV(0, 0, 0);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_RWTexture2D
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWTexture2D;
        TextureUAV _rtrcObj;

        auto &operator=(TextureUAV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const RC<Texture> &tex)
        {
            _rtrcObj = tex->CreateUAV(0, 0);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_Texture3D
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Texture3D;
        TextureSRV _rtrcObj;

        auto &operator=(TextureSRV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_RWTexture3D
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWTexture3D;
        TextureUAV _rtrcObj;

        auto &operator=(TextureUAV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_Texture2DArray
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Texture2DArray;
        TextureSRV _rtrcObj;

        auto &operator=(TextureSRV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_RWTexture2DArray
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWTexture2DArray;
        TextureUAV _rtrcObj;

        auto &operator=(TextureUAV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_Texture3DArray
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Texture3DArray;
        TextureSRV _rtrcObj;

        auto &operator=(TextureSRV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_RWTexture3DArray
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWTexture3DArray;
        TextureUAV _rtrcObj;

        auto &operator=(TextureUAV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_Buffer
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Buffer;
        BufferSRV _rtrcObj;

        auto &operator=(BufferSRV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_RWBuffer
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWBuffer;
        BufferUAV _rtrcObj;

        auto &operator=(BufferUAV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_StructuredBuffer
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::StructuredBuffer;
        using Element = T;
        BufferSRV _rtrcObj;

        auto &operator=(BufferSRV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_RWStructuredBuffer
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWStructuredBuffer;
        using Element = T;
        BufferUAV _rtrcObj;

        auto &operator=(BufferUAV value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxy_ConstantBuffer : T
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::ConstantBuffer;
        using Struct = T;
        RC<DynamicBuffer> _rtrcObj;

        auto &operator=(RC<DynamicBuffer> value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const T &value)
        {
            static_cast<T &>(*this) = value;
            return *this;
        }
    };

    struct MemberProxy_SamplerState
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Sampler;
        RC<Sampler> _rtrcObj;

        auto &operator=(RC<Sampler> value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }
    };

    template<typename T>
    struct MemberProxyTrait
    {
        using MemberProxyElement = T;
        static constexpr size_t ArraySize = 0;
    };

    template<typename T, size_t N>
    struct MemberProxyTrait<T[N]>
    {
        using MemberProxyElement = T;
        static constexpr size_t ArraySize = N;
    };

    template<typename T>
    concept RtrcGroupStruct = requires { typename T::_rtrcGroupTypeFlag; };

    template<RtrcGroupStruct T>
    consteval size_t GetUniformDWordCount()
    {
        size_t dwordCount = 0;
        T::ForEachFlattenMember(
            [&dwordCount]<bool IsUniform, typename M, typename A>
            (const char *name, RHI::ShaderStageFlag stages, const A &accessor)
        {
            if constexpr(IsUniform)
            {
                constexpr size_t memSize = ConstantBufferDetail::GetConstantBufferDWordCount<M>();
                bool needNewLine;
                if constexpr(std::is_array_v<M> || RtrcStruct<M>)
                {
                    needNewLine = true;
                }
                else
                {
                    needNewLine = (dwordCount % 4) + memSize > 4;
                }
                if(needNewLine)
                {
                    dwordCount = ((dwordCount + 3) >> 2) << 2;
                }
                dwordCount += memSize;
            }
        });
        return dwordCount;
    }

    template<RtrcGroupStruct T, typename F>
    void ForEachFlattenUniform(const F &f)
    {
        size_t deviceDWordOffset = 0;
        T::ForEachFlattenMember(
            [&deviceDWordOffset, &f]<bool IsUniform, typename M, typename A>
            (const char *name, RHI::ShaderStageFlag stages, const A &accessor)
        {
            if constexpr(IsUniform)
            {
                const size_t memberSize = ConstantBufferDetail::GetConstantBufferDWordCount<M>();
                bool needNewLine;
                if constexpr(std::is_array_v<M> || RtrcStruct<M>)
                {
                    needNewLine = true;
                }
                else
                {
                    needNewLine = (deviceDWordOffset % 4) + memberSize > 4;
                }
                if(needNewLine)
                {
                    deviceDWordOffset = ((deviceDWordOffset + 3) >> 2) << 2;
                }
                constexpr T *nullT = nullptr;
                const size_t hostOffset = reinterpret_cast<size_t>(accessor(nullT));
                assert(hostOffset % 4 == 0);
                const size_t hostDWordOffset = hostOffset / 4;
                ConstantBufferDetail::ForEachFlattenMember<M, F>(name, f, hostDWordOffset, deviceDWordOffset);
                deviceDWordOffset += memberSize;
            }
        });
    }

    template<RtrcGroupStruct T>
    const BindingGroupLayout::Desc &ToBindingGroupLayout()
    {
        static BindingGroupLayout::Desc ret = []
        {
            BindingGroupLayout::Desc desc;
            T::ForEachFlattenMember(
                [&desc]<bool IsUniform, typename M, typename A>
                (const char *name, RHI::ShaderStageFlag stages, const A &accessor)
            {
                if constexpr(!IsUniform)
                {
                    using MemberElement = typename MemberProxyTrait<M>::MemberProxyElement;
                    constexpr size_t arraySize = MemberProxyTrait<M>::ArraySize;
                    BindingGroupLayout::BindingDesc binding;
                    binding.name = name;
                    binding.type = MemberElement::BindingType;
                    binding.stages = stages;
                    binding.arraySize = arraySize ? std::make_optional(static_cast<uint32_t>(arraySize)) : std::nullopt;
                    desc.bindings.push_back(binding);
                }
            });
            if constexpr(GetUniformDWordCount<T>() > 0)
            {
                BindingGroupLayout::BindingDesc binding;
                binding.name = "_rtrcGeneratedUniformBufferName";
                binding.type = RHI::BindingType::ConstantBuffer;
                binding.stages = RHI::ShaderStageFlags::All;
                desc.bindings.push_back(binding);
            }
            return desc;
        }();
        return ret;
    }

} // namespace BindingGroupDSL

#define rtrc_group1(NAME) rtrc_group2(NAME, NAME)
#define rtrc_group2(NAME, NAME_STR)                                                             \
    struct NAME;                                                                                \
    struct _rtrcGroupBase##NAME                                                                 \
    {                                                                                           \
        using _rtrcSelf = NAME;                                                                 \
        struct _rtrcGroupTypeFlag{};                                                            \
        static ::Rtrc::StructDetail::Sizer<1> _rtrcMemberCounter(...);                          \
        template<typename F>                                                                    \
        static constexpr void ForEachMember(const F &f)                                         \
        {                                                                                       \
            ::Rtrc::StructDetail::ForEachMember<_rtrcSelf>(f);                                  \
        }                                                                                       \
        template<typename F, typename A = std::identity>                                        \
        static constexpr void ForEachFlattenMember(                                             \
            const F &f,                                                                         \
            ::Rtrc::RHI::ShaderStageFlag stageMask = ::Rtrc::RHI::ShaderStageFlags::All,        \
            const std::identity &accessor = {})                                                 \
        {                                                                                       \
            ::Rtrc::StructDetail::ForEachMember<_rtrcSelf>([&]<bool IsUniform, typename M>      \
            (M _rtrcSelf::* ptr, const char *name, ::Rtrc::RHI::ShaderStageFlag stages)         \
            {                                                                                   \
                auto newAccessor = [&]<typename P>(P p) constexpr -> decltype(auto)             \
                    { static_assert(std::is_pointer_v<P>); return &(accessor(p)->*ptr); };      \
                if constexpr(::Rtrc::BindingGroupDSL::RtrcGroupStruct<M>)                       \
                {                                                                               \
                    M::ForEachFlattenMember(f, stageMask & stages, newAccessor);                \
                }                                                                               \
                else                                                                            \
                {                                                                               \
                    f.template operator()<IsUniform, M>(name, stageMask & stages, newAccessor); \
                }                                                                               \
            });                                                                                 \
        }                                                                                       \
        static constexpr std::string GroupName = #NAME_STR;                                     \
        using float2 = ::Rtrc::Vector2f;                                                        \
        using float3 = ::Rtrc::Vector3f;                                                        \
        using float4 = ::Rtrc::Vector4f;                                                        \
        using int2   = ::Rtrc::Vector2i;                                                        \
        using int3   = ::Rtrc::Vector3i;                                                        \
        using int4   = ::Rtrc::Vector4i;                                                        \
        using uint   = uint32_t;                                                                \
        using uint2  = ::Rtrc::Vector2u;                                                        \
        using uint3  = ::Rtrc::Vector3u;                                                        \
        using uint4  = ::Rtrc::Vector4u;                                                        \
    };                                                                                          \
    struct NAME : _rtrcGroupBase##NAME

#define rtrc_group(...) RTRC_MACRO_OVERLOADING(rtrc_group, __VA_ARGS__)

#define RTRC_INLINE_STAGE_DECLERATION(STAGES)     \
    ([]{                                          \
        using ::Rtrc::RHI::ShaderStageFlags::VS;  \
        using ::Rtrc::RHI::ShaderStageFlags::FS;  \
        using ::Rtrc::RHI::ShaderStageFlags::CS;  \
        using ::Rtrc::RHI::ShaderStageFlags::All; \
        return (STAGES);                          \
    }())

#define rtrc_define2(TYPE, NAME) RTRC_DEFINE_IMPL(TYPE, NAME, ::Rtrc::RHI::ShaderStageFlags::All)
#define rtrc_define3(TYPE, NAME, STAGES) RTRC_DEFINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES))
#define rtrc_define(...) RTRC_MACRO_OVERLOADING(rtrc_define, __VA_ARGS__)

#define rtrc_uniform(TYPE, NAME)                                                                  \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                                             \
        f.template operator()<true>(&_rtrcSelf::NAME, #NAME, ::Rtrc::RHI::ShaderStageFlags::All); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                                            \
    using _rtrcMemberType##NAME = TYPE;                                                           \
    _rtrcMemberType##NAME NAME

#define RTRC_DEFINE_IMPL(TYPE, NAME, STAGES)                                   \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                          \
        f.template operator()<false>(&_rtrcSelf::NAME, #NAME, STAGES);         \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                         \
    using _rtrcMemberType##NAME = ::Rtrc::BindingGroupDSL::MemberProxy_##TYPE; \
    _rtrcMemberType##NAME NAME

#define rtrc_inline2(TYPE, NAME) RTRC_INLINE_IMPL(TYPE, NAME, ::Rtrc::RHI::ShaderStageFlags::All)
#define rtrc_inline3(TYPE, NAME, STAGES) RTRC_INLINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES))
#define rtrc_inline(...) RTRC_MACRO_OVERLOADING(rtrc_inline, __VA_ARGS__)

#define RTRC_INLINE_IMPL(TYPE, NAME, STAGES)                           \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                  \
        f.template operator()<false>(&_rtrcSelf::NAME, #NAME, STAGES); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                 \
    TYPE NAME

template<BindingGroupDSL::RtrcGroupStruct T>
const BindingGroupLayout::Desc &GetBindingGroupLayoutDesc()
{
    return BindingGroupDSL::ToBindingGroupLayout<T>();
}

template<BindingGroupDSL::RtrcGroupStruct T>
void ApplyBindingGroup(BindingGroup &group, const T &value)
{
    int index = 0;
    T::ForEachFlattenMember(
        [&]<bool IsUniform, typename M, typename A>(const char *name, RHI::ShaderStageFlag stages, const A &accessor)
    {
        static_assert(!IsUniform);
        using MemberElement = typename BindingGroupDSL::MemberProxyTrait<M>::MemberProxyElement;
        constexpr size_t arraySize = BindingGroupDSL::MemberProxyTrait<M>::ArraySize;
        static_assert(arraySize == 0, "ApplyBindingGroup: array is not implemented");
        group.Set(index++, accessor(&value)->_rtrcObj);
    });
}

template<BindingGroupDSL::RtrcGroupStruct T>
void ApplyBindingGroup(ConstantBufferManagerInterface *cbMgr, BindingGroup &group, const T &value)
{
    int index = 0;
    T::ForEachFlattenMember(
        [&]<bool IsUniform, typename M, typename A>(const char *name, RHI::ShaderStageFlag stages, const A &accessor)
    {
        if constexpr(!IsUniform)
        {
            using MemberElement = typename BindingGroupDSL::MemberProxyTrait<M>::MemberProxyElement;
            constexpr size_t arraySize = BindingGroupDSL::MemberProxyTrait<M>::ArraySize;
            static_assert(arraySize == 0, "ApplyBindingGroup: array is not implemented");
            if constexpr(MemberElement::BindingType == RHI::BindingType::ConstantBuffer)
            {
                if(accessor(&value)->_rtrcObj)
                {
                    group.Set(index++, accessor(&value)->_rtrcObj);
                }
                else
                {
                    using CBufferStruct = typename MemberElement::Struct;
                    if(!cbMgr)
                    {
                        throw Exception(fmt::format(
                            "ApplyBindingGroup: Constant buffer manager is nullptr when "
                            "setting constant buffer {} without giving pre-created cb object", name));
                    }
                    auto cbuffer = cbMgr->CreateConstantBuffer(static_cast<const CBufferStruct &>(*accessor(&value)));
                    group.Set(index++, std::move(cbuffer));
                }
            }
            else
            {
                group.Set(index++, accessor(&value)->_rtrcObj);
            }
        }
    });
    if constexpr(constexpr size_t dwordCount = BindingGroupDSL::GetUniformDWordCount<T>())
    {
        std::vector<unsigned char> deviceData(dwordCount * 4);
        BindingGroupDSL::ForEachFlattenUniform<T>(
            [&]<typename M>(const char *name, size_t hostOffset, size_t deviceOffset)
        {
            std::memcpy(deviceData.data() + deviceOffset, reinterpret_cast<const char *>(&value) + hostOffset, sizeof(M));
        });

        auto cbuffer = cbMgr->CreateConstantBuffer(deviceData.data(), deviceData.size());
        group.Set(index++, std::move(cbuffer));
    }
}

template<BindingGroupDSL::RtrcGroupStruct T>
void ApplyBindingGroup(const RC<BindingGroup> &group, const T &value)
{
    Rtrc::ApplyBindingGroup(*group, value);
}

template<BindingGroupDSL::RtrcGroupStruct T>
void ApplyBindingGroup(DynamicBufferManager *cbMgr, const RC<BindingGroup> &group, const T &value)
{
    Rtrc::ApplyBindingGroup(cbMgr, *group, value);
}

template<typename T>
void BindingGroup::Set(const T &value)
{
    static_assert(BindingGroupDSL::RtrcGroupStruct<T>);
    Rtrc::ApplyBindingGroup(manager_->_internalGetDefaultConstantBufferManager(), *this, value);
}

RTRC_END
