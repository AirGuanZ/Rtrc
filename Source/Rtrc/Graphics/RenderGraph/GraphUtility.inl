#pragma once

#include <Rtrc/RHI/RHIDeclaration.h>

RTRC_BEGIN

/**
 * RGClearColor
 * RGClearDepthStencil
 * RGClearRWBuffer
 * RGClearRWStructuredBuffer
 * RGCopyBuffer
 * RGCopyColorTexture
 * RGClearRWTexture2D
 * RGBlitTexture
 * RGDispatchWithThreadGroupCount
 * RGDispatchWithThreadCount
 * RGDispatchIndirect
 * RGAddRenderPass
 * RGReadbackBuffer
 * RGReadbackTexture
 */

inline RGPass RGClearColor(
    GraphRef    graph,
    std::string name,
    RGTexture   tex,
    const Vector4f &clearValue)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex, RG::ClearColor);
    pass->SetCallback([tex, clearValue]
    {
        RGGetCommandBuffer().ClearColorTexture2D(tex->Get(), clearValue);
    });
    return pass;
}

inline RGPass RGClearDepthStencil(
    GraphRef    graph,
    std::string name,
    RGTexture   tex,
    float       depth,
    uint8_t     stencil)
{
    assert(HasDepthAspect(tex->GetFormat()));
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex, RG::ClearDepthStencil);
    pass->SetCallback([tex, depth, stencil]
    {
        auto &cmds = RGGetCommandBuffer();
        if(HasStencilAspect(tex->GetFormat()))
        {
            cmds.ClearDepthStencil(tex->Get(), depth, stencil);
        }
        else
        {
            cmds.ClearDepth(tex->Get(), depth);
        }
    });
    return pass;
}

inline RGPass RGClearRWBuffer(
    GraphRef    graph,
    std::string name,
    RGBuffer    buffer,
    uint32_t    value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(buffer, RG::CS_RWBuffer_WriteOnly);
    pass->SetCallback([buffer, value]
    {
        auto &cmds = RGGetCommandBuffer();
        cmds.GetDevice()->GetClearBufferUtils().ClearRWBuffer(cmds, buffer->Get(), value);
    });
    return pass;
}

inline RGPass RGClearRWStructuredBuffer(
    GraphRef    graph,
    std::string name,
    RGBuffer    buffer,
    uint32_t    value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(buffer, RG::CS_RWStructuredBuffer_WriteOnly);
    pass->SetCallback([buffer, value]
    {
        auto &cmds = RGGetCommandBuffer();
        cmds.GetDevice()->GetClearBufferUtils().ClearRWStructuredBuffer(cmds, buffer->Get(), value);
    });
    return pass;
}

inline RGPass RGCopyBuffer(
    GraphRef    graph,
    std::string name,
    RGBuffer    src,
    size_t      srcOffset,
    RGBuffer    dst,
    size_t      dstOffset,
    size_t      size)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(src, RG::CopySrc);
    pass->Use(dst, RG::CopyDst);
    pass->SetCallback([=]
    {
        RGGetCommandBuffer().CopyBuffer(dst, dstOffset, src, srcOffset, size);
    });
    return pass;
}

inline RGPass RGCopyBuffer(
    GraphRef    graph,
    std::string name,
    RGBuffer    src,
    RGBuffer    dst,
    size_t      size)
{
    return RGCopyBuffer(graph, std::move(name), src, 0, dst, 0, size);
}

inline RGPass RGCopyBuffer(
    GraphRef    graph,
    std::string name,
    RGBuffer    src,
    RGBuffer    dst)
{
    assert(src->GetSize() == dst->GetSize());
    return RGCopyBuffer(graph, std::move(name), src, dst, src->GetSize());
}

