#include <Rtrc/Core/Math/Exact/Intersection.h>

RTRC_BEGIN

namespace
{
    Expansion3 HomoAdd(const Expansion3 &a, const Expansion3 &b)
    {
        return
        {
            a.x * b.z + b.x * a.z,
            a.y * b.z + b.y * a.z,
            a.z * b.z
        };
    }

    Expansion3 HomoSub(const Expansion3 &a, const Expansion3 &b)
    {
        return
        {
            a.x * b.z - b.x * a.z,
            a.y * b.z - b.y * a.z,
            a.z * b.z
        };
    }

    Expansion4 HomoAdd(const Expansion4 &a, const Expansion4 &b)
    {
        return
        {
            a.x * b.w + b.x * a.w,
            a.y * b.w + b.y * a.w,
            a.z * b.w + b.z * a.w,
            a.w * b.w
        };
    }

    Expansion4 HomoSub(const Expansion4 &a, const Expansion4 &b)
    {
        return
        {
            a.x * b.w - b.x * a.w,
            a.y * b.w - b.y * a.w,
            a.z * b.w - b.z * a.w,
            a.w * b.w
        };
    }
}

int FindMaximalNormalAxis(const Vector3f &a, const Vector3f &b, const Vector3f &c)
{
    return FindMaximalNormalAxis(a.To<double>(), b.To<double>(), c.To<double>());
}

int FindMaximalNormalAxis(const Vector3d &a, const Vector3d &b, const Vector3d &c)
{
    const auto ax = SExpansion(a.x);
    const auto ay = SExpansion(a.y);
    const auto az = SExpansion(a.z);
    const auto ux = SExpansion(b.x) - ax;
    const auto uy = SExpansion(b.y) - ay;
    const auto uz = SExpansion(b.z) - az;
    const auto vx = SExpansion(c.x) - ax;
    const auto vy = SExpansion(c.y) - ay;
    const auto vz = SExpansion(c.z) - az;

    auto nx = uy * vz - uz * vy;
    auto ny = uz * vx - ux * vz;
    auto nz = ux * vy - uy * vx;
    if(nx.GetSign() < 0) { nx.SetNegative(); }
    if(ny.GetSign() < 0) { ny.SetNegative(); }
    if(nz.GetSign() < 0) { nz.SetNegative(); }

    const decltype(nx) *best = &nx;
    int result = 0;
    if(ny > *best)
    {
        best = &ny;
        result = 1;
    }
    if(nz > *best)
    {
        result = 2;
    }
    return result;
}

int FindMaximalNormalAxis(const Expansion4 &a, const Expansion4 &b, const Expansion4 &c)
{
    const Expansion4 u = HomoSub(b, a);
    const Expansion4 v = HomoSub(c, a);

    // The divisors for nx, ny, and nz are the same, so they can be ignored when comparing their absolute values.
    auto nx = u.y * v.z - u.z * v.y;
    auto ny = u.z * v.x - u.x * v.z;
    auto nz = u.x * v.y - u.y * v.x;
    if(nx.GetSign() < 0) { nx.SetNegative(); }
    if(ny.GetSign() < 0) { ny.SetNegative(); }
    if(nz.GetSign() < 0) { nz.SetNegative(); }

    const decltype(nx) *best = &nx;
    int result = 0;
    if(ny > *best)
    {
        best = &ny;
        result = 1;
    }
    if(nz > *best)
    {
        result = 2;
    }
    return result;
}

Expansion3 IntersectLine2D(const Expansion3 &a, const Expansion3 &b, const Expansion3 &c, const Expansion3 &d)
{
    const Expansion3 A = HomoSub(b, a);
    const Expansion3 B = HomoSub(c, d);
    const Expansion3 C = HomoSub(c, a);
    const Expansion2 det = Expansion2(A.x * B.y - B.x * A.y, A.z * B.z);
    const Expansion2 det1 = Expansion2(C.x * B.y - B.x * C.y, C.z * B.z);
    const Expansion2 t = Expansion2(det1.x * det.y, det1.y * det.x);
    return HomoAdd(a, Expansion3(A.x * t.x, A.y * t.x, A.z * t.y));
}

