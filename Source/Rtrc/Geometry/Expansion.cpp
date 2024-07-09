#include <cfenv>
#include <ranges>

#include <ryu/ryu.h>

#include <Rtrc/Core/String.h>
#include <Rtrc/Geometry/Expansion.h>

#if _MSC_VER
#pragma fp_contract(off)
#pragma fenv_access(off)
#pragma float_control(precise, on)
#else
#warning "float_control is not implemented. Expansion arithemetic operations may become unreliable."
#endif

RTRC_GEO_BEGIN

namespace ExpansionDetail
{

    template<typename> struct FloatingPointTypeTrait;

    template<>
    struct FloatingPointTypeTrait<float>
    {
        using EquallySizedUInt = uint32_t;
        static constexpr int TOTAL_BITS = 32;
        static constexpr int SPLIT_BITS = 12;
        static constexpr int EXP_BITS = 8;
        static constexpr int MTS_BITS = 23;
    };

    template<>
    struct FloatingPointTypeTrait<double>
    {
        using EquallySizedUInt = uint64_t;
        static constexpr int TOTAL_BITS = 64;
        static constexpr int SPLIT_BITS = 27;
        static constexpr int EXP_BITS = 11;
        static constexpr int MTS_BITS = 52;
    };

    template<typename F>
    using EquallySizedUInt = typename FloatingPointTypeTrait<F>::EquallySizedUInt;

    template<typename F>
    constexpr int TOTAL_BITS = FloatingPointTypeTrait<F>::TOTAL_BITS;

    template<typename F>
    constexpr int SPLIT_BITS = FloatingPointTypeTrait<F>::SPLIT_BITS;

    template<typename F>
    constexpr int MTS_BITS = FloatingPointTypeTrait<F>::MTS_BITS;

    template<typename F>
    constexpr int EXP_BITS = FloatingPointTypeTrait<F>::EXP_BITS;

    template<typename F>
    uint32_t ExtractExpPart(F f)
    {
        using I = EquallySizedUInt<F>;
        const EquallySizedUInt<F> i = std::bit_cast<I>(f);
        return (i >> MTS_BITS<F>) & ((I(1) << EXP_BITS<F>) - 1);
    }

    template<typename F>
    EquallySizedUInt<F> ExtractMtsPart(F f)
    {
        using I = EquallySizedUInt<F>;
        const EquallySizedUInt<F> i = std::bit_cast<I>(f);
        return i & ((I(1) << MTS_BITS<F>) - 1);
    }

    template<typename F>
    std::pair<int, int> GetBitRange(F f)
    {
        if(f == F(0))
        {
            return { 0, 0 };
        }
        using I = EquallySizedUInt<F>;
        static_assert(sizeof(I) == sizeof(F) && std::is_unsigned_v<I>);
        const I exp = ExtractExpPart(f);
        const I mts = ExtractMtsPart(f);
        const int lsb = (std::min)(std::countr_zero(mts), MTS_BITS<F>);
        const int msb = exp != 0 ? MTS_BITS<F> : (MTS_BITS<F> - 1 - std::countl_zero(mts));
        const int offset = (std::max<int>)(exp, 1) - ((1 << (EXP_BITS<F> - 1)) - 1);
        return { lsb + offset, msb + offset };
    }

    template<typename F>
    bool CheckNonOverlappingPropertyImpl(F af, F bf)
    {
        if(af == F(0) || bf == F(0))
        {
            return true;
        }

        const auto rangeA = GetBitRange(af);
        const auto rangeB = GetBitRange(bf);
        return rangeA.second < rangeB.first || rangeB.second < rangeA.first;
    }

    std::string UIntToBitString(uint64_t i, int bits)
    {
        std::string ret;
        ret.reserve(bits + 1);
        for(int b = 0; b < bits; ++b)
        {
            ret += i & (1 << (bits - 1 - b)) ? "1" : "0";
        }
        return ret;
    }

} // namespace ExpansionDetail

bool CheckRuntimeFloatingPointSettingsForExpansion()
{
    return std::fegetround() == FE_TONEAREST;
}

namespace ExpansionUtility
{

    using namespace ExpansionDetail;

    template<typename F>
    std::string ToBitString(F a, bool splitComponents, bool bitRange)
    {
        using I = EquallySizedUInt<F>;
        const I i = std::bit_cast<I>(a);
        if(!splitComponents)
        {
            if(bitRange)
            {
                const auto range = GetBitRange(a);
                return UIntToBitString(i, TOTAL_BITS<F>) + std::format(" ({}, {})", range.first, range.second);
            }
            return UIntToBitString(i, TOTAL_BITS<F>);
        }
        const std::string signBit = i & (1ull << (TOTAL_BITS<F> - 1)) ? "1" : "0";
        const std::string expBits = UIntToBitString(ExtractExpPart(a), EXP_BITS<F>);
        const std::string mtsBits = UIntToBitString(ExtractMtsPart(a), MTS_BITS<F>);
        if(bitRange)
        {
            const auto range = GetBitRange(a);
            return signBit + " " + expBits + " " + mtsBits + std::format(" ({}, {})", range.first, range.second);
        }
        return signBit + " " + expBits + " " + mtsBits;
    }

    template std::string ToBitString(float, bool, bool);
    template std::string ToBitString(double, bool, bool);

    template<typename F>
    void FastTwoSum(F a, F b, F &x, F &y)
    {
        x = a + b;
        const F bv = x - a;
        y = b - bv;
    }

    template<typename F>
    void TwoSum(F a, F b, F &x, F &y)
    {
        x = a + b;
        const F bv = x - a;
        const F av = x - bv;
        const F br = b - bv;
        const F ar = a - av;
        y = ar + br;
    }

