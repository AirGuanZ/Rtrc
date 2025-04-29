#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector.h>

RTRC_GEO_BEGIN

template<typename T>
class IndexedPositions
{
public:

    IndexedPositions() = default;

    // By setting indices to empty, they will be implicitly defined as 0, 1, 2, ...
    IndexedPositions(Span<Vector3<T>> positions, Span<uint32_t> indices)
        : positions(positions), indices(indices)
    {

    }

    const Vector3<T> &operator[](uint32_t i) const
    {
        if(indices.IsEmpty())
        {
            return positions[i];
        }
        return positions[indices[i]];
    }

    uint32_t GetSize() const
    {
        return indices.IsEmpty() ? positions.size() : indices.size();
    }

    Span<Vector3<T>> GetPositions() const
    {
        return positions;
    }

    uint32_t GetPositionCount() const
    {
        return positions.size();
    }

private:

    Span<Vector3<T>> positions;
    Span<uint32_t> indices;
};

template<typename T>
void ComputeAngleAveragedNormals(
    Span<Vector3<T>>    positions,
    Span<uint32_t>      indices,
    MutSpan<Vector3<T>> outNormals);

// By setting inputIndices to empty, they will be implicitly defined as 0, 1, 2, ...
template<typename T>
void MergeCoincidentVertices(
    const IndexedPositions<double> &input,
    std::vector<Vector3<T>>        &outputPositions,
    std::vector<uint32_t>          &outputIndices);

RTRC_GEO_END

#include <Rtrc/Geometry/Utility.inl>
