#pragma once

#include <Graphics/Device/BindingGroup.h>
#include <Graphics/Device/RGResourceForward.h>

RTRC_BEGIN

namespace BindingGroupStructDetail
{

    template<typename T> struct UNormTexelFormatWrapper { using WrappedType = T; };
    template<typename T> struct SNormTexelFormatWrapper { using WrappedType = T; };

    template<typename T> struct UNormTexelFormatWrapperTrait : std::false_type {};
    template<typename T> struct UNormTexelFormatWrapperTrait<UNormTexelFormatWrapper<T>> : std::true_type {};

    template<typename T> struct SNormTexelFormatWrapperTrait : std::false_type {};
    template<typename T> struct SNormTexelFormatWrapperTrait<SNormTexelFormatWrapper<T>> : std::true_type {};
    
    using FloatTexelFormatList = TypeList<float,        Vector2f, Vector3f, Vector4f>;
    using IntTexelFormatList   = TypeList<int,          Vector2i, Vector3i, Vector4i>;
    using UIntTexelFormatList  = TypeList<unsigned int, Vector2u, Vector3u, Vector4u>;

    using BasicTexelFormatList = CatTypeLists<
        FloatTexelFormatList,
        IntTexelFormatList,
        UIntTexelFormatList>;
    
    using UNormTexelFormatList = TypeList<
        UNormTexelFormatWrapper<float>,
        UNormTexelFormatWrapper<Vector2f>,
        UNormTexelFormatWrapper<Vector3f>,
        UNormTexelFormatWrapper<Vector4f>>;

    using SNormTexelFormatList = TypeList<
        SNormTexelFormatWrapper<float>,
        SNormTexelFormatWrapper<Vector2f>,
        SNormTexelFormatWrapper<Vector3f>,
        SNormTexelFormatWrapper<Vector4f>>;

    using TexelFormatList = CatTypeLists<
        BasicTexelFormatList,
        UNormTexelFormatList,
        SNormTexelFormatList>;

    template<typename T>
    concept TexelFormat = TexelFormatList::Contains<T>;

    template<TexelFormat T>
    constexpr bool ValidateFormat(RHI::Format format)
    {
        constexpr bool SNormAccess = SNormTexelFormatList::Contains<T>;
        constexpr bool UNormAccess = UNormTexelFormatList::Contains<T>;
        constexpr bool FloatAccess = FloatTexelFormatList::Contains<T> || SNormAccess || UNormAccess;
        constexpr bool UIntAccess  = UIntTexelFormatList::Contains<T>;
        constexpr bool IntAccess   = IntTexelFormatList::Contains<T>;
        if(FloatAccess && !CanBeAccessedAsFloatInShader(format))
        {
            return false;
        }
        if(IntAccess && !CanBeAccessedAsIntInShader(format))
        {
            return false;
        }
        if(UIntAccess && !CanBeAccessedAsUIntInShader(format))
        {
            return false;
        }

        // Disable validation for snorm/unorm declaration because we cannot determine
        // whether the resource will be accessed by uav reads here
        // 
        // if(SNormAccess != NeedSNormAsTypedUAV(format))
        // {
        //     return false;
        // }
        // if(UNormAccess != NeedUNormAsTypedUAV(format))
        // {
        //     return false;
        // }

        return true;
    }
    
    template<typename T>
    class StructuredBufferImpl
    {
    public:

        StructuredBufferImpl &operator=(const BufferSrv &srv)
        {
            assert(srv.GetRHIObject()->GetDesc().stride == sizeof(T));
            srv_ = srv;
            return *this;
        }

        StructuredBufferImpl &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetStructuredSrv(sizeof(T));
        }

        StructuredBufferImpl &operator=(const RG::BufferResource *buffer)
        {
            return *this = _internalRGGet(buffer)->GetStructuredSrv(sizeof(T));
        }

    private:

