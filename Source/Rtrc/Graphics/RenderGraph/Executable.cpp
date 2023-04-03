#include <Rtrc/Graphics/RenderGraph/Compiler.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>

RTRC_RG_BEGIN

Executer::Executer(ObserverPtr<Device> device)
    : device_(device)
{
    
}

void Executer::Execute(const RenderGraph &graph)
{
    ExecutableGraph compiledResult;
    Compiler(device_).Compile(graph, compiledResult);
    graph.executableResource_ = &compiledResult.resources;
    Execute(compiledResult);
    graph.executableResource_ = nullptr;
}

void Executer::Execute(const ExecutableGraph &graph)
{
    for(auto &section : graph.sections)
    {
        auto commandBuffer = device_->CreateCommandBuffer();
        commandBuffer.Begin();

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
            const bool emitDebugLabel = pass.name && pass.callback;
            if(emitDebugLabel)
            {
                commandBuffer.GetRHIObject()->BeginDebugEvent(RHI::DebugLabel{ .name = *pass.name });
            }
            if(pass.callback)
            {
                PassContext passContext(graph.resources, commandBuffer);
                (*pass.callback)(passContext);
            }
            if(emitDebugLabel)
            {
                commandBuffer.GetRHIObject()->EndDebugEvent();
            }
        }

        if(!section.postTextureBarriers.IsEmpty())
        {
            commandBuffer.GetRHIObject()->ExecuteBarriers(section.postTextureBarriers);
        }

        commandBuffer.End();

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
            section.signalFence);
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
