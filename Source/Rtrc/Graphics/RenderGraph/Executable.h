#pragma once

#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Graphics/Resource/Frame/FrameResourceManager.h>

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
        RC<StatefulTexture>                                          texture;
        TextureSubresourceMap<std::optional<StatefulTexture::State>> finalState;
    };

    std::vector<BufferRecord>  indexToBuffer;
    std::vector<TextureRecord> indexToTexture;
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

    Executer(
        RHI::DevicePtr            device,
        CommandBufferAllocator   *commandBufferAllocator,
        TransientResourceManager *transientResourceManager);

    explicit Executer(FrameResourceManager *frameResourceManager);

    void Execute(const RenderGraph &graph);

private:

    void Execute(const ExecutableGraph &graph);

    RHI::DevicePtr            device_;
    CommandBufferAllocator   *commandBufferAllocator_;
    TransientResourceManager *transientResourceManager_;
};

RTRC_RG_END
