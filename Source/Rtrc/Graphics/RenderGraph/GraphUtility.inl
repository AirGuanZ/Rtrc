#pragma once

RTRC_RG_BEGIN

/**
 * ClearTexture
 * ClearRWBuffer
 * ClearRWStructuredBuffer
 * CopyBuffer
 * CopyColorTexture
 * ClearRWTexture2D
 * BlitTexture
 * DispatchWithThreadGroupCount
 * DispatchWithThreadCount
 * DispatchIndirect
 */

inline Pass *ClearTexture(
    RenderGraphRef  graph,
    std::string     name,
    RGTexture       tex,
    const Vector4f &clearValue)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex, ClearDst);
    pass->SetCallback([tex, clearValue](PassContext &context)
    {
        context.GetCommandBuffer().ClearColorTexture2D(tex->Get(), clearValue);
    });
    return pass;
}

inline Pass *ClearRWBuffer(
    RenderGraphRef graph,
    std::string    name,
    RGBuffer       buffer,
    uint32_t       value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(buffer, CS_RWBuffer_WriteOnly);
    pass->SetCallback([buffer, value]
    {
        auto &cmds = GetCurrentCommandBuffer();
        cmds.GetDevice()->GetClearBufferUtils().ClearRWBuffer(cmds, buffer->Get(), value);
    });
    return pass;
}

inline Pass *ClearRWStructuredBuffer(
    RenderGraphRef graph,
    std::string    name,
    RGBuffer       buffer,
    uint32_t       value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(buffer, CS_RWStructuredBuffer_WriteOnly);
    pass->SetCallback([buffer, value]
    {
        auto &cmds = GetCurrentCommandBuffer();
        cmds.GetDevice()->GetClearBufferUtils().ClearRWStructuredBuffer(cmds, buffer->Get(), value);
    });
    return pass;
}

inline Pass *CopyBuffer(
    RenderGraphRef graph,
    std::string    name,
    RGBuffer       src,
    size_t         srcOffset,
    RGBuffer       dst,
    size_t         dstOffset,
    size_t         size)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(src, CopySrc);
    pass->Use(dst, CopyDst);
    pass->SetCallback([=]
    {
        GetCurrentCommandBuffer().CopyBuffer(dst, dstOffset, src, srcOffset, size);
    });
    return pass;
}

inline Pass *CopyBuffer(
    RenderGraphRef graph,
    std::string    name,
    RGBuffer       src,
    RGBuffer       dst,
    size_t         size)
{
    return CopyBuffer(graph, std::move(name), src, 0, dst, 0, size);
}

inline Pass *CopyBuffer(
    RenderGraphRef graph,
    std::string    name,
    RGBuffer       src,
    RGBuffer       dst)
{
    assert(src->GetSize() == dst->GetSize());
    return CopyBuffer(graph, std::move(name), src, dst, src->GetSize());
}

inline Pass *CopyColorTexture(
    RenderGraphRef    graph,
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
            pass->Use(src, TexSubrsc{ m, a }, CopySrc);
        }
    }
    for(unsigned a = dstSubrscs.arrayLayer; a < dstSubrscs.arrayLayer + dstSubrscs.layerCount; ++a)
    {
        assert(a < dst->GetArraySize());
        for(unsigned m = dstSubrscs.mipLevel; m < dstSubrscs.mipLevel + dstSubrscs.levelCount; ++m)
        {
            assert(m < dst->GetMipLevels());
            pass->Use(dst, TexSubrsc{ m, a }, CopyDst);
        }
    }
    pass->SetCallback([src, srcSubrscs, dst, dstSubrscs]
    {
        auto &cmds = GetCurrentCommandBuffer();
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
                assert(tsrc->GetSize(srcMipLevel) == tdst->GetSize(dstMipLevel));
                cmds.CopyColorTexture(*tdst, dstMipLevel, dstArrayLayer, *tsrc, srcMipLevel, srcArrayLayer);
            }
        }
    });
    return pass;
}

inline Pass *CopyColorTexture(
    RenderGraphRef    graph,
    std::string       name,
    RGTexture         src,
    RGTexture         dst,
    const TexSubrscs &subrscs)
{
    return CopyColorTexture(graph, std::move(name), src, subrscs, dst, subrscs);
}

inline Pass *CopyColorTexture(
    RenderGraphRef graph,
    std::string    name,
    RGTexture      src,
    RGTexture      dst)
{
    assert(src->GetArraySize() == dst->GetArraySize());
    assert(src->GetMipLevels() == dst->GetMipLevels());
    assert(src->GetSize() == dst->GetSize());
    return CopyColorTexture(graph, std::move(name), src, dst, TexSubrscs
                            {
                                .mipLevel = 0,
                                .levelCount = src->GetMipLevels(),
                                .arrayLayer = 0,
                                .layerCount = dst->GetArraySize()
                            });
}

