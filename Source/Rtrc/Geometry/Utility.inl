#pragma once

#include <map>

#include <Rtrc/Core/Math/Common.h>

RTRC_GEO_BEGIN

template<typename T>
void ComputeAngleAveragedNormals(
    Span<Vector3<T>>    positions,
    Span<uint32_t>      indices,
    MutSpan<Vector3<T>> outNormals)
{
    assert(outNormals.size() == positions.size());

    std::ranges::fill(outNormals, Vector3<T>(0));
    
    const uint32_t faceCount = indices.size() / 3;
    for(uint32_t f = 0; f < faceCount; ++f)
    {
        const uint32_t v0 = indices[3 * f + 0];
        const uint32_t v1 = indices[3 * f + 1];
        const uint32_t v2 = indices[3 * f + 2];
        const Vector3<T> &p0 = positions[v0];
        const Vector3<T> &p1 = positions[v1];
        const Vector3<T> &p2 = positions[v2];
        const Vector3<T> n01 = Rtrc::Normalize(p1 - p0);
        const Vector3<T> n12 = Rtrc::Normalize(p2 - p1);
        const Vector3<T> n20 = Rtrc::Normalize(p0 - p2);
        const T cos0 = Rtrc::Dot(+n01, -n20);
        const T cos1 = Rtrc::Dot(+n12 ,-n01);
        const T cos2 = Rtrc::Dot(+n20 ,-n12);
        const T theta0 = std::acos(Rtrc::Saturate(cos0));
        const T theta1 = std::acos(Rtrc::Saturate(cos1));
        const T theta2 = std::acos(Rtrc::Saturate(cos2));
        const Vector3<T> normal = Rtrc::Normalize(Rtrc::Cross(p2 - p0, p1 - p0));
        outNormals[v0] += normal * theta0;
        outNormals[v1] += normal * theta1;
        outNormals[v2] += normal * theta2;
    }

    for(Vector3<T> &normal : outNormals)
    {
        normal = Rtrc::NormalizeIfNotZero(normal);
    }
}

template<typename T>
void MergeCoincidentVertices(
    const IndexedPositions<double> &input,
    std::vector<Vector3<T>>        &outputPositions,
    std::vector<uint32_t>          &outputIndices)
{
    outputPositions.clear();
    outputIndices.clear();

    std::map<Vector3<T>, uint32_t> positionToIndex;
    for(uint32_t i = 0; i < input.GetSize(); ++i)
    {
        const Vector3<T> &position = input[i];

        auto it = positionToIndex.find(position);
        if(it != positionToIndex.end())
        {
            outputIndices.push_back(it->second);
            continue;
        }

        const uint32_t newIndex = outputPositions.size();
        outputPositions.push_back(position);
        outputIndices.push_back(newIndex);
        positionToIndex.insert({ position, newIndex });
    }
}

RTRC_GEO_END
