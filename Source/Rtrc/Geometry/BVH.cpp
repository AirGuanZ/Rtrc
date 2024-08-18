#include <Rtrc/Geometry/BVH.h>

#include <bvh/bvh.hpp>
#include <bvh/leaf_collapser.hpp>
#include <bvh/locally_ordered_clustering_builder.hpp>

RTRC_GEO_BEGIN

namespace BVHDetail
{

    template<typename T>
    bvh::Vector3<T> ConvertVector3(const Vector3<T> &v)
    {
        return { v.x, v.y, v.z };
    }

    template<typename T>
    bvh::BoundingBox<T> ConvertBoundingBox(const AABB3<T> &boundingBox)
    {
        return { BVHDetail::ConvertVector3(boundingBox.lower), BVHDetail::ConvertVector3(boundingBox.upper) };
    }

} // namespace BVHDetail

template <typename T>
BVH<T> BVH<T>::Build(Span<AABB3<T>> primitiveBounds)
{
    if(primitiveBounds.IsEmpty())
    {
        return {};
    }

    AABB3<T> globalBoundingBoxes;
    std::vector<bvh::BoundingBox<T>> bvhPrimitiveBounds(primitiveBounds.GetSize());
    std::vector<bvh::Vector3<T>> bvhPrimitiveCenters(primitiveBounds.GetSize());
    for(uint32_t i = 0; i < primitiveBounds.size(); ++i)
    {
        globalBoundingBoxes |= primitiveBounds[i];
        bvhPrimitiveBounds[i] = BVHDetail::ConvertBoundingBox(primitiveBounds[i]);
        bvhPrimitiveCenters[i] = BVHDetail::ConvertVector3(primitiveBounds[i].ComputeCenter());
    }

    bvh::Bvh<T> tree;
    bvh::LocallyOrderedClusteringBuilder<bvh::Bvh<T>, uint32_t>(tree).build(
        BVHDetail::ConvertBoundingBox(globalBoundingBoxes),
        bvhPrimitiveBounds.data(), bvhPrimitiveCenters.data(), bvhPrimitiveBounds.size());
    bvh::LeafCollapser<bvh::Bvh<T>>(tree).collapse();

    BVH ret;
    ret.nodes_.resize(tree.node_count);

    for(uint32_t ni = 0; ni < tree.node_count; ++ni)
    {
        auto &src = tree.nodes[ni];
        auto &dst = ret.nodes_[ni];

        dst.boundingBox.lower.x = src.bounds[0];
        dst.boundingBox.lower.y = src.bounds[2];
        dst.boundingBox.lower.z = src.bounds[4];
        dst.boundingBox.upper.x = src.bounds[1];
        dst.boundingBox.upper.y = src.bounds[3];
        dst.boundingBox.upper.z = src.bounds[5];

        if(src.is_leaf())
        {
            const size_t primitiveOffset = ret.primitiveIndices_.size();
            const size_t srcPrimitiveEnd = src.first_child_or_primitive + src.primitive_count;
            for(size_t i = src.first_child_or_primitive; i < srcPrimitiveEnd; ++i)
            {
                ret.primitiveIndices_.push_back(static_cast<uint32_t>(tree.primitive_indices[i]));
            }
            const size_t newPrimitiveOffset = ret.primitiveIndices_.size();

            assert(primitiveOffset != newPrimitiveOffset);
            dst.childIndex = static_cast<uint32_t>(primitiveOffset);
            dst.childCount = static_cast<uint32_t>(newPrimitiveOffset - primitiveOffset);
        }
        else
        {
            dst.childIndex = static_cast<uint32_t>(src.first_child_or_primitive);
            dst.childCount = 0;
        }
    }

    return ret;
}

template BVH<float> BVH<float>::Build(Span<AABB3<float>>);
template BVH<double> BVH<double>::Build(Span<AABB3<double>>);

RTRC_GEO_END
