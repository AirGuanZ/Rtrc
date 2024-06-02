#pragma once

#include <Rtrc/Graphics/Shader/DSL/Entry/ComputeEntry.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_BEGIN

template<typename...Args>
RGPass RGDispatchWithThreadGroupCount(
    GraphRef                                            graph,
    std::string                                         name,
    const eDSL::ComputeEntry<Args...>                  &entry,
    const Vector3u                                     &threadGroupCount,
    const eDSL::ComputeEntryDetail::InvokeType<Args>&...args)
{
    auto pass = graph->CreatePass(std::move(name));
    eDSL::DeclareRenderGraphResourceUses(pass, entry, args...);
    pass->SetCallback([entry, threadGroupCount, args...]
    {
        auto& commandBuffer = RGGetCommandBuffer();
        eDSL::SetupComputeEntry(commandBuffer, entry, args...);
        commandBuffer.Dispatch(threadGroupCount);
    });
    return pass;
}

template<typename...Args>
RGPass RGDispatchWithThreadCount(
    GraphRef                                            graph,
    std::string                                         name,
    const eDSL::ComputeEntry<Args...>                  &entry,
    const Vector3u                                     &threadCount,
    const eDSL::ComputeEntryDetail::InvokeType<Args>&...args)
{
    const Vector3u threadGroupCount = entry.GetShader()->ComputeThreadGroupCount(threadCount);
    return RGDispatchWithThreadGroupCount(graph, std::move(name), entry, threadGroupCount, args...);
}

#define DEFINE_DISPATCH(EXPR, ...)                                                  \
    template<typename...Args>                                                       \
    RGPass RGDispatchWithThreadGroupCount(                                          \
        GraphRef                                            graph,                  \
        std::string                                         name,                   \
        const eDSL::ComputeEntry<Args...>                  &entry,                  \
        __VA_ARGS__,                                                                \
        const eDSL::ComputeEntryDetail::InvokeType<Args>&...args)                   \
    {                                                                               \
        return RGDispatchWithThreadGroupCount(                                      \
            graph, std::move(name), entry, EXPR, args...);                          \
    }                                                                               \
    template<typename...Args>                                                       \
    RGPass RGDispatchWithThreadCount(                                               \
        GraphRef                                            graph,                  \
        std::string                                         name,                   \
        const eDSL::ComputeEntry<Args...>                  &entry,                  \
        __VA_ARGS__,                                                                \
        const eDSL::ComputeEntryDetail::InvokeType<Args>&...args)                   \
    {                                                                               \
        return RGDispatchWithThreadCount(                                           \
            graph, std::move(name), entry, EXPR, args...);                          \
    }

DEFINE_DISPATCH(Vector3u(t.x, t.y, t.z), const Vector3i &t)
DEFINE_DISPATCH(Vector3u(t.x, t.y, 1), const Vector2i &t)
DEFINE_DISPATCH(Vector3u(t.x, t.y, 1), const Vector2u &t)
DEFINE_DISPATCH(Vector3u(x, 1, 1), unsigned int x)
DEFINE_DISPATCH(Vector3u(x, y, 1), unsigned int x, unsigned int y)
DEFINE_DISPATCH(Vector3u(x, y, z), unsigned int x, unsigned int y, unsigned int z)

#undef DEFINE_DISPATCH

RTRC_END