    template<typename F>
    bool CheckNonOverlappingProperty(F af, F bf)
    {
        return ExpansionDetail::CheckNonOverlappingPropertyImpl<F>(af, bf);
    }

    template bool CheckNonOverlappingProperty(float, float);
    template bool CheckNonOverlappingProperty(double, double);

    template<typename F>
    int GrowExpansion(const F *e, int eCount, F b, F *h)
    {
        int ret = 0;

        F Q = b;
        for(int i = 0; i < eCount; ++i)
        {
            F newQ, newH;
            TwoSum(Q, e[i], newQ, newH);
            Q = newQ;
            if(newH != F(0))
            {
                h[ret++] = newH;
            }
        }

        if(Q != F(0) || ret == 0)
        {
            h[ret++] = Q;
        }

        return ret;
    }

    template int GrowExpansion<float>(const float *, int, float b, float *);
    template int GrowExpansion<double>(const double *, int, double b, double *);
    
    template<typename F>
    int ExpansionSum(const F *e, int eCount, const F *f, int fCount, F *h)
    {
        int ret = GrowExpansion<F>(e, eCount, f[0], h);
        for(int i = 1; i < fCount; ++i)
        {
            ret = GrowExpansion<F>(h, ret, f[i], h);
        }
        return ret;
    }

    template int ExpansionSum<float>(const float *, int, const float *, int, float *);
    template int ExpansionSum<double>(const double *, int, const double *, int, double *);

    template<typename F>
    int ExpansionSumNegative(const F *e, int eCount, const F *f, int fCount, F *h)
    {
        int ret = GrowExpansion<F>(e, eCount, -f[0], h);
        for(int i = 1; i < fCount; ++i)
        {
            ret = GrowExpansion<F>(h, ret, -f[i], h);
        }
        return ret;
    }

    template int ExpansionSumNegative<float>(const float *, int, const float *, int, float *);
    template int ExpansionSumNegative<double>(const double *, int, const double *, int, double *);

    template<typename F>
    void Split(F a, int s, F &aLo, F &aHi)
    {
        const F c = F((1ull << s) + 1ull) * a;
        const F aBig = c - a;
        aHi = c - aBig;
        aLo = a - aHi;
    }

    template<typename F>
    void TwoProduct(F a, F b, F &x, F &y)
    {
        x = a * b;

        static_assert(TOTAL_BITS<F> % 2 == 0);
        F aHi, aLo, bHi, bLo;
        Split(a, SPLIT_BITS<F>, aLo, aHi);
        Split(b, SPLIT_BITS<F>, bLo, bHi);

        const F e1 = x  - (aHi * bHi);
        const F e2 = e1 - (aLo * bHi);
        const F e3 = e2 - (aHi * bLo);
        y = (aLo * bLo) - e3;
    }

    template<typename F>
    int ScaleExpansion(const F *e, int eCount, F b, F *h)
    {
        int ret = 0;

        F Q, h0;
        TwoProduct(e[0], b, Q, h0);
        if(h0 != 0)
        {
            h[ret++] = h0;
        }

        for(int i = 1; i < eCount; ++i)
        {
            F Ti, ti;
            TwoProduct(e[i], b, Ti, ti);

            F tQ0, th0;
            TwoSum(Q, ti, tQ0, th0);
            if(th0 != 0)
            {
                h[ret++] = th0;
            }

            F tQ1, th1;
            FastTwoSum(Ti, tQ0, tQ1, th1);
            if(th1 != 0)
            {
                h[ret++] = th1;
            }
            Q = tQ1;
        }

        if(Q != 0 || ret == 0)
        {
            h[ret++] = Q;
        }

        return ret;
    }

    template int ScaleExpansion(const float *, int, float, float *);
    template int ScaleExpansion(const double *, int, double, double *);

    template <typename F>
    int CompressExpansion(const F *e, int eCount, F *h)
    {
        F Q = e[eCount - 1];

        int bottom = eCount - 1;
        for(int i = bottom - 1; i >= 0; --i)
        {
            F q;
            FastTwoSum(Q, e[i], Q, q);
            if(q != 0)
            {
                h[bottom] = Q;
                --bottom;
                Q = q;
            }
        }
        h[bottom] = Q;

        int top = 0;
        for(int i = bottom + 1; i < eCount; ++i)
        {
            F q;
            FastTwoSum(h[i], Q, Q, q);
            if(q != 0)
            {
                h[top] = q;
                ++top;
            }
        }
        h[top] = Q;
        return top + 1;
    }

    template int CompressExpansion(const float *, int, float *);
    template int CompressExpansion(const double *, int, double *);

    template<typename F>
    F ToUnitRoundUp(const F *e, int eCount)
    {
        assert(eCount > 0);
        if(eCount == 1)
        {
            return e[0];
        }
        assert(e[0] != 0 && e[1] != 0);
        if(e[1] < 0)
        {
            return e[0];
        }
        return std::nextafter(e[0], (std::numeric_limits<F>::max)());
    }

    template float ToUnitRoundUp(const float *, int);
    template double ToUnitRoundUp(const double *, int);

    template<typename F>
    F ToUnitRoundDown(const F* e, int eCount)
    {
        assert(eCount > 0);
        if(eCount == 1)
        {
            return e[0];
        }
        assert(e[0] != 0 && e[1] != 0);
        if(e[1] > 0)
        {
            return e[0];
        }
        return std::nextafter(e[0], std::numeric_limits<F>::lowest());
    }

    template float ToUnitRoundDown(const float *, int);
    template double ToUnitRoundDown(const double *, int);

} // namespace ExpansionUtility

RTRC_GEO_END
