#pragma once

#include <ranges>

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Pipeline/PipelineStateCommands.h>
#include <Rtrc/Utils/SignalSlot.h>

RTRC_BEGIN

class SubMaterialToGraphicsPipeline : public Uncopyable
{
public:

    ~SubMaterialToGraphicsPipeline();

    template<typename CreateFunc>
    RC<GraphicsPipeline> GetOrCreate(
        const RC<SubMaterial> &subMat,
        const RC<Shader>      &shader,
        const MeshLayout      *meshLayout,
        CreateFunc           &&createFunc);

private:

    struct PipelineKey
    {
        SubMaterial::UniqueID materialID;
        Shader::UniqueID shaderID;
        const MeshLayout *meshLayout;

        auto operator<=>(const PipelineKey &) const = default;
        size_t Hash() const { return Rtrc::Hash(materialID, shaderID, meshLayout); }
    };

    struct PipelineRecord
    {
        RC<GraphicsPipeline> pipeline;
        tbb::spin_mutex pipelineCreateMutex;
        Connection onSubMaterialDestroyed;
    };

    PipelineRecord &GetPipelineRecord(const PipelineKey &key);
    void RemovePipelineRecord(const PipelineKey &key);

    // An item is removed when the submaterial is destroyed
    std::unordered_map<PipelineKey, PipelineRecord> pipelineCache_;
    tbb::spin_rw_mutex pipelineCacheMutex_;
};

inline SubMaterialToGraphicsPipeline::~SubMaterialToGraphicsPipeline()
{
    std::unordered_map<PipelineKey, PipelineRecord> cache;
    {
        std::unique_lock lock(pipelineCacheMutex_);
        cache.swap(pipelineCache_);
    }

    for(auto &record : std::ranges::views::values(cache))
    {
        record.onSubMaterialDestroyed.Disconnect();
    }
}

template<typename CreateFunc>
RC<GraphicsPipeline> SubMaterialToGraphicsPipeline::GetOrCreate(
    const RC<SubMaterial> &subMat, const RC<Shader> &shader, const MeshLayout *meshLayout, CreateFunc &&createFunc)
{
    PipelineKey key;
    key.materialID = subMat->GetUniqueID();
    key.shaderID = shader->GetUniqueID();
    key.meshLayout = meshLayout;

    PipelineRecord &record = GetPipelineRecord(key);

    std::lock_guard lock(record.pipelineCreateMutex);
    if(record.pipeline)
    {
        return record.pipeline;
    }

    auto pipeline = std::invoke(std::forward<CreateFunc>(createFunc));
    record.pipeline = pipeline;
    record.onSubMaterialDestroyed = subMat->OnDestroy([this, key]
    {
        RemovePipelineRecord(key);
    });
    return pipeline;
}

inline SubMaterialToGraphicsPipeline::PipelineRecord &
    SubMaterialToGraphicsPipeline::GetPipelineRecord(const PipelineKey &key)
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

inline void SubMaterialToGraphicsPipeline::RemovePipelineRecord(const PipelineKey &key)
{
    std::unique_lock lock(pipelineCacheMutex_);
    if(auto it = pipelineCache_.find(key); it != pipelineCache_.end())
    {
        it->second.onSubMaterialDestroyed.Disconnect();
        pipelineCache_.erase(it);
    }
}

RTRC_END
