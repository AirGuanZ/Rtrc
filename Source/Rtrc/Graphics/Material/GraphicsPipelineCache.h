#pragma once

#include <ranges>

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Utility/SignalSlot.h>

RTRC_BEGIN

class GraphicsPipelineCache : public Uncopyable
{
public:

    ~GraphicsPipelineCache();

    template<typename CreateFunc>
    RC<GraphicsPipeline> GetOrCreate(
        const MaterialPass *pass,
        UniqueId            shaderId,
        const MeshLayout   *meshLayout,
        CreateFunc        &&createFunc);

private:

    struct PipelineKey
    {
        UniqueId          materialID;
        UniqueId          shaderID;
        const MeshLayout *meshLayout;

        auto operator<=>(const PipelineKey &) const = default;
        size_t Hash() const { return Rtrc::Hash(materialID, shaderID, meshLayout); }
    };

    struct PipelineRecord
    {
        RC<GraphicsPipeline> pipeline;
        tbb::spin_mutex      pipelineCreateMutex;
        Connection           onPassDestroyed;
    };

    PipelineRecord &GetPipelineRecord(const PipelineKey &key);
    void RemovePipelineRecord(const PipelineKey &key);

    // An item is removed when the material pass is destroyed
    std::unordered_map<PipelineKey, PipelineRecord, HashOperator<PipelineKey>> pipelineCache_;
    tbb::spin_rw_mutex pipelineCacheMutex_;
};

inline GraphicsPipelineCache::~GraphicsPipelineCache()
{
    std::unordered_map<PipelineKey, PipelineRecord, HashOperator<PipelineKey>> cache;
    {
        std::unique_lock lock(pipelineCacheMutex_);
        cache.swap(pipelineCache_);
    }

    for(auto &record : std::ranges::views::values(cache))
    {
        record.onPassDestroyed.Disconnect();
    }
}

template<typename CreateFunc>
RC<GraphicsPipeline> GraphicsPipelineCache::GetOrCreate(
    const MaterialPass *pass, UniqueId shaderId, const MeshLayout *meshLayout, CreateFunc &&createFunc)
{
    PipelineKey key;
    key.materialID = pass->GetUniqueID();
    key.shaderID = shaderId;
    key.meshLayout = meshLayout;

    PipelineRecord &record = GetPipelineRecord(key);

    std::lock_guard lock(record.pipelineCreateMutex);
    if(record.pipeline)
    {
        return record.pipeline;
    }

    auto pipeline = std::invoke(std::forward<CreateFunc>(createFunc));
    record.pipeline = pipeline;
    record.onPassDestroyed = pass->OnDestroy([this, key]
    {
        RemovePipelineRecord(key);
    });
    return pipeline;
}

inline GraphicsPipelineCache::PipelineRecord &
    GraphicsPipelineCache::GetPipelineRecord(const PipelineKey &key)
{
    {
        std::shared_lock readLock(pipelineCacheMutex_);
        if(auto it = pipelineCache_.find(key); it != pipelineCache_.end())
        {
            return it->second;
        }
    }
    std::unique_lock writeLock(pipelineCacheMutex_);
    return pipelineCache_[key];
}

inline void GraphicsPipelineCache::RemovePipelineRecord(const PipelineKey &key)
{
    std::unique_lock lock(pipelineCacheMutex_);
    if(auto it = pipelineCache_.find(key); it != pipelineCache_.end())
    {
        it->second.onPassDestroyed.Disconnect();
        pipelineCache_.erase(it);
    }
}

RTRC_END