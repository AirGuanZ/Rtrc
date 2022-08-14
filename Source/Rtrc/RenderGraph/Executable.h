#pragma once

#include <Rtrc/RenderGraph/Graph.h>
#include <Rtrc/RHI/Helper/FrameResourceManager.h>

RTRC_RG_BEGIN

struct ExecutableResources
{
    std::vector<RC<RHI::StatefulBuffer>>  indexToRHIBuffers;
    std::vector<RC<RHI::StatefulTexture>> indexToRHITextures;

    std::vector<RHI::StatefulBuffer::State> indexToFinalBufferStates;
    std::vector<RHI::TextureSubresourceMap<RHI::StatefulTexture::State>> indexToFinalTextureStates;
};

struct ExecutablePass
{
    std::vector<RHI::TextureTransitionBarrier> beforeTextureBarriers;
    std::vector<RHI::BufferTransitionBarrier>  beforeBufferBarriers;
    Pass::Callback *callback;
};

struct ExecutableSection
{
    RHI::QueuePtr queue;

    RHI::BackBufferSemaphorePtr waitAcquireSemaphore;
    RHI::PipelineStageFlag      waitAcquireSemaphoreStages;

    std::vector<int>                    waitSemaphores;
    std::vector<RHI::PipelineStageFlag> waitSemaphoreStages;

    std::vector<RHI::TextureAcquireBarrier> acquireTextureBarriers;
    std::vector<RHI::BufferAcquireBarrier>  acquireBufferBarriers;

    std::vector<Box<ExecutablePass>> passes;

    std::vector<RHI::TextureTransitionBarrier> afterTextureBarriers;
    std::vector<RHI::BufferTransitionBarrier>  afterBufferBarriers;

    std::vector<RHI::TextureReleaseBarrier> releaseTextureBarriers;
    std::vector<RHI::BufferReleaseBarrier>  releaseBufferBarriers;

    std::vector<int>                    signalSemaphores;
    std::vector<RHI::PipelineStageFlag> signalSemaphoreStages;

    RHI::BackBufferSemaphorePtr signalPresentSemaphore;
    RHI::PipelineStageFlag      signalPresentSemaphoreStages;

    RHI::FencePtr signalFence;
};

struct ExecutableGraph
{
    int semaphoreCount;
    ExecutableResources resources;
    std::vector<Box<ExecutableSection>> sections;
};

class Executer
{
public:

    Executer(RHI::DevicePtr device, RC<CommandBufferAllocator> commandBufferAllocator);

    void Execute(const RenderGraph &graph);

private:

    struct SemaphoreRecord
    {
        uint64_t signaledValue;
        RHI::SemaphorePtr semaphore;
    };

    void Execute(const ExecutableGraph &graph);

    RHI::DevicePtr               device_;
    RC<CommandBufferAllocator>   commandBufferAllocator_;
    std::vector<SemaphoreRecord> semaphorePool_;
};

RTRC_RG_END
