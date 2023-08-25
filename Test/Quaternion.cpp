#include <catch2/catch_all.hpp>

#include <Rtrc/Core/Math/Angle.h>
#include <Rtrc/Core/Math/Quaternion.h>

using namespace Rtrc;

TEST_CASE("Quaternion Rotate")
{
    auto m = Quaternion::FromRotationAlongZ(PI / 2);
    auto v0 = Vector3f(1, 0, 0);
    auto v1 = m.ApplyRotation(v0);
    REQUIRE(Catch::Approx(v1[0]).margin(1e-5f) == 0);
    REQUIRE(Catch::Approx(v1[1]).margin(1e-5f) == 1);
    REQUIRE(Catch::Approx(v1[2]).margin(1e-5f) == 0);
}
