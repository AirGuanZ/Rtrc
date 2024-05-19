#pragma once

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Texture/Texture.h>
#include <Rtrc/Graphics/RenderGraph/Pass.h>

RTRC_BEGIN

class Sampler;

namespace BindingGroupDetail
{

    class MemberProxy_Texture2D
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
            return *this = tex->GetSrv(0, 0, 0);
        }

        auto &operator=(const RGTexSrv &srv)
        {
            data_ = srv;
            return *this;
        }

        auto &operator=(RGTexImpl *tex)
        {
            return *this = tex->GetSrv();
        }
    };
    
    class MemberProxy_RWTexture2D
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
            return *this = tex->GetUav(0, 0);
        }

        auto &operator=(const RGTexUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGTexImpl *tex)
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
            return *this = buffer->GetTexelSrv();
        }

        auto &operator=(const RGBufSrv &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
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
            return *this = buffer->GetTexelUav();
        }

        auto &operator=(const RGBufUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
        {
            return *this = buffer->GetTexelUav();
        }

    private:

        Variant<BufferUav, RGBufUav> data_;
    };
    
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
            return *this = buffer->GetStructuredSrv();
        }

        auto &operator=(const RGBufSrv &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
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
            return *this = buffer->GetStructuredUav();
        }

        auto &operator=(const RGBufUav &v)
        {
            data_ = v;
            return *this;
        }

        auto &operator=(RGBufImpl *buffer)
        {
            return *this = buffer->GetStructuredUav();
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
