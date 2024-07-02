#pragma once

#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

// Reference:
//    Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates

// Returns 1 if pa, pb, pc is counter-clockwise. 0 if colpanar. -1 if clockwise
template<typename T>
int Orient2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc);

// Returns 1 if from pd's perspective, pa, pb, pc is clockwise. 0 if all points are coplanar. -1 otherwise
template<typename T>
int Orient3D(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd);

RTRC_GEO_END
