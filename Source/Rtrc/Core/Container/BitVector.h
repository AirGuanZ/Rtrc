#pragma once

#include <cassert>
#include <compare>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <vector>

#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

namespace BitVectorDetail
{

    template<std::unsigned_integral BaseType>
    static constexpr BaseType ONE_MASK = (std::numeric_limits<BaseType>::max)();

    template<std::unsigned_integral BaseType>
    static constexpr BaseType ZERO_MASK = 0;

    template<std::unsigned_integral BaseType>
    static constexpr uint32_t BITS = std::numeric_limits<BaseType>::digits;

    template<std::unsigned_integral BaseType>
    bool Get(const BaseType *storage, size_t index)
    {
        constexpr uint32_t UNIT_BITS = BITS<BaseType>;
        const size_t unitIndex = index / UNIT_BITS;
        const size_t indexInUnit = index - unitIndex * UNIT_BITS;
        return (storage[unitIndex] >> indexInUnit) & 1u;
    }

    template<std::unsigned_integral BaseType>
    void Set(BaseType *storage, size_t index, bool value)
    {
        constexpr uint32_t UNIT_BITS = BITS<BaseType>;
        const size_t unitIndex = index / UNIT_BITS;
        const size_t indexInUnit = index - unitIndex * UNIT_BITS;
        const BaseType mask = BaseType(1) << indexInUnit;
        if(value)
        {
            storage[unitIndex] |= mask;
        }
        else
        {
            storage[unitIndex] &= ~mask;
        }
    }

    template<std::unsigned_integral BaseType>
    void Fill(BaseType *storage, size_t begin, size_t end, bool value)
    {
        constexpr uint32_t UNIT_BITS = BITS<BaseType>;
        assert(begin <= end);

        const size_t unitBegin = begin / UNIT_BITS;
        const size_t unitEnd = end / UNIT_BITS;

        // Everything is in the same unit
        if(unitBegin == unitEnd)
        {
            const size_t bitBegin = begin % UNIT_BITS;
            const size_t bitEnd = end % UNIT_BITS;
            if(bitBegin < bitEnd)
            {
                const BaseType mask = ((BaseType(1) << (bitEnd - bitBegin)) - 1) << bitBegin;
                if(value)
                {
                    storage[unitBegin] |= mask;
                }
                else
                {
                    storage[unitBegin] &= ~mask;
                }
            }
            return;
        }

        // Fill the first incomplete unit
        const size_t bitBegin = begin % UNIT_BITS;
        if(bitBegin != 0)
        {
            const BaseType mask = ONE_MASK<BaseType> << bitBegin;
            if(value)
            {
                storage[unitBegin] |= mask;
            }
            else
            {
                storage[unitBegin] &= ~mask;
            }
        }

        // Fill the complete units
        const size_t firstFullUnit = unitBegin + (bitBegin != 0 ? 1 : 0);
        const size_t lastFullUnit = unitEnd;
        if(firstFullUnit < lastFullUnit)
        {
            std::fill(storage + firstFullUnit, storage + lastFullUnit, value ? ONE_MASK<BaseType> : ZERO_MASK<BaseType>);
        }

        // Fill the last incomplete unit
        const size_t bitEnd = end % UNIT_BITS;
        if(bitEnd != 0)
        {
            const BaseType mask = (BaseType(1) << bitEnd) - 1;
            if(value)
            {
                storage[unitEnd] |= mask;
            }
            else
            {
                storage[unitEnd] &= ~mask;
            }
        }
    }

} // namespace BitVectorDetail

template<typename BaseType = uint32_t>
class BitVector
{
    static_assert(std::unsigned_integral<BaseType>);

    static constexpr uint32_t UNIT_BITS = BitVectorDetail::BITS<BaseType>;

    std::vector<BaseType> storage_;
    size_t size_ = 0;

public:

    template<bool IsConst>
    class BitIterator;

    class BitProxy
    {
        friend class BitVector;

        template<bool IsConst>
        friend class BitIterator;

        BaseType *storage_ = nullptr;
        size_t index_ = 0;

    public:

        operator bool() const;
        BitProxy &operator=(bool value);
    };

    template<bool IsConst>
    class BitIterator
    {
        friend class BitVector;

