#include <Rtrc/Geometry/Exact/Intersection.h>

RTRC_GEO_BEGIN

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
    using E = SExpansion;

    const E ax = E(a.x);
    const E ay = E(a.y);
    const E az = E(a.z);
    const E ux = E(b.x) - ax;
    const E uy = E(b.y) - ay;
    const E uz = E(b.z) - az;
    const E vx = E(c.x) - ax;
    const E vy = E(c.y) - ay;
    const E vz = E(c.z) - az;

    E nx = uy * vz - uz * vy;
    E ny = uz * vx - ux * vz;
    E nz = ux * vy - uy * vx;
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
    const auto tu = (A.x * B.y - B.x * A.y) * C.z;
    const auto td = (C.x * B.y - B.x * C.y) * A.z;

    const auto ab = HomoSub(b3d, a3d);
    return HomoAdd(a3d, Expansion4(ab.x * tu, ab.y * tu, ab.z * tu, ab.w * td));
}

Expansion4 IntersectLineTriangle3D(
    const Vector3d &p, const Vector3d &q,
    const Vector3d &a, const Vector3d &b, const Vector3d &c)
{
    using E = SExpansion;

    // Let A = p - a, B = q - p, C = (c - a) cross (b - a), then (A + B * t) dot C = 0

    const E ux = E(c.x) - E(a.x);
    const E uy = E(c.y) - E(a.y);
    const E uz = E(c.z) - E(a.z);

    const E vx = E(b.x) - E(a.x);
    const E vy = E(b.y) - E(a.y);
    const E vz = E(b.z) - E(a.z);

    const E Cx = uy * vz - uz * vy;
    const E Cy = uz * vx - ux * vz;
    const E Cz = ux * vy - uy * vx;

    const E Ax = E(p.x) - E(a.x);
    const E Ay = E(p.y) - E(a.y);
    const E Az = E(p.z) - E(a.z);

    const E Bx = E(q.x) - E(p.x);
    const E By = E(q.y) - E(p.y);
    const E Bz = E(q.z) - E(p.z);

    // t = -(A dot C) / (B dot C) = tu / td

    const E tu = -(Ax * Cx + Ay * Cy + Az * Cz);
    const E td = Bx * Cx + By * Cy + Bz * Cz;

    // [dxu, dyu, dzu] / td = (q - p) * t

    const E dxu = Bx * tu;
    const E dyu = By * tu;
    const E dzu = Bz * tu;

    // result = p + (q - p) * t = (p * td + [dxu, dyu, dzu]) / td

    return
    {
        Expansion(E(p.x) * td + dxu),
        Expansion(E(p.y) * td + dyu),
        Expansion(E(p.z) * td + dzu),
        Expansion(td)
    };
}

RTRC_GEO_END