        BufferSrv srv_;
    };

    template<typename T>
    class RWStructuredBufferImpl
    {
    public:

        RWStructuredBufferImpl &operator=(const BufferUav &uav)
        {
            assert(uav_.GetRHIObject()->GetDesc().stride == sizeof(T));
            uav_ = uav;
            return *this;
        }

        RWStructuredBufferImpl &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetStructuredUav(sizeof(T));
        }

        RWStructuredBufferImpl &operator=(const RG::BufferResource *buffer)
        {
            return *this = _internalRGGet(buffer)->GetStructuredUav(sizeof(T));
        }

    private:

        BufferUav uav_;
    };

    template<TexelFormat T>
    class BufferImpl
    {
    public:

        BufferImpl &operator=(const BufferSrv &srv)
        {
            assert(ValidateFormat<T>(srv.GetFinalFormat()));
            srv_ = srv;
            return *this;
        }

        BufferImpl &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetTexelSrv();
        }

        BufferImpl &operator=(const RG::BufferResource *buffer)
        {
            return *this = _internalRGGet(buffer)->GetTexelSrv();
        }

    private:

        BufferSrv srv_;
    };

    template<TexelFormat T>
    class RWBufferImpl
    {
    public:

        RWBufferImpl &operator=(const BufferUav &uav)
        {
            assert(ValidateFormat<T>(uav.GetFinalFormat()));
            uav_ = uav;
            return *this;
        }

        RWBufferImpl &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetTexelUav();
        }

        RWBufferImpl &operator=(const RG::BufferResource *buffer)
        {
            return *this = _internalRGGet(buffer)->GetTexelUav();
        }

    private:

        BufferUav uav_;
    };

    template<TexelFormat T>
    class Texture2DImpl
    {
    public:

        Texture2DImpl &operator=(const TextureSrv &srv)
        {
            assert(ValidateFormat<T>(srv.GetFinalFormat()));
            srv_ = srv;
            return *this;
        }

        Texture2DImpl &operator=(const RC<Texture> &texture)
        {
            return *this = texture->GetSrv();
        }

        Texture2DImpl &operator=(const RG::TextureResource *texture)
        {
            return *this = _internalRGGet(texture)->GetSrv();
        }

    private:

        TextureSrv srv_;
    };

    template<TexelFormat T>
    class RWTexture2DImpl
    {
    public:

        RWTexture2DImpl &operator=(const TextureUav &uav)
        {
            assert(ValidateFormat<T>(uav.GetFinalFormat()));
            uav_ = uav;
            return *this;
        }

        RWTexture2DImpl &operator=(const RC<Texture> &texture)
        {
            return *this = texture->GetUav();
        }

        RWTexture2DImpl &operator=(const RG::TextureResource *texture)
        {
            return *this = _internalRGGet(texture)->GetUav();
        }

    private:

        TextureUav uav_;
    };

    template<RtrcReflShaderStruct T>
    class ConstantBufferImpl
    {
    public:

        ConstantBufferImpl &operator=(const RC<SubBuffer> &buffer)
        {
            buffer_ = buffer;
            return *this;
        }

        ConstantBufferImpl &operator=(const T &data)
        {
            data_ = data;
            return *this;
        }

    private:

        RC<SubBuffer> buffer_;
        T data_;
    };

    class SamplerImpl : public RHI::SamplerDesc
    {
    public:

        SamplerImpl &operator=(const RC<Sampler> &sampler)
        {
            sampler_ = sampler;
            return *this;
        }

    private:

        RC<Sampler> sampler_;
    };

    class RaytracingAccelerationStructureImpl
    {
    public:

        RaytracingAccelerationStructureImpl &operator=(const RC<Tlas> &tlas)
        {
            tlas_ = tlas;
            return *this;
        }

        RaytracingAccelerationStructureImpl &operator=(const RG::TlasResource *tlas)
        {
            return *this = _internalRGGet(tlas);
        }

    private:

        RC<Tlas> tlas_;
    };

    struct BindingGroupStructBase
    {
        struct _rtrcReflStructBaseBindingTag {};

        using float2 = Vector2f;
        using float3 = Vector3f;
        using float4 = Vector4f;
        using uint   = unsigned int;
        using uint2  = Vector2u;
        using uint3  = Vector3u;
        using uint4  = Vector4u;
        using int2   = Vector2i;
        using int3   = Vector3i;
        using int4   = Vector4i;

        template<typename T> using Texture2D          = Texture2DImpl<T>;
        template<typename T> using RWTexture2D        = RWTexture2DImpl<T>;
        template<typename T> using Buffer             = BufferImpl<T>;
        template<typename T> using RWBuffer           = RWBufferImpl<T>;
        template<typename T> using StructuredBuffer   = StructuredBufferImpl<T>;
        template<typename T> using RWStructuredBuffer = RWStructuredBufferImpl<T>;

        template<RtrcReflShaderStruct T>
        using ConstantBuffer = ConstantBufferImpl<T>;
        using SamplerState = SamplerImpl;
        using RaytracingAccelerationStructure = RaytracingAccelerationStructureImpl;

        auto operator<=>(const BindingGroupStructBase &) const = default;
    };

} // namespace BindingGroupStructDetail

using _rtrcReflStructBase_binding = BindingGroupStructDetail::BindingGroupStructBase;

template<typename T>
concept RtrcReflBindingStruct = requires { typename T::_rtrcReflStructBaseBindingTag; };

RTRC_END