        std::conditional_t<IsConst, const BaseType*, BaseType*> storage_ = nullptr;
        size_t index_ = 0;

    public:

        std::conditional_t<IsConst, bool, BitProxy> operator*() const;

        BitIterator operator++();
        BitIterator operator++(int);
        BitIterator operator--();
        BitIterator operator--(int);

        std::strong_ordering operator<=>(const BitIterator &other) const;
        bool operator==(const BitIterator &other) const;
    };

    template<bool Zero>
    class BitIndexIterator
    {
        friend class BitVector;

        const BaseType *storage_ = nullptr;
        size_t storageSize_ = 0;
        size_t size_ = 0;
        size_t currentUnitIndex_ = 0;
        size_t currentBitIndex_ = 0;
        BaseType currentUnit_;

        BaseType FetchUnit(size_t index) const;
        void NextBit();
        void NextUnit();
        void Initialize(const BaseType *storage, size_t size, bool end);

    public:

        size_t operator*() const;
        BitIndexIterator operator++();
        BitIndexIterator operator++(int);

        std::strong_ordering operator<=>(const BitIndexIterator &other) const;
        bool operator==(const BitIndexIterator &other) const;
    };

    template<bool Zero>
    class BitIndexRange
    {
        friend class BitVector;

        const BitVector *vector = nullptr;

    public:

        BitIndexIterator<Zero> begin() const;
        BitIndexIterator<Zero> end() const;
    };

    BitVector() = default;
    BitVector(size_t size, bool value);

    BitVector(const BitVector &other);
    BitVector(BitVector &&other) noexcept;

    BitVector &operator=(const BitVector &other);
    BitVector &operator=(BitVector &&other) noexcept;

    void Swap(BitVector &other) noexcept;

    size_t GetSize() const;
    bool IsEmpty() const;

    void Fill(bool value);

    void Resize(size_t size, bool value = false);
    void Clear(); // Resize to 0, without shrinking memory

    size_t GetCapacity() const;
    void Reserve(size_t size);
    void ShrinkToFit(); // Shrink storage to fit the current size

    void PushBack(bool value);
    void PopBack();

    auto First() const { assert(size_ > 0); return (*this)[0]; }
    auto First() { assert(size_ > 0); return (*this)[0]; }

    auto Last() const { assert(size_ > 0); return (*this)[size_ - 1]; }
    auto Last() { assert(size_ > 0); return (*this)[size_ - 1]; }

    const BaseType *GetData() const;
          BaseType *GetData();

        bool At(size_t index) const;
    BitProxy At(size_t index);

        bool operator[](size_t index) const;
    BitProxy operator[](size_t index);

    BitIterator<true> begin() const;
    BitIterator<true> end() const;

    BitIterator<true> cbegin() const;
    BitIterator<true> cend() const;

    BitIterator<false> begin();
    BitIterator<false> end();

    // Iterate non-zero bit indices
    BitIndexIterator<false> OneIndexBegin() const;
    BitIndexIterator<false> OneIndexEnd() const;

    // Iterate zero bit indices
    BitIndexIterator<true> ZeroIndexBegin() const;
    BitIndexIterator<true> ZeroIndexEnd() const;

    // Example:
    //     for(size_t bitIndex : bitVector.OneIndices())
    //         ...
    BitIndexRange<false> OneIndices() const &;
    BitIndexRange<true>  ZeroIndices() const &;
};

template <typename BaseType>
BitVector<BaseType>::BitProxy::operator bool() const
{
    assert(storage_);
    return BitVectorDetail::Get(storage_, index_);
}

template <typename BaseType>
typename BitVector<BaseType>::BitProxy &BitVector<BaseType>::BitProxy::operator=(bool value)
{
    assert(storage_);
    BitVectorDetail::Set(storage_, index_, value);
    return *this;
}

template <typename BaseType>
template <bool IsConst>
std::conditional_t<IsConst, bool, typename BitVector<BaseType>::BitProxy> BitVector<BaseType>::BitIterator<IsConst>::operator*() const
{
    assert(storage_);
    if constexpr(IsConst)
    {
        return BitVectorDetail::Get(storage_, index_);
    }
    else
    {
        BitProxy proxy;
        proxy.storage_ = storage_;
        proxy.index_ = index_;
        return proxy;
    }
}

