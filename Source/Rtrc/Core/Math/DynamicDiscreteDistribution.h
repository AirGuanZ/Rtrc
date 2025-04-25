#pragma once

#include <cassert>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

struct DynamicWeightDiscreteDistributionDummyPayload {};

// See "Weighted random sampling with replacement with dynamic weights" (https://www.aarondefazio.com/tangentially/?p=58)
//     "Dynamic Generation of Discrete Random Variates"
template<typename F, typename Payload = DynamicWeightDiscreteDistributionDummyPayload>
class DynamicDiscreteDistribution
{
public:

    DynamicDiscreteDistribution(F expectedMinValue, F expectedMaxValue);
    DynamicDiscreteDistribution(F expectedMinValue, F expectedMaxValue, uint32_t seed);

    uint32_t Add(F weight, Payload payload = {});
    void Update(uint32_t index, uint32_t weight);
    void Remove(uint32_t index);

    bool IsEmpty() const;

    void RecomputeWeights();

    uint32_t Sample();
    uint32_t Sample(Payload &payload);

private:

    using Self = DynamicDiscreteDistribution;

    struct Item
    {
        F weight;
        int level;
        uint32_t indexInLevel;
        [[no_unique_address]] Payload payload;
    };

    struct Level
    {
        F sumWeights;
        F levelMax;
        std::vector<uint32_t> items;
    };

    static int ComputeLevel(F value);

    void ExpandLevels(int levelBegin, int levelEnd);

    std::vector<Item> items_;
    std::vector<uint32_t> freeItemIndices_;

    F sumWeights_;
    int levelBegin_;
    int levelEnd_;
    int levelCount_;
    
    // levels_[i] stores items within [2^(i+levelBegin_), 2^(i+1+levelBegin_))
    std::vector<Level> levels_;

    std::default_random_engine rng_;
};

template <typename F, typename Payload>
DynamicDiscreteDistribution<F, Payload>::DynamicDiscreteDistribution(F expectedMinValue, F expectedMaxValue)
    : DynamicDiscreteDistribution(expectedMinValue, expectedMaxValue, std::random_device{}())
{

}

template <typename F, typename Payload>
DynamicDiscreteDistribution<F, Payload>::DynamicDiscreteDistribution(
    F expectedMinValue, F expectedMaxValue, uint32_t seed)
    : sumWeights_(0), levelBegin_(0), levelEnd_(0), levelCount_(0), rng_(seed)
{
    assert(0 < expectedMinValue && expectedMinValue <= expectedMaxValue);
    const int levelBegin = Self::ComputeLevel(expectedMinValue);
    const int levelEnd = Self::ComputeLevel(expectedMaxValue) + 1;
    ExpandLevels(levelBegin, levelEnd);
}

template <typename F, typename Payload>
uint32_t DynamicDiscreteDistribution<F, Payload>::Add(F weight, Payload payload)
{
    if(freeItemIndices_.empty())
    {
        const uint32_t newIndex = static_cast<uint32_t>(items_.size());
        items_.emplace_back();
        freeItemIndices_.push_back(newIndex);
    }

    const uint32_t itemIndex = freeItemIndices_.back();
    freeItemIndices_.pop_back();
    Item &item = items_[itemIndex];

    item.weight = weight;
    item.level = Self::ComputeLevel(weight);
    item.payload = std::move(payload);

    Self::ExpandLevels((std::min)(levelBegin_, item.level), (std::max)(levelEnd_, item.level + 1));
    Level &level = levels_[item.level - levelBegin_];
    assert(weight <= level.levelMax);

    item.indexInLevel = static_cast<uint32_t>(level.items.size());
    level.items.push_back(itemIndex);
    level.sumWeights += weight;

    sumWeights_ += weight;

    return itemIndex;
}

template <typename F, typename Payload>
int DynamicDiscreteDistribution<F, Payload>::ComputeLevel(F value)
{
    return static_cast<int>(std::floor(std::log2(value)));
}

