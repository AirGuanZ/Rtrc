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

        dst.boundingBox[0] = src.bounds[0];
        dst.boundingBox[1] = src.bounds[2];
        dst.boundingBox[2] = src.bounds[4];
        dst.boundingBox[3] = src.bounds[1];
        dst.boundingBox[4] = src.bounds[3];
        dst.boundingBox[5] = src.bounds[5];

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

bool TriangleBVH::Node::IsLeaf() const
{
    return childCount != 0;
}

RTRC_END
