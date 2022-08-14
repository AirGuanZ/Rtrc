#pragma once

#include <Rtrc/RHI/Helper/TextureSubresourceMap.h>

RTRC_RHI_BEGIN

class StatefulBuffer : public Uncopyable
{
public:

    struct State
    {
        PipelineStageFlag  stages;
        ResourceAccessFlag accesses;
    };

    StatefulBuffer()
        : StatefulBuffer(nullptr)
    {
        
    }

    explicit StatefulBuffer(
        BufferPtr          rhiBuffer,
        PipelineStageFlag  stages = PipelineStage::None,
        ResourceAccessFlag accesses = ResourceAccess::None)
        : buffer_(std::move(rhiBuffer)), state_{ stages, accesses }
    {
        
    }

    void SetUnSynchronizedAccess(PipelineStageFlag stages, ResourceAccessFlag accesses)
    {
        state_.stages = stages;
        state_.accesses = accesses;
    }

    BufferPtr &GetRHIBuffer()
    {
        return buffer_;
    }

    PipelineStageFlag GetUnSynchronizedPipelineStages() const
    {
        return state_.stages;
    }

    ResourceAccessFlag GetUnSynchronizedAccesses() const
    {
        return state_.accesses;
    }

private:

    BufferPtr buffer_;
    State state_;
};

class StatefulTexture : public Uncopyable
{
public:

    struct State
    {
        TextureLayout      layout;
        PipelineStageFlag  stages;
        ResourceAccessFlag accesses;
    };

    StatefulTexture()
        : StatefulTexture(nullptr)
    {
        
    }

    explicit StatefulTexture(
        Texture2DPtr       rhiTexture,
        TextureLayout      layout = TextureLayout::Undefined,
        PipelineStageFlag  stages = PipelineStage::None,
        ResourceAccessFlag accesses = ResourceAccess::None)
        : texture_(std::move(rhiTexture))
    {
        states_.Fill(State{ layout, stages, accesses });
    }

    void SetLayout(uint32_t mipLevel, uint32_t arrayLayer, TextureLayout layout)
    {
        states_(mipLevel, arrayLayer).layout = layout;
    }

    void SetUnSynchronizedAccess(
        uint32_t mipLevel, uint32_t arrayLayer, PipelineStageFlag stages, ResourceAccessFlag accesses)
    {
        auto &state = states_(mipLevel, arrayLayer);
        state.stages = stages;
        state.accesses = accesses;
    }

    Texture2DPtr &GetRHITexture()
    {
        return texture_;
    }

    TextureLayout GetLayout(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        return states_(mipLevel, arrayLayer).layout;
    }

    PipelineStageFlag GetUnSynchronizedPipelineStages(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        return states_(mipLevel, arrayLayer).stages;
    }

    ResourceAccessFlag GetUnSynchronizedAccesses(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        return states_(mipLevel, arrayLayer).accesses;
    }

private:

    Texture2DPtr texture_;
    TextureSubresourceMap<State> states_;
};

RTRC_RHI_END