inline RGPass RGCopyColorTexture(
    GraphRef          graph,
    std::string       name,
    RGTexture         src,
    const TexSubrscs &srcSubrscs,
    RGTexture         dst,
    const TexSubrscs &dstSubrscs)
{
    assert(srcSubrscs.layerCount == dstSubrscs.layerCount);
    assert(srcSubrscs.levelCount == dstSubrscs.levelCount);
    auto pass = graph->CreatePass(std::move(name));
    for(unsigned a = srcSubrscs.arrayLayer; a < srcSubrscs.arrayLayer + srcSubrscs.layerCount; ++a)
    {
        assert(a < src->GetArraySize());
        for(unsigned m = srcSubrscs.mipLevel; m < srcSubrscs.mipLevel + srcSubrscs.levelCount; ++m)
        {
            assert(m < src->GetMipLevels());
            pass->Use(src, TexSubrsc{ m, a }, RG::CopySrc);
        }
    }
    for(unsigned a = dstSubrscs.arrayLayer; a < dstSubrscs.arrayLayer + dstSubrscs.layerCount; ++a)
    {
        assert(a < dst->GetArraySize());
        for(unsigned m = dstSubrscs.mipLevel; m < dstSubrscs.mipLevel + dstSubrscs.levelCount; ++m)
        {
            assert(m < dst->GetMipLevels());
            pass->Use(dst, TexSubrsc{ m, a }, RG::CopyDst);
        }
    }
    pass->SetCallback([src, srcSubrscs, dst, dstSubrscs]
    {
        auto &cmds = RGGetCommandBuffer();
        auto tsrc = src->Get();
        auto tdst = dst->Get();
        for(unsigned ai = 0; ai < srcSubrscs.layerCount; ++ai)
        {
            const unsigned srcArrayLayer = srcSubrscs.arrayLayer + ai;
            const unsigned dstArrayLayer = dstSubrscs.arrayLayer + ai;
            for(unsigned mi = 0; mi < srcSubrscs.levelCount; ++mi)
            {
                const unsigned srcMipLevel = srcSubrscs.mipLevel + mi;
                const unsigned dstMipLevel = dstSubrscs.mipLevel + mi;
                assert(tsrc->GetMipLevelSize(srcMipLevel) == tdst->GetMipLevelSize(dstMipLevel));
                cmds.CopyColorTexture(*tdst, dstMipLevel, dstArrayLayer, *tsrc, srcMipLevel, srcArrayLayer);
            }
        }
    });
    return pass;
}

inline RGPass RGCopyColorTexture(
    GraphRef          graph,
    std::string       name,
    RGTexture         src,
    RGTexture         dst,
    const TexSubrscs &subrscs)
{
    return RGCopyColorTexture(graph, std::move(name), src, subrscs, dst, subrscs);
}

inline RGPass RGCopyColorTexture(
    GraphRef    graph,
    std::string name,
    RGTexture   src,
    RGTexture   dst)
{
    assert(src->GetArraySize() == dst->GetArraySize());
    assert(src->GetMipLevels() == dst->GetMipLevels());
    assert(src->GetSize() == dst->GetSize());
    return RGCopyColorTexture(graph, std::move(name), src, dst, TexSubrscs
                            {
                                .mipLevel = 0,
                                .levelCount = src->GetMipLevels(),
                                .arrayLayer = 0,
                                .layerCount = dst->GetArraySize()
                            });
}

inline RGPass RGClearRWTexture2D(
    GraphRef        graph,
    std::string     name,
    RGTexture       tex2D,
    const Vector4f &value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex2D, RG::CS_RWTexture);
    pass->SetCallback([tex2D, value]
    {
        auto& cmds = RGGetCommandBuffer();
        cmds.GetDevice()->GetClearTextureUtils().ClearRWTexture2D(cmds, tex2D->GetUavImm(), value);
    });
    return pass;
}

inline RGPass RGClearRWTexture2D(
    GraphRef        graph,
    std::string     name,
    RGTexture       tex2D,
    const Vector4u &value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex2D, RG::CS_RWTexture);
    pass->SetCallback([tex2D, value]
    {
        auto& cmds = RGGetCommandBuffer();
        cmds.GetDevice()->GetClearTextureUtils().ClearRWTexture2D(cmds, tex2D->GetUavImm(), value);
    });
    return pass;
}

inline RGPass RGClearRWTexture2D(
    GraphRef        graph,
    std::string     name,
    RGTexture       tex2D,
    const Vector4i &value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex2D, RG::CS_RWTexture);
    pass->SetCallback([tex2D, value]
    {
        auto& cmds = RGGetCommandBuffer();
        cmds.GetDevice()->GetClearTextureUtils().ClearRWTexture2D(cmds, tex2D->GetUavImm(), value);
    });
    return pass;
}

