#pragma once

#include <numbers>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<std::floating_point T>
constexpr T Deg2Rad(T deg) { return deg * (std::numbers::pi_v<T> / 180.0f); }

template<std::floating_point T>
constexpr T Rad2Deg(T rad) { return rad * (180.0f / std::numbers::pi_v<T>); }

RTRC_END
