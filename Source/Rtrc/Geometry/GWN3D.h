#pragma once

#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Geometry/BVH.h>
#include <Rtrc/Geometry/Utility.h>

RTRC_GEO_BEGIN

// geometry normal is assumed to align with cross(b - a, c - a)
// TODO: detect co-planar input
double ComputeGWN3DForFace(const Vector3d &pi, const Vector3d &pj, const Vector3d &pk, const Vector3d &p);

// 3D generalized winding number evaluator.
// See 'Robust Inside-Outside Segmentation using Generalized Winding Numbers'.
class GWN3D
{
public:

    static GWN3D Build(const IndexedPositions<double> &input);

    // TODO: detect co-planar input
    double Evaluate(const Vector3d &point) const;

private:

    struct PatchBoundaryEdge
    {
        uint32_t a;
        uint32_t b;
        int32_t count;
    };

    struct Node
    {
        AABB3d boundingBox;
        uint32_t childNode; // UINT32_MAX indicates this is a leaf node
        Span<PatchBoundaryEdge> edges;
    };

    std::vector<Node>     nodes_;
    std::vector<Vector3d> positions_;
};

RTRC_GEO_END