template <typename BaseType>
template <bool IsConst>
typename BitVector<BaseType>::template BitIterator<IsConst> BitVector<BaseType>::BitIterator<IsConst>::operator++()
{
    ++index_;
    return *this;
}

template <typename BaseType>
template <bool IsConst>
typename BitVector<BaseType>::template BitIterator<IsConst> BitVector<BaseType>::BitIterator<IsConst>::operator++(int)
{
    const auto result = *this;
    ++*this;
    return result;
}

template <typename BaseType>
template <bool IsConst>
typename BitVector<BaseType>::template BitIterator<IsConst> BitVector<BaseType>::BitIterator<IsConst>::operator--()
{
    assert(index_ > 0);
    --index_;
    return *this;
}

template <typename BaseType>
template <bool IsConst>
typename BitVector<BaseType>::template BitIterator<IsConst> BitVector<BaseType>::BitIterator<IsConst>::operator--(int)
{
    const auto result = *this;
    --*this;
    return result;
}

template <typename BaseType>
template <bool IsConst>
std::strong_ordering BitVector<BaseType>::BitIterator<IsConst>::operator<=>(const BitIterator &other) const
{
    assert(storage_ == other.storage_);
    return index_ <=> other.index_;
}

template <typename BaseType>
template <bool IsConst>
bool BitVector<BaseType>::BitIterator<IsConst>::operator==(const BitIterator &other) const
{
    assert(storage_ == other.storage_);
    return index_ == other.index_;
}

template <typename BaseType>
template <bool Zero>
BaseType BitVector<BaseType>::BitIndexIterator<Zero>::FetchUnit(size_t index) const
{
    assert(index < storageSize_);

    BaseType unit = storage_[index];
    if constexpr(Zero)
    {
        unit = ~unit;
    }

    if(index + 1 == storageSize_) // This is the last unit
    {
        if(const uint32_t validBits = size_ % BitVectorDetail::BITS<BaseType>) // Clear trailing garbage bits in the last unit
        {
            const BaseType mask = (BaseType(1) << validBits) - 1;
            unit &= mask;
        }
    }
    return unit;
}

template <typename BaseType>
template <bool Zero>
void BitVector<BaseType>::BitIndexIterator<Zero>::NextBit()
{
    assert(currentBitIndex_ < size_ && "Cannot advance a out-of-bound bit iterator");

    if(!currentUnit_)
    {
        NextUnit();
        return;
    }

    const uint32_t localIndex = std::countr_zero(currentUnit_);
    currentUnit_ &= ~(BaseType(1) << localIndex);
    currentBitIndex_ = currentUnitIndex_ * BitVectorDetail::BITS<BaseType> + localIndex;
}

template <typename BaseType>
template <bool Zero>
void BitVector<BaseType>::BitIndexIterator<Zero>::NextUnit()
{
    while(true)
    {
        if(++currentUnitIndex_ >= storageSize_)
        {
            currentBitIndex_ = size_;
            return;
        }

        currentUnit_ = FetchUnit(currentUnitIndex_);
        if(currentUnit_)
        {
            const uint32_t localIndex = std::countr_zero(currentUnit_);
            currentUnit_ &= ~(BaseType(1) << localIndex);
            currentBitIndex_ = currentUnitIndex_ * BitVectorDetail::BITS<BaseType> +localIndex;
            return;
        }
    }
}

template <typename BaseType>
template <bool Zero>
void BitVector<BaseType>::BitIndexIterator<Zero>::Initialize(const BaseType* storage, size_t size, bool end)
{
    storage_ = storage;
    storageSize_ = (size + BitVectorDetail::BITS<BaseType> - 1) / BitVectorDetail::BITS<BaseType>;
    size_ = size;
    currentUnitIndex_ = 0;
    currentBitIndex_ = 0;
    if(end)
    {
        currentUnit_ = 0;
        currentBitIndex_ = size_;
        // Leave currentUnitIndex_ unchanged as it will not be used anymore
    }
    else
    {
        if(size_)
        {
            currentUnit_ = FetchUnit(0);
            NextBit();
        }
        else
        {
            currentUnit_ = 0;
        }
    }
}

template <typename BaseType>
template <bool Zero>
size_t BitVector<BaseType>::BitIndexIterator<Zero>::operator*() const
{
    assert(currentBitIndex_ < size_ && "deferencing end iterator");
    return currentBitIndex_;
}

