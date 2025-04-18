#pragma once

#include <ankerl/unordered_dense.h>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

class GraphicsPipelineCache
{
public:

    void SetDevice(Ref<Device> device)
    {
        this->SetPipelineManager(device->GetPipelineManager());
    }

    void SetPipelineManager(Ref<PipelineManager> pipelineManager)
    {
        pipelineManager_ = pipelineManager;
        pipelines_.clear();
    }

    RC<GraphicsPipeline> Get(const GraphicsPipelineDesc &key)
    {
        assert(pipelineManager_);

        {
            std::shared_lock lock(mutex_);
            if(auto it = pipelines_.find(key); it != pipelines_.end())
            {
                return it->second;
            }
        }

        std::unique_lock lock(mutex_);
        if(auto it = pipelines_.find(key); it != pipelines_.end())
        {
            return it->second;
        }

        auto pipeline = pipelineManager_->CreateGraphicsPipeline(key);

        auto it = pipelines_.insert({ key, pipeline }).first;
        it->first.shader.reset();
        it->first.shaderId = key.shader->GetUniqueID();
        return it->second;
    }

private:

    using HashMap = ankerl::unordered_dense::map<
        GraphicsPipelineDesc, RC<GraphicsPipeline>, HashOperator<GraphicsPipelineDesc>>;

    Ref<PipelineManager> pipelineManager_;
    std::shared_mutex    mutex_;
    HashMap              pipelines_;
};

RTRC_END
