#include <Rtrc/Geometry/BVH.h>

#include <bvh/v2/default_builder.h>

RTRC_GEO_BEGIN

namespace BVHDetail
{

    namespace b2 = bvh::v2;

    template<typename T>
    b2::Vec<T, 2> ConvertVector(const Vector2<T> &v)
    {
        return { v.x, v.y };
    }

    template<typename T>
    b2::Vec<T, 3> ConvertVector(const Vector3<T> &v)
    {
        return { v.x, v.y, v.z };
    }

    template<typename T>
    b2::BBox<T, 2> ConvertBoundingBox(const AABB2<T> &boundingBox)
    {
        return { BVHDetail::ConvertVector(boundingBox.lower), BVHDetail::ConvertVector(boundingBox.upper) };
    }

    template<typename T>
    b2::BBox<T, 3> ConvertBoundingBox(const AABB3<T> &boundingBox)
    {
        return { BVHDetail::ConvertVector(boundingBox.lower), BVHDetail::ConvertVector(boundingBox.upper) };
    }

} // namespace BVHDetail

template <typename T, int Dim>
BVHImpl<T, Dim> BVHImpl<T, Dim>::Build(Span<BoundingBox> primitiveBounds)
{
    namespace b2 = bvh::v2;

    if(primitiveBounds.IsEmpty())
    {
        return {};
    }

    BoundingBox globalBoundingBoxes;
    std::vector<b2::BBox<T, Dim>> bvhPrimitiveBounds(primitiveBounds.GetSize());
    std::vector<b2::Vec<T, Dim>> bvhPrimitiveCenters(primitiveBounds.GetSize());
    for(uint32_t i = 0; i < primitiveBounds.size(); ++i)
    {
        globalBoundingBoxes |= primitiveBounds[i];
        bvhPrimitiveBounds[i] = BVHDetail::ConvertBoundingBox(primitiveBounds[i]);
        bvhPrimitiveCenters[i] = BVHDetail::ConvertVector(primitiveBounds[i].ComputeCenter());
    }

    const auto tree = b2::DefaultBuilder<b2::Node<T, Dim>>().build(bvhPrimitiveBounds, bvhPrimitiveCenters);

    BVHImpl ret;
    ret.nodes_.resize(tree.nodes.size());

    for(uint32_t ni = 0; ni < tree.nodes.size(); ++ni)
    {
        auto &src = tree.nodes[ni];
        auto &dst = ret.nodes_[ni];

        static_assert(Dim == 2 || Dim == 3);
        if constexpr(Dim == 2)
        {
            dst.boundingBox.lower.x = src.bounds[0];
            dst.boundingBox.lower.y = src.bounds[2];
            dst.boundingBox.upper.x = src.bounds[1];
            dst.boundingBox.upper.y = src.bounds[3];
        }
        else
        {
            dst.boundingBox.lower.x = src.bounds[0];
            dst.boundingBox.lower.y = src.bounds[2];
            dst.boundingBox.lower.z = src.bounds[4];
            dst.boundingBox.upper.x = src.bounds[1];
            dst.boundingBox.upper.y = src.bounds[3];
            dst.boundingBox.upper.z = src.bounds[5];
        }

        const size_t srcPrimitiveBegin = src.index.first_id();

        if(src.is_leaf())
        {
            const size_t primitiveOffset = ret.primitiveIndices_.size();
            const size_t srcPrimitiveEnd = srcPrimitiveBegin + src.index.prim_count();
            for(size_t i = srcPrimitiveBegin; i < srcPrimitiveEnd; ++i)
            {
                const uint32_t primitiveID = tree.prim_ids[i];
                ret.primitiveIndices_.push_back(primitiveID);
            }
            const size_t newPrimitiveOffset = ret.primitiveIndices_.size();

            assert(primitiveOffset != newPrimitiveOffset);
            dst.childIndex = static_cast<uint32_t>(primitiveOffset);
            dst.childCount = static_cast<uint32_t>(newPrimitiveOffset - primitiveOffset);
        }
        else
        {
            dst.childIndex = static_cast<uint32_t>(srcPrimitiveBegin);
            dst.childCount = 0;
        }
    }

    return ret;
}

template BVHImpl<float, 2> BVHImpl<float, 2>::Build(Span<AABB2<float>>);
template BVHImpl<float, 3> BVHImpl<float, 3>::Build(Span<AABB3<float>>);
template BVHImpl<double, 2> BVHImpl<double, 2>::Build(Span<AABB2<double>>);
template BVHImpl<double, 3> BVHImpl<double, 3>::Build(Span<AABB3<double>>);

RTRC_GEO_END
