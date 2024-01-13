#include <Rtrc/ToolKit/Gizmo/GizmoBuilder.h>

RTRC_BEGIN

GizmoBuilder::GizmoBuilder()
{
    colors_.push({ 1, 1, 1 });
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

RTRC_END
