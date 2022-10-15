#pragma once

#include <Rtrc/Graphics/Object/RenderContext.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RG_BEGIN

struct ExecutableResources
{
    struct BufferRecord
    {
        RC<Buffer> buffer;
        UnsynchronizedBufferAccess finalState;
    };

    struct Texture2DRecord
    {
        RC<Texture2D> texture;
        TextureSubresourceMap<std::optional<UnsynchronizedTextureAccess>> finalState;
    };

    std::vector<BufferRecord> indexToBuffer;
    std::vector<Texture2DRecord> indexToTexture2D;
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

    Executer(RenderContext &renderContext);

    void Execute(const RenderGraph &graph);

private:

    void Execute(const ExecutableGraph &graph);

    RenderContext &renderContext_;
};

RTRC_RG_END
