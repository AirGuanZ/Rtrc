#include <catch2/catch_all.hpp>

#include <Rtrc/Core/Math/Exact/Expansion.h>
#include <Rtrc/Core/Math/Exact/Vector.h>

using namespace Rtrc;
using namespace ExpansionUtility;

TEST_CASE("CheckNonOverlappingProperty")
{
    // Borrowed from https://www.freemancw.com/2021/06/floating-point-expansions-the-nonoverlapping-property/

    float x = 1.5f; // x = 3 * 2^(-1)
    float y = 2.0f; // y = 2^(1)

    // x and y are initially nonoverlapping because 1.5 < 2.0.
    REQUIRE(CheckNonOverlappingProperty(x, y));

    // The range (2.0f, 0.5f] overlaps 1.5f because x = 3 * 2^(-1) and nothing in the
    // range is < 2^(-1). Since the nonoverlapping property is symmetrical, we note
    // that y will take on values that are of the form r*2^s, but all s values will 
    // be <= -1, and 1.5 > 2^(-1).
    y = std::nextafter(y, 0.0f);
    while(y >= 0.5f)
    {
        REQUIRE_FALSE(CheckNonOverlappingProperty(x, y));
        y = std::nextafter(y, 0.0f);
    }

    // The range (0.5f, 0.125f] does not overlap 1.5f because x = 3 * 2^(-1) and 
    // nothing in the range is > 2^(-1).
    while(y >= 0.125f)
    {
        REQUIRE(CheckNonOverlappingProperty(x, y));
        y = std::nextafter(y, 0.0f);
    }

    // The range (2^(-127), 0.0f) does not overlap 1.5f because x = 3 * 2^(-1) and 
    // nothing in the range is > 2^(-1).
    y = std::nextafter((std::numeric_limits<double>::min)(), 0.0f);
    while(y > 0.0f)
    {
        REQUIRE(CheckNonOverlappingProperty(x, y));
        y = std::nextafter(y, 0.0f);
    }
}

TEST_CASE("Expansion")
{
    REQUIRE(SExpansion(0.5) < SExpansion(0.6));
    REQUIRE(SExpansion(1e30) + SExpansion(1e-30) + SExpansion(1e-30) - SExpansion(1e30) == SExpansion(2.0) * SExpansion(1e-30));
    REQUIRE((SExpansion(1e30) + SExpansion(1e-30) + SExpansion(1e-30) - SExpansion(1e30)).CheckSanity());
    REQUIRE(SExpansion(1e30) + SExpansion(1e-30) + SExpansion(1e-30) - SExpansion(1e30) != SExpansion(1.999999) * SExpansion(1e-30));
    REQUIRE((SExpansion(1e10) + SExpansion(1e-30)) * SExpansion(1e10) == SExpansion(1e10) * SExpansion(1e10) + SExpansion(1e-30) * SExpansion(1e10));

    {
        auto a = SExpansion(1e10);
        auto b = SExpansion(1e-20);
        auto c = SExpansion(1e30);
        auto d = SExpansion(1e-40);
        REQUIRE((a + b) * (c + d) == a * c + a * d + b * c + b * d);
        REQUIRE((a + b) * (c + d) < a * c + a * d + b * c + b * d + SExpansion(1e-200));
        REQUIRE((a + b) * (c + d) > a * c + a * d + b * c + b * d - SExpansion(1e-200));
        REQUIRE((a + b) * (c + d) == SExpansion(0.5) * (a * c + a * d + b * c + b * d + SExpansion(1e-200) + a * c + a * d + b * c + b * d - SExpansion(1e-200)));
    }

    {
        auto a = SExpansion(1e10);
        auto b = SExpansion(1e-20);
        auto c = SExpansion(1e30);
        auto d = SExpansion(1e-40);
        auto m = SExpansion(0.5) * (a * c + a * d + b * c + b * d + SExpansion(1e-200) + a * c + a * d + b * c + b * d - SExpansion(1e-200));
        auto n = m;
        n.Compress();
        REQUIRE(m == n);
    }

    {
        auto a = (SExpansion(1e10) + SExpansion(1e-30)) * SExpansion(1e10);
        auto b = a;
        b.Compress();
        REQUIRE(a == b);
    }
}

TEST_CASE("CompareHomo")
{
    struct CompareHomogeneousPoint
    {
        bool operator()(const Expansion3 &a, const Expansion3 &b) const
        {
            if(const int c0 = (a.x * b.z).Compare(a.z * b.x); c0 != 0)
            {
                return c0 < 0;
            }
            return a.y * b.z < a.z * b.y;
        }

        bool operator()(const Expansion4 &a, const Expansion4 &b) const
        {
            if(const int c0 = (a.x * b.w).Compare(a.w * b.x); c0 != 0)
            {
                return c0 < 0;
            }
            if(const int c1 = (a.y * b.w).Compare(a.w * b.y); c1 != 0)
            {
                return c1 < 0;
            }
            return a.z * b.w < a.w * b.z;
        }
    };

    auto E3 = [](double x, double y, double z)
    {
        return Expansion3(Vector3d(x, y, z));
    };
    auto E4 = [](double x, double y, double z, double w)
    {
        return Expansion4(Vector4d(x, y, z, w));
    };

    REQUIRE_FALSE(CompareHomogeneousPoint{}(E4(0, 0, 0,-1), E4(0, 0, 0, 1)));
    REQUIRE_FALSE(CompareHomogeneousPoint{}(E4(0, 0, 0, 1), E4(0, 0, 0, -1)));
}
