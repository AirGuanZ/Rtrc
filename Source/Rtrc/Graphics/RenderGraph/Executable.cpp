#include <Rtrc/Graphics/RenderGraph/Compiler.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>

RTRC_RG_BEGIN

Executer::Executer(Ref<Device> device)
    : device_(device)
{
    constexpr int chunkSizeHint = 128 * 1024 * 1024;
    transientResourcePool_ = device_->GetRawDevice()->CreateTransientResourcePool({ chunkSizeHint }).ToRC();
}

void Executer::NewFrame()
{
    if(transientResourcePool_)
    {
        const int session = transientResourcePool_->StartHostSynchronizationSession();
        device_->GetSynchronizer().OnFrameComplete(
            [session, pool = transientResourcePool_] { pool->NotifyExternalHostSynchronization(session); });
    }
}

void Executer::ExecutePartially(Ref<RenderGraph> graph, bool enableTransientResourcePool)
{
    ExecuteInternal(graph, enableTransientResourcePool, true);
}

void Executer::Execute(Ref<RenderGraph> graph, bool enableTransientResourcePool)
{
    ExecuteInternal(graph, enableTransientResourcePool, false);
}

void Executer::ExecuteInternal(Ref<RenderGraph> graph, bool enableTransientResourcePool, bool partialExecution)
{
    ExecutableGraph compiledResult;
    assert(graph->recording_);
    {
        Compiler compiler(device_);
        if(enableTransientResourcePool && transientResourcePool_)
        {
            compiler.SetTransientResourcePool(transientResourcePool_);
        }
        compiler.SetPartialExecution(partialExecution);
        compiler.Compile(*graph, compiledResult);
    }
    {
        graph->executableResource_ = &compiledResult.resources;
        ExecuteImpl(compiledResult);
        graph->executableResource_ = nullptr;
    }
    {
        graph->executedPasses_.reserve(graph->executedPasses_.size() + graph->passes_.size());
        for(auto &pass : graph->passes_)
        {
            graph->executedPasses_.push_back(std::move(pass));
        }
        graph->passes_.clear();
    }
    if(!partialExecution)
    {
        graph->recording_ = false;
    }
}

void Executer::ExecuteImpl(const ExecutableGraph &graph)
{
    const LabelStack::Node *nameNode = nullptr;

    for(auto &section : graph.sections)
    {
        auto commandBuffer = device_->CreateCommandBuffer();
        commandBuffer.Begin();

        auto PushNameNode = [&](const LabelStack::Node *node)
        {
            commandBuffer.GetRHIObject()->BeginDebugEvent(RHI::DebugLabel{ .name = node->name });
        };
        auto PopNameNode = [&](const LabelStack::Node *)
        {
            commandBuffer.GetRHIObject()->EndDebugEvent();
        };

        for(auto &pass : section.passes)
        {
            if(pass.preGlobalBarrier || !pass.preBufferBarriers.empty() || !pass.preTextureBarriers.empty())
            {
                if(pass.preGlobalBarrier)
                {
                    commandBuffer.GetRHIObject()->ExecuteBarriers(
                        *pass.preGlobalBarrier, pass.preTextureBarriers, pass.preBufferBarriers);
                }
                else
                {
                    commandBuffer.GetRHIObject()->ExecuteBarriers(pass.preTextureBarriers, pass.preBufferBarriers);
                }
            }
            
            const bool emitDebugLabel = pass.callback;
            if(emitDebugLabel)
            {
                LabelStack::Transfer(nameNode, pass.nameNode, PushNameNode, PopNameNode);
                nameNode = pass.nameNode;
            }

            if(pass.callback)
            {
                PassContext passContext(graph.resources, commandBuffer);
#if RTRC_RG_DEBUG
                passContext.declaredResources_ = &pass.declaredResources;
#endif
                (*pass.callback)();
            }
        }

        if(!section.postTextureBarriers.IsEmpty())
        {
            commandBuffer.GetRHIObject()->ExecuteBarriers(section.postTextureBarriers);
        }

        const bool isLastSection = &section == &graph.sections.back();
        if(isLastSection)
        {
            LabelStack::Transfer(nameNode, nullptr, PushNameNode, PopNameNode);
        }

        commandBuffer.End();

        RHI::FenceOPtr fence = section.signalFence;
        if(!fence && isLastSection && graph.completeFence)
        {
            fence = graph.completeFence;
        }

        graph.queue->Submit(
            RHI::Queue::BackBufferSemaphoreDependency
            {
                .semaphore = section.waitAcquireSemaphore,
                .stages = section.waitAcquireSemaphoreStages
            },
            {},
            commandBuffer.GetRHIObject(),
            RHI::Queue::BackBufferSemaphoreDependency
            {
                .semaphore = section.signalPresentSemaphore,
                .stages = section.signalPresentSemaphoreStages
            },
            {},
            fence);
    }

    if(graph.completeFence && (graph.sections.empty() || graph.sections.back().signalFence))
    {
        graph.queue->Submit({}, {}, {}, {}, {}, graph.completeFence);
    }

    const RHI::QueueSessionID queueSessionID = graph.queue->GetCurrentSessionID();

    for(auto &record : graph.resources.indexToBuffer)
    {
        if(record.buffer)
        {
            auto state = record.finalState;
            state.queueSessionID = queueSessionID;
            record.buffer->SetState(state);
        }
    }

    for(auto &record : graph.resources.indexToTexture)
    {
        if(record.texture)
        {
            for(auto subrsc : EnumerateSubTextures(record.finalState))
            {
                if(auto &optionalState = record.finalState(subrsc.mipLevel, subrsc.arrayLayer))
                {
                    auto state = *optionalState;
                    state.queueSessionID = queueSessionID;
                    record.texture->SetState(subrsc.mipLevel, subrsc.arrayLayer, state);
                }
            }
        }
    }
}

RTRC_RG_END