Expansion4 IntersectLine3D(const Vector3d &a3d, const Vector3d &b3d, const Vector3d &c3d, const Vector3d &d3d, int projectAxis)
{
    // Project to plane. Must ensure the projected lines don't become degenerate.

    if(projectAxis < 0)
    {
        projectAxis = FindMaximalNormalAxis(a3d, b3d, c3d);
    }
    auto Project = [&](const Vector3d &v)
    {
        return Vector2d(v[(projectAxis + 1) % 3], v[(projectAxis + 2) % 3]);
    };
    const Vector2d a = Project(a3d);
    const Vector2d b = Project(b3d);
    const Vector2d c = Project(c3d);
    const Vector2d d = Project(d3d);

    // Find t such that inct = a + (b - a) * t. By Cramer's rule, t = det1 / det.

    auto E = [](double v) { return SExpansion<Expansion::WordType, 1>(v); };

    const auto Ax = E(b.x) - E(a.x);
    const auto Ay = E(b.y) - E(a.y);
    const auto Bx = E(c.x) - E(d.x);
    const auto By = E(c.y) - E(d.y);
    const auto Cx = E(c.x) - E(a.x);
    const auto Cy = E(c.y) - E(a.y);
    const auto det = Ax * By - Bx * Ay;
    const auto det1 = Cx * By - Bx * Cy;

    // [dxu, dyu, dzu] / det = (b - a) * t

    const auto dxu = (E(b3d.x) - E(a3d.x)) * det1;
    const auto dyu = (E(b3d.y) - E(a3d.y)) * det1;
    const auto dzu = (E(b3d.z) - E(a3d.z)) * det1;
    
    return {
        Expansion(E(a3d.x) * det + dxu),
        Expansion(E(a3d.y) * det + dyu),
        Expansion(E(a3d.z) * det + dzu),
        Expansion(det)
    };
}

Expansion4 IntersectLine3D(const Expansion4 &a3d, const Expansion4 &b3d, const Expansion4 &c3d, const Expansion4 &d3d, int projectAxis)
{
    // Project to plane. Must ensure the projected lines don't become degenerate.

    if(projectAxis < 0)
    {
        projectAxis = FindMaximalNormalAxis(a3d, b3d, c3d);
    }
    auto Project = [&](const Expansion4 &v)
    {
        return Expansion3(v[(projectAxis + 1) % 3], v[(projectAxis + 2) % 3], v.w);
    };
    const Expansion3 a = Project(a3d);
    const Expansion3 b = Project(b3d);
    const Expansion3 c = Project(c3d);
    const Expansion3 d = Project(d3d);

    // Find t such that inct = a + (b - a) * t. By Cramer's rule, t = det1 / det.

    const auto A = HomoSub(b, a);
    const auto B = HomoSub(c, d);
    const auto C = HomoSub(c, a);
    const auto td = (A.x * B.y - B.x * A.y) * C.z;
    const auto tu = (C.x * B.y - B.x * C.y) * A.z;

    const auto ab = HomoSub(b3d, a3d);
    return HomoAdd(a3d, Expansion4(ab.x * tu, ab.y * tu, ab.z * tu, ab.w * td));
}

Expansion4 IntersectLineTriangle3D(
    const Vector3d &p, const Vector3d &q,
    const Vector3d &a, const Vector3d &b, const Vector3d &c)
{
    // Let A = p - a, B = q - p, C = (c - a) cross (b - a), then (A + B * t) dot C = 0

    const auto ux = SExpansion(c.x) - SExpansion(a.x);
    const auto uy = SExpansion(c.y) - SExpansion(a.y);
    const auto uz = SExpansion(c.z) - SExpansion(a.z);

    const auto vx = SExpansion(b.x) - SExpansion(a.x);
    const auto vy = SExpansion(b.y) - SExpansion(a.y);
    const auto vz = SExpansion(b.z) - SExpansion(a.z);

    const auto Cx = uy * vz - uz * vy;
    const auto Cy = uz * vx - ux * vz;
    const auto Cz = ux * vy - uy * vx;

    const auto Ax = SExpansion(p.x) - SExpansion(a.x);
    const auto Ay = SExpansion(p.y) - SExpansion(a.y);
    const auto Az = SExpansion(p.z) - SExpansion(a.z);

    const auto Bx = SExpansion(q.x) - SExpansion(p.x);
    const auto By = SExpansion(q.y) - SExpansion(p.y);
    const auto Bz = SExpansion(q.z) - SExpansion(p.z);

    // t = -(A dot C) / (B dot C) = tu / td

    const auto tu = -(Ax * Cx + Ay * Cy + Az * Cz);
    const auto td = Bx * Cx + By * Cy + Bz * Cz;

    // [dxu, dyu, dzu] / td = (q - p) * t

    const auto dxu = Bx * tu;
    const auto dyu = By * tu;
    const auto dzu = Bz * tu;

    // result = p + (q - p) * t = (p * td + [dxu, dyu, dzu]) / td

    return
    {
        Expansion(SExpansion(p.x) * td + dxu),
        Expansion(SExpansion(p.y) * td + dyu),
        Expansion(SExpansion(p.z) * td + dzu),
        Expansion(td)
    };
}

RTRC_END
