#include <Rtrc/Graphics/RenderGraph/Compiler.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>

RTRC_RG_BEGIN

Executer::Executer(RenderContext &renderContext)
    : renderContext_(renderContext)
{
    
}

void Executer::Execute(const RenderGraph &graph)
{
    ExecutableGraph compiledResult;
    Compiler(renderContext_).Compile(graph, compiledResult);
    graph.executableResource_ = &compiledResult.resources;
    Execute(compiledResult);
    graph.executableResource_ = nullptr;
}

void Executer::Execute(const ExecutableGraph &graph)
{
    for(auto &section : graph.sections)
    {
        auto commandBuffer = renderContext_.CreateCommandBuffer();
        commandBuffer.Begin();

        for(auto &pass : section.passes)
        {
            if(!pass.beforeBufferBarriers.empty() || !pass.beforeTextureBarriers.empty())
            {
                commandBuffer.GetRHIObject()->ExecuteBarriers(pass.beforeTextureBarriers, pass.beforeBufferBarriers);
            }
            if(pass.name)
            {
                commandBuffer.GetRHIObject()->BeginDebugEvent(RHI::DebugLabel{ .name = *pass.name });
            }
            if(pass.callback)
            {
                PassContext passContext(graph.resources, commandBuffer);
                (*pass.callback)(passContext);
            }
            if(pass.name)
            {
                commandBuffer.GetRHIObject()->EndDebugEvent();
            }
        }

        if(!section.afterTextureBarriers.IsEmpty())
        {
            commandBuffer.GetRHIObject()->ExecuteBarriers(section.afterTextureBarriers);
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
            record.buffer->SetUnsyncAccess(record.finalState);
        }
    }

    for(auto &record : graph.resources.indexToTexture)
    {
        if(record.texture)
        {
            for(auto subrsc : EnumerateSubTextures(record.finalState))
            {
                auto &optionalState = record.finalState(subrsc.mipLevel, subrsc.arrayLayer);
                if(optionalState)
                {
                    record.texture->SetUnsyncAccess(subrsc.mipLevel, subrsc.arrayLayer, *optionalState);
                }
            }
        }
    }
}

RTRC_RG_END