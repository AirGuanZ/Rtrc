#include <catch2/catch_all.hpp>

#include <Rtrc/Core/Math/Angle.h>
#include <Rtrc/Core/Math/Matrix3x3.h>

using namespace Rtrc;

TEST_CASE("Matrix3x3f Rotate")
{
    auto m = Matrix3x3f::RotateZ(PI / 2);
    auto v0 = Vector3f(1, 0, 0);
    auto v1 = m * v0;
    REQUIRE(Catch::Approx(v1[0]).margin(1e-5f) == 0);
    REQUIRE(Catch::Approx(v1[1]).margin(1e-5f) == 1);
    REQUIRE(Catch::Approx(v1[2]).margin(1e-5f) == 0);
}

TEST_CASE("Inverse Matrix3x3f")
{
    auto a = Matrix3x3f(6, 2, 3, 4, 7, 7, 8, 1, 5);
    auto b = a * Inverse(a);
    auto c = Matrix3x3f::Identity();
    for(int y = 0; y < 3; ++y)
    {
        for(int x = 0; x < 3; ++x)
        {
            REQUIRE(Catch::Approx(b[y][x]).margin(1e-5f) == c[y][x]);
        }
    }
}
