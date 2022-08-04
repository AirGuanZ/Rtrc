#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_RG_BEGIN

template<typename T>
class TextureSubresourceMap
{
public:

    TextureSubresourceMap()
        : mipLevelCount_(0), arrayLayerCount_(0)
    {

    }

    explicit TextureSubresourceMap(const RHI::Texture2DDesc &desc, const T &value = {})
        : TextureSubresourceMap(desc.mipLevels, desc.arraySize, value)
    {

    }

    explicit TextureSubresourceMap(const RHI::TexturePtr &tex, const T &value = {})
        : TextureSubresourceMap(tex->Get2DDesc(), value)
    {

    }

    TextureSubresourceMap(uint32_t mipLevelCount, uint32_t arrayLayerCount, const T &value = {})
        : mipLevelCount_(mipLevelCount), arrayLayerCount_(arrayLayerCount)
    {
        data_.resize(mipLevelCount * arrayLayerCount, value);
    }

    uint32_t GetMipLevelCount() const
    {
        return mipLevelCount_;
    }

    uint32_t GetArrayLayerCount() const
    {
        return arrayLayerCount_;
    }

    T &operator()(uint32_t mipLevel, uint32_t arrayLayer)
    {
        return data_[GetIndex(mipLevel, arrayLayer)];
    }

    const T &operator()(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        return data_[GetIndex(mipLevel, arrayLayer)];
    }

    void Fill(const T &value)
    {
        std::fill(data_.begin(), data_.end(), value);
    }

    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }

private:

    uint32_t GetIndex(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        assert(arrayLayer < arrayLayerCount_ &&mipLevel < mipLevelCount_);
        return arrayLayer * mipLevelCount_ + mipLevel;
    }

    uint32_t mipLevelCount_;
    uint32_t arrayLayerCount_;
    std::vector<T> data_;
};

class PersistentBuffer : public Uncopyable
{
public:

    PersistentBuffer()
        : PersistentBuffer(nullptr)
    {
        
    }

    explicit PersistentBuffer(
        RHI::BufferPtr          rhiBuffer,
        RHI::PipelineStageFlag  stages = RHI::PipelineStage::None,
        RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None)
        : buffer_(std::move(rhiBuffer)), stages_(stages), accesses_(accesses)
    {
        
    }

    void SetUnSynchronizedAccess(RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
    {
        stages_ = stages;
        accesses_ = accesses;
    }

    RHI::BufferPtr &GetRHIBuffer()
    {
        return buffer_;
    }

    RHI::PipelineStageFlag GetUnSynchronizedPipelineStages() const
    {
        return stages_;
    }

    RHI::ResourceAccessFlag GetUnSynchronizedAccesses() const
    {
        return accesses_;
    }

private:

    RHI::BufferPtr buffer_;
    RHI::PipelineStageFlag  stages_;
    RHI::ResourceAccessFlag accesses_;
};

class PersistentTexture : public Uncopyable
{
public:

    PersistentTexture()
        : PersistentTexture(nullptr)
    {
        
    }

    explicit PersistentTexture(
        RHI::TexturePtr         rhiTexture,
        RHI::TextureLayout      layout = RHI::TextureLayout::Undefined,
        RHI::PipelineStageFlag  stages = RHI::PipelineStage::None,
        RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None)
        : texture_(std::move(rhiTexture))
    {
        states_.Fill(State{ layout, stages, accesses });
    }

    void SetLayout(uint32_t mipLevel, uint32_t arrayLayer, RHI::TextureLayout layout)
    {
        states_(mipLevel, arrayLayer).layout = layout;
    }

    void SetUnSynchronizedAccess(
        uint32_t mipLevel, uint32_t arrayLayer, RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
    {
        auto &state = states_(mipLevel, arrayLayer);
        state.stages = stages;
        state.accesses = accesses;
    }

    RHI::TexturePtr &GetRHITexture()
    {
        return texture_;
    }

    RHI::TextureLayout GetLayout(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        return states_(mipLevel, arrayLayer).layout;
    }

    RHI::PipelineStageFlag GetUnSynchronizedPipelineStages(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        return states_(mipLevel, arrayLayer).stages;
    }

    RHI::ResourceAccessFlag GetUnSynchronizedAccesses(uint32_t mipLevel, uint32_t arrayLayer) const
    {
        return states_(mipLevel, arrayLayer).accesses;
    }

private:

    struct State
    {
        RHI::TextureLayout layout;
        RHI::PipelineStageFlag stages;
        RHI::ResourceAccessFlag accesses;
    };

    RHI::TexturePtr texture_;
    TextureSubresourceMap<State> states_;
};

RTRC_RG_END
