#pragma once

#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

// See Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates
template<typename T>
int Orient2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc);
template<typename T>
int Orient3D(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd);

RTRC_GEO_END
