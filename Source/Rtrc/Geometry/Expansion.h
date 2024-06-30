#pragma once

#include <cassert>

#include <Rtrc/Core/Container/Span.h>

RTRC_GEO_BEGIN

bool CheckRuntimeFloatingPointSettingsForExpansion();

namespace ExpansionUtility
{

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

} // namespace ExpansionUtility

// See Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates
class Expansion
{
public:

    struct NoInitFlag { };
    static constexpr NoInitFlag NO_INIT;

    Expansion();

    explicit Expansion(NoInitFlag);
    explicit Expansion(double source);

    Expansion(const Expansion &other);
    Expansion(Expansion &&other) noexcept;

    Expansion &operator=(const Expansion &other);
    Expansion &operator=(Expansion &&other) noexcept;

    void Swap(Expansion &other) noexcept;

    bool CheckSanity() const;

    uint32_t GetLength() const;
    uint32_t GetCapacity() const;
    Span<double> GetItems() const;
    double ToDouble() const;

    int GetSign() const;

    void Reserve(uint32_t capacity);
    void Shrink();
    void ShrinkToInlineIfPossible();

    void SetSum(const Expansion &lhs, const Expansion &rhs);
    void SetDiff(const Expansion &lhs, const Expansion &rhs);
    void SetMul(const Expansion &lhs, const Expansion &rhs);
    void SetMul(const Expansion &lhs, double rhs);

    void Compress();
    void SetNegative();
    int Compare(const Expansion &rhs) const;

    Expansion operator-() const;

    Expansion &operator+=(const Expansion &rhs);
    Expansion &operator-=(const Expansion &rhs);
    Expansion &operator*=(const Expansion &rhs);
    Expansion &operator*=(double rhs);

    std::strong_ordering operator<=>(const Expansion &rhs) const;
    bool operator==(const Expansion &rhs) const;

    std::string ToString() const;

private:
    
          double *GetItemPointer();
    const double *GetItemPointer() const;

    static constexpr size_t InlinedItemCount = 32;
    static constexpr size_t StorageSize = (std::max)(InlinedItemCount * sizeof(double), sizeof(void *));

    uint32_t capacity_;
    uint32_t size_;
    unsigned char storage_[StorageSize];
};

Expansion operator+(const Expansion &lhs, const Expansion &rhs);
Expansion operator-(const Expansion &lhs, const Expansion &rhs);
Expansion operator*(const Expansion &lhs, const Expansion &rhs);
Expansion operator*(const Expansion &lhs, double rhs);
Expansion operator*(double lhs, const Expansion &rhs);

std::ostream &operator<<(std::ostream &stream, const Expansion &e);

inline Expansion::Expansion()
    : Expansion(0.0)
{

}

inline Expansion::Expansion(NoInitFlag)
    : capacity_(InlinedItemCount)
{
#if RTRC_INTELLISENSE
    size_ = 0;
    std::memset(storage_, 0, sizeof(storage_));
#endif
}

inline Expansion::Expansion(double source)
    : capacity_(InlinedItemCount), size_(1)
{
    assert(CheckRuntimeFloatingPointSettingsForExpansion());
    GetItemPointer()[0] = source;
}

inline Expansion::Expansion(Expansion &&other) noexcept
    : Expansion()
{
    Swap(other);
}

inline Expansion::Expansion(const Expansion &other)
    : Expansion(NO_INIT)
{
    Reserve(other.GetLength());
    size_ = other.GetLength();
    std::memcpy(GetItemPointer(), other.GetItemPointer(), sizeof(double) * size_);
}

inline Expansion &Expansion::operator=(Expansion &&other) noexcept
{
    Swap(other);
    return *this;
}

inline Expansion &Expansion::operator=(const Expansion &other)
{
    Expansion t(other);
    Swap(t);
    return *this;
}

inline void Expansion::Swap(Expansion &other) noexcept
{
    std::swap(capacity_, other.capacity_);
    std::swap(size_, other.size_);
    std::swap(storage_, other.storage_);
}

inline bool Expansion::CheckSanity() const
{
    if(!size_)
    {
        return false;
    }
    auto items = GetItemPointer();
    for(uint32_t i = 1; i < size_; ++i)
    {
        if(std::abs(items[i - 1]) >= std::abs(items[i]))
        {
            return false;
        }
        if(!ExpansionUtility::CheckNonOverlappingProperty(items[i - 1], items[i]))
        {
            return false;
        }
    }
    return true;
}

inline uint32_t Expansion::GetLength() const
{
    return size_;
}

inline uint32_t Expansion::GetCapacity() const
{
    return capacity_;
}

inline Span<double> Expansion::GetItems() const
{
    return Span<double>(GetItemPointer(), GetLength());
}

inline double Expansion::ToDouble() const
{
    return *std::ranges::fold_left_first(GetItems(), std::plus<>{});
}

inline int Expansion::GetSign() const
{
    assert(size_);
    const double word = GetItemPointer()[size_ - 1];
    return word < 0 ? -1 : (word > 0 ? 1 : 0);
}

inline void Expansion::Reserve(uint32_t capacity)
{
    capacity = (std::max<uint32_t>)(capacity, InlinedItemCount);
    if(capacity_ >= capacity)
    {
        return;
    }

    void *newPointer = std::malloc(sizeof(double) * capacity);
    if(newPointer)
    {
        throw std::bad_alloc();
    }

    void *oldPointer = GetItemPointer();
    if(size_)
    {
        std::memcpy(newPointer, oldPointer, size_ * sizeof(double));
    }
    if(capacity_ > InlinedItemCount)
    {
        std::free(oldPointer);
    }

    *reinterpret_cast<void **>(storage_) = newPointer;
    capacity_ = capacity;
}

