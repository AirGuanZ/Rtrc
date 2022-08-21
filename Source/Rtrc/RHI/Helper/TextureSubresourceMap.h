#pragma once

#include <Rtrc/RHI/RHI.h>

RTRC_RHI_BEGIN

template<typename T>
class TextureSubresourceMap
{
public:

    TextureSubresourceMap()
        : mipLevelCount_(0), arrayLayerCount_(0)
    {

    }

    explicit TextureSubresourceMap(const Texture2DDesc &desc, const T &value = {})
        : TextureSubresourceMap(desc.mipLevels, desc.arraySize, value)
    {

    }

    explicit TextureSubresourceMap(const Texture2DPtr &tex, const T &value = {})
        : TextureSubresourceMap(tex->GetDesc(), value)
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

class EnumerateSubTextures
{
    class Iterator
    {
        uint32_t currentMipLevel_;
        uint32_t currentArrayLayer_;
        uint32_t mipLevelCount_;

    public:

        Iterator(uint32_t currentMipLevel, uint32_t currentArrayLayer, uint32_t mipLevelCount)
            : currentMipLevel_(currentMipLevel), currentArrayLayer_(currentArrayLayer), mipLevelCount_(mipLevelCount)
        {
            
        }

        TextureSubresource operator*() const
        {
            return { currentMipLevel_, currentArrayLayer_ };
        }

        bool operator!=(const Iterator &other)
        {
            assert(mipLevelCount_ == other.mipLevelCount_);
            return currentMipLevel_ != other.currentMipLevel_ || currentArrayLayer_ != other.currentArrayLayer_;
        }

        auto operator++()
        {
            if(++currentMipLevel_ >= mipLevelCount_)
            {
                currentMipLevel_ = 0;
                ++currentArrayLayer_;
            }
            return *this;
        }
    };

    uint32_t arrayLayerCount_;
    uint32_t mipLevelCount_;

public:

    EnumerateSubTextures(uint32_t mipLevels, uint32_t arrayLayers)
        : arrayLayerCount_(arrayLayers), mipLevelCount_(mipLevels)
    {
        
    }

    explicit EnumerateSubTextures(const Texture2DDesc &desc)
        : EnumerateSubTextures(desc.mipLevels, desc.arraySize)
    {
        
    }

    template<typename T>
    explicit EnumerateSubTextures(const TextureSubresourceMap<T> &map)
        : EnumerateSubTextures(map.GetMipLevelCount(), map.GetArrayLayerCount())
    {
        
    }

    auto begin() const
    {
        return Iterator{ 0, 0, mipLevelCount_ };
    }

    auto end() const
    {
        return Iterator{ 0, arrayLayerCount_, mipLevelCount_ };
    }
};

RTRC_RG_END
