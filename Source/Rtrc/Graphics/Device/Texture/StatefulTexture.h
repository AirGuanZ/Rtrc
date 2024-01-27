#pragma once

#include <Rtrc/Graphics/Device/Texture/Texture.h>
#include <Rtrc/Graphics/Device/Texture/TextureSubrscMap.h>

RTRC_BEGIN

struct TexSubrscState
{
    RHI::QueueSessionID     queueSessionID = RHI::INITIAL_QUEUE_SESSION_ID;
    RHI::TextureLayout      layout         = RHI::TextureLayout::Undefined;
    RHI::PipelineStageFlag  stages         = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses       = RHI::ResourceAccess::None;

    TexSubrscState() = default;
    TexSubrscState(
        RHI::QueueSessionID     queueSessionID,
        RHI::TextureLayout      layout,
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);
};

class StatefulTexture : public Texture
{
public:

    static RC<StatefulTexture> FromTexture(RC<Texture> tex);

    ~StatefulTexture() override;

    void SetState(const TexSubrscState &state);
    void SetState(uint32_t mipLevel, uint32_t arrayLayer, const TexSubrscState &state);
    void SetState(const TexSubrsc &subrsc, const TexSubrscState &state);
    void SetLayoutToUndefined();
    const TexSubrscState &GetState(uint32_t mipLevel, uint32_t arrayLayer) const;
    const TexSubrscState &GetState(const TexSubrsc &subrsc) const;

protected:

    TextureSubrscMap<TexSubrscState> state_;
};

class WrappedStatefulTexture : public StatefulTexture
{
public:

    explicit WrappedStatefulTexture(RC<Texture> texture = {}, const TexSubrscState &state = {});

private:

    RC<Texture> texture_;
};

inline TexSubrscState::TexSubrscState(
    RHI::QueueSessionID     queueSessionID,
    RHI::TextureLayout      layout,
    RHI::PipelineStageFlag  stages,
    RHI::ResourceAccessFlag accesses)
    : queueSessionID(queueSessionID), layout(layout), stages(stages), accesses(accesses)
{
    
}

inline RC<StatefulTexture> StatefulTexture::FromTexture(RC<Texture> tex)
{
    return MakeRC<WrappedStatefulTexture>(std::move(tex), TexSubrscState{});
}

inline StatefulTexture::~StatefulTexture()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
        manager_ = nullptr;
    }
}

inline void StatefulTexture::SetState(const TexSubrscState &state)
{
    state_.Fill(state);
}

inline void StatefulTexture::SetState(uint32_t mipLevel, uint32_t arrayLayer, const TexSubrscState &state)
{
    state_(mipLevel, arrayLayer) = state;
}

inline void StatefulTexture::SetState(const TexSubrsc &subrsc, const TexSubrscState &state)
{
    this->SetState(subrsc.mipLevel, subrsc.arrayLayer, state);
}

inline void StatefulTexture::SetLayoutToUndefined()
{
    for(auto &state : state_)
    {
        state.layout = RHI::TextureLayout::Undefined;
    }
}

inline const TexSubrscState &StatefulTexture::GetState(uint32_t mipLevel, uint32_t arrayLayer) const
{
    return state_(mipLevel, arrayLayer);
}

inline const TexSubrscState &StatefulTexture::GetState(const TexSubrsc &subrsc) const
{
    return this->GetState(subrsc.mipLevel, subrsc.arrayLayer);
}

inline WrappedStatefulTexture::WrappedStatefulTexture(RC<Texture> texture, const TexSubrscState &state)
    : texture_(std::move(texture))
{
    state_ = TextureSubrscMap<TexSubrscState>(texture_->GetDesc());
    state_.Fill(state);

    rhiTexture_ = texture_->GetRHIObject();
    desc_ = texture_->GetDesc();
}

RTRC_END
