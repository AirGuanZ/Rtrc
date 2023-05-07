#pragma once

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Buffer/DynamicBuffer.h>
#include <Rtrc/Graphics/Device/Texture/Texture.h>
#include <Rtrc/Utility/Macro/MacroOverloading.h>
#include <Rtrc/Utility/Struct.h>

RTRC_BEGIN

class DynamicBuffer;
class Sampler;

namespace BindingGroupDSL
{

    enum class BindingFlagBit : uint8_t
    {
        Bindless = 1 << 0,
        VariableArraySize = 1 << 1,
    };
    RTRC_DEFINE_ENUM_FLAGS(BindingFlagBit)
    using BindingFlags = EnumFlagsBindingFlagBit;

    struct MemberProxy_Texture2D
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Texture;
        TextureSrv _rtrcObj;

        auto &operator=(TextureSrv value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const RC<Texture> &tex)
        {
            _rtrcObj = tex->CreateSrv(0, 0, 0);
            return *this;
        }
    };
    
    struct MemberProxy_RWTexture2D
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWTexture;
        TextureUav _rtrcObj;

        auto &operator=(TextureUav value)
        {
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const RC<Texture> &tex)
        {
            _rtrcObj = tex->CreateUav(0, 0);
            return *this;
        }
    };

    using MemberProxy_Texture3D = MemberProxy_Texture2D;
    using MemberProxy_RWTexture3D = MemberProxy_Texture2D;

    using MemberProxy_Texture2DArray = MemberProxy_Texture2D;
    using MemberProxy_RWTexture2DArray = MemberProxy_Texture2D;

    using MemberProxy_Texture3DArray = MemberProxy_Texture2D;
    using MemberProxy_RWTexture3DArray = MemberProxy_Texture2D;
    
    struct MemberProxy_Buffer
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Buffer;
        BufferSrv _rtrcObj;

        auto &operator=(BufferSrv value)
        {
            assert(value.GetRHIObject()->GetDesc().format != RHI::Format::Unknown);
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetTexelSrv();
        }
    };
    
    struct MemberProxy_RWBuffer
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWBuffer;
        BufferUav _rtrcObj;

        auto &operator=(BufferUav value)
        {
            assert(value.GetRHIObject()->GetDesc().format != RHI::Format::Unknown);
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetTexelUav();
        }
    };
    
    struct MemberProxy_StructuredBuffer
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::StructuredBuffer;

        BufferSrv _rtrcObj;

        auto &operator=(BufferSrv value)
        {
            assert(value.GetRHIObject()->GetDesc().stride);
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetStructuredSrv();
        }
    };
    
    struct MemberProxy_RWStructuredBuffer
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWStructuredBuffer;

        BufferUav _rtrcObj;

        auto &operator=(BufferUav value)
        {
            assert(value.GetRHIObject()->GetDesc().stride);
            _rtrcObj = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetStructuredUav();
        }
    };

    template<typename T>
    struct MemberProxy_ConstantBuffer : T
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::ConstantBuffer;
        using Struct = T;
        RC<SubBuffer> _rtrcObj;

        auto &operator=(RC<SubBuffer> value)
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
    
    template<>
    struct MemberProxy_ConstantBuffer<void>
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::ConstantBuffer;

        RC<SubBuffer> _rtrcObj;

        auto &operator=(RC<SubBuffer> value)
        {
            _rtrcObj = std::move(value);
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

    struct MemberProxy_RaytracingAccelerationStructure
    {
        static constexpr RHI::BindingType BindingType = RHI::BindingType::AccelerationStructure;
        RC<Tlas> _rtrcObj;

        auto &operator=(RC<Tlas> value)
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
    
    template<RtrcGroupStruct T, typename F, typename A = std::identity>                                 
    static constexpr void ForEachFlattenMember(
        const F &f, RHI::ShaderStageFlags stageMask = RHI::ShaderStage::All, const A &accessor = {})
    {
        StructDetail::ForEachMember<T>([&]<bool IsUniform, typename M>
            (M T:: * ptr, const char *name, RHI::ShaderStageFlags stages, BindingFlags flags)
        {
            auto newAccessor = [&]<typename P>(P p) constexpr -> decltype(auto)
            {
                static_assert(std::is_pointer_v<P>); return &(accessor(p)->*ptr);
            };
            if constexpr(::Rtrc::BindingGroupDSL::RtrcGroupStruct<M>)
            {
                BindingGroupDSL::ForEachFlattenMember<M>(f, stageMask &stages, newAccessor);
            }
            else
            {
                f.template operator() < IsUniform, M > (name, stageMask & stages, newAccessor, flags);
            }
        });
    }

    template<RtrcGroupStruct T>
    consteval size_t GetUniformDWordCount()
    {
        size_t dwordCount = 0;
        BindingGroupDSL::ForEachFlattenMember<T>(
            [&dwordCount]<bool IsUniform, typename M, typename A>
            (const char *name, RHI::ShaderStageFlags stages, const A &accessor, BindingFlags flags)
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
        BindingGroupDSL::ForEachFlattenMember<T>(
            [&deviceDWordOffset, &f]<bool IsUniform, typename M, typename A>
            (const char *name, RHI::ShaderStageFlags stages, const A &accessor, BindingFlags flags)
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
            BindingGroupDSL::ForEachFlattenMember<T>(
                [&desc]<bool IsUniform, typename M, typename A>
                (const char *name, RHI::ShaderStageFlags stages, const A &accessor, BindingFlags flags)
            {
                if constexpr(!IsUniform)
                {
                    using MemberElement = typename MemberProxyTrait<M>::MemberProxyElement;
                    constexpr size_t arraySize = MemberProxyTrait<M>::ArraySize;
                    BindingGroupLayout::BindingDesc binding;
                    binding.type = MemberElement::BindingType;
                    binding.stages = stages;
                    binding.arraySize = arraySize ? std::make_optional(static_cast<uint32_t>(arraySize)) : std::nullopt;
                    binding.bindless = flags.Contains(BindingFlagBit::Bindless);
                    desc.bindings.push_back(binding);
                    desc.variableArraySize |= flags.Contains(BindingFlagBit::VariableArraySize);
                }
            });
            if constexpr(GetUniformDWordCount<T>() > 0)
            {
                BindingGroupLayout::BindingDesc binding;
                binding.type = RHI::BindingType::ConstantBuffer;
                binding.stages = T::_rtrcGroupDefaultStages;
                desc.bindings.push_back(binding);
            }
            if(desc.variableArraySize && GetUniformDWordCount<T>() > 0)
            {
                assert(desc.bindings[desc.bindings.size() - 2].bindless);
                std::swap(desc.bindings[desc.bindings.size() - 2], desc.bindings[desc.bindings.size() - 1]);
            }
            return desc;
        }();
        return ret;
    }

    constexpr BindingFlags CreateBindingFlags(bool isBindless, bool hasVariableArraySize)
    {
        BindingFlags ret = 0;
        if(isBindless)
        {
            ret |= BindingFlagBit::Bindless;
        }
        if(hasVariableArraySize)
        {
            ret |= BindingFlagBit::VariableArraySize;
        }
        return ret;
    }

} // namespace BindingGroupDSL

#define rtrc_group1(NAME) rtrc_group2(NAME, ::Rtrc::RHI::ShaderStage::All)
#define rtrc_group2(NAME, DEFAULT_STAGES) RTRC_GROUP_IMPL(NAME, RTRC_INLINE_STAGE_DECLERATION(DEFAULT_STAGES))

#define RTRC_GROUP_IMPL(NAME, DEFAULT_STAGES)                                                               \
    struct NAME;                                                                                            \
    struct _rtrcGroupBase##NAME                                                                             \
    {                                                                                                       \
        using _rtrcSelf = NAME;                                                                             \
        struct _rtrcGroupTypeFlag{};                                                                        \
        static ::Rtrc::StructDetail::Sizer<1> _rtrcMemberCounter(...);                                      \
        static constexpr auto _rtrcGroupDefaultStages = (DEFAULT_STAGES);                                   \
        using float2   = ::Rtrc::Vector2f;                                                                  \
        using float3   = ::Rtrc::Vector3f;                                                                  \
        using float4   = ::Rtrc::Vector4f;                                                                  \
        using int2     = ::Rtrc::Vector2i;                                                                  \
        using int3     = ::Rtrc::Vector3i;                                                                  \
        using int4     = ::Rtrc::Vector4i;                                                                  \
        using uint     = uint32_t;                                                                          \
        using uint2    = ::Rtrc::Vector2u;                                                                  \
        using uint3    = ::Rtrc::Vector3u;                                                                  \
        using uint4    = ::Rtrc::Vector4u;                                                                  \
        using float4x4 = ::Rtrc::Matrix4x4f;                                                                \
    };                                                                                                      \
    struct NAME : _rtrcGroupBase##NAME

#define rtrc_group(...) RTRC_MACRO_OVERLOADING(rtrc_group, __VA_ARGS__)

#define RTRC_INLINE_STAGE_DECLERATION(STAGES)                               \
    ([]{                                                                    \
        using ::Rtrc::RHI::ShaderStage::VS;                                 \
        using ::Rtrc::RHI::ShaderStage::FS;                                 \
        using ::Rtrc::RHI::ShaderStage::CS;                                 \
        using ::Rtrc::RHI::ShaderStage::RT_RGS;                             \
        using ::Rtrc::RHI::ShaderStage::RT_MS;                              \
        using ::Rtrc::RHI::ShaderStage::RT_CHS;                             \
        using ::Rtrc::RHI::ShaderStage::RT_IS;                              \
        using ::Rtrc::RHI::ShaderStage::RT_AHS;                             \
        constexpr auto Graphics = ::Rtrc::RHI::ShaderStage::AllGraphics;    \
        constexpr auto Callable = ::Rtrc::RHI::ShaderStage::CallableShader; \
        constexpr auto RTCommon = ::Rtrc::RHI::ShaderStage::AllRTCommon;    \
        constexpr auto RTHit    = ::Rtrc::RHI::ShaderStage::AllRTHit;       \
        constexpr auto RT       = ::Rtrc::RHI::ShaderStage::AllRT;          \
        using ::Rtrc::RHI::ShaderStage::All;                                \
        return (STAGES);                                                    \
    }())

#define rtrc_define2(TYPE, NAME)         RTRC_DEFINE_IMPL(TYPE, NAME, _rtrcGroupDefaultStages, false, false)
#define rtrc_define3(TYPE, NAME, STAGES) RTRC_DEFINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES), false, false)
#define rtrc_define(...)                 RTRC_MACRO_OVERLOADING(rtrc_define, __VA_ARGS__)

#define rtrc_bindless2(TYPE, NAME)         RTRC_DEFINE_IMPL(TYPE, NAME, _rtrcGroupDefaultStages, true, false)
#define rtrc_bindless3(TYPE, NAME, STAGES) RTRC_DEFINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES), true, false)
#define rtrc_bindless(...)                 RTRC_MACRO_OVERLOADING(rtrc_bindless, __VA_ARGS__)

#define rtrc_bindless_variable_size2(TYPE, NAME) \
    RTRC_DEFINE_IMPL(TYPE, NAME, _rtrcGroupDefaultStages, true, true)
#define rtrc_bindless_variable_size3(TYPE, NAME, STAGES) \
    RTRC_DEFINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES), true, true)
