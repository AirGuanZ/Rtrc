#include <Rtrc/Rtrc.h>

using namespace Rtrc;

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

    LogInfo("Geodesic path found!");

    std::vector<Vector3d> pathPoints;
    pathPoints.reserve(path.size());
    for(auto &point : path)
    {
        pathPoints.push_back(Geo::EvaluateGeodesicPathPoint<double>(connectivity, positions, point));
    }

    const double pathLength = Geo::ComputeDiscreteCurveLength(pathPoints, false);
    const uint32_t numSegments = (std::max<uint32_t>)(pathLength / 0.002f, 2);
    const auto samplePoints = Geo::UniformSampleDiscreteCurve(pathPoints, false, numSegments);

    std::vector<uint32_t> outputIndices;
    std::vector<Vector3d> outputPositions;
    for(auto &center : samplePoints)
    {
        const Vector3d size = Vector3d(0.002f);
        const Vector3d lower = center - 0.5 * size;
        const Vector3d upper = center + 0.5 * size;
        Geo::GenerateAABB(lower, upper, outputPositions, outputIndices);
    }

    Geo::WriteOFFFile<double>("Asset/Sample/18.ShortestGeodesicPath/Output.off", outputPositions, outputIndices);
}
