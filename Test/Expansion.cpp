#include <catch2/catch_all.hpp>

#include <Rtrc/Geometry/Expansion.h>

using namespace Rtrc;
using namespace Geo;
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
    using E = Expansion;

    REQUIRE(E(0.5) < E(0.6));
    REQUIRE(E(1e30) + E(1e-30) + E(1e-30) - E(1e30) == E(2.0) * E(1e-30));
    REQUIRE((E(1e30) + E(1e-30) + E(1e-30) - E(1e30)).CheckSanity());
    REQUIRE(E(1e30) + E(1e-30) + E(1e-30) - E(1e30) != E(1.999999) * E(1e-30));
    REQUIRE((E(1e10) + E(1e-30)) * E(1e10) == E(1e10) * E(1e10) + E(1e-30) * E(1e10));

    {
        auto a = E(1e10);
        auto b = E(1e-20);
        auto c = E(1e30);
        auto d = E(1e-40);
        REQUIRE((a + b) * (c + d) == a * c + a * d + b * c + b * d);
        REQUIRE((a + b) * (c + d) < a * c + a * d + b * c + b * d + E(1e-200));
        REQUIRE((a + b) * (c + d) > a * c + a * d + b * c + b * d - E(1e-200));
        REQUIRE((a + b) * (c + d) == E(0.5) * (a * c + a * d + b * c + b * d + E(1e-200) + a * c + a * d + b * c + b * d - E(1e-200)));
    }

    {
        auto a = E(1e10);
        auto b = E(1e-20);
        auto c = E(1e30);
        auto d = E(1e-40);
        auto m = E(0.5) * (a * c + a * d + b * c + b * d + E(1e-200) + a * c + a * d + b * c + b * d - E(1e-200));
        auto n = m;
        n.Compress();
        REQUIRE(m == n);
    }

    {
        auto a = (E(1e10) + E(1e-30)) * E(1e10);
        auto b = a;
        b.Compress();
        REQUIRE(a == b);
    }
}