#define rtrc_bindless_variable_size(...) RTRC_MACRO_OVERLOADING(rtrc_bindless_variable_size, __VA_ARGS__)

#define rtrc_uniform(TYPE, NAME)                                        \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                   \
        f.template operator()<true>(                                    \
            &_rtrcSelf::NAME, #NAME, _rtrcGroupDefaultStages,           \
            ::Rtrc::BindingGroupDSL::CreateBindingFlags(false, false)); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                  \
    using _rtrcMemberType##NAME = TYPE;                                 \
    _rtrcMemberType##NAME NAME

#define RTRC_DEFINE_IMPL(TYPE, NAME, STAGES, IS_BINDLESS, VARIABLE_ARRAY_SIZE)              \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                                       \
        f.template operator()<false>(                                                       \
            &_rtrcSelf::NAME, #NAME, STAGES,                                                \
            ::Rtrc::BindingGroupDSL::CreateBindingFlags(IS_BINDLESS, VARIABLE_ARRAY_SIZE)); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                                      \
    using _rtrcMemberType##NAME = ::Rtrc::BindingGroupDSL::MemberProxy_##TYPE;              \
    _rtrcMemberType##NAME NAME

#define rtrc_inline2(TYPE, NAME) RTRC_INLINE_IMPL(TYPE, NAME, _rtrcGroupDefaultStages)
#define rtrc_inline3(TYPE, NAME, STAGES) RTRC_INLINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES))
#define rtrc_inline(...) RTRC_MACRO_OVERLOADING(rtrc_inline, __VA_ARGS__)

