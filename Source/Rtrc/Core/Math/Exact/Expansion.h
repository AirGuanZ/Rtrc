#pragma once

#include <cassert>
#include <ostream>
#include <ranges>

#include <boost/multiprecision/cpp_bin_float.hpp>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/String.h>

RTRC_BEGIN

// See Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates

namespace ExpansionUtility
{

    bool CheckRuntimeFloatingPointSettingsForExpansion();

    template<typename F>
    std::string ToBitString(F a, bool splitComponents = false, bool bitRange = false);

    // Assumes no INF/NAN
    template<typename F>
    bool CheckNonOverlappingProperty(F af, F bf);

    template<typename F>
    int GrowExpansion(const F *e, int eCount, F b, F* h);

    template<typename F>
    int ExpansionSum(const F *e, int eCount, const F *f, int fCount, F *h);

    template<typename F>
    int ExpansionSumNegative(const F *e, int eCount, const F *f, int fCount, F *h);

    template<typename F>
    int ScaleExpansion(const F *e, int eCount, F b, F *h);

    template<typename F>
    int CompressExpansion(const F *e, int eCount, F *h);

    constexpr size_t MaxInlinedItemCount = 16;

    template<bool HasMember> class CapacityMember;
    template<> class CapacityMember<true> { protected: size_t capacity_ = 0; };
    template<> class CapacityMember<false> { };

    consteval size_t StaticStorageFilter(size_t v) { return v <= MaxInlinedItemCount ? v : 0; }
    consteval size_t StaticStorageAdd(size_t a, size_t b) { return StaticStorageFilter(a && b ? (a + b) : 0); }
    consteval size_t StaticStorageMul(size_t a, size_t b) { return StaticStorageFilter(a && b ? (2 * a * b) : 0); }
    consteval bool StaticStorageGreaterEqual(size_t a, size_t b) { return !a ? true : (!b ? false : (a >= b)); }

    // Combine multiprecision numbers with arithmetic expansions to avoid exponent overflow/underflow

    using float_24_7 = float;
    using float_53_11 = double;
    using float_100_27 = boost::multiprecision::number<
        boost::multiprecision::backends::cpp_bin_float<
        100, boost::multiprecision::backends::digit_base_2,
        void, int32_t, -67108862, 67108863>>;
    using float_200_55 = boost::multiprecision::number<
        boost::multiprecision::backends::cpp_bin_float<
        200, boost::multiprecision::backends::digit_base_2,
        void, int64_t, -18014398509481982, 18014398509481983>>;

    using DefaultWord = float_100_27;

    template<typename Word>
    using PromoteWord = std::conditional_t<
        (sizeof(Word) > sizeof(DefaultWord)), Word, DefaultWord>;

} // namespace ExpansionUtility

/* StaticStorage == 0 means supporting dynamic storage */
template<typename Word, size_t StaticStorage>
class SExpansion : public ExpansionUtility::CapacityMember<StaticStorage == 0>
{
public:

    using WordType = Word;

    static constexpr bool SupportDynamicStorage = StaticStorage == 0;
    
    SExpansion();
    SExpansion(Word source);

    SExpansion(const SExpansion &other);
    SExpansion(SExpansion &&other) noexcept;

    SExpansion &operator=(const SExpansion &other);
    SExpansion &operator=(SExpansion &&other) noexcept;

    template<size_t StaticStorage2> requires (!StaticStorage || StaticStorage2 <= StaticStorage)
    SExpansion(const SExpansion<Word, StaticStorage2> &other);
    template<size_t StaticStorage2> requires (!StaticStorage || StaticStorage2 <= StaticStorage)
    SExpansion &operator=(const SExpansion<Word, StaticStorage2> &other);

    ~SExpansion();

    void Swap(SExpansion &other) noexcept;

    bool CheckSanity() const;

    uint32_t GetLength() const;
    uint32_t GetCapacity() const;
    Span<Word> GetItems() const;
    Word ToWord() const;
    float ToFloat() const;
    double ToDouble() const;
    operator double() const { return ToDouble(); }

    int GetSign() const;

    void Reserve(uint32_t capacity);

