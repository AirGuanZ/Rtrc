#include <Rtrc/Rtrc.h>

using namespace Rtrc;

template<typename Func>
void ForEachSegmentOnGeodesicPath(
    const Geo::HalfedgeMesh              &connectivity,
    Span<Vector3d>                        positions,
    Span<Geo::GeodesicPathICH::PathPoint> points,
    const Func                           &func)
{
    auto EvaluatePathPoint = [&](const Geo::GeodesicPathICH::PathPoint &point)
    {
        return point.Match(
            [&](const Geo::GeodesicPathICH::VertexPoint &vertexPoint)
            {
                return positions[vertexPoint.v];
            },
            [&](const Geo::GeodesicPathICH::EdgePoint &edgePoint)
            {
                const Vector3d &a = positions[connectivity.Head(edgePoint.halfedge)];
                const Vector3d &b = positions[connectivity.Tail(edgePoint.halfedge)];
                return Lerp(a, b, edgePoint.normalizedPosition);
            });
    };

    Vector3d prevPoint = EvaluatePathPoint(points[0]);
    for(size_t i = 1; i < points.size(); ++i)
    {
        const Vector3d currPoint = EvaluatePathPoint(points[i]);
        func(prevPoint, currPoint);
        prevPoint = currPoint;
    }
}

std::vector<Vector3d> SampleGeodesicPath(
    const Geo::HalfedgeMesh              &connectivity,
    Span<Vector3d>                        positions,
    Span<Geo::GeodesicPathICH::PathPoint> points,
    double                                segmentLength)
{
    double totalLength = 0;
    ForEachSegmentOnGeodesicPath(connectivity, positions, points, [&](const Vector3d &a, const Vector3d &b)
    {
        totalLength += Distance(a, b);
    });

    const uint32_t N = (std::max)(static_cast<uint32_t>(totalLength / segmentLength) + 1, 2u);
    segmentLength = totalLength / (N - 1);

    std::vector<Vector3d> result;
    double accumulatedLength = 0;
    ForEachSegmentOnGeodesicPath(connectivity, positions, points, [&](const Vector3d &a, const Vector3d &b)
    {
        const double L = Distance(a, b);
        double t = 0;
        while(true)
        {
            const double r = segmentLength - accumulatedLength;
            if(t + r <= L)
            {
                t += r;
                accumulatedLength = 0;
                if(result.size() < N)
                {
                    result.push_back(Lerp(a, b, t / L));
                }
            }
            else
            {
                accumulatedLength += L - t;
                break;
            }
        }
    });

    result.push_back(positions[points.last().As<Geo::GeodesicPathICH::VertexPoint>().v]);

    return result;
}

int main()
{
    const auto rawMesh = Geo::RawMesh::Load("Asset/Sample/18.ShortestGeodesicPath/Bunny.obj");
    const auto positions = rawMesh.GetPositionDataDouble();
    const auto indices = rawMesh.GetPositionIndices();
    const auto connectivity = Geo::HalfedgeMesh::Build(indices);

    Geo::GeodesicPathICH ICH{ connectivity, positions };
    const auto path = ICH.FindShortestPath(0, 128);
    if(path.empty())
    {
        LogError("Fail to find a geodesic path");
        return -1;
    }
    const auto samplePoints = SampleGeodesicPath(connectivity, positions, path, 0.002f);

    LogInfo("Geodesic path found!");

    const auto cubeMesh = Geo::RawMesh::Load("Asset/Sample/18.ShortestGeodesicPath/Cube.obj");
    const Geo::IndexedPositions cubePositions{ cubeMesh.GetPositionData(), cubeMesh.GetPositionIndices() };

    std::vector<Vector3f> outputPositions;
    for(const Vector3d &translation : samplePoints)
    {
        for(size_t i = 0; i < cubePositions.GetSize(); ++i)
        {
            const Vector3f &cubePosition = cubePositions[i];
            outputPositions.push_back(0.001f * cubePosition + translation.To<float>());
        }
    }

    Geo::WriteOFFFile<float>("Asset/Sample/18.ShortestGeodesicPath/Output.off", outputPositions, {});
}