inline RGPass RGBlitTexture(
    GraphRef         graph,
    std::string      name,
    RGTexture        src,
    const TexSubrsc &srcSubrsc,
    RGTexture        dst,
    const TexSubrsc &dstSubrsc,
    bool             usePointSampling = false,
    float            gamma = 1.0f)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(src, RG::PS_Texture);
    pass->Use(dst, RG::ColorAttachmentWriteOnly);
    pass->SetCallback([=]
    {
        auto& cmds = RGGetCommandBuffer();
        cmds.GetDevice()->GetCopyTextureUtils().RenderFullscreenTriangle(
            cmds,
            src->GetSrvImm(srcSubrsc.mipLevel, 1, srcSubrsc.arrayLayer),
            dst->GetRtvImm(dstSubrsc.mipLevel, dstSubrsc.arrayLayer),
            usePointSampling ? CopyTextureUtils::Point : CopyTextureUtils::Linear, gamma);
    });
    return pass;
}

inline RGPass RGBlitTexture(
    GraphRef    graph,
    std::string name,
    RGTexture   src,
    RGTexture   dst,
    bool        usePointSampling = false,
    float       gamma = 1.0f)
{
    assert(src->GetArraySize() == 1);
    assert(src->GetMipLevels() == 1);
    assert(dst->GetArraySize() == 1);
    assert(dst->GetMipLevels() == 1);
    return RGBlitTexture(graph, std::move(name), src, { 0, 0 }, dst, { 0, 0 }, usePointSampling, gamma);
}

template<RGBindingGroupInput...Ts>
RGPass RGDispatchWithThreadGroupCount(
    GraphRef        graph,
    std::string     name,
    RC<Shader>      shader,
    const Vector3i &threadGroupCount,
    const Ts&...    bindingGroups)
{
    auto pass = graph->CreatePass(std::move(name));
    auto DeclareUse = [&]<RGBindingGroupInput T>(const T &group)
    {
        if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
        {
            DeclareRenderGraphResourceUses(pass, group, RHI::PipelineStageFlag::ComputeShader);
        }
    };
    (DeclareUse(bindingGroups), ...);
    pass->SetCallback([s = std::move(shader), threadGroupCount, ...bindingGroups = bindingGroups]
    {
        auto &commandBuffer = RGGetCommandBuffer();
        auto device = commandBuffer.GetDevice();

        std::vector<RC<BindingGroup>> finalGroups;
        auto BindGroup = [&]<RGBindingGroupInput T>(const T &group)
        {
            if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
            {
                finalGroups.push_back(device->CreateBindingGroupWithCachedLayout(group));
            }
            else
            {
                finalGroups.push_back(group);
            }
        };
        (BindGroup(bindingGroups), ...);

        commandBuffer.BindComputePipeline(s->GetComputePipeline());
        commandBuffer.BindComputeGroups(finalGroups);
        commandBuffer.Dispatch(threadGroupCount);
    });
    return pass;
}

template<RGStaticShader S, RGBindingGroupInput...Ts>
RGPass RGDispatchWithThreadGroupCount(
    GraphRef        graph,
    const Vector3i &threadGroupCount,
    const Ts&...    bindingGroups)
{
    return RGDispatchWithThreadGroupCount(
        graph, S::Name, graph->GetDevice()->GetShader<S::Name>(), threadGroupCount, bindingGroups...);
}

template<RGBindingGroupInput...Ts>
RGPass RGDispatchWithThreadCount(
    GraphRef        graph,
    std::string     name,
    RC<Shader>      shader,
    const Vector3i &threadCount,
    const Ts&...    bindingGroups)
{
    const Vector3i groupCount = shader->ComputeThreadGroupCount(threadCount);
    return RGDispatchWithThreadGroupCount(graph, std::move(name), std::move(shader), groupCount, bindingGroups...);
}

template<RGStaticShader S, RGBindingGroupInput...Ts>
RGPass RGDispatchWithThreadCount(
    GraphRef        graph,
    const Vector3i &threadCount,
    const Ts&...    bindingGroups)
{
    return RGDispatchWithThreadCount(
        graph, S::Name, graph->GetDevice()->GetShader<S::Name>(), threadCount, bindingGroups...);
}

