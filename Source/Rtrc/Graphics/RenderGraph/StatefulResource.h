#pragma once

#include <Rtrc/Graphics/Resource/TextureSubresourceMap.h>

RTRC_RG_BEGIN

class StatefulBuffer : public Uncopyable
{
public:

    struct State
    {
        RHI::PipelineStageFlag  stages   = RHI::PipelineStage::None;
        RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;
    };

    StatefulBuffer()
        : StatefulBuffer(nullptr)
    {
        
    }

    explicit StatefulBuffer(RHI::BufferPtr rhiBuffer)
        : buffer_(std::move(rhiBuffer)), state_{ RHI::PipelineStage::None, RHI::ResourceAccess::None }
    {
        
    }

    RHI::BufferPtr &GetRHIBuffer()
    {
        return buffer_;
    }

    void SetUnsynchronizedState(const State &state)
    {
        state_ = state;
    }

    const State &GetUnsynchronizedState() const
    {
        return state_;
    }

private:

    RHI::BufferPtr buffer_;
    State state_;
};

class StatefulTexture : public Uncopyable
{
public:

    struct State
    {
        RHI::TextureLayout      layout   = RHI::TextureLayout::Undefined;
        RHI::PipelineStageFlag  stages   = RHI::PipelineStage::None;
        RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;
    };

    StatefulTexture()
        : StatefulTexture(nullptr)
    {
        
    }

    explicit StatefulTexture(RHI::TexturePtr rhiTexture)
        : texture_(std::move(rhiTexture))
    {
        states_ = TextureSubresourceMap<State>(texture_->GetMipLevels(), texture_->GetArraySize());
        states_.Fill(State{ RHI::TextureLayout::Undefined, RHI::PipelineStage::None, RHI::ResourceAccess::None });
    }

    RHI::TexturePtr &GetRHITexture()
    {
        return texture_;
    }

    void SetUnsynchronizedState(uint32_t mipLevel, uint32_t arrayLayer, const State &state)
    {
        states_(mipLevel, arrayLayer) = state;
    }

    const State &GetUnsynchronizedState(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        return states_(mipLevel, arrayLayer);
    }

private:

    RHI::TexturePtr texture_;
    TextureSubresourceMap<State> states_;
};

RTRC_RG_END
