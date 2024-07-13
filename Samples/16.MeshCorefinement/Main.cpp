#include <Rtrc/Rtrc.h>

using namespace Rtrc;
using namespace Geo;

std::vector<Vector3d> LoadMesh(const std::string &filename)
{
    std::vector<Vector3d> result;
    const auto rawMesh = RawMesh::Load(filename);
    for(uint32_t i : rawMesh.GetPositionIndices())
    {
        result.push_back(rawMesh.GetPositionData()[i].To<double>());
    }
    return result;
}

int main()
{
    const auto inputA = LoadMesh("./Asset/Sample/16.MeshCorefinement/MeshA.obj");
    const auto inputB = LoadMesh("./Asset/Sample/16.MeshCorefinement/MeshB.obj");

    std::vector<Vector3d> outputA, outputB;
    Corefine(inputA, inputB, outputA, outputB);

    WriteOFFFile<double>("./Asset/Sample/16.MeshCorefinement/OutputA.off", outputA, {});
    WriteOFFFile<double>("./Asset/Sample/16.MeshCorefinement/OutputB.off", outputB, {});
}
