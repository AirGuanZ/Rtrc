#pragma once

#include <cassert>
#include <ostream>
#include <ranges>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Core/String.h>

RTRC_GEO_BEGIN

// See Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates

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

    template<typename F>
    F ToUnitRoundUp(const F *e, int eCount);

    template<typename F>
    F ToUnitRoundDown(const F *e, int eCount);

    constexpr size_t MaxInlinedItemCount = 16;

    template<bool HasMember> class CapacityMember;
    template<> class CapacityMember<true> { protected: size_t capacity_ = 0; };
    template<> class CapacityMember<false> { };

    consteval size_t StaticStorageFilter(size_t v) { return v <= MaxInlinedItemCount ? v : 0; }
    consteval size_t StaticStorageAdd(size_t a, size_t b) { return StaticStorageFilter(a && b ? (a + b) : 0); }
    consteval size_t StaticStorageMul(size_t a, size_t b) { return StaticStorageFilter(a && b ? (2 * a * b) : 0); }
    consteval bool StaticStorageGreaterEqual(size_t a, size_t b) { return !a ? true : (!b ? false : (a >= b)); }

} // namespace ExpansionUtility

/* StaticStorage == 0 means supporting dynamic storage */
template<size_t StaticStorage>
class SExpansion : public ExpansionUtility::CapacityMember<StaticStorage == 0>
{
public:

    static constexpr bool SupportDynamicStorage = StaticStorage == 0;
    
    SExpansion();
    SExpansion(double source);

    SExpansion(const SExpansion &other);
    SExpansion(SExpansion &&other) noexcept;

    SExpansion &operator=(const SExpansion &other);
    SExpansion &operator=(SExpansion &&other) noexcept;

    template<size_t StaticStorage2> requires (!StaticStorage || StaticStorage2 <= StaticStorage)
    SExpansion(const SExpansion<StaticStorage2> &other);
    template<size_t StaticStorage2> requires (!StaticStorage || StaticStorage2 <= StaticStorage)
    SExpansion &operator=(const SExpansion<StaticStorage2> &other);

    void Swap(SExpansion &other) noexcept;

    bool CheckSanity() const;

    uint32_t GetLength() const;
    uint32_t GetCapacity() const;
    Span<double> GetItems() const;
    double ToDouble() const;
    double ToDoubleRoundUp() const;
    double ToDoubleRoundDown() const;

    int GetSign() const;

    void Reserve(uint32_t capacity);

    template<size_t SL, size_t SR>
    void SetSum(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs);
    template<size_t SL, size_t SR>
    void SetDiff(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs);
    template<size_t SL, size_t SR>
    void SetMul(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs);
    template<size_t SL>
    void SetMul(const SExpansion<SL> &lhs, double rhs);

    template<size_t S> requires !StaticStorage
    SExpansion &operator+=(const SExpansion<S> &rhs);
    template<size_t S> requires !StaticStorage
    SExpansion &operator-=(const SExpansion<S> &rhs);
    template<size_t S> requires !StaticStorage
    SExpansion &operator*=(const SExpansion<S> &rhs);
    template<typename T>  requires !StaticStorage && (std::is_same_v<T, double> || std::is_same_v<T, float>)
    SExpansion &operator*=(T rhs);

    void Compress();
    void SetNegative();

    template<int S>
    int Compare(const SExpansion<S> &rhs) const;

    SExpansion operator-() const;

    template<int S>
    std::strong_ordering operator<=>(const SExpansion<S> &rhs) const;
    template<int S>
    bool operator==(const SExpansion<S> &rhs) const;

    std::string ToString() const;
    
          double *GetItemPointer();
    const double *GetItemPointer() const;

private:

    static constexpr size_t InlinedItemCount = StaticStorage ? StaticStorage : ExpansionUtility::MaxInlinedItemCount;
    static constexpr size_t StorageSize = (std::max)(InlinedItemCount * sizeof(double), sizeof(void *));
    static_assert(InlinedItemCount <= ExpansionUtility::MaxInlinedItemCount);

