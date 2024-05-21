#pragma once

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Texture/Texture.h>
#include <Rtrc/Graphics/RenderGraph/Pass.h>

RTRC_BEGIN

class Sampler;

namespace BindingGroupDetail
{

    template<typename T, int Dim, bool IsArray>
    class MemberProxy_Texture
    {
        Variant<TextureSrv, RGTexSrv> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::Texture;

        TextureSrv GetRtrcObject() const
        {
            return data_.Match(
                [&](const TextureSrv &v) { return v; },
                [&](const RGTexSrv &v) { return v.GetSrv(); });
        }

        void DeclareRenderGraphResourceUsage(RGPass pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGTexSrv>())
            {
                d->ForEachSubresourceAccess(
                    [&](
                        const TexSubrsc        &subrsc,
                        RHI::TextureLayout      layout,
                        RHI::ResourceAccessFlag access)
                    {
                        pass->Use(
                            d->GetResource(),
                            subrsc,
                            RGUseInfo
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
            *this = tex->GetSrv(0, 0, 0);
            return *this;
        }

        auto &operator=(const RGTexSrv &srv)
        {
            data_ = srv;
            return *this;
        }

        auto &operator=(RGTexImpl *tex)
        {
            *this = tex->GetSrv();
            return *this;
        }
    };

    template<typename T, int Dim, bool IsArray>
    class MemberProxy_RWTexture
    {
        Variant<TextureUav, RGTexUav> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWTexture;

        bool writeOnly = false;

        TextureUav GetRtrcObject() const
        {
            return data_.Match(
                [&](const TextureUav &v) { return v; },
                [&](const RGTexUav &v) { return v.GetUav(); });
        }

        void DeclareRenderGraphResourceUsage(RGPassImpl *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGTexUav>())
            {
                d->ForEachSubresourceAccess(
                    [&](
                        const TexSubrsc        &subrsc,
                        RHI::TextureLayout      layout,
                        RHI::ResourceAccessFlag access)
                    {
                        pass->Use(
                            d->GetResource(),
                            subrsc,
                            RGUseInfo
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
            *this = tex->GetUav(0, 0);
            return *this;
        }

        auto &operator=(const RGTexUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGTexImpl *tex)
        {
            *this = tex->GetUav();
            return *this;
        }
    };

    template<typename T = void> using MemberProxy_Texture1D = MemberProxy_Texture<T, 1, false>;
    template<typename T = void> using MemberProxy_Texture2D = MemberProxy_Texture<T, 2, false>;
    template<typename T = void> using MemberProxy_Texture3D = MemberProxy_Texture<T, 3, false>;

    template<typename T = void> using MemberProxy_Texture1DArray = MemberProxy_Texture<T, 1, true>;
    template<typename T = void> using MemberProxy_Texture2DArray = MemberProxy_Texture<T, 2, true>;
    template<typename T = void> using MemberProxy_Texture3DArray = MemberProxy_Texture<T, 3, true>;

    template<typename T = void> using MemberProxy_RWTexture1D = MemberProxy_RWTexture<T, 1, false>;
    template<typename T = void> using MemberProxy_RWTexture2D = MemberProxy_RWTexture<T, 2, false>;
    template<typename T = void> using MemberProxy_RWTexture3D = MemberProxy_RWTexture<T, 3, false>;

    template<typename T = void> using MemberProxy_RWTexture1DArray = MemberProxy_RWTexture<T, 1, true>;
    template<typename T = void> using MemberProxy_RWTexture2DArray = MemberProxy_RWTexture<T, 2, true>;
    template<typename T = void> using MemberProxy_RWTexture3DArray = MemberProxy_RWTexture<T, 3, true>;

    template<typename T = void>
    class MemberProxy_Buffer
    {
        Variant<BufferSrv, RGBufSrv> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::Buffer;

        BufferSrv GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferSrv &v) { return v; },
                [&](const RGBufSrv &v) { return v.GetSrv(); });
        }

        void DeclareRenderGraphResourceUsage(RGPassImpl *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGBufSrv>())
            {
                pass->Use(d->GetResource(), RGUseInfo
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
            *this = buffer->GetTexelSrv();
            return *this;
        }

        auto &operator=(const RGBufSrv &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
        {
            *this = buffer->GetTexelSrv();
            return *this;
        }
    };

    template<typename T = void>
    class MemberProxy_RWBuffer
    {
    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWBuffer;

        bool writeOnly = false;

        BufferUav GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferUav &v) { return v; },
                [&](const RGBufUav &v) { return v.GetUav(); });
        }

        void DeclareRenderGraphResourceUsage(RGPassImpl *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGBufUav>())
            {
                pass->Use(d->GetResource(), RGUseInfo
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
            *this = buffer->GetTexelUav();
            return *this;
        }

        auto &operator=(const RGBufUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
        {
            *this = buffer->GetTexelUav();
            return *this;
        }

    private:

        Variant<BufferUav, RGBufUav> data_;
    };

    template<typename T = void>
    class MemberProxy_StructuredBuffer
    {
        Variant<BufferSrv, RGBufSrv> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::StructuredBuffer;

        BufferSrv GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferSrv &v) { return v; },
                [&](const RGBufSrv &v) { return v.GetSrv(); });
        }

        void DeclareRenderGraphResourceUsage(RGPassImpl *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGBufSrv>())
            {
                pass->Use(d->GetResource(), RGUseInfo
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
            *this = buffer->GetStructuredSrv();
            return *this;
        }

        auto &operator=(const RGBufSrv &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
        {
            *this = buffer->GetStructuredSrv();
            return *this;
        }
    };

    template<typename T = void>
    class MemberProxy_RWStructuredBuffer
    {
    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWStructuredBuffer;

        bool writeOnly = false;

        BufferUav GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferUav &v) { return v; },
                [&](const RGBufUav &v) { return v.GetUav(); });
        }

