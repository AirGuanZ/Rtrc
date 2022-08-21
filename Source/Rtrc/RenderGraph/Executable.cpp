#include <Rtrc/RenderGraph/Compiler.h>
#include <Rtrc/RenderGraph/Executable.h>

RTRC_RG_BEGIN

Executer::Executer(
    RHI::DevicePtr               device,
    RC<CommandBufferAllocator>   commandBufferAllocator,
    RC<TransientResourceManager> transientResourceManager)
    : device_(std::move(device))
    , commandBufferAllocator_(std::move(commandBufferAllocator))
    , transientResourceManager_(std::move(transientResourceManager))
{
    
}

void Executer::Execute(const RenderGraph &graph)
{
    ExecutableGraph compiledResult;
    Compiler(device_, *transientResourceManager_).Compile(graph, compiledResult);
    graph.executableResource_ = &compiledResult.resources;
    Execute(compiledResult);
    graph.executableResource_ = nullptr;
}

void Executer::Execute(const ExecutableGraph &graph)
{
    for(auto &section : graph.sections)
    {
        auto commandBuffer = commandBufferAllocator_->AllocateCommandBuffer(graph.queue->GetType());
        commandBuffer->Begin();

        for(auto &pass : section.passes)
        {
            if(!pass.beforeBufferBarriers.empty() || !pass.beforeTextureBarriers.empty())
            {
                commandBuffer->ExecuteBarriers(pass.beforeTextureBarriers, pass.beforeBufferBarriers);
            }

            if(pass.callback)
            {
                PassContext passContext(graph.resources, commandBuffer);
                (*pass.callback)(passContext);
            }
        }

        if(!section.afterTextureBarriers.IsEmpty())
        {
            commandBuffer->ExecuteBarriers(section.afterTextureBarriers);
        }

        commandBuffer->End();

        graph.queue->Submit(
            RHI::Queue::BackBufferSemaphoreDependency
            {
                .semaphore = section.waitAcquireSemaphore,
                .stages = section.waitAcquireSemaphoreStages
            },
            {},
            commandBuffer,
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
            record.buffer->SetUnsynchronizedState(record.finalState);
        }
    }

    for(auto &record : graph.resources.indexToTexture)
    {
        if(record.texture)
        {
            for(auto subrsc : RHI::EnumerateSubTextures(record.finalState))
            {
                auto &optionalState = record.finalState(subrsc.mipLevel, subrsc.arrayLayer);
                if(optionalState)
                {
                    record.texture->SetUnsynchronizedState(subrsc.mipLevel, subrsc.arrayLayer, optionalState.value());
                }
            }
        }
    }
}

RTRC_RG_END