    template<size_t SL, size_t SR>
    void SetSum(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs);
    template<size_t SL, size_t SR>
    void SetDiff(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs);
    template<size_t SL, size_t SR>
    void SetMul(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs);
    template<size_t SL>
    void SetMul(const SExpansion<Word, SL> &lhs, Word rhs);

    template<size_t S> requires !StaticStorage
    SExpansion &operator+=(const SExpansion<Word, S> &rhs);
    template<size_t S> requires !StaticStorage
    SExpansion &operator-=(const SExpansion<Word, S> &rhs);
    template<size_t S> requires !StaticStorage
    SExpansion &operator*=(const SExpansion<Word, S> &rhs);
    SExpansion &operator*=(Word rhs);

    void Compress();
    void SetNegative();

    template<int S>
    int Compare(const SExpansion<Word, S> &rhs) const;

    SExpansion operator-() const;

    template<int S>
    std::strong_ordering operator<=>(const SExpansion<Word, S> &rhs) const;
    template<int S>
    bool operator==(const SExpansion<Word, S> &rhs) const;

    std::string ToString() const;
    
          Word *GetItemPointer();
    const Word *GetItemPointer() const;

private:

    static constexpr size_t InlinedItemCount = StaticStorage ? StaticStorage : ExpansionUtility::MaxInlinedItemCount;
    static constexpr size_t StorageSize = (std::max)(InlinedItemCount * sizeof(Word), sizeof(void *));
    static_assert(InlinedItemCount <= ExpansionUtility::MaxInlinedItemCount);

    uint32_t size_;
    unsigned char storage_[StorageSize];
};

template<typename Word>
SExpansion(Word) -> SExpansion<ExpansionUtility::PromoteWord<Word>, 1>;

using Expansion  = SExpansion<ExpansionUtility::DefaultWord, 0>;

inline Expansion ToDynamicExpansion(double v)
{
    return Expansion(v);
}

template<typename Word, size_t SL, size_t SR>
auto operator+(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs)
{
    SExpansion<Word, ExpansionUtility::StaticStorageAdd(SL, SR)> ret;
    ret.SetSum(lhs, rhs);
    return ret;
}

template<typename Word, size_t SL, size_t SR>
auto operator-(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs)
{
    SExpansion<Word, ExpansionUtility::StaticStorageAdd(SL, SR)> ret;
    ret.SetDiff(lhs, rhs);
    return ret;
}

template<typename Word, size_t SL, size_t SR>
auto operator*(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs)
{
    SExpansion<Word, ExpansionUtility::StaticStorageMul(SL, SR)> ret;
    ret.SetMul(lhs, rhs);
    return ret;
}

template<typename Word, size_t SL>
auto operator*(const SExpansion<Word, SL> &lhs, Word rhs)
{
    SExpansion<Word, ExpansionUtility::StaticStorageMul(SL, 1)> ret;
    ret.SetMul(lhs, rhs);
    return ret;
}