#define RTRC_INLINE_IMPL(TYPE, NAME, STAGES)                            \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                   \
        f.template operator()<false>(                                   \
            &_rtrcSelf::NAME, #NAME, STAGES,                            \
            ::Rtrc::BindingGroupDSL::CreateBindingFlags(false, false)); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                  \
    TYPE NAME

template<BindingGroupDSL::RtrcGroupStruct T>
const BindingGroupLayout::Desc &GetBindingGroupLayoutDesc()
{
    return BindingGroupDSL::ToBindingGroupLayout<T>();
}

template<BindingGroupDSL::RtrcGroupStruct T>
void ApplyBindingGroup(RHI::Device *device, ConstantBufferManagerInterface *cbMgr, BindingGroup &group, const T &value)
{
    RHI::BindingGroupUpdateBatch batch;
    int index = 0;
    bool swapUniformAndVariableSizedArray = false;
    BindingGroupDSL::ForEachFlattenMember<T>(
        [&]<bool IsUniform, typename M, typename A>
        (const char *name, RHI::ShaderStageFlags stages, const A &accessor, BindingGroupDSL::BindingFlags flags)
    {
        if constexpr(!IsUniform)
        {
            using MemberElement = typename BindingGroupDSL::MemberProxyTrait<M>::MemberProxyElement;
            constexpr size_t arraySize = BindingGroupDSL::MemberProxyTrait<M>::ArraySize;
            static_assert(
                arraySize == 0 || MemberElement::BindingType != RHI::BindingType::ConstantBuffer,
                "ConstantBuffer array hasn't been supported");
            if constexpr(arraySize > 0)
            {
                auto ApplyArrayElements = [&, as = arraySize](int actualIndex)
                {
                    for(size_t i = 0; i < as; ++i)
                    {
                        if constexpr(IsRC<std::remove_cvref_t<decltype((*accessor(&value))[i]._rtrcObj)>>)
                        {
                            if(auto &obj = (*accessor(&value))[i]._rtrcObj)
                            {
                                batch.Append(*group.GetRHIObject(), actualIndex, i, obj->GetRHIObject().Get());
                            }
                        }
                        else
                        {
                            if(auto &rhiObj = (*accessor(&value))[i]._rtrcObj.GetRHIObject())
                            {
                                batch.Append(*group.GetRHIObject(), actualIndex, i, rhiObj.Get());
                            }
                        }
                    }
                };

                assert(!swapUniformAndVariableSizedArray);
                swapUniformAndVariableSizedArray =
                    BindingGroupDSL::GetUniformDWordCount<T>() > 0 &&
                    flags.Contains(BindingGroupDSL::BindingFlagBit::VariableArraySize);

                ApplyArrayElements(swapUniformAndVariableSizedArray ? (index + 1) : index);
                ++index;
            }
            else if constexpr(MemberElement::BindingType == RHI::BindingType::ConstantBuffer)
            {
                if(accessor(&value)->_rtrcObj)
                {
                    auto cbuffer = accessor(&value)->_rtrcObj;
                    batch.Append(*group.GetRHIObject(), index++, RHI::ConstantBufferUpdate{
                        cbuffer->GetFullBufferRHIObject().Get(),
                        cbuffer->GetSubBufferOffset(),
                        cbuffer->GetSubBufferSize()
                    });
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
                    batch.Append(*group.GetRHIObject(), index++, RHI::ConstantBufferUpdate{
                        cbuffer->GetFullBufferRHIObject().Get(),
                        cbuffer->GetSubBufferOffset(),
                        cbuffer->GetSubBufferSize()
                    });
                }
            }
            else if constexpr(IsRC<std::remove_cvref_t<decltype(accessor(&value)->_rtrcObj)>>)
            {
                if(auto &obj = accessor(&value)->_rtrcObj)
                {
                    batch.Append(*group.GetRHIObject(), index, obj->GetRHIObject().Get());
                }
                ++index;
            }
            else
            {
                if(auto &rhiObj = accessor(&value)->_rtrcObj.GetRHIObject())
                {
                    batch.Append(*group.GetRHIObject(), index, rhiObj.Get());
                }
                ++index;
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

        const int actualIndex = swapUniformAndVariableSizedArray ? (index - 1) : index;
        auto cbuffer = cbMgr->CreateConstantBuffer(deviceData.data(), deviceData.size());
        batch.Append(*group.GetRHIObject(), actualIndex, RHI::ConstantBufferUpdate{
            cbuffer->GetFullBufferRHIObject().Get(),
            cbuffer->GetSubBufferOffset(),
            cbuffer->GetSubBufferSize()
        });
    }

    device->UpdateBindingGroups(batch);
}

template<BindingGroupDSL::RtrcGroupStruct T>
void ApplyBindingGroup(RHI::Device *device, DynamicBufferManager *cbMgr, const RC<BindingGroup> &group, const T &value)
{
    Rtrc::ApplyBindingGroup(device, cbMgr, *group, value);
}

template<typename T>
void BindingGroup::Set(const T &value)
{
    static_assert(BindingGroupDSL::RtrcGroupStruct<T>);
    Rtrc::ApplyBindingGroup(
        manager_->_internalGetRHIDevice().Get(), manager_->_internalGetDefaultConstantBufferManager(), *this, value);
}

RTRC_END
