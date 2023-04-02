#pragma once

#include <shared_mutex>

#include <absl/container/flat_hash_map.h>
#include <tbb/spin_mutex.h>

#include <Rtrc/Graphics/Device/Pipeline.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class Device;

class PipelineCache
{
public:

    explicit PipelineCache(ObserverPtr<Device> device);

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
    
    ObserverPtr<Device> device_;

    absl::flat_hash_map<GraphicsPipeline::Desc, Record, HashOperator<>> staticCache_;

    std::shared_mutex                                                   dynamicCacheMutex_;
    absl::flat_hash_map<GraphicsPipeline::Desc, Record, HashOperator<>> dynamicCache_;

    RC<SharedData> sharedData_;
};

RTRC_END