template<typename Word, size_t SR>
auto operator*(Word lhs, const SExpansion<Word, SR> &rhs)
{
    return rhs * lhs;
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage>::SExpansion()
{
    assert(ExpansionUtility::CheckRuntimeFloatingPointSettingsForExpansion());
    if constexpr(SupportDynamicStorage)
    {
        this->capacity_ = InlinedItemCount;
    }
    size_ = 1;
    GetItemPointer()[0] = 0.0;
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage>::SExpansion(Word source)
    : SExpansion()
{
    GetItemPointer()[0] = source;
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage>::SExpansion(const SExpansion &other)
    : SExpansion()
{
    this->Reserve(other.size_);
    size_ = other.size_;
    for(uint32_t i = 0; i < size_; ++i)
    {
        new(GetItemPointer() + i) Word(other.GetItemPointer()[i]);
    }
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage>::SExpansion(SExpansion &&other) noexcept
    : SExpansion()
{
    this->Swap(other);
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage> &SExpansion<Word, StaticStorage>::operator=(const SExpansion &other)
{
    SExpansion t(other);
    this->Swap(t);
    return *this;
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage> &SExpansion<Word, StaticStorage>::operator=(SExpansion &&other) noexcept
{
    this->Swap(other);
    return *this;
}

template <typename Word, size_t StaticStorage>
template <size_t StaticStorage2> requires (!StaticStorage || StaticStorage2 <= StaticStorage)
SExpansion<Word, StaticStorage>::SExpansion(const SExpansion<Word, StaticStorage2> &other)
    : SExpansion()
{
    Reserve(other.GetLength());
    size_ = other.GetLength();
    for(uint32_t i = 0; i < size_; ++i)
    {
        new(GetItemPointer() + i) Word(other.GetItemPointer()[i]);
    }
}

template <typename Word, size_t StaticStorage>
template <size_t StaticStorage2> requires (!StaticStorage || StaticStorage2 <= StaticStorage)
SExpansion<Word, StaticStorage> &SExpansion<Word, StaticStorage>::operator=(const SExpansion<Word, StaticStorage2> &other)
{
    SExpansion t(other);
    this->Swap(t);
    return *this;
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage>::~SExpansion()
{
    if constexpr(SupportDynamicStorage)
    {
        if(this->capacity_ > InlinedItemCount)
        {
            std::free(GetItemPointer());
        }
    }
}

template <typename Word, size_t StaticStorage>
void SExpansion<Word, StaticStorage>::Swap(SExpansion &other) noexcept
{
    if constexpr(SupportDynamicStorage)
    {
        std::swap(this->capacity_, other.capacity_);
    }
    std::swap(size_, other.size_);
    std::swap(storage_, other.storage_);
}

template <typename Word, size_t StaticStorage>
bool SExpansion<Word, StaticStorage>::CheckSanity() const
{
    if(!size_)
    {
        return false;
    }
    auto items = GetItemPointer();
    for(uint32_t i = 1; i < size_; ++i)
    {
        auto Abs = [](Word w) { return w < 0 ? -w : w; };
        if(Abs(items[i - 1]) >= Abs(items[i]))
        {
            return false;
        }
        if constexpr(std::is_floating_point_v<Word>)
        {
            if(!ExpansionUtility::CheckNonOverlappingProperty(items[i - 1], items[i]))
            {
                return false;
            }
        }
    }
    return true;
}

template <typename Word, size_t StaticStorage>
uint32_t SExpansion<Word, StaticStorage>::GetLength() const
{
    return size_;
}

template <typename Word, size_t StaticStorage>
uint32_t SExpansion<Word, StaticStorage>::GetCapacity() const
{
    if constexpr(SupportDynamicStorage)
    {
        return this->capacity_;
    }
    else
    {
        return StaticStorage;
    }
}

template <typename Word, size_t StaticStorage>
Span<Word> SExpansion<Word, StaticStorage>::GetItems() const
{
    return Span<Word>(GetItemPointer(), size_);
}

template <typename Word, size_t StaticStorage>
Word SExpansion<Word, StaticStorage>::ToWord() const
{
    return *std::ranges::fold_right_last(GetItems(), std::plus<>{});
}

template <typename Word, size_t StaticStorage>
float SExpansion<Word, StaticStorage>::ToFloat() const
{
    return static_cast<float>(ToWord());
}

template <typename Word, size_t StaticStorage>
double SExpansion<Word, StaticStorage>::ToDouble() const
{
    return static_cast<double>(ToWord());
}

template <typename Word, size_t StaticStorage>
int SExpansion<Word, StaticStorage>::GetSign() const
{
    const Word word = GetItems().last();
    return word < 0 ? -1 : (word > 0 ? 1 : 0);
}

template <typename Word, size_t StaticStorage>
void SExpansion<Word, StaticStorage>::Reserve(uint32_t capacity)
{
    if constexpr(SupportDynamicStorage)
    {
        capacity = (std::max<uint32_t>)(capacity, InlinedItemCount);
        if(this->capacity_ >= capacity)
        {
            return;
        }

        void *newPointer = std::malloc(sizeof(Word) * capacity);
        if(!newPointer)
        {
            throw std::bad_alloc();
        }

        void *oldPointer = GetItemPointer();
        for(uint32_t i = 0; i < size_; ++i)
        {
            new (static_cast<Word *>(newPointer) + i) Word(static_cast<Word *>(oldPointer)[i]);
        }
        if(this->capacity_ > InlinedItemCount)
        {
            std::free(oldPointer);
        }

        *reinterpret_cast<void **>(storage_) = newPointer;
        this->capacity_ = capacity;
    }
    else
    {
        assert(capacity <= StaticStorage);
    }
}

template <typename Word, size_t StaticStorage>
template <size_t SL, size_t SR>
void SExpansion<Word, StaticStorage>::SetSum(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs)
{
    static_assert(ExpansionUtility::StaticStorageGreaterEqual(
        StaticStorage, ExpansionUtility::StaticStorageAdd(SL, SR)));
    Reserve(lhs.GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSum(
        lhs.GetItemPointer(), lhs.GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
}

template <typename Word, size_t StaticStorage>
template <size_t SL, size_t SR>
void SExpansion<Word, StaticStorage>::SetDiff(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs)
{
    static_assert(ExpansionUtility::StaticStorageGreaterEqual(
        StaticStorage, ExpansionUtility::StaticStorageAdd(SL, SR)));

    Reserve(lhs.GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSumNegative(
        lhs.GetItemPointer(), lhs.GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
}

template <typename Word, size_t StaticStorage>
template <size_t SL, size_t SR>
void SExpansion<Word, StaticStorage>::SetMul(const SExpansion<Word, SL> &lhs, const SExpansion<Word, SR> &rhs)
{
    static_assert(ExpansionUtility::StaticStorageGreaterEqual(
        StaticStorage, ExpansionUtility::StaticStorageMul(SL, SR)));

    size_ = 1;
    GetItemPointer()[0] = 0.0;
    for(uint32_t i = 0; i < rhs.GetLength(); ++i)
    {
        const auto t = lhs * rhs.GetItemPointer()[i];
        Reserve(GetLength() + t.GetLength());
        size_ = ExpansionUtility::ExpansionSum(
            GetItemPointer(), GetLength(), t.GetItemPointer(), t.GetLength(), GetItemPointer());
    }
    this->Compress();
}

template <typename Word, size_t StaticStorage>
template <size_t SL>
void SExpansion<Word, StaticStorage>::SetMul(const SExpansion<Word, SL> &lhs, Word rhs)
{
    static_assert(ExpansionUtility::StaticStorageGreaterEqual(
        StaticStorage, ExpansionUtility::StaticStorageMul(SL, 1)));

    Reserve(lhs.GetLength() * 2);
    size_ = ExpansionUtility::ScaleExpansion(lhs.GetItemPointer(), lhs.GetLength(), rhs, GetItemPointer());
}

template <typename Word, size_t StaticStorage>
template <size_t S> requires !StaticStorage
SExpansion<Word, StaticStorage> &SExpansion<Word, StaticStorage>::operator+=(const SExpansion<Word, S> &rhs)
{
    Reserve(GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSum(
        GetItemPointer(), GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
    return *this;
}

template <typename Word, size_t StaticStorage>
template <size_t S> requires !StaticStorage
SExpansion<Word, StaticStorage> &SExpansion<Word, StaticStorage>::operator-=(const SExpansion<Word, S> &rhs)
{
    Reserve(GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSumNegative(
        GetItemPointer(), GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
    return *this;
}

template <typename Word, size_t StaticStorage>
template <size_t S> requires !StaticStorage
SExpansion<Word, StaticStorage> &SExpansion<Word, StaticStorage>::operator*=(const SExpansion<Word, S> &rhs)
{
    *this = *this * rhs;
    return *this;
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage> &SExpansion<Word, StaticStorage>::operator*=(Word rhs)
{
    static_assert(SupportDynamicStorage);
    Reserve(GetLength() * 2);
    size_ = ExpansionUtility::ScaleExpansion(GetItemPointer(), GetLength(), rhs, GetItemPointer());
    return *this;
}

template <typename Word, size_t StaticStorage>
void SExpansion<Word, StaticStorage>::Compress()
{
    const uint32_t newSize = ExpansionUtility::CompressExpansion(GetItemPointer(), size_, GetItemPointer());
    assert(newSize <= size_);
    size_ = newSize;
}

template <typename Word, size_t StaticStorage>
void SExpansion<Word, StaticStorage>::SetNegative()
{
    auto items = GetItemPointer();
    for(uint32_t i = 0; i < size_; ++i)
    {
        items[i] = -items[i];
    }
}

template <typename Word, size_t StaticStorage>
template <int S>
int SExpansion<Word, StaticStorage>::Compare(const SExpansion<Word, S> &rhs) const
{
    const SExpansion &lhs = *this;
    const int signLhs = lhs.GetSign();
    const int signRhs = rhs.GetSign();
    if(signLhs < signRhs)
    {
        return -1;
    }
    if(signLhs > signRhs)
    {
        return 1;
    }

    const uint32_t maxDiffSize = lhs.GetLength() + rhs.GetLength();
    constexpr size_t DIFF_SIZE_THRESHOLD = 1024 / sizeof(Word);
    if(maxDiffSize > DIFF_SIZE_THRESHOLD)
    {
        return (lhs - rhs).GetSign();
    }

    Word diff[DIFF_SIZE_THRESHOLD];
    const int diffSize = ExpansionUtility::ExpansionSumNegative(
        lhs.GetItemPointer(), lhs.GetLength(), rhs.GetItemPointer(), rhs.GetLength(), diff);
    assert(diffSize);
    const Word word = diff[diffSize - 1];
    return word < 0 ? -1 : (word > 0 ? 1 : 0);
}

template <typename Word, size_t StaticStorage>
SExpansion<Word, StaticStorage> SExpansion<Word, StaticStorage>::operator-() const
{
    SExpansion ret(*this);
    ret.SetNegative();
    return ret;
}

template <typename Word, size_t StaticStorage>
template <int S>
std::strong_ordering SExpansion<Word, StaticStorage>::operator<=>(const SExpansion<Word, S> &rhs) const
{
    const int compare = this->Compare(rhs);
    return compare < 0 ? std::strong_ordering::less :
        compare > 0 ? std::strong_ordering::greater :
        std::strong_ordering::equal;
}

template <typename Word, size_t StaticStorage>
template <int S>
bool SExpansion<Word, StaticStorage>::operator==(const SExpansion<Word, S> &rhs) const
{
    return this->Compare(rhs) == 0;
}

template <typename Word, size_t StaticStorage>
std::string SExpansion<Word, StaticStorage>::ToString() const
{
    auto ConvertWord = [](Word v)
    {
        if constexpr(std::is_floating_point_v<Word>)
        {
            return Rtrc::ToString(v);
        }
        else
        {
            return boost::multiprecision::to_string(v);
        }
    };
    auto items = GetItems() | std::views::transform([&](Word v) { return ConvertWord(v); });
    return "[" + Rtrc::Join(" + ", items.begin(), items.end()) + "]";
}

template <typename Word, size_t StaticStorage>
Word *SExpansion<Word, StaticStorage>::GetItemPointer()
{
    if constexpr(SupportDynamicStorage)
    {
        return this->capacity_ > InlinedItemCount ? *reinterpret_cast<Word **>(storage_)
                                                  : reinterpret_cast<Word *>(storage_);
    }
    else
    {
        return reinterpret_cast<Word *>(storage_);
    }
}

template <typename Word, size_t StaticStorage>
const Word *SExpansion<Word, StaticStorage>::GetItemPointer() const
{
    if constexpr(SupportDynamicStorage)
    {
        return this->capacity_ > InlinedItemCount ? *reinterpret_cast<const Word *const *>(storage_)
                                                  : reinterpret_cast<const Word *>(storage_);
    }
    else
    {
        return reinterpret_cast<const Word *>(storage_);
    }
}

template<typename Word, size_t StaticStorage>
std::ostream &operator<<(std::ostream &stream, const SExpansion<Word, StaticStorage> &e)
{
    return stream << e.ToString();
}

RTRC_END
