#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class CopyTextureUtils
{
public:

    enum SamplingMethod
    {
        Point,
        Linear
    };

    explicit CopyTextureUtils(ObserverPtr<Device> device);

    void RenderQuad(
        CommandBuffer    &commandBuffer,
        const TextureSrv &src,
        const TextureRtv &dst,
        SamplingMethod    samplingMethod);

private:

    const RC<GraphicsPipeline> &GetPipeline(RHI::Format format, bool pointSampling);

    ObserverPtr<Device> device_;

    RC<Shader> shaderUsingPointSampling_;
    RC<Shader> shaderUsingLinearSampling_;
    RC<BindingGroupLayout> bindingGroupLayout_;

    tbb::spin_rw_mutex mutex_;
    std::map<RHI::Format, RC<GraphicsPipeline>> dstFormatToPipelineUsingPointSampling_;
    std::map<RHI::Format, RC<GraphicsPipeline>> dstFormatToPipelineUsingLinearSampling_;
};

RTRC_END
