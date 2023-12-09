#pragma once

#include <Graphics/Device/Device.h>
#include <Graphics/RenderGraph/Graph.h>
#include <Core/SmartPointer/ObserverPtr.h>

RTRC_RG_BEGIN

struct ExecutableResources
{
    struct BufferRecord
    {
        RC<StatefulBuffer> buffer;
        BufferState finalState;
    };

    struct Texture2DRecord
    {
        RC<StatefulTexture> texture;
        TextureSubrscMap<std::optional<TextureSubrscState>> finalState;
    };

    std::vector<BufferRecord> indexToBuffer;
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
    RHI::PipelineStageFlag      signalPresentSemaphoreStages;

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

    explicit Executer(ObserverPtr<Device> device);

    void NewFrame();

    void Execute(ObserverPtr<const RenderGraph> graph, bool enableTransientResourcePool = true);
    
private:

    void ExecuteImpl(const ExecutableGraph &graph);

    ObserverPtr<Device> device_;
    RHI::TransientResourcePoolRPtr transientResourcePool_;
};

RTRC_RG_END
