#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/ToolKit/Resource/ResourceManager.h>

RTRC_BEGIN

class GizmoRenderer;

class GizmoBuilder : public Uncopyable
{
public:

    GizmoBuilder();

    void PushColor(const Vector3f &color);
    void PopColor();

    void DrawLine(const Vector3f &a, const Vector3f& b);
    void DrawTriangle(const Vector3f &a, const Vector3f &b, const Vector3f &c);
    void DrawQuad(const Vector3f &a, const Vector3f &b, const Vector3f &c, const Vector3f &d);

private:

    friend class GizmoRenderer;

    struct Line
    {
        Vector3f color;
        Vector3f a, b;
    };

    struct Triangle
    {
        Vector3f color;
        Vector3f a, b, c;
    };

    std::stack<Vector3f> colors_;

    std::vector<Line>     lines_;
    std::vector<Triangle> triangles_;
};

RTRC_END