template <typename BaseType>
template <bool Zero>
typename BitVector<BaseType>::template BitIndexIterator<Zero> BitVector<BaseType>::BitIndexIterator<Zero>::operator++()
{
    NextBit();
    return *this;
}

template <typename BaseType>
template <bool Zero>
typename BitVector<BaseType>::template BitIndexIterator<Zero> BitVector<BaseType>::BitIndexIterator<Zero>::operator++(int)
{
    const auto result = *this;
    ++*this;
    return result;
}

template <typename BaseType>
template <bool Zero>
std::strong_ordering BitVector<BaseType>::BitIndexIterator<Zero>::operator<=>(const BitIndexIterator &other) const
{
    assert(storage_ == other.storage_);
    assert(size_ == other.size_);
    return currentBitIndex_ <=> other.currentBitIndex_;
}

template <typename BaseType>
template <bool Zero>
bool BitVector<BaseType>::BitIndexIterator<Zero>::operator==(const BitIndexIterator &other) const
{
    assert(storage_ == other.storage_);
    assert(size_ == other.size_);
    return currentBitIndex_ == other.currentBitIndex_;
}

template <typename BaseType>
template <bool Zero>
typename BitVector<BaseType>::template BitIndexIterator<Zero> BitVector<BaseType>::BitIndexRange<Zero>::begin() const
{
    if constexpr(Zero)
    {
        return vector->ZeroIndexBegin();
    }
    else
    {
        return vector->OneIndexBegin();
    }
}

template <typename BaseType>
template <bool Zero>
typename BitVector<BaseType>::template BitIndexIterator<Zero> BitVector<BaseType>::BitIndexRange<Zero>::end() const
{
    if constexpr(Zero)
    {
        return vector->ZeroIndexEnd();
    }
    else
    {
        return vector->OneIndexEnd();
    }
}

template <typename BaseType>
BitVector<BaseType>::BitVector(size_t size, bool value)
{
    Resize(size, value);
}

template <typename BaseType>
BitVector<BaseType>::BitVector(const BitVector &other)
{
    storage_ = other.storage_;
    size_ = other.size_;
}

template <typename BaseType>
BitVector<BaseType>::BitVector(BitVector &&other) noexcept
{
    this->Swap(other);
}

template <typename BaseType>
BitVector<BaseType> &BitVector<BaseType>::operator=(const BitVector &other)
{
    BitVector copy(other);
    this->Swap(copy);
    return *this;
}

template <typename BaseType>
BitVector<BaseType> &BitVector<BaseType>::operator=(BitVector &&other) noexcept
{
    this->Swap(other);
    return *this;
}

template <typename BaseType>
void BitVector<BaseType>::Swap(BitVector &other) noexcept
{
    storage_.swap(other.storage_);
    std::swap(size_, other.size_);
}

template <typename BaseType>
size_t BitVector<BaseType>::GetSize() const
{
    return size_;
}

template <typename BaseType>
bool BitVector<BaseType>::IsEmpty() const
{
    return size_ == 0;
}

template <typename BaseType>
void BitVector<BaseType>::Fill(bool value)
{
#if RTRC_DEBUG
    if(size_ > 0)
    {
        BitVectorDetail::Fill(storage_.data(), 0, size_, value);
    }
#else
    std::ranges::fill(storage_, value ? BitVectorDetail::ONE_MASK<BaseType> : BitVectorDetail::ZERO_MASK<BaseType>);
#endif
}

template <typename BaseType>
void BitVector<BaseType>::Resize(size_t size, bool value)
{
    if(size_ < size)
    {
        Reserve(size);
        BitVectorDetail::Fill(storage_.data(), size_, size, value);
    }
#if RTRC_DEBUG
    else if(size_ > size)
    {
        BitVectorDetail::Fill(storage_.data(), size, size_, false);
    }
#endif
    size_ = size;
}

template <typename BaseType>
void BitVector<BaseType>::Clear()
{
    size_ = 0;
}

template <typename BaseType>
size_t BitVector<BaseType>::GetCapacity() const
{
    return storage_.size() * UNIT_BITS;
}

