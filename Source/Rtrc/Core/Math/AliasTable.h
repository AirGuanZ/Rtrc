#pragma once

#include <cassert>
#include <numeric>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<typename F, typename T = int>
class AliasTable
{
    static_assert(std::is_floating_point_v<F> && std::is_integral_v<T>);

public:

    struct TableUnit
    {
        F acceptProb;
        T anotherIndex;
    };

    using self_t = AliasTable<F, T>;

    AliasTable() = default;

    AliasTable(const F *prob, T n);

    AliasTable(self_t &&moveFrom) noexcept = default;

    AliasTable(const self_t &copyFrom) = default;

    self_t &operator=(self_t &&moveFrom) noexcept = default;

    self_t &operator=(const self_t &copyFrom) = default;

    void Initialize(const F *prob, T n);

    bool IsAvailable() const noexcept;

    void Destroy();

    const std::vector<TableUnit> &GetTable() const;

    T Sample(F u) const noexcept;

    T Sample(F u1, F u2) const noexcept;

private:

    std::vector<TableUnit> table_;
};

template<typename F, typename T>
AliasTable<F, T>::AliasTable(const F *prob, T n)
{
    this->Initialize(prob, n);
}

template<typename F, typename T>
void AliasTable<F, T>::Initialize(const F *prob, T n)
{
    // https://en.wikipedia.org/wiki/Alias_method

    assert(n > 0);

    const F sum = std::accumulate(prob, prob + n, F(0));
    const F ratio = n / sum;

    std::vector<T> overs;
    std::vector<T> unders;

    table_.clear();
    table_.resize(n);

    for(T i = 0; i < n; ++i)
    {
        const F p = prob[i] * ratio;

        table_[i].acceptProb = p;
        table_[i].anotherIndex = i;

        if(p > 1)
        {
            overs.push_back(i);
        }
        else if(p < 1)
        {
            unders.push_back(i);
        }
    }

    while(!overs.empty() && !unders.empty())
    {
        const T over  = overs.back();
        const T under = unders.back();
        overs.pop_back();
        unders.pop_back();

        table_[over].acceptProb -= 1 - table_[under].acceptProb;
        table_[under].anotherIndex = over;

        if(table_[over].acceptProb > 1)
        {
            overs.push_back(over);
        }
        else if(table_[over].acceptProb < 1)
        {
            unders.push_back(over);
        }
    }

    for(auto i : overs)
    {
        table_[i].acceptProb = 1;
    }
    for(auto i : unders)
    {
        table_[i].acceptProb = 1;
    }
}

template<typename F, typename T>
void AliasTable<F, T>::Destroy()
{
    table_.clear();
}

template<typename F, typename T>
const std::vector<typename AliasTable<F, T>::TableUnit> &
    AliasTable<F, T>::GetTable() const
{
    return table_;
}

template<typename F, typename T>
bool AliasTable<F, T>::IsAvailable() const noexcept
{
    return !table_.empty();
}

template<typename F, typename T>
T AliasTable<F, T>::Sample(F u) const noexcept
{
    assert(IsAvailable());
    assert(0 <= u && u <= 1);

    const T n = static_cast<T>(table_.size());
    const F nu = n * u;

    const T i = (std::min)(static_cast<T>(nu), n - 1);
    const F s = nu - i;

    if(s <= table_[i].acceptProb)
    {
        return i;
    }
    return table_[i].anotherIndex;
}

template<typename F, typename T>
T AliasTable<F, T>::Sample(F u1, F u2) const noexcept
{
    assert(IsAvailable());
    assert(0 <= u1 && u1 <= 1);
    assert(0 <= u2 && u2 <= 1);

    const T n = static_cast<T>(table_.size());
    const F nu = n * u1;

    const T i = (std::min)(static_cast<T>(nu), n - 1);

    if(u2 <= table_[i].acceptProb)
    {
        return i;
    }
    return table_[i].anotherIndex;
}

RTRC_END
