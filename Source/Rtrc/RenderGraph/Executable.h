#pragma once

#include <Rtrc/RenderGraph/Graph.h>
#include <Rtrc/RHI/Helper/FrameResourceManager.h>

RTRC_RG_BEGIN

class TransientResourceManager;

struct ExecutableResources
{
    struct BufferRecord
    {
        RC<StatefulBuffer>    buffer;
        StatefulBuffer::State finalState;
    };

    struct TextureRecord
    {
        RC<StatefulTexture>                                               texture;
        RHI::TextureSubresourceMap<std::optional<StatefulTexture::State>> finalState;
    };

    std::vector<BufferRecord>  indexToBuffer;
    std::vector<TextureRecord> indexToTexture;
};

struct ExecutablePass
{
    std::vector<RHI::TextureTransitionBarrier> beforeTextureBarriers;
    std::vector<RHI::BufferTransitionBarrier>  beforeBufferBarriers;
    const Pass::Callback *callback;
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

    Executer(
        RHI::DevicePtr               device,
        RC<CommandBufferAllocator>   commandBufferAllocator,
        RC<TransientResourceManager> transientResourceManager);

    void Execute(const RenderGraph &graph);

private:

    void Execute(const ExecutableGraph &graph);

    RHI::DevicePtr               device_;
    RC<CommandBufferAllocator>   commandBufferAllocator_;
    RC<TransientResourceManager> transientResourceManager_;
};

RTRC_RG_END