        void DeclareRenderGraphResourceUsage(RGPassImpl *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGBufUav>())
            {
                pass->Use(d->GetResource(), RGUseInfo
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
            *this = buffer->GetStructuredUav();
            return *this;
        }

        auto &operator=(const RGBufUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
        {
            *this = buffer->GetStructuredUav();
            return *this;
        }

    private:

        Variant<BufferUav, RGBufUav> data_;
    };

    class MemberProxy_ByteAddressBuffer
    {
        Variant<BufferSrv, RGBufSrv> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::ByteAddressBuffer;

        BufferSrv GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferSrv &v) { return v; },
                [&](const RGBufSrv &v) { return v.GetSrv(); });
        }

        void DeclareRenderGraphResourceUsage(RGPassImpl *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGBufSrv>())
            {
                pass->Use(d->GetResource(), RGUseInfo
                          {
                              .layout = RHI::TextureLayout::Undefined,
                              .stages = stages,
                              .accesses = d->GetResourceAccess()
                          });
            }
        }

        auto &operator=(BufferSrv value)
        {
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetByteAddressSrv();
        }

        auto &operator=(const RGBufSrv &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
        {
            return *this = buffer->GetByteAddressSrv();
        }
    };

    class MemberProxy_RWByteAddressBuffer
    {
    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWByteAddressBuffer;

        bool writeOnly = false;

        BufferUav GetRtrcObject() const
        {
            return data_.Match(
                [&](const BufferUav &v) { return v; },
                [&](const RGBufUav &v) { return v.GetUav(); });
        }

        void DeclareRenderGraphResourceUsage(RGPassImpl *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGBufUav>())
            {
                pass->Use(d->GetResource(), RGUseInfo
                          {
                              .layout = RHI::TextureLayout::Undefined,
                              .stages = stages,
                              .accesses = d->GetResourceAccess(writeOnly)
                          });
            }
        }

        auto &operator=(BufferUav value)
        {
            data_ = std::move(value);
            return *this;
        }

        auto &operator=(const RC<SubBuffer> &buffer)
        {
            return *this = buffer->GetByteAddressUav();
        }

        auto &operator=(const RGBufUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
        {
            return *this = buffer->GetByteAddressUav();
        }

    private:

        Variant<BufferUav, RGBufUav> data_;
    };

    template<typename T = void>
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
        Variant<RC<Tlas>, RGTlasImpl *> data_;

    public:

        static constexpr RHI::BindingType BindingType = RHI::BindingType::AccelerationStructure;

        RC<Tlas> GetRtrcObject() const
        {
            return data_.Match(
                [&](const RC<Tlas> &t) { return t; },
                [&](RGTlasImpl *t) { return t->Get(); });
        }

        void DeclareRenderGraphResourceUsage(RGPassImpl *pass, RHI::PipelineStageFlag stages) const
        {
            if(auto d = data_.AsIf<RGTlasImpl*>())
            {
                auto buffer = (*d)->GetInternalBuffer();
                pass->Use(buffer, RGUseInfo
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

        auto &operator=(RGTlasImpl *tlas)
        {
            data_ = tlas;
            return *this;
        }
    };

} // namespace BindingGroupDetail

RTRC_END
