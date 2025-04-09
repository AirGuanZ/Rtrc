#include <Rtrc/Core/Math/Angle.h>
#include <Rtrc/Core/Math/Frame.h>
#include <Rtrc/ToolKit/Gizmo/GizmoBuilder.h>

RTRC_BEGIN

GizmoBuilder::GizmoBuilder()
{
    colors_.push({ 1, 1, 1 });
}

void GizmoBuilder::SetColor(const Vector3f &color)
{
    colors_.top() = color;
}

void GizmoBuilder::PushColor(const Vector3f &color)
{
    colors_.push(color);
}

void GizmoBuilder::PopColor()
{
    colors_.pop();
}

void GizmoBuilder::DrawLine(const Vector3f &a, const Vector3f &b)
{
    lines_.push_back({ colors_.top(), a, b });
}

void GizmoBuilder::DrawTriangle(const Vector3f &a, const Vector3f &b, const Vector3f &c)
{
    triangles_.push_back({ colors_.top(), a, b, c });
}

void GizmoBuilder::DrawQuad(const Vector3f &a, const Vector3f &b, const Vector3f &c, const Vector3f &d)
{
    DrawTriangle(a, b, c);
    DrawTriangle(a, c, d);
}

void GizmoBuilder::DrawWireCuboid(const Vector3f &o, const Vector3f &x, const Vector3f &y, const Vector3f &z)
{
    const Vector3f a0 = o;
    const Vector3f b0 = o + x;
    const Vector3f c0 = o + x + y;
    const Vector3f d0 = o + y;

    const Vector3f a1 = a0 + z;
    const Vector3f b1 = b0 + z;
    const Vector3f c1 = c0 + z;
    const Vector3f d1 = d0 + z;

    DrawLine(a0, b0);
    DrawLine(b0, c0);
    DrawLine(c0, d0);
    DrawLine(d0, a0);

    DrawLine(a1, b1);
    DrawLine(b1, c1);
    DrawLine(c1, d1);
    DrawLine(d1, a1);

    DrawLine(a0, a1);
    DrawLine(b0, b1);
    DrawLine(c0, c1);
    DrawLine(d0, d1);
}

void GizmoBuilder::DrawCuboid(const Vector3f &o, const Vector3f &x, const Vector3f &y, const Vector3f &z)
{
    const Vector3f a0 = o;
    const Vector3f b0 = o + x;
    const Vector3f c0 = o + x + y;
    const Vector3f d0 = o + y;

    const Vector3f a1 = a0 + z;
    const Vector3f b1 = b0 + z;
    const Vector3f c1 = c0 + z;
    const Vector3f d1 = d0 + z;

    DrawQuad(a0, b0, c0, d0);
    DrawQuad(a1, b1, c1, d1);
    DrawQuad(a0, a1, b1, b0);
    DrawQuad(b0, b1, c1, c0);
    DrawQuad(c0, c1, d1, d0);
    DrawQuad(d0, d1, a1, a0);
}

void GizmoBuilder::DrawWireCube(const Vector3f &o, float sidelen)
{
    DrawWireCuboid(
        o - Vector3f(0.5f * sidelen),
        { sidelen, 0, 0 }, { 0, sidelen, 0 }, { 0, 0, sidelen });
}

void GizmoBuilder::DrawCube(const Vector3f &o, float sidelen)
{
    DrawCuboid(
        o - Vector3f(0.5f * sidelen),
        { sidelen, 0, 0 }, { 0, sidelen, 0 }, { 0, 0, sidelen });
}

void GizmoBuilder::DrawWireDisk(const Vector3f &o, const Vector3f &nor, float radius, int subdiv)
{
    const Framef frame = Framef::FromZ(nor);
    const float step = 2 * std::numbers::pi_v<float> / subdiv;
    auto GetPoint = [&](int i)
    {
        const float phi = i * step;
        const Vector2f hp = { std::cos(phi), std::sin(phi) };
        const Vector3f p = o + radius * frame.LocalToGlobal({ hp.x, hp.y, 0 });
        return p;
    };

    Vector3f prev = GetPoint(0);
    for(int i = 0; i < subdiv; ++i)
    {
        const Vector3f curr = GetPoint(i + 1);
        DrawLine(prev, curr);
        prev = curr;
    }
}

RTRC_END