inline Pass *ClearRWTexture2D(
    RenderGraphRef  graph,
    std::string     name,
    RGTexture       tex2D,
    const Vector4f &value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex2D, CS_RWTexture);
    pass->SetCallback([tex2D, value]
    {
        auto& cmds = GetCurrentCommandBuffer();
        cmds.GetDevice()->GetClearTextureUtils().ClearRWTexture2D(cmds, tex2D->GetUavImm(), value);
    });
    return pass;
}

inline Pass *ClearRWTexture2D(
    RenderGraphRef  graph,
    std::string     name,
    RGTexture       tex2D,
    const Vector4u &value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex2D, CS_RWTexture);
    pass->SetCallback([tex2D, value]
    {
        auto& cmds = GetCurrentCommandBuffer();
        cmds.GetDevice()->GetClearTextureUtils().ClearRWTexture2D(cmds, tex2D->GetUavImm(), value);
    });
    return pass;
}

inline Pass *ClearRWTexture2D(
    RenderGraphRef  graph,
    std::string     name,
    RGTexture       tex2D,
    const Vector4i &value)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(tex2D, CS_RWTexture);
    pass->SetCallback([tex2D, value]
    {
        auto& cmds = GetCurrentCommandBuffer();
        cmds.GetDevice()->GetClearTextureUtils().ClearRWTexture2D(cmds, tex2D->GetUavImm(), value);
    });
    return pass;
}

inline Pass *BlitTexture(
    RenderGraphRef   graph,
    std::string      name,
    RGTexture        src,
    const TexSubrsc &srcSubrsc,
    RGTexture        dst,
    const TexSubrsc &dstSubrsc,
    bool             usePointSampling,
    float            gamma)
{
    auto pass = graph->CreatePass(std::move(name));
    pass->Use(src, PS_Texture);
    pass->Use(dst, ColorAttachmentWriteOnly);
    pass->SetCallback([=](PassContext &context)
    {
        auto& cmds = GetCurrentCommandBuffer();
        cmds.GetDevice()->GetCopyTextureUtils().RenderFullscreenTriangle(
            context.GetCommandBuffer(),
            src->GetSrvImm(srcSubrsc.mipLevel, 1, srcSubrsc.arrayLayer),
            dst->GetRtvImm(dstSubrsc.mipLevel, dstSubrsc.arrayLayer),
            usePointSampling ? CopyTextureUtils::Point : CopyTextureUtils::Linear, gamma);
    });
    return pass;
}

inline Pass *BlitTexture(
    RenderGraphRef graph,
    std::string    name,
    RGTexture      src,
    RGTexture      dst,
    bool           usePointSampling,
    float          gamma)
{
    assert(src->GetArraySize() == 1);
    assert(src->GetMipLevels() == 1);
    assert(dst->GetArraySize() == 1);
    assert(dst->GetMipLevels() == 1);
    return BlitTexture(graph, std::move(name), src, { 0, 0 }, dst, { 0, 0 }, usePointSampling, gamma);
}