    uint32_t size_;
    unsigned char storage_[StorageSize];
};

SExpansion(double)->SExpansion<1>;

using Expansion  = SExpansion<0>;
using Expansion2 = Vector2<Expansion>;
using Expansion3 = Vector3<Expansion>;
using Expansion4 = Vector4<Expansion>;

inline Expansion ToExpansion(double v)
{
    return SExpansion(v);
}

inline Expansion2 ToExpansion(const Vector2d& v)
{
    return { SExpansion(v.x), SExpansion(v.y) };
}

inline Expansion3 ToExpansion(const Vector3d &v)
{
    return { SExpansion(v.x), SExpansion(v.y), SExpansion(v.z) };
}

inline Expansion4 ToExpansion(const Vector4d &v)
{
    return { SExpansion(v.x), SExpansion(v.y), SExpansion(v.z), SExpansion(v.w) };
}

template<size_t SL, size_t SR>
auto operator+(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs)
{
    SExpansion<ExpansionUtility::StaticStorageAdd(SL, SR)> ret;
    ret.SetSum(lhs, rhs);
    return ret;
}

template<size_t SL, size_t SR>
auto operator-(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs)
{
    SExpansion<ExpansionUtility::StaticStorageAdd(SL, SR)> ret;
    ret.SetDiff(lhs, rhs);
    return ret;
}

template<size_t SL, size_t SR>
auto operator*(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs)
{
    SExpansion<ExpansionUtility::StaticStorageMul(SL, SR)> ret;
    ret.SetMul(lhs, rhs);
    return ret;
}

template<size_t SL>
auto operator*(const SExpansion<SL> &lhs, double rhs)
{
    SExpansion<ExpansionUtility::StaticStorageMul(SL, 1)> ret;
    ret.SetMul(lhs, rhs);
    return ret;
}

template<size_t SR>
auto operator*(double lhs, const SExpansion<SR> &rhs)
{
    return rhs * lhs;
}

template <size_t StaticStorage>
SExpansion<StaticStorage>::SExpansion()
{
    assert(CheckRuntimeFloatingPointSettingsForExpansion());
    if constexpr(SupportDynamicStorage)
    {
        this->capacity_ = InlinedItemCount;
    }
    size_ = 1;
    GetItemPointer()[0] = 0.0;
}

template <size_t StaticStorage>
SExpansion<StaticStorage>::SExpansion(double source)
    : SExpansion()
{
    GetItemPointer()[0] = source;
}

template <size_t StaticStorage>
SExpansion<StaticStorage>::SExpansion(const SExpansion &other)
    : SExpansion()
{
    this->Reserve(other.size_);
    size_ = other.size_;
    std::memcpy(GetItemPointer(), other.GetItemPointer(), sizeof(double) * size_);
}

template <size_t StaticStorage>
SExpansion<StaticStorage>::SExpansion(SExpansion &&other) noexcept
    : SExpansion()
{
    this->Swap(other);
}

template <size_t StaticStorage>
SExpansion<StaticStorage> &SExpansion<StaticStorage>::operator=(const SExpansion &other)
{
    SExpansion t(other);
    this->Swap(t);
    return *this;
}

template <size_t StaticStorage>
SExpansion<StaticStorage> &SExpansion<StaticStorage>::operator=(SExpansion &&other) noexcept
{
    this->Swap(other);
    return *this;
}

template <size_t StaticStorage>
template <size_t StaticStorage2> requires (!StaticStorage || StaticStorage2 <= StaticStorage)
SExpansion<StaticStorage>::SExpansion(const SExpansion<StaticStorage2> &other)
    : SExpansion()
{
    Reserve(other.GetLength());
    size_ = other.GetLength();
    std::memcpy(GetItemPointer(), other.GetItemPointer(), sizeof(double) * size_);
}

