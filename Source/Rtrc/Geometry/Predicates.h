#pragma once

#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

// Reference:
//    Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates
//    www.cs.cmu.edu/afs/cs/project/quake/public/code/predicates.c

// Returns 1 if pa, pb, pc is counter-clockwise. 0 if colpanar. -1 if clockwise.
int Orient2D(const Expansion2 &pa, const Expansion2 &pb, const Expansion2 &pc);

int Orient2D(const Expansion *pa, const Expansion *pb, const Expansion *pc);

// Orient2D for homogeneous coordinate.
//     v := (vh.x / vh.z, vh.y / vh.z)
int Orient2DHomogeneous(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc);

// Returns 1 if from pd's perspective, pa, pb, pc is clockwise. 0 if all points are coplanar. -1 otherwise.
int Orient3D(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc, const Expansion3 &pd);

// Returns 1 if pd is inside the circle passing through pa, pb, pc. 0 if cocircular. -1 if otherwise.
int InCircle(const Expansion2 &pa, const Expansion2 &pb, const Expansion2 &pc, const Expansion2 &pd);

template<std::floating_point T>
int Orient2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc);

template<std::floating_point T>
int Orient3D(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd);

template<std::floating_point T>
int InCircle(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc, const Vector2<T> &pd);

RTRC_GEO_END
