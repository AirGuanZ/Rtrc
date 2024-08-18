#pragma once

#include <Rtrc/Core/Math/Exact/Vector.h>

RTRC_BEGIN

// Reference:
//    Adaptive Precision Floating-Point Arithmetic and Fast Robust Geometric Predicates
//    www.cs.cmu.edu/afs/cs/project/quake/public/code/predicates.c

// Orient2D: returns 1 if pa, pb, pc is counter-clockwise. 0 if colpanar. -1 if clockwise.
// Orient3D: returns 1 if from pd's perspective, pa, pb, pc is clockwise. 0 if all points are coplanar. -1 otherwise.
// InCicle2D: returns 1 if pd is outside the circle passing through pa, pb, pc. 0 if cocircular. -1 if otherwise.

int Orient2D(const Expansion2 &pa, const Expansion2 &pb, const Expansion2 &pc);
int Orient2D(const Expansion *pa, const Expansion *pb, const Expansion *pc);
int Orient2DHomogeneous(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc);

int Orient3D(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc, const Expansion3 &pd);

int InCircle2D(const Expansion2 &pa, const Expansion2 &pb, const Expansion2 &pc, const Expansion2 &pd);
int InCircle2D(const Expansion *pa, const Expansion *pb, const Expansion *pc, const Expansion *pd);
int InCircle2DHomogeneous(const Expansion3 &pa, const Expansion3 &pb, const Expansion3 &pc, const Expansion3 &pd);

template<typename T>
int Orient2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc);
template<typename T>
int Orient3D(const Vector3<T> &pa, const Vector3<T> &pb, const Vector3<T> &pc, const Vector3<T> &pd);
template<typename T>
int InCircle2D(const Vector2<T> &pa, const Vector2<T> &pb, const Vector2<T> &pc, const Vector2<T> &pd);

template<typename T>
bool AreCoLinear(const Vector3<T> &a, const Vector3<T> &b, const Vector3<T> &c);

RTRC_END
