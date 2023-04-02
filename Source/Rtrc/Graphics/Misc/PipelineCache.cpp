#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Misc/PipelineCache.h>

RTRC_BEGIN

PipelineCache::PipelineCache(ObserverPtr<Device> device)
    : device_(std::move(device))
{
    sharedData_ = MakeRC<SharedData>();
}

RC<GraphicsPipeline> PipelineCache::GetGraphicsPipeline(const GraphicsPipeline::Desc &desc)
{
    GraphicsPipeline::Desc key = desc;
    key.shaderId = key.shader->GetUniqueID();
    key.shader.reset();

    if(auto it = staticCache_.find(key); it != staticCache_.end())
    {
        return it->second.pipeline;
    }

    {
        std::shared_lock readLock(dynamicCacheMutex_);
        if(auto it = dynamicCache_.find(key); it != dynamicCache_.end())
        {
            return it->second.pipeline;
        }
    }

    RC<GraphicsPipeline> newPipeline;
    {
        std::unique_lock writeLock(dynamicCacheMutex_);
        newPipeline = device_->CreateGraphicsPipeline(desc);
        dynamicCache_.insert({ key, { newPipeline } });
    }

    desc.shader->OnDestroy([id = key.shaderId, s = sharedData_]
    {
        std::lock_guard lock(s->mutex);
        s->invalidatedShaders.push_back(id);
    });

    return newPipeline;
}

void PipelineCache::CommitChanges()
{
    staticCache_.merge(dynamicCache_);

    std::vector<UniqueId> invalidatedShaders;
    {
        std::lock_guard lock(sharedData_->mutex);
        sharedData_->invalidatedShaders.swap(invalidatedShaders);
    }
    std::ranges::sort(invalidatedShaders);
    for(auto it = staticCache_.begin(); it != staticCache_.end();)
    {
        if(std::ranges::binary_search(invalidatedShaders, it->first.shaderId))
        {
            staticCache_.erase(it++);
        }
        else
        {
            ++it;
        }
    }
}

RTRC_END
