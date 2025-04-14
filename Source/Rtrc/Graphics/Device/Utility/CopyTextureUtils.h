#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class CopyTextureUtils
{
public:

    enum SamplingMethod
    {
        Point,
        Linear
    };

    explicit CopyTextureUtils(Ref<Device> device);

    void RenderFullscreenTriangle(
        CommandBuffer    &commandBuffer,
        const TextureSrv &src,
        const TextureRtv &dst,
        SamplingMethod    samplingMethod,
        float             gamma = 1.0f);

private:

    struct Key
    {
        RHI::Format    dstFormat;
        SamplingMethod samplingMethod;
        bool           enableGamma;

        auto operator<=>(const Key &) const = default;
    };

    const RC<GraphicsPipeline> &GetPipeline(const Key &key);

    Ref<Device> device_;
    RC<Shader> shaderPointSampling_;
    RC<Shader> shaderLinearSampling_;
    RC<Shader> shaderPointSamplingGamma_;
    RC<Shader> shaderLinearSamplingGamma_;

    std::shared_mutex mutex_;
    std::map<Key, RC<GraphicsPipeline>> keyToPipeline_;
};

RTRC_END