template<RGBindingGroupInput...Ts>
RGPass RGDispatchIndirect(
    GraphRef       graph,
    std::string    name,
    RC<Shader>     shader,
    RGBuffer       indirectBuffer,
    size_t         indirectBufferOffset,
    const Ts&...   bindingGroups)
{
    auto pass = graph->CreatePass(std::move(name));
    auto DeclareUse = [&]<RGBindingGroupInput T>(const T & group)
    {
        if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
        {
            DeclareRenderGraphResourceUses(pass, group, RHI::PipelineStageFlag::ComputeShader);
        }
    };
    (DeclareUse(bindingGroups), ...);
    pass->Use(indirectBuffer, RG::IndirectDispatchRead);
    pass->SetCallback(
        [s = std::move(shader), indirectBuffer, indirectBufferOffset, ...bindingGroups = bindingGroups]
    {
        auto &commandBuffer = RGGetCommandBuffer();
        auto device = commandBuffer.GetDevice();

        std::vector<RC<BindingGroup>> finalGroups;
        auto BindGroup = [&]<RGBindingGroupInput T>(const T &group)
        {
            if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
            {
                finalGroups.push_back(device->CreateBindingGroupWithCachedLayout(group));
            }
            else
            {
                finalGroups.push_back(group);
            }
        };
        (BindGroup(bindingGroups), ...);

        commandBuffer.BindComputePipeline(s->GetComputePipeline());
        commandBuffer.BindComputeGroups(finalGroups);
        commandBuffer.DispatchIndirect(indirectBuffer->Get(), indirectBufferOffset);
    });
    return pass;
}

template<RGStaticShader S, RGBindingGroupInput...Ts>
RGPass RGDispatchIndirect(
    GraphRef     graph,
    RGBuffer     indirectBuffer,
    size_t       indirectBufferOffset,
    const Ts&... bindingGroups)
{
    return RGDispatchIndirect(
        graph, S::Name, graph->GetDevice()->GetShader<S::Name>(),
        indirectBuffer, indirectBufferOffset, bindingGroups...);
}

#define DEFINE_DISPATCH(EXPR, ...)                                                  \
    template<RGBindingGroupInput...Ts>                                              \
    RGPass RGDispatchWithThreadGroupCount(                                          \
        GraphRef  graph,                                                            \
        std::string     name,                                                       \
        RC<Shader>      shader,                                                     \
        __VA_ARGS__,                                                                \
        const Ts&...    bindingGroups)                                              \
        {                                                                           \
            return RGDispatchWithThreadGroupCount(                                  \
                graph, std::move(name), std::move(shader), EXPR, bindingGroups...); \
        }                                                                           \
    template<RGBindingGroupInput...Ts>                                              \
    RGPass RGDispatchWithThreadCount(                                               \
        GraphRef  graph,                                                            \
        std::string     name,                                                       \
        RC<Shader>      shader,                                                     \
        __VA_ARGS__,                                                                \
        const Ts&...    bindingGroups)                                              \
        {                                                                           \
            return RGDispatchWithThreadCount(                                       \
                graph, std::move(name), std::move(shader), EXPR, bindingGroups...); \
        }                                                                           \
    template<RGStaticShader S, RGBindingGroupInput...Ts>                            \
    RGPass RGDispatchWithThreadGroupCount(                                          \
        GraphRef  graph,                                                            \
        __VA_ARGS__,                                                                \
        const Ts&...    bindingGroups)                                              \
        {                                                                           \
            return RGDispatchWithThreadGroupCount<S>(                               \
                graph, EXPR, bindingGroups...);                                     \
        }                                                                           \
    template<RGStaticShader S, RGBindingGroupInput...Ts>                            \
    RGPass RGDispatchWithThreadCount(                                               \
        GraphRef  graph,                                                            \
        __VA_ARGS__,                                                                \
        const Ts&...    bindingGroups)                                              \
        {                                                                           \
            return RGDispatchWithThreadCount<S>(                                    \
                graph, EXPR, bindingGroups...);                                     \
        }

DEFINE_DISPATCH(Vector3i(t.x, t.y, t.z), const Vector3u &t)
DEFINE_DISPATCH(Vector3i(t.x, t.y, 1), const Vector2i &t)
DEFINE_DISPATCH(Vector3i(t.x, t.y, 1), const Vector2u &t)
DEFINE_DISPATCH(Vector3i(x, 1, 1), unsigned int x)
DEFINE_DISPATCH(Vector3i(x, y, 1), unsigned int x, unsigned int y)
DEFINE_DISPATCH(Vector3i(x, y, z), unsigned int x, unsigned int y, unsigned int z)

#undef DEFINE_DISPATCH

struct RGColorAttachment
{
    RGTexRtv               rtv;
    RHI::AttachmentLoadOp  loadOp = RHI::AttachmentLoadOp::Load;
    RHI::AttachmentStoreOp storeOp = RHI::AttachmentStoreOp::Store;
    RHI::ColorClearValue   clearValue = {};
    bool                   isWriteOnly = false;
};

