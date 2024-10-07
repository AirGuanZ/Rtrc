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
    const uint32_t numSegments = (std::max<uint32_t>)(static_cast<uint32_t>(pathLength / 0.002), 2);
    const auto samplePoints = Geo::UniformSampleDiscreteCurve(pathPoints, false, numSegments);

    std::vector<uint32_t> outputIndices; std::vector<Vector3d> outputPositions;
    Geo::GenerateTube(samplePoints, false, 0.002, 16, true, outputPositions, outputIndices);

    Geo::WriteOFFFile<double>("Asset/Sample/18.ShortestGeodesicPath/Output.off", outputPositions, outputIndices);
}