template <size_t StaticStorage>
template <size_t StaticStorage2> requires (!StaticStorage || StaticStorage2 <= StaticStorage)
SExpansion<StaticStorage> &SExpansion<StaticStorage>::operator=(const SExpansion<StaticStorage2> &other)
{
    SExpansion t(other);
    this->Swap(t);
    return *this;
}

template <size_t StaticStorage>
void SExpansion<StaticStorage>::Swap(SExpansion &other) noexcept
{
    if constexpr(SupportDynamicStorage)
    {
        std::swap(this->capacity_, other.capacity_);
    }
    std::swap(size_, other.size_);
    std::swap(storage_, other.storage_);
}

template <size_t StaticStorage>
bool SExpansion<StaticStorage>::CheckSanity() const
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

template <size_t StaticStorage>
uint32_t SExpansion<StaticStorage>::GetLength() const
{
    return size_;
}

template <size_t StaticStorage>
uint32_t SExpansion<StaticStorage>::GetCapacity() const
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

template <size_t StaticStorage>
Span<double> SExpansion<StaticStorage>::GetItems() const
{
    return Span<double>(GetItemPointer(), size_);
}

template <size_t StaticStorage>
double SExpansion<StaticStorage>::ToDouble() const
{
    return *std::ranges::fold_left_first(GetItems(), std::plus<>{});
}

template <size_t StaticStorage>
double SExpansion<StaticStorage>::ToDoubleRoundUp() const
{
    return ExpansionUtility::ToUnitRoundUp(GetItemPointer(), size_);
}

template <size_t StaticStorage>
double SExpansion<StaticStorage>::ToDoubleRoundDown() const
{
    return ExpansionUtility::ToUnitRoundDown(GetItemPointer(), size_);
}

template <size_t StaticStorage>
int SExpansion<StaticStorage>::GetSign() const
{
    const double word = GetItems().last();
    return word < 0 ? -1 : (word > 0 ? 1 : 0);
}

