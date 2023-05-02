#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

namespace GBufferBindingDetail
{

    rtrc_group(GBuffers_All)
    {
        rtrc_define(Texture2D, _internalGBuffer_Normal);
        rtrc_define(Texture2D, _internalGBuffer_AlbedoMetallic);
        rtrc_define(Texture2D, _internalGBuffer_Roughness);
        rtrc_define(Texture2D, _internalGBuffer_Depth);
    };

    rtrc_group(GBuffers_NormalDepth)
    {
        rtrc_define(Texture2D, _internalGBuffer_Normal);
        rtrc_define(Texture2D, _internalGBuffer_Depth);
    };

    rtrc_group(GBuffers_Depth)
    {
        rtrc_define(Texture2D, _internalGBuffer_Depth);
    };

    template<bool Normal, bool Albedo, bool Metallic, bool Roughness, bool Depth>
    struct GBufferBindingTrait
    {

    };

    template<>
    struct GBufferBindingTrait<true, true, true, true, true>
    {
        using Bindings = GBuffers_All;
    };

    template<>
    struct GBufferBindingTrait<true, false, false, false, true>
    {
        using Bindings = GBuffers_NormalDepth;
    };

    template<>
    struct GBufferBindingTrait<false, false, false, false, true>
    {
        using Bindings = GBuffers_Depth;
    };

    template<typename T> concept NeedNormal         = requires(T & t) { t._internalGBuffer_Normal;         };
    template<typename T> concept NeedAlbedoMetallic = requires(T & t) { t._internalGBuffer_AlbedoMetallic; };
    template<typename T> concept NeedRoughness      = requires(T & t) { t._internalGBuffer_Roughness;      };
    template<typename T> concept NeedDepth          = requires(T & t) { t._internalGBuffer_Depth;          };

    template<bool Normal, bool Albedo, bool Metallic, bool Roughness, bool Depth, bool EnableStencilRead>
    void DeclarePassUses(RG::Pass *pass, const GBuffers &gbuffers, RHI::PipelineStageFlag stages)
    {
        const RG::UseInfo use =
        {
            .layout = RHI::TextureLayout::ShaderTexture,
            .stages = stages,
            .accesses = RHI::ResourceAccess::TextureRead
        };
        if constexpr(Normal)
        {
            pass->Use(gbuffers.normal, use);
        }
        if constexpr(Albedo || Metallic)
        {
            pass->Use(gbuffers.albedoMetallic, use);
        }
        if constexpr(Roughness)
        {
            pass->Use(gbuffers.roughness, use);
        }
        if constexpr(Depth)
        {
            if constexpr(EnableStencilRead)
            {
                pass->Use(gbuffers.depth, RG::UseInfo
                {
                    .layout = RHI::TextureLayout::DepthShaderTexture_StencilReadOnlyAttachment,
                    .stages = stages | RHI::PipelineStage::DepthStencil,
                    .accesses = RHI::ResourceAccess::TextureRead | RHI::ResourceAccess::DepthStencilRead
                });
            }
            else
            {
                pass->Use(gbuffers.depth, use);
            }
        }
        else if constexpr(EnableStencilRead)
        {
            pass->Use(gbuffers.depth, RG::UseInfo
            {
                .layout   = RHI::TextureLayout::DepthStencilReadOnlyAttachment,
                .stages   = RHI::PipelineStage::DepthStencil,
                .accesses = RHI::ResourceAccess::DepthStencilRead
            });
        }
    }

    template<typename T>
    concept IsGBufferBindings = std::is_same_v<T, GBuffers_All>
                             || std::is_same_v<T, GBuffers_NormalDepth>
                             || std::is_same_v<T, GBuffers_Depth>;

    template<IsGBufferBindings T, bool EnableStencilRead>
    void DeclarePassUses(RG::Pass *pass, const GBuffers &gbuffers, RHI::PipelineStageFlag stages)
    {
        constexpr bool Normal    = NeedNormal<T>;
        constexpr bool Albedo    = NeedAlbedoMetallic<T>;
        constexpr bool Metallic  = NeedAlbedoMetallic<T>;
        constexpr bool Roughness = NeedRoughness<T>;
        constexpr bool Depth     = NeedDepth<T>;
        DeclarePassUses<Normal, Albedo, Metallic, Roughness, Depth, EnableStencilRead>(pass, gbuffers, stages);
    }

    template<IsGBufferBindings T, bool EnableStencilRead>
    void FillBindingGroupData(T &data, const GBuffers &gbuffers, RG::PassContext &context)
    {
        if constexpr(NeedNormal<T>)
        {
            data._internalGBuffer_Normal = gbuffers.normal->Get(context);
        }
        if constexpr(NeedAlbedoMetallic<T>)
        {
            data._internalGBuffer_AlbedoMetallic = gbuffers.albedoMetallic->Get(context);
        }
        if constexpr(NeedRoughness<T>)
        {
            data._internalGBuffer_Roughness = gbuffers.roughness->Get(context);
        }
        if constexpr(NeedDepth<T>)
        {
            constexpr auto SrvFlags = EnableStencilRead ? RHI::TextureViewFlags::DepthSrv_StencilAttachmentReadOnly
                                                        : RHI::TextureViewFlags::None;
            data._internalGBuffer_Depth = gbuffers.depth->Get(context)->CreateSrv(SrvFlags);
        }
    }

} // namespace GBufferBindingDetail

template<bool Normal, bool Albedo, bool Metallic, bool Roughness, bool Depth>
using GBufferBindings = typename GBufferBindingDetail::GBufferBindingTrait<
    Normal, Albedo, Metallic, Roughness, Depth>::Bindings;

using GBufferBindings_All         = GBufferBindings<true, true, true, true, true>;
using GBufferBindings_NormalDepth = GBufferBindings<true, false, false, false, true>;
using GBufferBindings_Depth       = GBufferBindings<false, false, false, false, true>;

template<bool EnableStencilRead, typename T>
void DeclareGBufferUses(RG::Pass *pass, const GBuffers &gbuffers, RHI::PipelineStageFlag stages)
{
    if constexpr(GBufferBindingDetail::IsGBufferBindings<T>)
    {
        GBufferBindingDetail::DeclarePassUses<T, EnableStencilRead>(pass, gbuffers, stages);
    }
    else
    {
        using TT = std::remove_reference_t<decltype(std::declval<T&>().gbuffers)>;
        GBufferBindingDetail::DeclarePassUses<TT, EnableStencilRead>(pass, gbuffers, stages);
    }
}

template<bool EnableStencilRead, typename T>
void FillBindingGroupGBuffers(T &data, const GBuffers &gbuffers, RG::PassContext &context)
{
    if constexpr(GBufferBindingDetail::IsGBufferBindings<T>)
    {
        GBufferBindingDetail::FillBindingGroupData<T, EnableStencilRead>(data, gbuffers, context);
    }
    else
    {
        using TT = std::remove_reference_t<decltype(std::declval<T &>().gbuffers)>;
        GBufferBindingDetail::FillBindingGroupData<TT, EnableStencilRead>(data.gbuffers, gbuffers, context);
    }
}

RTRC_RENDERER_END