template<RenderGraphBindingGroupInput...Ts>
Pass *DispatchWithThreadGroupCount(
    RenderGraphRef  graph,
    std::string     name,
    RC<Shader>      shader,
    const Vector3i &threadGroupCount,
    const Ts&...    bindingGroups)
{
    auto pass = graph->CreatePass(std::move(name));
    auto DeclareUse = [&]<RenderGraphBindingGroupInput T>(const T &group)
    {
        if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
        {
            DeclareRenderGraphResourceUses(pass, group, RHI::PipelineStageFlag::ComputeShader);
        }
    };
    (DeclareUse(bindingGroups), ...);
    pass->SetCallback([s = std::move(shader), threadGroupCount, ...bindingGroups = bindingGroups]
    {
        auto &commandBuffer = GetCurrentCommandBuffer();
        auto device = commandBuffer.GetDevice();

        std::vector<RC<BindingGroup>> finalGroups;
        auto BindGroup = [&]<RenderGraphBindingGroupInput T>(const T &group)
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

template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts>
Pass *DispatchWithThreadGroupCount(
    RenderGraphRef  graph,
    const Vector3i &threadGroupCount,
    const Ts&...    bindingGroups)
{
    return RG::DispatchWithThreadGroupCount(
        graph, S::Name, graph->GetDevice()->GetShader<S::Name>(), threadGroupCount, bindingGroups...);
}

template<RenderGraphBindingGroupInput...Ts>
Pass *DispatchWithThreadCount(
    RenderGraphRef  graph,
    std::string     name,
    RC<Shader>      shader,
    const Vector3i &threadCount,
    const Ts&...    bindingGroups)
{
    const Vector3i groupCount = shader->ComputeThreadGroupCount(threadCount);
    return RG::DispatchWithThreadGroupCount(graph, std::move(name), std::move(shader), groupCount, bindingGroups...);
}

template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts>
Pass *DispatchWithThreadCount(
    RenderGraphRef  graph,
    const Vector3i &threadCount,
    const Ts&...    bindingGroups)
{
    return RG::DispatchWithThreadCount(
        graph, S::Name, graph->GetDevice()->GetShader<S::Name>(), threadCount, bindingGroups...);
}

template<RenderGraphBindingGroupInput...Ts>
Pass *DispatchIndirect(
    RenderGraphRef graph,
    std::string    name,
    RC<Shader>     shader,
    RGBuffer       indirectBuffer,
    size_t         indirectBufferOffset,
    const Ts&...   bindingGroups)
{
    auto pass = graph->CreatePass(std::move(name));
    auto DeclareUse = [&]<RenderGraphBindingGroupInput T>(const T & group)
    {
        if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
        {
            DeclareRenderGraphResourceUses(pass, group, RHI::PipelineStageFlag::ComputeShader);
        }
    };
    (DeclareUse(bindingGroups), ...);
    pass->Use(indirectBuffer, IndirectDispatchRead);
    pass->SetCallback(
        [s = std::move(shader), indirectBuffer, indirectBufferOffset, ...bindingGroups = bindingGroups]
    {
        auto &commandBuffer = GetCurrentCommandBuffer();
        auto device = commandBuffer.GetDevice();

        std::vector<RC<BindingGroup>> finalGroups;
        auto BindGroup = [&]<RenderGraphBindingGroupInput T>(const T &group)
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

template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts>
Pass *DispatchIndirect(
    RenderGraphRef graph,
    RGBuffer       indirectBuffer,
    size_t         indirectBufferOffset,
    const Ts&...   bindingGroups)
{
    return RG::DispatchIndirect(
        graph, S::Name, graph->GetDevice()->GetShader<S::Name>(),
        indirectBuffer, indirectBufferOffset, bindingGroups...);
}

#define DEFINE_DISPATCH(EXPR, ...)                                                  \
    template<RenderGraphBindingGroupInput...Ts>                                     \
    Pass *DispatchWithThreadGroupCount(                                             \
        RenderGraphRef  graph,                                                      \
        std::string     name,                                                       \
        RC<Shader>      shader,                                                     \
        __VA_ARGS__,                                                                \
        const Ts&...    bindingGroups)                                              \
        {                                                                           \
            return DispatchWithThreadGroupCount(                                    \
                graph, std::move(name), std::move(shader), EXPR, bindingGroups...); \
        }                                                                           \
    template<RenderGraphBindingGroupInput...Ts>                                     \
    Pass *DispatchWithThreadCount(                                                  \
        RenderGraphRef  graph,                                                      \
        std::string     name,                                                       \
        RC<Shader>      shader,                                                     \
        __VA_ARGS__,                                                                \
        const Ts&...    bindingGroups)                                              \
        {                                                                           \
            return DispatchWithThreadCount(                                         \
                graph, std::move(name), std::move(shader), EXPR, bindingGroups...); \
        }                                                                           \
    template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts>          \
    Pass *DispatchWithThreadGroupCount(                                             \
        RenderGraphRef  graph,                                                      \
        __VA_ARGS__,                                                                \
        const Ts&...    bindingGroups)                                              \
        {                                                                           \
            return DispatchWithThreadGroupCount<S>(                                 \
                graph, EXPR, bindingGroups...);                                     \
        }                                                                           \
    template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts>          \
    Pass *DispatchWithThreadCount(                                                  \
        RenderGraphRef  graph,                                                      \
        __VA_ARGS__,                                                                \
        const Ts&...    bindingGroups)                                              \
        {                                                                           \
            return DispatchWithThreadCount<S>(                                      \
                graph, EXPR, bindingGroups...);                                     \
        }

DEFINE_DISPATCH(Vector3i(t.x, t.y, t.z), const Vector3u &t)
DEFINE_DISPATCH(Vector3i(t.x, t.y, 1), const Vector2i &t)
DEFINE_DISPATCH(Vector3i(t.x, t.y, 1), const Vector2u &t)
DEFINE_DISPATCH(Vector3i(x, 1, 1), unsigned int x)
DEFINE_DISPATCH(Vector3i(x, y, 1), unsigned int x, unsigned int y)
DEFINE_DISPATCH(Vector3i(x, y, z), unsigned int x, unsigned int y, unsigned int z)

#undef DEFINE_DISPATCH

RTRC_RG_END
