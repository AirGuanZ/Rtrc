#include <Rtrc/Geometry/BVH.h>
#include <Rtrc/Geometry/GWN3D.h>
#include <Rtrc/Geometry/Utility.h>

RTRC_GEO_BEGIN

double ComputeGWN3DForFace(const Vector3d &pi, const Vector3d &pj, const Vector3d &pk, const Vector3d &p)
{
    const Vector3d a = pi - p;
    const Vector3d b = pj - p;
    const Vector3d c = pk - p;
    const double al = Length(a);
    const double bl = Length(b);
    const double cl = Length(c);
    const double U = Det3x3(
        a.x, b.x, c.x,
        a.y, b.y, c.y,
        a.z, b.z, c.z);
    const double D = al * bl * cl + Dot(a, b) * cl + Dot(b, c) * al + Dot(c, a) * bl;
    const double tanHalfResult = U / D;
    return 2.0 * std::atan(tanHalfResult);
}

GWN3D GWN3D::Build(const IndexedPositions<double> &input)
{
    std::vector<Vector3d> positions;
    std::vector<uint32_t> indices;
    MergeCoincidentVertices(input, positions, indices);

    BVH3D<double> bvh;
    {
        std::vector<AABB3d> triangleBounds;
        triangleBounds.reserve(indices.size() / 3);
        for(size_t i = 0; i < indices.size(); i += 3)
        {
            auto &bound = triangleBounds.emplace_back();
            bound |= positions[indices[i + 0]];
            bound |= positions[indices[i + 1]];
            bound |= positions[indices[i + 2]];
        }
        bvh = BVH3D<double>::Build(triangleBounds);
    }

    auto bvhNodes = bvh.GetNodes();
    auto bvhPrimitiveIndices = bvh.GetPrimitiveIndices();

    assert(!bvhNodes.IsEmpty());
    std::vector<Node> nodes;
    nodes.resize(bvhNodes.size());

    for(int nodeIndex = static_cast<int>(nodes.size()) - 1; nodeIndex >= 0; --nodeIndex)
    {
        auto &inNode = bvhNodes[nodeIndex];
        auto &outNode = nodes[nodeIndex];

        outNode.boundingBox = inNode.boundingBox;
        if(!outNode.boundingBox.IsEmpty())
        {
            const Vector3d center = outNode.boundingBox.ComputeCenter();
            const Vector3d extent = outNode.boundingBox.ComputeExtent();
            outNode.boundingBox.lower = center - 0.51 * extent;
            outNode.boundingBox.upper = center + 0.51 * extent;
        }
        outNode.childNode = inNode.IsLeaf() ? UINT32_MAX : inNode.childIndex;
        
        std::map<Vector2u, int32_t> edgeToCount;
        auto AddEdge = [&edgeToCount](uint32_t a, uint32_t b, int32_t count)
        {
            if(a > b)
            {
                std::swap(a, b);
                count = -count;
            }
            edgeToCount[{ a, b }] += count;
        };

        if(inNode.IsLeaf())
        {
            for(uint32_t ci = 0; ci < inNode.childCount; ++ci)
            {
                const uint32_t pi = bvhPrimitiveIndices[inNode.childIndex + ci];
                const uint32_t i0 = indices[3 * pi + 0];
                const uint32_t i1 = indices[3 * pi + 1];
                const uint32_t i2 = indices[3 * pi + 2];
                AddEdge(i0, i1, 1);
                AddEdge(i1, i2, 1);
                AddEdge(i2, i0, 1);
            }
        }
        else
        {
            for(auto& childNode : { nodes[outNode.childNode], nodes[outNode.childNode + 1] })
            {
                for(auto& edge : childNode.edges)
                {
                    AddEdge(edge.a, edge.b, edge.count);
                }
            }
        }
    }

    GWN3D result;
    result.nodes_ = std::move(nodes);
    result.positions_ = std::move(positions);
    return result;
}

double GWN3D::Evaluate(const Vector3d &point) const
{
    static constexpr uint32_t StackCapacity = 64;
    thread_local uint32_t stack[StackCapacity];
    uint32_t stackSize = 0;
    stack[stackSize++] = 0;

    double result = 0;
    while(stackSize > 0)
    {
        const uint32_t nodeIndex = stack[stackSize--];
        const Node &node = nodes_[nodeIndex];

        if(node.childNode != UINT32_MAX && node.boundingBox.Contains(point))
        {
            assert(stackSize + 2 <= StackCapacity);
            stack[stackSize++] = node.childNode;
            stack[stackSize++] = node.childNode + 1;
        }
        else
        {
            const Vector3d c = node.boundingBox.ComputeCenter();
            for(auto& edge : node.edges)
            {
                const Vector3d &a = positions_[edge.a];
                const Vector3d &b = positions_[edge.b];
                result += edge.count * ComputeGWN3DForFace(a, b, c, point);
            }
        }
    }

    return result;
}

RTRC_GEO_END
