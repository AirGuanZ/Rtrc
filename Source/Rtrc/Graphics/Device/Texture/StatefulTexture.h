#pragma once

#include <Rtrc/Graphics/Device/Texture/Texture.h>
#include <Rtrc/Graphics/Device/Texture/TextureSubrscMap.h>

RTRC_BEGIN

struct TextureSubrscState
{
    RHI::TextureLayout      layout   = RHI::TextureLayout::Undefined;
    RHI::PipelineStageFlag  stages   = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;

    TextureSubrscState() = default;
    TextureSubrscState(RHI::TextureLayout layout, RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses);
};

class StatefulTexture : public Texture
{
public:

    static RC<StatefulTexture> FromTexture(RC<Texture> tex);

    ~StatefulTexture() override;

    void SetState(const TextureSubrscState &state);
    void SetState(uint32_t mipLevel, uint32_t arrayLayer, const TextureSubrscState &state);
    void SetLayoutToUndefined();
    const TextureSubrscState &GetState(uint32_t mipLevel, uint32_t arrayLayer) const;

protected:

    TextureSubrscMap<TextureSubrscState> state_;
};

class WrappedStatefulTexture : public StatefulTexture
{
public:

    explicit WrappedStatefulTexture(RC<Texture> texture = {}, const TextureSubrscState &state = {});

private:

    RC<Texture> texture_;
};

inline TextureSubrscState::TextureSubrscState(
    RHI::TextureLayout layout, RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
    : layout(layout), stages(stages), accesses(accesses)
{
    
}

inline RC<StatefulTexture> StatefulTexture::FromTexture(RC<Texture> tex)
{
    return MakeRC<WrappedStatefulTexture>(std::move(tex), TextureSubrscState{});
}

inline StatefulTexture::~StatefulTexture()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
        manager_ = nullptr;
    }
}

inline void StatefulTexture::SetState(const TextureSubrscState &state)
{
    state_.Fill(state);
}

inline void StatefulTexture::SetState(uint32_t mipLevel, uint32_t arrayLayer, const TextureSubrscState &state)
{
    state_(mipLevel, arrayLayer) = state;
}

inline void StatefulTexture::SetLayoutToUndefined()
{
    for(auto &state : state_)
    {
        state.layout = RHI::TextureLayout::Undefined;
    }
}

inline const TextureSubrscState &StatefulTexture::GetState(uint32_t mipLevel, uint32_t arrayLayer) const
{
    return state_(mipLevel, arrayLayer);
}

inline WrappedStatefulTexture::WrappedStatefulTexture(RC<Texture> texture, const TextureSubrscState &state)
    : texture_(std::move(texture))
{
    state_ = TextureSubrscMap<TextureSubrscState>(texture_->GetDesc());
    state_.Fill(state);

    rhiTexture_ = texture_->GetRHIObject();
    desc_ = texture_->GetDesc();
}

RTRC_END