inline void Expansion::Shrink()
{
    assert(capacity_ >= size_ && capacity_ >= InlinedItemCount);
    if(capacity_ == size_ || capacity_ == InlinedItemCount)
    {
        return;
    }

    assert(size_);
    void *newPointer = std::malloc(sizeof(double) * size_);
    if(newPointer)
    {
        // Fail to shrink is not fatal. Don't throw bad_alloc.
        return;
    }

    void *oldPointer = GetItemPointer();
    std::memcpy(newPointer, oldPointer, size_ * sizeof(double));
    if(capacity_ > InlinedItemCount)
    {
        std::free(oldPointer);
    }

    *reinterpret_cast<void **>(storage_) = newPointer;
    capacity_ = size_;
}

inline void Expansion::ShrinkToInlineIfPossible()
{
    assert(capacity_ >= size_ && capacity_ >= InlinedItemCount);
    if(size_ > InlinedItemCount || capacity_ == InlinedItemCount)
    {
        return;
    }

    void *oldPointer = GetItemPointer();
    std::memcpy(storage_, oldPointer, size_ * sizeof(double));
    std::free(oldPointer);
    capacity_ = InlinedItemCount;
}

inline void Expansion::SetSum(const Expansion &lhs, const Expansion &rhs)
{
    Reserve(lhs.GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSum(
        lhs.GetItemPointer(), lhs.GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
}

inline void Expansion::SetDiff(const Expansion &lhs, const Expansion &rhs)
{
    Reserve(lhs.GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSumNegative(
        lhs.GetItemPointer(), lhs.GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
}

inline void Expansion::SetMul(const Expansion &lhs, const Expansion &rhs)
{
    size_ = 1;
    GetItemPointer()[0] = 0.0;

    for(uint32_t i = 0; i < rhs.GetLength(); ++i)
    {
        *this += lhs * rhs.GetItemPointer()[i];
    }
}

inline void Expansion::SetMul(const Expansion &lhs, double rhs)
{
    Reserve(lhs.GetLength() * 2);
    size_ = ExpansionUtility::ScaleExpansion(lhs.GetItemPointer(), lhs.GetLength(), rhs, GetItemPointer());
}

inline void Expansion::Compress()
{
    const uint32_t newSize = ExpansionUtility::CompressExpansion(GetItemPointer(), size_, GetItemPointer());
    assert(newSize <= size_);
    size_ = newSize;
}

inline void Expansion::SetNegative()
{
    auto items = GetItemPointer();
    for(uint32_t i = 0; i < size_; ++i)
    {
        items[i] = -items[i];
    }
}

inline int Expansion::Compare(const Expansion &rhs) const
{
    const Expansion &lhs = *this;
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
    if(maxDiffSize > 256)
    {
        return (lhs - rhs).GetSign();
    }

    double diff[256];
    const int diffSize = ExpansionUtility::ExpansionSumNegative(
        lhs.GetItemPointer(), lhs.GetLength(), rhs.GetItemPointer(), rhs.GetLength(), diff);
    assert(diffSize);
    const double word = diff[diffSize - 1];
    return word < 0 ? -1 : (word > 0 ? 1 : 0);
}

inline Expansion Expansion::operator-() const
{
    Expansion ret(*this);
    ret.SetNegative();
    return ret;
}

inline Expansion &Expansion::operator+=(const Expansion &rhs)
{
    Reserve(GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSum(
        GetItemPointer(), GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
    return *this;
}

inline Expansion &Expansion::operator-=(const Expansion &rhs)
{
    Reserve(GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSumNegative(
        GetItemPointer(), GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
    return *this;
}

inline Expansion &Expansion::operator*=(const Expansion &rhs)
{
    *this = *this * rhs;
    return *this;
}

inline Expansion &Expansion::operator*=(double rhs)
{
    Reserve(GetLength() * 2);
    size_ = ExpansionUtility::ScaleExpansion(GetItemPointer(), GetLength(), rhs, GetItemPointer());
    return *this;
}

inline std::strong_ordering Expansion::operator<=>(const Expansion &rhs) const
{
    const int compare = Compare(rhs);
    return compare < 0 ? std::strong_ordering::less :
           compare > 0 ? std::strong_ordering::greater :
                         std::strong_ordering::equal;
}

inline bool Expansion::operator==(const Expansion &rhs) const
{
    return Compare(rhs) == 0;
}

inline double *Expansion::GetItemPointer()
{
    return capacity_ > InlinedItemCount ? *reinterpret_cast<double **>(storage_)
                                        : reinterpret_cast<double*>(storage_);
}

inline const double *Expansion::GetItemPointer() const
{
    return capacity_ > InlinedItemCount ? *reinterpret_cast<const double * const*>(storage_)
                                        : reinterpret_cast<const double*>(storage_);
}

inline Expansion operator+(const Expansion &lhs, const Expansion &rhs)
{
    Expansion ret;
    ret.SetSum(lhs, rhs);
    return ret;
}

inline Expansion operator-(const Expansion &lhs, const Expansion &rhs)
{
    Expansion ret;
    ret.SetDiff(lhs, rhs);
    return ret;
}

inline Expansion operator*(const Expansion &lhs, const Expansion &rhs)
{
    Expansion ret;
    ret.SetMul(lhs, rhs);
    return ret;
}

inline Expansion operator*(const Expansion &lhs, double rhs)
{
    Expansion ret;
    ret.SetMul(lhs, rhs);
    return ret;
}

inline Expansion operator*(double lhs, const Expansion &rhs)
{
    Expansion ret;
    ret.SetMul(rhs, lhs);
    return ret;
}

inline std::ostream &operator<<(std::ostream &stream, const Expansion &e)
{
    return stream << e.ToString();
}

RTRC_GEO_END
