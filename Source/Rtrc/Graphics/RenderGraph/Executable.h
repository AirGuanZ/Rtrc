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
    const Pass::Callback                      *callback;
    const LabelStack::Node             *nameNode;

#if RTRC_RG_DEBUG
    std::set<const Resource *> declaredResources;
#endif
};

struct ExecutableSection
{
    RHI::BackBufferSemaphorePtr waitAcquireSemaphore;
    RHI::PipelineStageFlag      waitAcquireSemaphoreStages;

    std::vector<ExecutablePass> passes;

    StaticVector<RHI::TextureTransitionBarrier, 1> postTextureBarriers;

    RHI::BackBufferSemaphorePtr signalPresentSemaphore;
    RHI::PipelineStageFlag      signalPresentSemaphoreStages;

    RHI::FencePtr signalFence;
};

struct ExecutableGraph
{
    RHI::QueuePtr                  queue;
    ExecutableResources            resources;
    std::vector<ExecutableSection> sections;
    RHI::FencePtr                  completeFence;
};

class Executer
{
public:

    explicit Executer(ObserverPtr<Device> device);

    void Execute(ObserverPtr<const RenderGraph> graph);
    
private:

    void ExecuteImpl(const ExecutableGraph &graph);

    ObserverPtr<Device> device_;
};

RTRC_RG_END
