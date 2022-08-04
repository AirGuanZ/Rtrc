#include <Rtrc/RenderGraph/Executable.h>

RTRC_RG_BEGIN

Executer::Executer(RHI::DevicePtr device)
    : device_(std::move(device))
{
    
}

void Executer::Execute(const ExecutableGraph &graph, CommandBufferAllocator &commandBufferAllocator)
{
    // prepare semaphores

    semaphorePool_.reserve(graph.semaphoreCount);
    for(int i = static_cast<int>(semaphorePool_.size()); i < graph.semaphoreCount; ++i)
    {
        semaphorePool_.push_back(
        {
            .signaledValue = 0,
            .semaphore = device_->CreateSemaphore(0)
        });
    }

    for(int i = 0; i < graph.semaphoreCount; ++i)
    {
        ++semaphorePool_[i].signaledValue;
    }

    // execute

    for(auto &section : graph.sections)
    {
        auto commandBuffer = commandBufferAllocator.AllocateCommandBuffer(section->queue->GetType());
        commandBuffer->Begin();

        if(section->passes.empty())
        {
            commandBuffer->ExecuteBarriers(section->acquireTextureBarriers, section->acquireBufferBarriers);
        }

        for(size_t i = 0; i < section->passes.size(); ++i)
        {
            auto &pass = *section->passes[i];

            if(i == 0)
            {
                commandBuffer->ExecuteBarriers(
                    section->acquireTextureBarriers, section->acquireBufferBarriers,
                    pass.beforeTextureBarriers, pass.beforeBufferBarriers);
            }
            else
            {
                commandBuffer->ExecuteBarriers(pass.beforeTextureBarriers, pass.beforeBufferBarriers);
            }

            if(pass.callback)
            {
                PassContext passContext;
                (*pass.callback)(passContext);
            }
        }

        commandBuffer->ExecuteBarriers(
            section->afterTextureBarriers, section->afterBufferBarriers,
            section->releaseTextureBarriers, section->releaseBufferBarriers);
        commandBuffer->End();

        std::vector<RHI::Queue::SemaphoreDependency> waitSemaphoreDependencies;
        assert(section->waitSemaphores.size() == section->waitSemaphoreStages.size());
        for(size_t i = 0; i < section->waitSemaphores.size(); ++i)
        {
            auto &semaphoreRecord = semaphorePool_[section->waitSemaphores[i]];
            waitSemaphoreDependencies.push_back(
                {
                    .semaphore = semaphoreRecord.semaphore,
                    .stages = section->waitSemaphoreStages[i],
                    .value = semaphoreRecord.signaledValue
                });
        }

        std::vector<RHI::Queue::SemaphoreDependency> signalSemaphoreDependencies;
        assert(section->signalSemaphores.size() == section->signalSemaphoreStages.size());
        for(size_t i = 0; i < section->signalSemaphores.size(); ++i)
        {
            auto &semaphoreRecord = semaphorePool_[section->signalSemaphores[i]];
            signalSemaphoreDependencies.push_back(
                {
                    .semaphore = semaphoreRecord.semaphore,
                    .stages = section->signalSemaphoreStages[i],
                    .value = semaphoreRecord.signaledValue
                });
        }

        section->queue->Submit(
            RHI::Queue::BackBufferSemaphoreDependency
            {
                .semaphore = section->waitAcquireSemaphore,
                .stages = section->waitAcquireSemaphoreStages
            },
            waitSemaphoreDependencies,
            commandBuffer,
            RHI::Queue::BackBufferSemaphoreDependency
            {
                .semaphore = section->signalPresentSemaphore,
                .stages = section->signalPresentSemaphoreStages
            },
            signalSemaphoreDependencies,
            section->signalFence);
    }
}

RTRC_RG_END
