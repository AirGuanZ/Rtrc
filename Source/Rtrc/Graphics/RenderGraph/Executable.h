#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

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
    std::vector<RHI::TextureTransitionBarrier> beforeTextureBarriers;
    std::vector<RHI::BufferTransitionBarrier>  beforeBufferBarriers;
    const Pass::Callback *callback;
    const std::string *name;
};

struct ExecutableSection
{
    RHI::BackBufferSemaphorePtr waitAcquireSemaphore;
    RHI::PipelineStageFlag      waitAcquireSemaphoreStages;

    std::vector<ExecutablePass> passes;

    StaticVector<RHI::TextureTransitionBarrier, 1> afterTextureBarriers;

    RHI::BackBufferSemaphorePtr signalPresentSemaphore;
    RHI::PipelineStageFlag      signalPresentSemaphoreStages;

    RHI::FencePtr signalFence;
};

struct ExecutableGraph
{
    RHI::QueuePtr                  queue;
    ExecutableResources            resources;
    std::vector<ExecutableSection> sections;
};

class Executer
{
public:

    Executer(Device *device);

    void Execute(const RenderGraph &graph);

    void Execute(const Box<RenderGraph> &graph) { Execute(*graph); }

    void Execute(const Box<const RenderGraph> &graph) { Execute(*graph); }

private:

    void Execute(const ExecutableGraph &graph);

    Device *device_;
};

RTRC_RG_END
