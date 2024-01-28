#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

RTRC_RG_BEGIN

struct ExecutableResources
{
    struct BufferRecord
    {
        RC<StatefulBuffer> buffer;
        BufferState        finalState;
    };

    struct Texture2DRecord
    {
        RC<StatefulTexture>                             texture;
        TextureSubrscMap<std::optional<TexSubrscState>> finalState;
    };

    std::vector<BufferRecord>    indexToBuffer;
    std::vector<Texture2DRecord> indexToTexture;
};

struct ExecutablePass
{
    std::optional<RHI::GlobalMemoryBarrier>    preGlobalBarrier;
    std::vector<RHI::TextureTransitionBarrier> preTextureBarriers;
    std::vector<RHI::BufferTransitionBarrier>  preBufferBarriers;
    Pass::Callback                            *callback;
    const LabelStack::Node                    *nameNode;

#if RTRC_RG_DEBUG
    std::set<const Resource *> declaredResources;
#endif
};

struct ExecutableSection
{
    RHI::BackBufferSemaphoreOPtr waitAcquireSemaphore;
    RHI::PipelineStageFlag       waitAcquireSemaphoreStages;

    std::vector<ExecutablePass> passes;

    StaticVector<RHI::TextureTransitionBarrier, 1> postTextureBarriers;

    RHI::BackBufferSemaphoreOPtr signalPresentSemaphore;
    RHI::PipelineStageFlag       signalPresentSemaphoreStages;

    RHI::FenceOPtr signalFence;
};

struct ExecutableGraph
{
    RHI::QueueRPtr                 queue;
    ExecutableResources            resources;
    std::vector<ExecutableSection> sections;
    RHI::FenceOPtr                 completeFence;
};

class Executer
{
public:

    explicit Executer(Ref<Device> device);

    void NewFrame();

    // Execute recorded graph passes immediately.
    // * Note that this function doesn't handle special external resource synchronizations like transitioning swapchain
    //   image into present state. Therefore, a non-partial execution is still needed even if no new pass is created
    //   after the partial execution.
    // * Any touched transient resource will be allocated and converted to a registered external resource, to ensure
    //   its content to be preserved for later execution(s).
    // * Typical usage:
    //    CreatePasses0(graph);
    //    executer->ExecutePartially(graph);
    //    CreatePasses1(graph);
    //    executer->ExecutePartially(graph);
    //    CreatePasses2(graph);
    //    executer->Execute(graph);
    void ExecutePartially(Ref<RenderGraph> graph, bool enableTransientResourcePool = true);

    // Execute recorded graph passes. The graph can't record new passes after this function is called.
    void Execute(Ref<RenderGraph> graph, bool enableTransientResourcePool = true);
    
private:

    void ExecuteInternal(Ref<RenderGraph> graph, bool enableTransientResourcePool, bool partialExecution);

    void ExecuteImpl(const ExecutableGraph &graph);

    Ref<Device> device_;
    RHI::TransientResourcePoolRPtr transientResourcePool_;
};

RTRC_RG_END
