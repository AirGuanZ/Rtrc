#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanWorkGraphPipeline, WorkGraphPipeline)
{
public:

    VulkanWorkGraphPipeline()
    {
        throw Exception("Not implemented");
    }

    const WorkGraphPipelineDesc &GetDesc() const RTRC_RHI_OVERRIDE
    {
        static const WorkGraphPipelineDesc ret;
        return ret;
    }
};

RTRC_RHI_VK_END
