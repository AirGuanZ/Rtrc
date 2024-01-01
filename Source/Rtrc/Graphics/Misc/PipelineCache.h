#pragma once

#include <shared_mutex>

#include <tbb/spin_mutex.h>
#include <ankerl/unordered_dense.h>

#include <Rtrc/Graphics/Device/Pipeline.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class Device;

class PipelineCache
{
public:

    explicit PipelineCache(Ref<Device> device);

    RC<GraphicsPipeline> GetGraphicsPipeline(const GraphicsPipeline::Desc &desc);

    // Thread safety: exclusive with GetGraphicsPipeline
    void CommitChanges();

private:

    struct Record
    {
        RC<GraphicsPipeline> pipeline;
    };

    struct SharedData
    {
        tbb::spin_mutex       mutex;
        std::vector<UniqueId> invalidatedShaders;
    };
    
    Ref<Device> device_;

    ankerl::unordered_dense::map<GraphicsPipeline::Desc, Record, HashOperator<>> staticCache_;

    std::shared_mutex dynamicCacheMutex_;
    ankerl::unordered_dense::map<GraphicsPipeline::Desc, Record, HashOperator<>> dynamicCache_;

    RC<SharedData> sharedData_;
};

RTRC_END
