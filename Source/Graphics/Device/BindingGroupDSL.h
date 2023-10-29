#pragma once

#include <Graphics/Device/AccelerationStructure.h>
#include <Graphics/Device/BindingGroup.h>
#include <Graphics/Device/Buffer/Buffer.h>
#include <Graphics/Device/Buffer/DynamicBuffer.h>
#include <Graphics/Device/Texture/Texture.h>
#include <Graphics/RenderGraph/Pass.h>
#include <Core/Macro/MacroOverloading.h>
#include <Core/Struct.h>

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

    class MemberProxy_Texture2D
    {
        Variant<TextureSrv, RG::TextureResourceSrv> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::Texture;

        TextureSrv GetRtrcObject() const
        {
            return data_.Match(
                [&](const TextureSrv &v) { return v; },
                [&](const RG::TextureResourceSrv &v) { return v.GetSrv(); });
        }

        void DeclareRenderGraphResourceUsage(RG::Pass *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RG::TextureResourceSrv>())
            {
                d->ForEachSubresourceAccess(
                    [&](
                        const RHI::TextureSubresource &subrsc,
                        RHI::TextureLayout             layout,
                        RHI::ResourceAccessFlag        access)
                    {
                        pass->Use(
                            d->GetResource(),
                            subrsc,
                            RG::UseInfo
                            {
                                .layout = layout,
                                .stages = stages,
                                .accesses = access
                            });
                    });
            }
        }

        auto &operator=(TextureSrv value)
        {
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const RC<Texture> &tex)
        {
            return *this = tex->GetSrv(0, 0, 0);
        }

        auto &operator=(const RG::TextureResourceSrv &srv)
        {
            data_ = srv;
            return *this;
        }

        auto &operator=(RG::TextureResource *tex)
        {
            return *this = tex->GetSrv();
        }
    };
    
    class MemberProxy_RWTexture2D
    {
        Variant<TextureUav, RG::TextureResourceUav> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWTexture;

        bool writeOnly = false;

        TextureUav GetRtrcObject() const
        {
            return data_.Match(
                [&](const TextureUav &v) { return v; },
                [&](const RG::TextureResourceUav &v) { return v.GetUav(); });
        }

        void DeclareRenderGraphResourceUsage(RG::Pass *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RG::TextureResourceUav>())
            {
                d->ForEachSubresourceAccess(
                    [&](
                        const RHI::TextureSubresource &subrsc,
                        RHI::TextureLayout             layout,
                        RHI::ResourceAccessFlag        access)
                    {
                        pass->Use(
                            d->GetResource(),
                            subrsc,
                            RG::UseInfo
                            {
                                .layout = layout,
                                .stages = stages,
                                .accesses = access
                            });
                    }, writeOnly);
            }
        }
        
        auto &operator=(TextureUav value)
        {
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const RC<Texture> &tex)
        {
            return *this = tex->GetUav(0, 0);
        }

        auto &operator=(const RG::TextureResourceUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RG::TextureResource *tex)
        {
            return *this = tex->GetUav();
        }
    };

    using MemberProxy_Texture3D = MemberProxy_Texture2D;
    using MemberProxy_RWTexture3D = MemberProxy_Texture2D;

    using MemberProxy_Texture2DArray = MemberProxy_Texture2D;
    using MemberProxy_RWTexture2DArray = MemberProxy_Texture2D;

    using MemberProxy_Texture3DArray = MemberProxy_Texture2D;
    using MemberProxy_RWTexture3DArray = MemberProxy_Texture2D;
    
    class MemberProxy_Buffer
    {
        Variant<BufferSrv, RG::BufferResourceSrv> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::Buffer;

        BufferSrv GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferSrv &v) { return v; },
                [&](const RG::BufferResourceSrv &v) { return v.GetSrv(); });
        }

        void DeclareRenderGraphResourceUsage(RG::Pass *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RG::BufferResourceSrv>())
            {
                pass->Use(d->GetResource(), RG::UseInfo
                    {
                        .layout = RHI::TextureLayout::Undefined,
                        .stages = stages,
                        .accesses = d->GetResourceAccess()
                    });
            }
        }

        auto &operator=(BufferSrv value)
        {
            assert(value.GetRHIObject()->GetDesc().format != RHI::Format::Unknown);
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetTexelSrv();
        }

        auto &operator=(const RG::BufferResourceSrv &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RG::BufferResource *buffer)
        {
            return *this = buffer->GetTexelSrv();
        }
    };
    
    class MemberProxy_RWBuffer
    {
    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWBuffer;

        bool writeOnly = false;

        BufferUav GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferUav &v) { return v; },
                [&](const RG::BufferResourceUav &v) { return v.GetUav(); });
        }

        void DeclareRenderGraphResourceUsage(RG::Pass *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RG::BufferResourceUav>())
            {
                pass->Use(d->GetResource(), RG::UseInfo
                    {
                        .layout = RHI::TextureLayout::Undefined,
                        .stages = stages,
                        .accesses = d->GetResourceAccess(writeOnly)
                    });
            }
        }

        auto &operator=(BufferUav value)
        {
            assert(value.GetRHIObject()->GetDesc().format != RHI::Format::Unknown);
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetTexelUav();
        }

        auto &operator=(const RG::BufferResourceUav &v)
        {
            data_ = v;
            return v;
        }

        auto &operator=(RG::BufferResource *buffer)
        {
            return *this = buffer->GetTexelUav();
        }

    private:

        Variant<BufferUav, RG::BufferResourceUav> data_;
    };
    
    class MemberProxy_StructuredBuffer
    {
        Variant<BufferSrv, RG::BufferResourceSrv> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::StructuredBuffer;

        BufferSrv GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferSrv &v) { return v; },
                [&](const RG::BufferResourceSrv &v) { return v.GetSrv(); });
        }

        void DeclareRenderGraphResourceUsage(RG::Pass *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RG::BufferResourceSrv>())
            {
                pass->Use(d->GetResource(), RG::UseInfo
                    {
                        .layout = RHI::TextureLayout::Undefined,
                        .stages = stages,
                        .accesses = d->GetResourceAccess()
                    });
            }
        }

        auto &operator=(BufferSrv value)
        {
            assert(value.GetRHIObject()->GetDesc().stride);
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetStructuredSrv();
        }

        auto &operator=(const RG::BufferResourceSrv &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RG::BufferResource *buffer)
        {
            return *this = buffer->GetStructuredSrv();
        }
    };
    
    class MemberProxy_RWStructuredBuffer
    {
    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWStructuredBuffer;

        bool writeOnly = false;

        BufferUav GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferUav &v) { return v; },
                [&](const RG::BufferResourceUav &v) { return v.GetUav(); });
        }

        void DeclareRenderGraphResourceUsage(RG::Pass *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RG::BufferResourceUav>())
            {
                pass->Use(d->GetResource(), RG::UseInfo
                    {
                        .layout = RHI::TextureLayout::Undefined,
                        .stages = stages,
                        .accesses = d->GetResourceAccess(writeOnly)
                    });
            }
        }

        auto &operator=(BufferUav value)
        {
            assert(value.GetRHIObject()->GetDesc().stride);
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetStructuredUav();
        }

        auto &operator=(const RG::BufferResourceUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RG::BufferResource *buffer)
        {
            return *this = buffer->GetStructuredUav();
        }

    private:

        Variant<BufferUav, RG::BufferResourceUav> data_;
    };

    template<typename T>
    class MemberProxy_ConstantBuffer : public T
    {
        RC<SubBuffer> data_;

    public:

        using Struct = T;

        static constexpr RHI::BindingType BindingType = RHI::BindingType::ConstantBuffer;

        RC<SubBuffer> GetRtrcObject() const { return data_; }

        auto &operator=(RC<SubBuffer> value)
        {
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const T &value)
        {
            static_cast<T &>(*this) = value;
            return *this;
        }
    };
    
    template<>
    class MemberProxy_ConstantBuffer<void>
    {
        RC<SubBuffer> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::ConstantBuffer;

        RC<SubBuffer> GetRtrcObject() const { return data_; }

        auto &operator=(RC<SubBuffer> value)
        {
            data_ = std::move(value);
            return *this;
        }
    };

    class MemberProxy_SamplerState
    {
        RC<Sampler> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::Sampler;

        RC<Sampler> GetRtrcObject() const { return data_; }

        auto &operator=(RC<Sampler> value)
        {
            data_ = std::move(value);
            return *this;
        }
    };

    class MemberProxy_RaytracingAccelerationStructure
    {
        Variant<RC<Tlas>, RG::TlasResource *> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::AccelerationStructure;

        RC<Tlas> GetRtrcObject() const
        {
            return data_.Match(
                [&](const RC<Tlas> &t) { return t; },
                [&](RG::TlasResource *t) { return t->Get(); });
        }

        void DeclareRenderGraphResourceUsage(RG::Pass *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RG::TlasResource*>())
            {
                auto buffer = (*d)->GetInternalBuffer();
                pass->Use(buffer, RG::UseInfo
                    {
                        .layout = RHI::TextureLayout::Undefined,
                        .stages = stages,
                        .accesses = RHI::ResourceAccess::ReadAS
                    });
            }
        }

        auto &operator=(RC<Tlas> value)
        {
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(RG::TlasResource *tlas)
        {
            data_ = tlas;
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

    template<typename T>
    concept RtrcGroupDummyMember = requires { typename T::_rtrcGroupDummyMemberFlag; };

    struct RtrcGroupDummyMemberType { struct _rtrcGroupDummyMemberFlag {}; };
    
    template<RtrcGroupStruct T, typename F, typename A = std::identity>                                 
    static constexpr void ForEachFlattenMember(
        const F &f, RHI::ShaderStageFlags stageMask = RHI::ShaderStage::All, const A &accessor = {})
    {
        StructDetail::ForEachMember<T>([&]<bool IsUniform, typename M>
            (M T:: * ptr, const char *name, RHI::ShaderStageFlags stages, BindingFlags flags)
        {
            if constexpr(!RtrcGroupDummyMember<M>)
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
                    f.template operator()<IsUniform, M>(name, stageMask & stages, newAccessor, flags);
                }
            }
        });
    }

    template<RtrcGroupStruct T>
    consteval bool HasUniform()
    {
        bool ret = false;
        BindingGroupDSL::ForEachFlattenMember<T>(
            [&ret]<bool IsUniform, typename M, typename A>
            (const char *, RHI::ShaderStageFlags, const A &, BindingFlags)
        {
            if constexpr(IsUniform)
            {
                ret = true;
            }
        });
        return ret;
    }

    template<RtrcGroupStruct T>
    size_t GetUniformDWordCount()
    {
        size_t dwordCount = 0;
        BindingGroupDSL::ForEachFlattenMember<T>(
            [&dwordCount]<bool IsUniform, typename M, typename A>
            (const char *name, RHI::ShaderStageFlags stages, const A &accessor, BindingFlags flags)
        {
            if constexpr(IsUniform)
            {
                size_t memSize;
                if constexpr(RtrcReflShaderStruct<M>)
                {
                    memSize = ReflectedStruct::GetDeviceDWordCount<M>();
                }
                else
                {
                    memSize = ConstantBufferDetail::GetConstantBufferDWordCount<M>();
                }
                bool needNewLine;
                if constexpr(std::is_array_v<M> || RtrcReflShaderStruct<M>)
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

    template<RtrcGroupStruct T>
    void FlattenUniformsToConstantBufferData(const T &data, void *output)
    {
        size_t deviceDWordOffset = 0;
        BindingGroupDSL::ForEachFlattenMember<T>(
            [&]<bool IsUniform, typename M, typename A>
            (const char *name, RHI::ShaderStageFlags stages, const A & accessor, BindingFlags flags)
        {
            if constexpr(IsUniform)
            {
                const size_t memberSize = ConstantBufferDetail::GetConstantBufferDWordCount<M>();

                bool needNewLine;
                if constexpr(std::is_array_v<M> || RtrcReflShaderStruct<M>)
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

                ConstantBufferDetail::FlattenToConstantBufferData<M>(&data, output, hostOffset / 4, deviceDWordOffset);
                
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
            if constexpr(HasUniform<T>())
            {
                BindingGroupLayout::BindingDesc binding;
                binding.type = RHI::BindingType::ConstantBuffer;
                binding.stages = T::_rtrcGroupDefaultStages;
                desc.bindings.push_back(binding);
            }
            if(desc.variableArraySize && HasUniform<T>())
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

    template<RHI::ShaderStageFlags STAGES>
    struct _rtrcGroupCommonBase
    {
        using _rtrcBase = _rtrcGroupCommonBase;
        struct _rtrcGroupTypeFlag {};
        static StructDetail::Sizer<1> _rtrcMemberCounter(...) { return {}; }
        static constexpr uint32_t _rtrcGroupDefaultStages = STAGES;
    };

    struct _rtrcGroupTypeBase
    {
        using float2 = Vector2f;
        using float3 = Vector3f;
        using float4 = Vector4f;
        using int2 = Vector2i;
        using int3 = Vector3i;
        using int4 = Vector4i;
        using uint = uint32_t;
        using uint2 = Vector2u;
        using uint3 = Vector3u;
        using uint4 = Vector4u;
        using float4x4 = Matrix4x4f;
    };

    template<bool COND, typename A, typename B>
    auto ConditionalType(A *, B *)
    {
        using Ret = std::conditional_t<COND, A, B>;
        return static_cast<Ret *>(nullptr);
    }

} // namespace BindingGroupDSL

#define rtrc_group1(NAME)                 rtrc_group2(NAME, ::Rtrc::RHI::ShaderStage::All)
#define rtrc_group2(NAME, DEFAULT_STAGES) RTRC_GROUP_IMPL(NAME, RTRC_INLINE_STAGE_DECLERATION(DEFAULT_STAGES))
#define rtrc_group(...)                   RTRC_MACRO_OVERLOADING(rtrc_group, __VA_ARGS__)

#define RTRC_GROUP_IMPL(NAME, DEFAULT_STAGES) \
    struct NAME : ::Rtrc::BindingGroupDSL::_rtrcGroupCommonBase<(DEFAULT_STAGES)>, ::Rtrc::BindingGroupDSL::_rtrcGroupTypeBase

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

#define rtrc_cond_define3(COND, TYPE, NAME)         RTRC_CONDITIONAL_DEFINE_IMPL(COND, TYPE, NAME, _rtrcSelf##NAME::_rtrcGroupDefaultStages, false, false)
#define rtrc_cond_define4(COND, TYPE, NAME, STAGES) RTRC_CONDITIONAL_DEFINE_IMPL(COND, TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES), false, false)
#define rtrc_cond_define(...)                       RTRC_MACRO_OVERLOADING(rtrc_cond_define, __VA_ARGS__)

#define rtrc_define2(TYPE, NAME)         RTRC_DEFINE_IMPL(TYPE, NAME, _rtrcSelf##NAME::_rtrcGroupDefaultStages, false, false)
#define rtrc_define3(TYPE, NAME, STAGES) RTRC_DEFINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES), false, false)
#define rtrc_define(...)                 RTRC_MACRO_OVERLOADING(rtrc_define, __VA_ARGS__)

#define rtrc_bindless2(TYPE, NAME)         RTRC_DEFINE_IMPL(TYPE, NAME, _rtrcSelf##NAME::_rtrcGroupDefaultStages, true, false)
#define rtrc_bindless3(TYPE, NAME, STAGES) RTRC_DEFINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES), true, false)
#define rtrc_bindless(...)                 RTRC_MACRO_OVERLOADING(rtrc_bindless, __VA_ARGS__)

#define rtrc_bindless_variable_size2(TYPE, NAME) \
    RTRC_DEFINE_IMPL(TYPE, NAME, _rtrcSelf##NAME::_rtrcGroupDefaultStages, true, true)
#define rtrc_bindless_variable_size3(TYPE, NAME, STAGES) \
    RTRC_DEFINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES), true, true)
#define rtrc_bindless_variable_size(...) RTRC_MACRO_OVERLOADING(rtrc_bindless_variable_size, __VA_ARGS__)

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define RTRC_UNIFORM_IMPL(TYPE, NAME) \
    using _rtrcMemberType##NAME = TYPE; _rtrcMemberType##NAME NAME; using _requireComma##NAME = int
#else
#define RTRC_UNIFORM_IMPL(TYPE, NAME)                                    \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                               \
    using _rtrcMemberType##NAME = TYPE;                                  \
    _rtrcMemberType##NAME NAME;                                          \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                    \
        f.template operator()<true>(                                     \
            &_rtrcSelf##NAME::NAME, #NAME, _rtrcSelf##NAME::_rtrcGroupDefaultStages, \
            ::Rtrc::BindingGroupDSL::CreateBindingFlags(false, false));  \
    RTRC_META_STRUCT_POST_MEMBER(NAME) using _requireComma##NAME = int
#endif

#define rtrc_uniform(TYPE, NAME) RTRC_UNIFORM_IMPL(TYPE, NAME)
#define rtrc_cond_uniform(COND, TYPE, NAME)                                                                           \
    RTRC_UNIFORM_IMPL(                                                                                                \
        std::remove_pointer_t<decltype(::Rtrc::BindingGroupDSL::ConditionalType<COND>(                                \
            static_cast<TYPE*>(nullptr), static_cast<::Rtrc::BindingGroupDSL::RtrcGroupDummyMemberType*>(nullptr)))>, \
        NAME)

#define RTRC_DEFINE_IMPL(TYPE, NAME, STAGES, IS_BINDLESS, VARIABLE_ARRAY_SIZE) \
    RTRC_CONDITIONAL_DEFINE_IMPL(true, TYPE, NAME, STAGES, IS_BINDLESS, VARIABLE_ARRAY_SIZE)

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define RTRC_CONDITIONAL_DEFINE_IMPL(COND, TYPE, NAME, STAGES, IS_BINDLESS, VARIABLE_ARRAY_SIZE)   \
    using _rtrcMemberType##NAME = std::conditional_t<(COND),                                       \
        ::Rtrc::BindingGroupDSL::MemberProxy_##TYPE,                                               \
        ::Rtrc::BindingGroupDSL::RtrcGroupDummyMemberType>;                                        \
    _rtrcMemberType##NAME NAME; using _requireComma##NAME = int

#else
#define RTRC_CONDITIONAL_DEFINE_IMPL(COND, TYPE, NAME, STAGES, IS_BINDLESS, VARIABLE_ARRAY_SIZE)   \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                                                         \
    using _rtrcMemberType##NAME = std::conditional_t<(COND),                                       \
        ::Rtrc::BindingGroupDSL::MemberProxy_##TYPE,                                               \
        ::Rtrc::BindingGroupDSL::RtrcGroupDummyMemberType>;                                        \
    _rtrcMemberType##NAME NAME;                                                                    \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                                              \
        f.template operator()<false>(                                                              \
            &_rtrcSelf##NAME::NAME, #NAME, STAGES,                                                 \
            ::Rtrc::BindingGroupDSL::CreateBindingFlags(IS_BINDLESS, VARIABLE_ARRAY_SIZE));        \
    RTRC_META_STRUCT_POST_MEMBER(NAME) using _requireComma##NAME = int
#endif

#define rtrc_inline2(TYPE, NAME)         RTRC_INLINE_IMPL(TYPE, NAME, _rtrcSelf##NAME::_rtrcGroupDefaultStages)
#define rtrc_inline3(TYPE, NAME, STAGES) RTRC_INLINE_IMPL(TYPE, NAME, RTRC_INLINE_STAGE_DECLERATION(STAGES))
#define rtrc_inline(...)                 RTRC_MACRO_OVERLOADING(rtrc_inline, __VA_ARGS__)

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define RTRC_INLINE_IMPL(TYPE, NAME, STAGES) TYPE NAME; using _requireComma##NAME = int
#else
#define RTRC_INLINE_IMPL(TYPE, NAME, STAGES)                            \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                              \
    TYPE NAME;                                                          \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                   \
        f.template operator()<false>(                                   \
            &_rtrcSelf##NAME::NAME, #NAME, STAGES,                      \
            ::Rtrc::BindingGroupDSL::CreateBindingFlags(false, false)); \
    RTRC_META_STRUCT_POST_MEMBER(NAME) using _requireComma##NAME = int
#endif

template<BindingGroupDSL::RtrcGroupStruct T>
const BindingGroupLayout::Desc &GetBindingGroupLayoutDesc()
{
    return BindingGroupDSL::ToBindingGroupLayout<T>();
}

template<BindingGroupDSL::RtrcGroupStruct T>
void DeclareRenderGraphResourceUses(RG::Pass *pass, const T &value, RHI::PipelineStageFlag stages)
{
    BindingGroupDSL::ForEachFlattenMember<T>(
        [&]<bool IsUniform, typename M, typename A>
        (const char *name, RHI::ShaderStageFlags shaderStages, const A &accessor, BindingGroupDSL::BindingFlags flags)
    {
        const RHI::PipelineStageFlag pipelineStages = ShaderStagesToPipelineStages(shaderStages) & stages;
        if constexpr(!IsUniform)
        {
            using MemberElement = typename BindingGroupDSL::MemberProxyTrait<M>::MemberProxyElement;
            constexpr size_t arraySize = BindingGroupDSL::MemberProxyTrait<M>::ArraySize;
            static_assert(
                arraySize == 0 || MemberElement::BindingType != RHI::BindingType::ConstantBuffer,
                "ConstantBuffer array hasn't been supported");
            if constexpr(arraySize > 0)
            {
                constexpr bool CanDeclare = requires
                {
                    (*accessor(&value))[0].DeclareRenderGraphResourceUsage(pass, pipelineStages);
                };
                for(size_t i = 0; i < arraySize; ++i)
                {
                    if constexpr(CanDeclare)
                    {
                        (*accessor(&value))[i].DeclareRenderGraphResourceUsage(pass, pipelineStages);
                    }
                }
            }
            else
            {
                constexpr bool CanDeclare = requires
                {
                    accessor(&value)->DeclareRenderGraphResourceUsage(pass, pipelineStages);
                };
                if constexpr(CanDeclare)
                {
                    accessor(&value)->DeclareRenderGraphResourceUsage(pass, pipelineStages);
                }
            }
        }
    });
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
                        if constexpr(IsRC<std::remove_cvref_t<decltype((*accessor(&value))[i].GetRtrcObject())>>)
                        {
                            if(auto obj = (*accessor(&value))[i].GetRtrcObject())
                            {
                                batch.Append(*group.GetRHIObject(), actualIndex, i, obj->GetRHIObject());
                            }
                        }
                        else
                        {
                            if(auto rhiObj = (*accessor(&value))[i].GetRtrcObject().GetRHIObject())
                            {
                                batch.Append(*group.GetRHIObject(), actualIndex, i, std::move(rhiObj));
                            }
                        }
                    }
                };

                assert(!swapUniformAndVariableSizedArray);
                swapUniformAndVariableSizedArray =
                    BindingGroupDSL::HasUniform<T>() &&
                    flags.Contains(BindingGroupDSL::BindingFlagBit::VariableArraySize);

                ApplyArrayElements(swapUniformAndVariableSizedArray ? (index + 1) : index);
                ++index;
            }
            else if constexpr(MemberElement::BindingType == RHI::BindingType::ConstantBuffer)
            {
                if(accessor(&value)->GetRtrcObject())
                {
                    auto cbuffer = accessor(&value)->GetRtrcObject();
                    batch.Append(*group.GetRHIObject(), index++, RHI::ConstantBufferUpdate
                    {
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
            else if constexpr(IsRC<std::remove_cvref_t<decltype(accessor(&value)->GetRtrcObject())>>)
            {
                if(auto obj = accessor(&value)->GetRtrcObject())
                {
                    batch.Append(*group.GetRHIObject(), index, obj->GetRHIObject());
                }
                ++index;
            }
            else
            {
                if(auto rhiObj = accessor(&value)->GetRtrcObject().GetRHIObject())
                {
                    batch.Append(*group.GetRHIObject(), index, std::move(rhiObj));
                }
                ++index;
            }
        }
    });
    if constexpr(BindingGroupDSL::HasUniform<T>())
    {
        const size_t dwordCount = BindingGroupDSL::GetUniformDWordCount<T>();

        std::vector<unsigned char> deviceData(dwordCount * 4);
        BindingGroupDSL::FlattenUniformsToConstantBufferData(value, deviceData.data());
        
        const int actualIndex = swapUniformAndVariableSizedArray ? (index - 1) : index;
        auto cbuffer = cbMgr->CreateConstantBuffer(deviceData.data(), deviceData.size());
        batch.Append(*group.GetRHIObject(), actualIndex, RHI::ConstantBufferUpdate
        {
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
