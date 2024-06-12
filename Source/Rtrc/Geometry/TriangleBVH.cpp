#include <bvh/bvh.hpp>
#include <bvh/leaf_collapser.hpp>
#include <bvh/locally_ordered_clustering_builder.hpp>

#include <Rtrc/Core/Math/AABB.h>
#include <Rtrc/Core/Math/Intersection.h>
#include <Rtrc/Geometry/TriangleBVH.h>

RTRC_BEGIN

namespace TriangleBVHDetail
{

    bvh::Vector3<float> ConvertVector3(const Vector3f &v)
    {
        return { v.x, v.y, v.z };
    }

    bvh::BoundingBox<float> ConvertBoundingBox(const AABB3f &boundingBox)
    {
        return { ConvertVector3(boundingBox.lower), ConvertVector3(boundingBox.upper) };
    }

} // namespace TriangleBVHDetail

TriangleBVH TriangleBVH::Build(Span<Vector3f> positions, Span<uint32_t> indices)
{
    using namespace TriangleBVHDetail;

    // Build bvh

    assert(indices.size() % 3 == 0);
    const uint32_t triangleCount = indices.size() / 3;

    AABB3f globalBoundingBox;
    std::vector<bvh::BoundingBox<float>> primitiveBoundingBoxes(triangleCount);
    std::vector<bvh::Vector3<float>> primitiveCenters(triangleCount);
    for(uint32_t i = 0, j = 0; i < triangleCount; ++i, j += 3)
    {
        const uint32_t v0 = indices[j + 0];
        const uint32_t v1 = indices[j + 1];
        const uint32_t v2 = indices[j + 2];

        AABB3f primitiveBoundingBox;
        primitiveBoundingBox |= positions[v0];
        primitiveBoundingBox |= positions[v1];
        primitiveBoundingBox |= positions[v2];
        primitiveBoundingBoxes[i] = ConvertBoundingBox(primitiveBoundingBox);

        globalBoundingBox |= primitiveBoundingBox;

        const Vector3f center = 1.0f / 3 * (positions[v0] + positions[v1] + positions[v2]);
        primitiveCenters[i] = ConvertVector3(center);
    }

    bvh::Bvh<float> tree;
    bvh::LocallyOrderedClusteringBuilder<bvh::Bvh<float>, uint32_t> builder(tree);

    builder.build(
        ConvertBoundingBox(globalBoundingBox), primitiveBoundingBoxes.data(), primitiveCenters.data(), triangleCount);

    bvh::LeafCollapser(tree).collapse();

    // Convert to nodes

    TriangleBVH ret;

    ret.nodes_.resize(tree.node_count);
    for(uint32_t ni = 0; ni < tree.node_count; ++ni)
    {
        auto &src = tree.nodes[ni];
        auto &dst = ret.nodes_[ni];
        std::memcpy(dst.boundingBox, src.bounds, sizeof(float) * 6);

        if(src.is_leaf())
        {
            const size_t triangleBegin = ret.triangles_.size();
            for(size_t i = src.first_child_or_primitive; i < src.first_child_or_primitive + src.primitive_count; ++i)
            {
                const uint32_t pi = tree.primitive_indices[i];
                const uint32_t v0 = indices[3 * pi + 0];
                const uint32_t v1 = indices[3 * pi + 1];
                const uint32_t v2 = indices[3 * pi + 2];

                auto &triangle = ret.triangles_.emplace_back();
                triangle.a  = positions[v0];
                triangle.ab = positions[v1] - triangle.a;
                triangle.ac = positions[v2] - triangle.a;

                ret.originalTriangleIndices_.push_back(pi);
            }
            const size_t triangleEnd = ret.triangles_.size();

            assert(triangleEnd != triangleBegin);
            dst.childIndex = static_cast<uint32_t>(triangleBegin);
            dst.childCount = static_cast<uint32_t>(triangleEnd - triangleBegin);
        }
        else
        {
            dst.childIndex = src.first_child_or_primitive;
            dst.childCount = 0;
        }
    }

    return ret;
}

template <typename Func>
void TriangleBVH::FindClosestIntersection(const Vector3f &o, const Vector3f &d, float maxT, const Func &func) const
{
    const Vector3f rcpD = { 1.0f / d.x, 1.0f / d.y, 1.0f / d.z };
    if(!IntersectRayBox(o, rcpD, 0.0f, maxT, nodes_[0].boundingBox))
    {
        return;
    }

    float t = maxT;
    uint32_t primitiveIndex = (std::numeric_limits<uint32_t>::max)();
    Vector2f baryUV;

    constexpr int STACK_SIZE = 128;
    uint32_t stack[STACK_SIZE];
    int top = 0;
    stack[top++] = 0;

    while(top)
    {
        const uint32_t taskNodeIndex = stack[--top];
        const Node &node = nodes_[taskNodeIndex];

        if(node.IsLeaf())
        {
            for(uint32_t i = node.childIndex; i < node.childIndex + node.childCount; ++i)
            {
                const Triangle &triangle = triangles_[i];
                float localT; Vector2f localBaryUV;
                if(IntersectRayTriangle(o, d, 0.0f, t, triangle.a, triangle.ab, triangle.ac, localT, localBaryUV))
                {
                    if(localT < t)
                    {
                        t = localT;
                        baryUV = localBaryUV;
                        primitiveIndex = i;
                    }
                }
            }
        }
        else
        {
            assert(top + 2 < STACK_SIZE);

            if(IntersectRayBox(o, rcpD, 0.0f, t, nodes_[node.childIndex].boundingBox))
            {
                stack[top++] = node.childIndex;
            }

            if(IntersectRayBox(o, rcpD, 0.0f, t, nodes_[node.childIndex + 1].boundingBox))
            {
                stack[top++] = node.childIndex + 1;
            }
        }
    }

    if(primitiveIndex != (std::numeric_limits<uint32_t>::max)())
    {
        const int originalTriangleIndex = originalTriangleIndices_[primitiveIndex];
        func(t, originalTriangleIndex, baryUV);
    }
}

bool TriangleBVH::Node::IsLeaf() const
{
    return childCount != 0;
}

RTRC_END