template <typename BaseType>
void BitVector<BaseType>::Reserve(size_t size)
{
    const size_t reservedUnits = (size + (UNIT_BITS - 1)) / UNIT_BITS;
    if(storage_.size() < reservedUnits)
    {
        storage_.resize(reservedUnits);
    }
}

template <typename BaseType>
void BitVector<BaseType>::ShrinkToFit()
{
    const size_t requiredUnits = (size_ + (UNIT_BITS - 1)) / UNIT_BITS;
    assert(storage_.size() >= requiredUnits);
    storage_.resize(requiredUnits);
    storage_.shrink_to_fit();
}

template <typename BaseType>
void BitVector<BaseType>::PushBack(bool value)
{
    if(GetCapacity() < size_ + 1)
    {
        storage_.push_back(0);
    }
    BitVectorDetail::Set(storage_.data(), size_, value);
    ++size_;
}

template <typename BaseType>
void BitVector<BaseType>::PopBack()
{
    assert(size_ > 0);
    Resize(size_ - 1);
}

template <typename BaseType>
const BaseType* BitVector<BaseType>::GetData() const
{
    return storage_.data();
}

template <typename BaseType>
BaseType* BitVector<BaseType>::GetData()
{
    return storage_.data();
}

template <typename BaseType>
bool BitVector<BaseType>::At(size_t index) const
{
    if(index >= size_)
    {
        throw std::out_of_range("Rtrc::BitVector: index is out of range");
    }
    return BitVectorDetail::Get(storage_.data(), index);
}

template <typename BaseType>
typename BitVector<BaseType>::BitProxy BitVector<BaseType>::At(size_t index)
{
    if(index >= size_)
    {
        throw std::out_of_range("Rtrc::BitVector: index is out of range");
    }
    BitProxy proxy;
    proxy.storage_ = storage_.data();
    proxy.index_ = index;
    return proxy;
}

template <typename BaseType>
bool BitVector<BaseType>::operator[](size_t index) const
{
    assert(index < size_);
    return BitVectorDetail::Get(storage_.data(), index);
}

template <typename BaseType>
typename BitVector<BaseType>::BitProxy BitVector<BaseType>::operator[](size_t index)
{
    assert(index < size_);
    BitProxy proxy;
    proxy.storage_ = storage_.data();
    proxy.index_ = index;
    return proxy;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIterator<true> BitVector<BaseType>::begin() const
{
    BitIterator<true> it;
    it.storage_ = storage_.data();
    it.index_ = 0;
    return it;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIterator<true> BitVector<BaseType>::end() const
{
    BitIterator<true> it;
    it.storage_ = storage_.data();
    it.index_ = size_;
    return it;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIterator<false> BitVector<BaseType>::begin()
{
    BitIterator<false> it;
    it.storage_ = storage_.data();
    it.index_ = 0;
    return it;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIterator<false> BitVector<BaseType>::end()
{
    BitIterator<false> it;
    it.storage_ = storage_.data();
    it.index_ = size_;
    return it;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIterator<true> BitVector<BaseType>::cbegin() const
{
    return begin();
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIterator<true> BitVector<BaseType>::cend() const
{
    return end();
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIndexIterator<false> BitVector<BaseType>::OneIndexBegin() const
{
    BitIndexIterator<false> result;
    result.Initialize(storage_.data(), size_, false);
    return result;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIndexIterator<false> BitVector<BaseType>::OneIndexEnd() const
{
    BitIndexIterator<false> result;
    result.Initialize(storage_.data(), size_, true);
    return result;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIndexIterator<true> BitVector<BaseType>::ZeroIndexBegin() const
{
    BitIndexIterator<true> result;
    result.Initialize(storage_.data(), size_, false);
    return result;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIndexIterator<true> BitVector<BaseType>::ZeroIndexEnd() const
{
    BitIndexIterator<true> result;
    result.Initialize(storage_.data(), size_, true);
    return result;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIndexRange<false> BitVector<BaseType>::OneIndices() const &
{
    BitIndexRange<false> result;
    result.vector = this;
    return result;
}

template <typename BaseType>
typename BitVector<BaseType>::template BitIndexRange<true> BitVector<BaseType>::ZeroIndices() const &
{
    BitIndexRange<true> result;
    result.vector = this;
    return result;
}

RTRC_END