struct RGDepthStencilAttachment
{
    RGTexDsv                    dsv;
    RHI::AttachmentLoadOp       loadOp = RHI::AttachmentLoadOp::Load;
    RHI::AttachmentStoreOp      storeOp = RHI::AttachmentStoreOp::Store;
    RHI::DepthStencilClearValue clearValue = {};
    bool                        isWriteOnly = false;
};

template<bool AutoViewportScissor, typename Callback>
RGPass RGAddRenderPass(
    GraphRef                 graph,
    std::string              name,
    Span<RGColorAttachment>  colorAttachments,
    RGDepthStencilAttachment depthStencilAttachment,
    Callback                 callback)
{
    auto pass = graph->CreatePass(std::move(name));
    for(auto &ca : colorAttachments)
    {
        pass->Use(
            ca.rtv.GetResource(),
            ca.isWriteOnly ? RG::ColorAttachmentWriteOnly : RG::ColorAttachment);
    }
    if(depthStencilAttachment.dsv.GetResource())
    {
        pass->Use(
            depthStencilAttachment.dsv.GetResource(),
            depthStencilAttachment.isWriteOnly ? RG::DepthStencilAttachmentWriteOnly : RG::DepthStencilAttachment);
    }
    StaticVector<RGColorAttachment, 8> clonedColorAttachments;
    for(auto &c : colorAttachments)
    {
        clonedColorAttachments.PushBack(c);
    }
    pass->SetCallback([c = std::move(callback), cas = std::move(clonedColorAttachments), d = depthStencilAttachment]
    {
        auto &cmds = RGGetCommandBuffer();

        StaticVector<RHI::ColorAttachment, 8> colorAttachments;
        for(auto &ca : cas)
        {
            colorAttachments.PushBack(RHI::ColorAttachment
            {
                .renderTargetView = ca.rtv.GetRtv().GetRHIObject(),
                .loadOp           = ca.loadOp,
                .storeOp          = ca.storeOp,
                .clearValue       = ca.clearValue
            });
        }
        cmds.BeginRenderPass(colorAttachments, RHI::DepthStencilAttachment
        {
            .depthStencilView = d.dsv ? d.dsv.GetDsv().GetRHIObject() : nullptr,
            .loadOp           = d.loadOp,
            .storeOp          = d.storeOp,
            .clearValue       = d.clearValue
        });
        RTRC_SCOPE_EXIT{ cmds.EndRenderPass(); };

        if constexpr(AutoViewportScissor)
        {
            cmds.SetViewportAndScissorAutomatically();
        }

        if constexpr(requires{ RGPassImpl::Callback(std::move(c)); })
        {
            c();
        }
        else
        {
            c(RGGetPassContext());
        }
    });
    return pass;
}

template<bool AutoViewportScissor, typename Callback>
RGPass RGAddRenderPass(
    GraphRef                graph,
    std::string             name,
    Span<RGColorAttachment> colorAttachments,
    Callback                callback)
{
    return RGAddRenderPass<AutoViewportScissor>(
        graph, std::move(name), colorAttachments, RGDepthStencilAttachment{}, std::move(callback));
}

template<bool AutoViewportScissor, typename Callback>
RGPass RGAddRenderPass(
    GraphRef                 graph,
    std::string              name,
    RGDepthStencilAttachment depthStencilAttachment,
    Callback                 callback)
{
    return RGAddRenderPass<AutoViewportScissor>(
        graph, std::move(name), {}, std::move(depthStencilAttachment), std::move(callback));
}

template<typename Callback>
RGPass RGAddRenderPass(
    GraphRef                 graph,
    std::string              name,
    Span<RGColorAttachment>  colorAttachments,
    RGDepthStencilAttachment depthStencilAttachment,
    Callback                 callback)
{
    return RGAddRenderPass<false>(
        graph, std::move(name), colorAttachments, depthStencilAttachment, std::move(callback));
}

template<typename Callback>
RGPass RGAddRenderPass(
    GraphRef                graph,
    std::string             name,
    Span<RGColorAttachment> colorAttachments,
    Callback                callback)
{
    return RGAddRenderPass<false>(
        graph, std::move(name), colorAttachments, RGDepthStencilAttachment{}, std::move(callback));
}

template<typename Callback>
RGPass RGAddRenderPass(
    GraphRef                 graph,
    std::string              name,
    RGDepthStencilAttachment depthStencilAttachment,
    Callback                 callback)
{
    return RGAddRenderPass<false>(
        graph, std::move(name), {}, std::move(depthStencilAttachment), std::move(callback));
}

