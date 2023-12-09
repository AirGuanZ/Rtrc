#include <Graphics/RenderGraph/Compiler.h>
#include <Graphics/RenderGraph/Executable.h>

RTRC_RG_BEGIN

Executer::Executer(ObserverPtr<Device> device)
    : device_(device)
{
    transientResourcePool_ = device_->GetRawDevice()->CreateTransientResourcePool({ 128 * 1024 * 1024 }).ToRC();
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

void Executer::Execute(ObserverPtr<const RenderGraph> graph, bool enableTransientResourcePool)
{
    ExecutableGraph compiledResult;
    {
        Compiler compiler(device_);
        if(enableTransientResourcePool && transientResourcePool_)
        {
            compiler.SetTransientResourcePool(transientResourcePool_);
        }
        compiler.Compile(*graph, compiledResult);
    }
    {
        graph->executableResource_ = &compiledResult.resources;
        ExecuteImpl(compiledResult);
        graph->executableResource_ = nullptr;
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

    for(auto &record : graph.resources.indexToBuffer)
    {
        if(record.buffer)
        {
            record.buffer->SetState(record.finalState);
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
                    record.texture->SetState(subrsc.mipLevel, subrsc.arrayLayer, *optionalState);
                }
            }
        }
    }
}

RTRC_RG_END