template <size_t StaticStorage>
void SExpansion<StaticStorage>::Reserve(uint32_t capacity)
{
    if constexpr(SupportDynamicStorage)
    {
        capacity = (std::max<uint32_t>)(capacity, InlinedItemCount);
        if(this->capacity_ >= capacity)
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

template <size_t StaticStorage>
template <size_t SL, size_t SR>
void SExpansion<StaticStorage>::SetSum(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs)
{
    static_assert(ExpansionUtility::StaticStorageGreaterEqual(
        StaticStorage, ExpansionUtility::StaticStorageAdd(SL, SR)));
    Reserve(lhs.GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSum(
        lhs.GetItemPointer(), lhs.GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
}

template <size_t StaticStorage>
template <size_t SL, size_t SR>
void SExpansion<StaticStorage>::SetDiff(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs)
{
    static_assert(ExpansionUtility::StaticStorageGreaterEqual(
        StaticStorage, ExpansionUtility::StaticStorageAdd(SL, SR)));

    Reserve(lhs.GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSumNegative(
        lhs.GetItemPointer(), lhs.GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
}

template <size_t StaticStorage>
template <size_t SL, size_t SR>
void SExpansion<StaticStorage>::SetMul(const SExpansion<SL> &lhs, const SExpansion<SR> &rhs)
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
}

template <size_t StaticStorage>
template <size_t SL>
void SExpansion<StaticStorage>::SetMul(const SExpansion<SL> &lhs, double rhs)
{
    static_assert(ExpansionUtility::StaticStorageGreaterEqual(
        StaticStorage, ExpansionUtility::StaticStorageMul(SL, 1)));

    Reserve(lhs.GetLength() * 2);
    size_ = ExpansionUtility::ScaleExpansion(lhs.GetItemPointer(), lhs.GetLength(), rhs, GetItemPointer());
}

template <size_t StaticStorage>
template <size_t S> requires !StaticStorage
SExpansion<StaticStorage> &SExpansion<StaticStorage>::operator+=(const SExpansion<S> &rhs)
{
    Reserve(GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSum(
        GetItemPointer(), GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
    return *this;
}

template <size_t StaticStorage>
template <size_t S> requires !StaticStorage
SExpansion<StaticStorage> &SExpansion<StaticStorage>::operator-=(const SExpansion<S> &rhs)
{
    Reserve(GetLength() + rhs.GetLength());
    size_ = ExpansionUtility::ExpansionSumNegative(
        GetItemPointer(), GetLength(), rhs.GetItemPointer(), rhs.GetLength(), GetItemPointer());
    return *this;
}

template <size_t StaticStorage>
template <size_t S> requires !StaticStorage
SExpansion<StaticStorage> &SExpansion<StaticStorage>::operator*=(const SExpansion<S> &rhs)
{
    *this = *this * rhs;
    return *this;
}

template <size_t StaticStorage>
template <typename T> requires !StaticStorage && (std::is_same_v<T, double> || std::is_same_v<T, float>)
SExpansion<StaticStorage> &SExpansion<StaticStorage>::operator*=(T rhs)
{
    Reserve(GetLength() * 2);
    size_ = ExpansionUtility::ScaleExpansion(GetItemPointer(), GetLength(), rhs, GetItemPointer());
    return *this;
}

template <size_t StaticStorage>
void SExpansion<StaticStorage>::Compress()
{
    const uint32_t newSize = ExpansionUtility::CompressExpansion(GetItemPointer(), size_, GetItemPointer());
    assert(newSize <= size_);
    size_ = newSize;
}

template <size_t StaticStorage>
void SExpansion<StaticStorage>::SetNegative()
{
    auto items = GetItemPointer();
    for(uint32_t i = 0; i < size_; ++i)
    {
        items[i] = -items[i];
    }
}

template <size_t StaticStorage>
template <int S>
int SExpansion<StaticStorage>::Compare(const SExpansion<S> &rhs) const
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

template <size_t StaticStorage>
SExpansion<StaticStorage> SExpansion<StaticStorage>::operator-() const
{
    SExpansion ret(*this);
    ret.SetNegative();
    return ret;
}

template <size_t StaticStorage>
template <int S>
std::strong_ordering SExpansion<StaticStorage>::operator<=>(const SExpansion<S> &rhs) const
{
    const int compare = this->Compare(rhs);
    return compare < 0 ? std::strong_ordering::less :
        compare > 0 ? std::strong_ordering::greater :
        std::strong_ordering::equal;
}

template <size_t StaticStorage>
template <int S>
bool SExpansion<StaticStorage>::operator==(const SExpansion<S> &rhs) const
{
    return this->Compare(rhs) == 0;
}

template <size_t StaticStorage>
std::string SExpansion<StaticStorage>::ToString() const
{
    auto items = GetItems() | std::views::transform([](double v) { return Rtrc::ToString(v); });
    return "[" + Rtrc::Join(" + ", items.begin(), items.end()) + "]";
}

template <size_t StaticStorage>
double *SExpansion<StaticStorage>::GetItemPointer()
{
    if constexpr(SupportDynamicStorage)
    {
        return this->capacity_ > InlinedItemCount ? *reinterpret_cast<double **>(storage_)
                                                  : reinterpret_cast<double*>(storage_);
    }
    else
    {
        return reinterpret_cast<double *>(storage_);
    }
}

template <size_t StaticStorage>
const double *SExpansion<StaticStorage>::GetItemPointer() const
{
    if constexpr(SupportDynamicStorage)
    {
        return this->capacity_ > InlinedItemCount ? *reinterpret_cast<const double *const *>(storage_)
                                                  : reinterpret_cast<const double *>(storage_);
    }
    else
    {
        return reinterpret_cast<const double *>(storage_);
    }
}

template<size_t StaticStorage>
std::ostream &operator<<(std::ostream &stream, const SExpansion<StaticStorage> &e)
{
    return stream << e.ToString();
}

RTRC_GEO_END