inline void RGReadbackBuffer(
    GraphRef    graph,
    std::string name,
    RGBuffer    buffer,
    void       *output,
    size_t      offset,
    size_t      size)
{
    RTRC_RG_SCOPED_PASS_GROUP(graph, std::move(name));

    if(!size)
    {
        return;
    }
    assert(offset + size <= buffer->GetSize());
    assert(buffer->GetDesc().hostAccessType == RHI::BufferHostAccessType::Readback ||
           buffer->GetDesc().usage.Contains(RHI::BufferUsage::TransferSrc));

    auto stagingBuffer = buffer;
    if(stagingBuffer->GetDesc().hostAccessType != RHI::BufferHostAccessType::Readback)
    {
        stagingBuffer = graph->CreateBuffer(RHI::BufferDesc
        {
            .size = size,
            .usage = RHI::BufferUsage::TransferDst,
            .hostAccessType = RHI::BufferHostAccessType::Readback
        }, "RGReadbackBuffer-StagingBuffer");
        RGCopyBuffer(graph, "RGReadbackBuffer-CopyToStagingBuffer", buffer, offset, stagingBuffer, 0, size);
        offset = 0;
    }

    auto pass = graph->CreatePass("RGReadbackBuffer-ReadbackStagingBuffer");
    pass->SyncQueueBeforeExecution();
    pass->ClearUAVOverlapGroup();
    pass->Use(stagingBuffer, RGUseInfo // Add dummy use info to make sure this pass is executed after copy pass
    {
        .stages = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::DummyReadAndWrite
    });
    pass->SetCallback([stagingBuffer, output, offset, size]
    {
        stagingBuffer->Get()->Download(output, offset, size);
    });
}

inline void RGReadbackTexture(
    GraphRef    graph,
    std::string name,
    RGTexture   texture,
    uint32_t    arrayLayer,
    uint32_t    mipLevel,
    void       *output)
{
    RTRC_RG_SCOPED_PASS_GROUP(graph, std::move(name));

    assert(texture->GetDesc().usage.Contains(RHI::TextureUsage::TransferSrc));

    const size_t rowAlignment = graph->GetDevice()->GetTextureBufferCopyRowPitchAlignment(texture->GetFormat());
    const size_t unalignedRowSize = GetTexelSize(texture->GetFormat()) * texture->GetWidth();
    const size_t alignedRowSize = UpAlignTo(unalignedRowSize, rowAlignment);
    const size_t stagingBufferSize = texture->GetHeight() * alignedRowSize;

    auto stagingBuffer = graph->CreateBuffer(RHI::BufferDesc
    {
        .size = stagingBufferSize,
        .usage = RHI::BufferUsage::TransferDst,
        .hostAccessType = RHI::BufferHostAccessType::Readback
    }, "RGReadbackTexture-StagingBuffer");

    auto copyPass = graph->CreatePass("RGReadbackTexture-CopyToStagingBuffer");
    copyPass->Use(texture, RG::CopySrc);
    copyPass->Use(stagingBuffer, RG::CopyDst);
    copyPass->ClearUAVOverlapGroup();
    copyPass->SetCallback([texture, stagingBuffer, alignedRowSize, arrayLayer, mipLevel]
    {
        RGGetCommandBuffer().CopyColorTexture2DToBuffer(
            stagingBuffer, 0, alignedRowSize, texture, arrayLayer, mipLevel);
    });

    auto readbackPass = graph->CreatePass("RGReadbackTexture-ReadbackStagingBuffer");
    readbackPass->SyncQueueBeforeExecution();
    readbackPass->ClearUAVOverlapGroup();
    readbackPass->Use(stagingBuffer, RGUseInfo
    {
        .stages = RHI::PipelineStage::ComputeShader,
        .accesses = RHI::ResourceAccess::DummyReadAndWrite
    });
    readbackPass->SetCallback([stagingBuffer, texture, output, unalignedRowSize, alignedRowSize, stagingBufferSize]
    {
        std::vector<unsigned char> data(stagingBufferSize);
        stagingBuffer->Get()->Download(data.data(), 0, data.size());
        for(unsigned y = 0; y < texture->GetHeight(); ++y)
        {
            auto src = data.data() + y * alignedRowSize;
            auto dst = reinterpret_cast<unsigned char *>(output) + y * unalignedRowSize;
            std::memcpy(dst, src, unalignedRowSize);
        }
    });
}

RTRC_END