template <typename F, typename Payload>
void DynamicDiscreteDistribution<F, Payload>::ExpandLevels(int levelBegin, int levelEnd)
{
    if(levelBegin_ <= levelBegin && levelEnd <= levelEnd_)
    {
        return;
    }

    if(levelBegin_ >= levelEnd_)
    {
        levelEnd_ = levelBegin_ = levelBegin;
    }

    levelBegin = (std::min)(levelBegin, levelBegin_);
    levelEnd = (std::max)(levelEnd, levelEnd_);

    const int levelCount = levelEnd - levelBegin;
    levels_.resize(levelCount);

    if(levelBegin < levelBegin_)
    {
        const int beginDiff = levelBegin_ - levelBegin;
        std::ranges::move_backward(
            levels_.begin(), levels_.begin() + levelCount_,
            levels_.begin() + beginDiff + levelCount_);
        for(int i = 0; i < beginDiff; ++i)
        {
            levels_[i].sumWeights = 0;
            if constexpr(std::is_integral_v<F>)
            {
                levels_[i].levelMax = 2 << (i + levelBegin + 1);
            }
            else
            {
                levels_[i].levelMax = std::pow<F>(2, i + levelBegin + 1);
            }
        }
    }

    if(levelEnd_ < levelEnd)
    {
        const int endDiff = levelEnd - levelEnd_;
        const int baseIndex = levelCount - endDiff;
        for(int i = 0; i < endDiff; ++i)
        {
            levels_[i + baseIndex].sumWeights = 0;
            if constexpr(std::is_integral_v<F>)
            {
                levels_[i + baseIndex].levelMax = 2 << (i + baseIndex + levelBegin + 1);
            }
            else
            {
                levels_[i + baseIndex].levelMax = std::pow<F>(2, i + baseIndex + levelBegin + 1);
            }
        }
    }

    levelBegin_ = levelBegin;
    levelEnd_ = levelEnd;
    levelCount_ = levelCount;
}

template <typename F, typename Payload>
void DynamicDiscreteDistribution<F, Payload>::Update(uint32_t index, uint32_t weight)
{
    Payload payload = std::move(items_[index].payload);
    Remove(index);
    [[maybe_unused]] const uint32_t newIndex = Add(weight, std::move(payload));
    assert(newIndex == index);
}

template <typename F, typename Payload>
void DynamicDiscreteDistribution<F, Payload>::Remove(uint32_t index)
{
    Item &item = items_[index];
    Level &level = levels_[item.level - levelBegin_];

    const int swappedItemIndex = level.items.back();
    if(swappedItemIndex != index)
    {
        std::swap(level.items.back(), level.items[item.indexInLevel]);
        items_[swappedItemIndex].indexInLevel = item.indexInLevel;
    }
    assert(level.items.back() == index);
    level.items.pop_back();

    level.sumWeights -= item.weight;
    sumWeights_ -= item.weight;

    freeItemIndices_.push_back(index);

    if((level.sumWeights != 0) != (!level.items.empty()) || level.sumWeights < 0 || sumWeights_ < 0)
    {
        RecomputeWeights();
    }
}

template <typename F, typename Payload>
bool DynamicDiscreteDistribution<F, Payload>::IsEmpty() const
{
    return freeItemIndices_.size() == items_.size();
}

template <typename F, typename Payload>
void DynamicDiscreteDistribution<F, Payload>::RecomputeWeights()
{
    sumWeights_ = 0;
    for(auto& level : levels_)
    {
        level.sumWeights = 0;
        for(uint32_t itemIndex : level.items)
        {
            level.sumWeights += items_[itemIndex].weight;
        }
        sumWeights_ += level.sumWeights;
    }
}

template <typename F, typename Payload>
uint32_t DynamicDiscreteDistribution<F, Payload>::Sample()
{
    Payload dummyPayload;
    return this->Sample(dummyPayload);
}

template <typename F, typename Payload>
uint32_t DynamicDiscreteDistribution<F, Payload>::Sample(Payload &payload)
{
    F u;
    if constexpr(std::is_integral_v<F>)
    {
        u = std::uniform_int_distribution<F>(1, sumWeights_)(rng_);
    }
    else
    {
        u = std::uniform_real_distribution<F>(0, sumWeights_)(rng_);
    }

    F s = 0;
    int levelIndex = levelCount_ - 1;
    for(int i = 0; i < levelCount_; ++i)
    {
        s += levels_[i].sumWeights;
        if(u <= s)
        {
            levelIndex = i;
            break;
        }
    }

    if(levels_[levelIndex].items.empty())
    {
        for(int i = 0; i < levelCount_; ++i)
        {
            if(!levels_[i].items.empty())
            {
                levelIndex = i;
                break;
            }
        }
    }

    const Level &level = levels_[levelIndex];
    assert(!level.items.empty());

    while(true)
    {
        const uint32_t indexInLevel = std::uniform_int_distribution<uint32_t>(0, level.items.size() - 1)(rng_);
        const uint32_t itemIndex = level.items[indexInLevel];
        const F weight = items_[itemIndex].weight;

        F r;
        if constexpr(std::is_integral_v<F>)
        {
            r = std::uniform_int_distribution<F>(1, level.levelMax - 1)(rng_);
        }
        else
        {
            r = std::uniform_real_distribution<F>(0, level.levelMax)(rng_);
        }

        if(r <= weight)
        {
            payload = items_[itemIndex].payload;
            return itemIndex;
        }
    }
}

RTRC_END
